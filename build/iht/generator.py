from __future__ import annotations

from pathlib import Path

_PLACEHOLDER = (
    "#pragma once\n"
    "// IHT placeholder — {stem}.gen.h\n"
    "// Code generation pending EnTT reflection integration.\n"
)


def emit_placeholder(source: Path, output: Path) -> None:
    content = _PLACEHOLDER.format(stem=source.stem)
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists() and output.read_text() == content:
        return
    output.write_text(content)
