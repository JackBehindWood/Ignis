from __future__ import annotations

import re
from enum import Enum, auto
from pathlib import Path


class _State(Enum):
    IDLE        = auto()
    AWAIT_TYPE  = auto()
    IN_CLASS    = auto()
    AWAIT_FIELD = auto()
    AWAIT_FUNC  = auto()


_RE_IG_CLASS    = re.compile(r'^\s*IG_CLASS\(([^)]*)\)')
_RE_IG_PROPERTY = re.compile(r'^\s*IG_PROPERTY\(([^)]*)\)')
_RE_IG_FUNCTION = re.compile(r'^\s*IG_FUNCTION\(([^)]*)\)')
_RE_TYPE_DECL   = re.compile(r'^(?:struct|class)\s+(\w+)')
# Group 1 = type token (may contain template brackets / qualifiers)
# Group 2 = field identifier
_RE_FIELD       = re.compile(r'^\s*(\w[\w:<>*& ]*?)\s+(\w+)\s*[=;{]')


def scan(path: Path) -> dict[str, dict]:
    """Return {TypeName: {metaclass, properties, functions}} for all IG_CLASS-annotated types."""
    result: dict[str, dict] = {}
    state       = _State.IDLE
    current_type: str | None = None
    pending_prop: str | None = None
    pending_func: str | None = None
    class_depth = 0

    try:
        lines = path.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return result

    for raw_line in lines:
        line    = raw_line.rstrip()
        stripped = line.strip()

        if state == _State.IDLE:
            m = _RE_IG_CLASS.match(line)
            if m:
                metaclass    = m.group(1).strip().strip('"')
                state        = _State.AWAIT_TYPE
                current_type = None
                _pending_metaclass = metaclass

        elif state == _State.AWAIT_TYPE:
            if not stripped:
                continue
            m = _RE_TYPE_DECL.match(line)
            if m:
                current_type = m.group(1)
                result[current_type] = {
                    'metaclass':  _pending_metaclass,
                    'properties': [],
                    'functions':  [],
                }
                class_depth = 0
                state       = _State.IN_CLASS
            else:
                state = _State.IDLE

        elif state == _State.IN_CLASS:
            # IG macros must appear at col 0; check before brace accounting
            m = _RE_IG_PROPERTY.match(line)
            if m:
                pending_prop = m.group(1).strip()
                state        = _State.AWAIT_FIELD
                continue

            m = _RE_IG_FUNCTION.match(line)
            if m:
                pending_func = m.group(1).strip()
                state        = _State.AWAIT_FUNC
                continue

            opens  = line.count('{')
            closes = line.count('}')
            class_depth += opens - closes

            if class_depth <= 0 and closes > 0:
                state        = _State.IDLE
                current_type = None
                class_depth  = 0

        elif state == _State.AWAIT_FIELD:
            if not stripped:
                continue
            # IG macros at col 0 mean the previous IG_PROPERTY had no following field — skip
            if _RE_IG_CLASS.match(line) or _RE_IG_PROPERTY.match(line) or _RE_IG_FUNCTION.match(line):
                state        = _State.IN_CLASS
                pending_prop = None
                # re-evaluate this line as IN_CLASS on next iteration would require goto;
                # instead handle the IG macros here explicitly
                m = _RE_IG_PROPERTY.match(line)
                if m:
                    pending_prop = m.group(1).strip()
                    state        = _State.AWAIT_FIELD
                m = _RE_IG_FUNCTION.match(line)
                if m:
                    pending_func = m.group(1).strip()
                    state        = _State.AWAIT_FUNC
                continue

            m = _RE_FIELD.match(line)
            if m:
                field_type = m.group(1).strip()
                field_name = m.group(2).strip()
                result[current_type]['properties'].append({
                    'prop_name':  pending_prop,
                    'field_type': field_type,
                    'field_name': field_name,
                })
            # Whether we matched or not, return to IN_CLASS
            state        = _State.IN_CLASS
            pending_prop = None

        elif state == _State.AWAIT_FUNC:
            if not stripped:
                continue
            result[current_type]['functions'].append(pending_func)
            state        = _State.IN_CLASS
            pending_func = None

    return result
