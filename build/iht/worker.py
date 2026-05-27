from __future__ import annotations

import os
from pathlib import Path

from .generator import emit_placeholder
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

    for path in changed:
        out = _output_path(path, scan_dirs, output_dir)
        emit_placeholder(path, out)
        manifest.update(path, project_root, [str(out.relative_to(project_root))])

    if changed or removed or mtime_updated:
        manifest.save()

    if changed or removed:
        print(f"IHT: {len(changed)} changed, {len(removed)} removed → {output_dir}")
    else:
        print("IHT: no changes")
