from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from pathlib import Path

from . import generator, scanner
from .manifest import FileManifest
from .scanner import IHTParseError


@dataclass
class ScanDomain:
    scan_dirs:        list[Path]
    output_dir:       Path
    registry_fn:      str
    registry_output:  Path
    metaclass_filter: str   # 'Component' or 'Script'


def _output_path(path: Path, scan_dirs: list[Path], output_dir: Path) -> Path:
    for scan_dir in scan_dirs:
        if path.is_relative_to(scan_dir):
            rel = path.relative_to(scan_dir)
            return output_dir / rel.parent / (path.stem + ".gen.h")
    return output_dir / (path.stem + ".gen.h")


def run(
    project_root:  Path,
    domains:       list[ScanDomain],
    manifest_path: Path,
) -> None:
    manifest = FileManifest(manifest_path)
    manifest.load()

    needs_save  = False
    any_changes = False

    for domain in domains:
        headers: list[Path] = []
        for scan_dir in domain.scan_dirs:
            for dirpath, _, filenames in os.walk(scan_dir):
                for name in filenames:
                    if name.endswith(".h"):
                        headers.append(Path(dirpath) / name)

        changed, removed, mtime_updated = manifest.diff(headers, project_root)

        for path in removed:
            for out_rel in manifest.get_outputs(path, project_root):
                out = project_root / out_rel
                if out.exists():
                    out.unlink()
            manifest.remove(path, project_root)

        changed_set: set[Path] = set(changed)
        all_schemas: dict[Path, dict] = {}

        for path in headers:
            if path in changed_set:
                try:
                    schema = scanner.scan(path)
                except IHTParseError as exc:
                    print(f"IHT error: {exc}", file=sys.stderr)
                    sys.exit(1)
                manifest.set_schema(path, project_root, schema)
            else:
                schema = manifest.get_schema(path, project_root)
            all_schemas[path] = schema

        for path in changed:
            schema  = all_schemas[path]
            out     = _output_path(path, domain.scan_dirs, domain.output_dir)
            emitted = generator.emit(path, schema, out)
            if emitted:
                manifest.update(path, project_root, [str(out.relative_to(project_root))])
            else:
                if out.exists():
                    out.unlink()
                manifest.update(path, project_root, [])

        current_names = sorted(
            name
            for schema in all_schemas.values()
            for name, info in schema.items()
            if info.get('metaclass') == domain.metaclass_filter
        )
        stored_names = manifest.registry_components.get(domain.registry_fn)

        registry_stale = (current_names != stored_names or not domain.registry_output.exists())

        if registry_stale:
            if domain.metaclass_filter == 'Script':
                generator.emit_script_registry_cpp(
                    all_schemas, domain.registry_output,
                    domain.scan_dirs, domain.output_dir,
                    domain.registry_fn,
                )
            else:
                generator.emit_scene_registry_cpp(
                    all_schemas, domain.registry_output,
                    domain.scan_dirs, domain.output_dir,
                    domain.registry_fn,
                )
            manifest.registry_components[domain.registry_fn] = current_names
            needs_save = True

        if changed or removed or mtime_updated:
            needs_save = True

        if changed or removed:
            any_changes = True
            print(f"IHT [{domain.registry_fn}]: {len(changed)} changed, {len(removed)} removed → {domain.output_dir}")

    if not any_changes:
        print("IHT: no changes")

    if needs_save:
        manifest.save()
