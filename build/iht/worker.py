from __future__ import annotations

import os
from pathlib import Path

from . import generator, scanner
from .manifest import FileManifest


def _output_path(path: Path, scan_dirs: list[Path], output_dir: Path) -> Path:
    for scan_dir in scan_dirs:
        if path.is_relative_to(scan_dir):
            rel = path.relative_to(scan_dir)
            return output_dir / rel.parent / (path.stem + ".gen.h")
    return output_dir / (path.stem + ".gen.h")


def run(
    project_root: Path,
    scan_dirs: list[Path],
    output_dir: Path,
    manifest_path: Path,
) -> None:
    manifest = FileManifest(manifest_path)
    manifest.load()

    headers: list[Path] = []
    for scan_dir in scan_dirs:
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

    # Scan all headers to build the complete component list for SceneRegistry.
    all_schemas: dict[Path, dict] = {}
    for path in headers:
        all_schemas[path] = scanner.scan(path)

    for path in changed:
        schema = all_schemas.get(path, {})
        out    = _output_path(path, scan_dirs, output_dir)
        emitted = generator.emit(path, schema, out)
        if emitted:
            manifest.update(path, project_root, [str(out.relative_to(project_root))])
        else:
            # No annotations — delete any stale .gen.h from a previous run
            if out.exists():
                out.unlink()
            manifest.update(path, project_root, [])

    registry_out = output_dir / "Ignis" / "Scene" / "SceneRegistry.gen.h"
    registry_stale = changed or removed or not registry_out.exists()
    if registry_stale:
        generator.emit_scene_registry(all_schemas, registry_out, scan_dirs, output_dir)

    if changed or removed or mtime_updated:
        manifest.save()

    if changed or removed:
        print(f"IHT: {len(changed)} changed, {len(removed)} removed → {output_dir}")
    else:
        print("IHT: no changes")
