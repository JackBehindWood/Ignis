from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Callable


# FNV-1a 32-bit hash — matches the runtime ComponentDescriptor type_id convention.
def _fnv1a_32(s: str) -> int:
    h = 0x811C9DC5
    for c in s.encode('utf-8'):
        h ^= c
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


@dataclass
class CodecEntry:
    match:  Callable[[str], bool]
    encode: str | None   # None = emit #error
    decode: str | None   # None = emit #error


def _exact(*names: str) -> Callable[[str], bool]:
    return lambda t: t in names


def _math_alias(*names: str) -> Callable[[str], bool]:
    return lambda t: t in names


CODEC_TABLE: list[CodecEntry] = [
    CodecEntry(match=_exact("float", "double"),
               encode="{val}",
               decode="{node}.as<float>()"),
    CodecEntry(match=_exact("bool"),
               encode="{val}",
               decode="{node}.as<bool>()"),
    CodecEntry(match=_exact("int", "int32_t", "uint32_t"),
               encode="{val}",
               decode="{node}.as<int>()"),
    CodecEntry(match=_exact("String"),
               encode="{val}",
               decode="{node}.as<std::string>()"),
    CodecEntry(match=_exact("Math::Vec2f"),
               encode="{val}",
               decode="{node}.as<Ignis::Math::Vec2f>()"),
    CodecEntry(match=_exact("Math::Vec3f"),
               encode="{val}",
               decode="{node}.as<Ignis::Math::Vec3f>()"),
    CodecEntry(match=_exact("Math::Vec4f"),
               encode="{val}",
               decode="{node}.as<Ignis::Math::Vec4f>()"),
    CodecEntry(match=_exact("Math::Quatf"),
               encode="{val}",
               decode="{node}.as<Ignis::Math::Quatf>()"),
    CodecEntry(match=_exact("AssetID"),
               encode="{val}.str()",
               decode="AssetID({{{node}.as<std::string>()}})"),
    CodecEntry(match=_exact("UUID"),
               encode="static_cast<uint64_t>({val})",
               decode="UUID({node}.as<uint64_t>())"),
    CodecEntry(match=_exact("SceneCamera"),
               encode="serialize_scene_camera({val})",
               decode="deserialize_scene_camera({node})"),
    # Unknown type — fallthrough produces #error
    CodecEntry(match=lambda _: True, encode=None, decode=None),
]


def _resolve_codec(field_type: str) -> CodecEntry:
    for entry in CODEC_TABLE:
        if entry.match(field_type):
            return entry
    return CODEC_TABLE[-1]  # unreachable; fallthrough entry always matches


def _serialize_line(prop: dict) -> str:
    entry = _resolve_codec(prop['field_type'])
    if entry.encode is None:
        ft = prop['field_type']
        fn = prop['field_name']
        return (
            f'#error "IHT codegen error: no YAML codec for type \'{ft}\' '
            f'(field \'{fn}\') — add a CodecEntry to generator.py"'
        )
    val    = f"c.{prop['field_name']}"
    encode = entry.encode.format(val=val)
    return f'            out << YAML::Key << "{prop["prop_name"]}" << YAML::Value << {encode};'


def _deserialize_line(prop: dict) -> str:
    entry = _resolve_codec(prop['field_type'])
    if entry.decode is None:
        ft = prop['field_type']
        fn = prop['field_name']
        return (
            f'#error "IHT codegen error: no YAML codec for type \'{ft}\' '
            f'(field \'{fn}\') — add a CodecEntry to generator.py"'
        )
    field_node = f'node["{prop["prop_name"]}"]'
    decode = entry.decode.format(
        node=field_node,
        alias=prop['field_type'],
        field=prop['prop_name'],
    )
    return (f'            if (node["{prop["prop_name"]}"]) '
            f'{{ c.{prop["field_name"]} = {decode}; }}')


def _emit_register_fn(type_name: str, info: dict) -> str:
    type_id = _fnv1a_32(type_name)
    props   = info['properties']

    serialize_lines   = '\n'.join(_serialize_line(p) for p in props)
    deserialize_lines = '\n'.join(_deserialize_line(p) for p in props)

    return f"""\
inline void register_{type_name}()
{{
    ComponentRegistry::register_component(ComponentDescriptor{{
        .type_name   = "{type_name}",
        .type_id     = 0x{type_id:08X}U,
        .add_to      = [](Entity& e) {{ e.add_component<{type_name}>(); }},
        .remove_from = [](Entity& e) {{ e.remove_component<{type_name}>(); }},
        .has_on      = [](const Entity& e) {{ return e.has_component<{type_name}>(); }},
        .serialize   = [](const Entity& e, YAML::Emitter& out)
        {{
            const auto& c = e.get_component<{type_name}>();
            out << YAML::Key << "{type_name}" << YAML::Value << YAML::BeginMap;
{serialize_lines}
            out << YAML::EndMap;
        }},
        .deserialize = [](Entity& e, const YAML::Node& node)
        {{
            auto& c = e.add_component<{type_name}>();
{deserialize_lines}
        }},
    }});
}}
"""


def emit(source: Path, schema: dict, output: Path) -> bool:
    """Generate a .gen.h for Component-metaclass types found in schema.

    Returns True if a file was written, False if the source has no annotated
    components (no output is produced for annotation-free headers).
    """
    components = {k: v for k, v in schema.items() if v.get('metaclass') == 'Component'}
    if not components:
        return False

    source_include = _source_include(source)
    fn_block = '\n'.join(_emit_register_fn(name, info) for name, info in components.items())

    content = f"""\
#pragma once
// Generated by IHT — do not edit
#include "{source_include}"
#include "Ignis/Scene/ComponentRegistry.h"
#include "Ignis/Scene/YAMLMathCodecs.h"

namespace Ignis::Reflect
{{

{fn_block}
}} // namespace Ignis::Reflect
"""

    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists() and output.read_text() == content:
        return True
    output.write_text(content)
    return True


def emit_scene_registry(all_schemas: dict[Path, dict], output: Path,
                        scan_dirs: list[Path], output_dir: Path) -> None:
    """Generate SceneRegistry.gen.h — the master bootstrap include."""
    seen_includes: dict[str, None] = {}
    registrations: list[str] = []

    for path, schema in sorted(all_schemas.items(), key=lambda kv: kv[0].stem):
        for type_name, info in schema.items():
            if info.get('metaclass') == 'Component':
                gen_rel = _output_path_rel(path, scan_dirs, output_dir)
                seen_includes[f'#include "{gen_rel}"'] = None
                registrations.append(f'    register_{type_name}();')

    if not seen_includes:
        return

    inc_block  = '\n'.join(seen_includes.keys())
    reg_block  = '\n'.join(registrations)

    content = f"""\
#pragma once
// Generated by IHT — do not edit
{inc_block}

namespace Ignis::Reflect
{{

inline void register_all_scene_components()
{{
{reg_block}

    IG_ASSERT(ComponentRegistry::validate_unique_ids(),
              "Duplicate component type_id detected — FNV-1a hash collision in IHT-generated descriptors");
}}

}} // namespace Ignis::Reflect
"""
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists() and output.read_text() == content:
        return
    output.write_text(content)


def emit_placeholder(source: Path, output: Path) -> None:
    _emit_placeholder(source, output)


def _emit_placeholder(source: Path, output: Path) -> None:
    content = (
        f"#pragma once\n"
        f"// IHT placeholder — {source.stem}.gen.h\n"
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists() and output.read_text() == content:
        return
    output.write_text(content)


def _source_include(source: Path) -> str:
    """Return include path relative to engine/src (e.g. 'Ignis/Scene/IDComponent.h')."""
    parts = source.parts
    for i, part in enumerate(parts):
        if part == 'src' and i + 1 < len(parts) and parts[i + 1] == 'Ignis':
            return '/'.join(parts[i + 1:])
    return source.name


def _output_path_rel(path: Path, scan_dirs: list[Path], output_dir: Path) -> str:
    """Return include path relative to output_dir (e.g. 'Ignis/Scene/IDComponent.gen.h')."""
    for scan_dir in scan_dirs:
        if path.is_relative_to(scan_dir):
            rel = path.relative_to(scan_dir)
            out = output_dir / rel.parent / (path.stem + '.gen.h')
            return str(out.relative_to(output_dir))
    return path.stem + '.gen.h'
