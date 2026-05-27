from __future__ import annotations

import hashlib
import json
import os
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class FileRecord:
    mtime: float
    sha256: str
    outputs: list[str] = field(default_factory=list)


class FileManifest:
    VERSION = 1

    def __init__(self, manifest_path: Path) -> None:
        self._path = manifest_path
        self._records: dict[str, FileRecord] = {}

    def load(self) -> None:
        if not self._path.exists():
            return
        try:
            with open(self._path) as f:
                data = json.load(f)
            if data.get("version") != self.VERSION:
                return
            for rel, entry in data.get("files", {}).items():
                self._records[rel] = FileRecord(
                    mtime=entry["mtime"],
                    sha256=entry["sha256"],
                    outputs=entry.get("outputs", []),
                )
        except (json.JSONDecodeError, KeyError):
            self._records = {}

    def save(self) -> None:
        self._path.parent.mkdir(parents=True, exist_ok=True)
        data = {
            "version": self.VERSION,
            "files": {
                rel: {"mtime": rec.mtime, "sha256": rec.sha256, "outputs": rec.outputs}
                for rel, rec in self._records.items()
            },
        }
        tmp = self._path.with_suffix(".tmp")
        with open(tmp, "w") as f:
            json.dump(data, f, indent=2)
        os.replace(tmp, self._path)

    def diff(
        self, paths: list[Path], project_root: Path
    ) -> tuple[list[Path], list[Path], bool]:
        """Return (changed, removed, mtime_only_updated).

        mtime_only_updated is True when git-checkout-style mtime drift was
        corrected in-place — caller must still save() to persist those updates.
        """
        on_disk: dict[str, Path] = {
            str(p.relative_to(project_root)): p for p in paths
        }
        changed: list[Path] = []
        mtime_updated = False

        for rel, p in on_disk.items():
            rec = self._records.get(rel)
            stat = p.stat()
            if rec is None or stat.st_mtime != rec.mtime:
                if rec is not None and _sha256(p) == rec.sha256:
                    # mtime drifted (e.g. git checkout) but content is identical —
                    # update stored mtime only; do NOT rewrite .gen.h downstream
                    rec.mtime = stat.st_mtime
                    mtime_updated = True
                else:
                    changed.append(p)

        removed = [project_root / rel for rel in self._records if rel not in on_disk]
        return changed, removed, mtime_updated

    def update(self, path: Path, project_root: Path, outputs: list[str]) -> None:
        stat = path.stat()
        rel = str(path.relative_to(project_root))
        self._records[rel] = FileRecord(
            mtime=stat.st_mtime,
            sha256=_sha256(path),
            outputs=outputs,
        )

    def remove(self, path: Path, project_root: Path) -> None:
        self._records.pop(str(path.relative_to(project_root)), None)

    def get_outputs(self, path: Path, project_root: Path) -> list[str]:
        rec = self._records.get(str(path.relative_to(project_root)))
        return rec.outputs if rec else []


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()
