from __future__ import annotations

import re
import sys
from enum import Enum, auto
from pathlib import Path


class IHTParseError(Exception):
    pass


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
_RE_NAMESPACE   = re.compile(r'^\s*namespace\s+([\w:]+)')

_QUALIFIERS = frozenset({'const', 'static', 'mutable', 'volatile', 'inline'})


def _parse_specifiers(raw: str) -> list[str]:
    return [s.strip() for s in raw.split(',') if s.strip()]


def _parse_field_decl(line: str) -> tuple[str, str] | None:
    """Parse a C++ field declaration into (type_string, field_name).

    Returns None if the declaration is not parseable (fewer than 2 tokens).
    Raises IHTParseError for a parenthesized initializer.
    """
    stripped = line.lstrip()
    if not stripped:
        return None

    # Step 1: detect parenthesized initializer — scan for ( before = { ; at <> depth 0
    angle_depth = 0
    for ch in stripped:
        if ch == '<':
            angle_depth += 1
        elif ch == '>':
            angle_depth -= 1
        elif angle_depth == 0:
            if ch == '(':
                raise IHTParseError('parenthesized field initializer')
            if ch in ('=', '{', ';'):
                break

    # Step 2: strip initializer — walk tracking <> depth, truncate at = { ; at depth 0
    decl_chars: list[str] = []
    angle_depth = 0
    for ch in stripped:
        if ch == '<':
            angle_depth += 1
            decl_chars.append(ch)
        elif ch == '>':
            angle_depth -= 1
            decl_chars.append(ch)
        elif angle_depth == 0 and ch in ('=', '{', ';'):
            break
        else:
            decl_chars.append(ch)

    decl = ''.join(decl_chars).strip()

    # Step 3: strip leading storage qualifiers
    words = decl.split()
    while words and words[0] in _QUALIFIERS:
        words.pop(0)
    decl = ' '.join(words)

    # Step 4: bracket-aware token split — split on whitespace only at <> depth 0
    tokens: list[str] = []
    current: list[str] = []
    angle_depth = 0
    for ch in decl:
        if ch == '<':
            angle_depth += 1
            current.append(ch)
        elif ch == '>':
            angle_depth -= 1
            current.append(ch)
        elif ch == ' ' and angle_depth == 0:
            tok = ''.join(current).strip()
            if tok:
                tokens.append(tok)
            current = []
        else:
            current.append(ch)
    tok = ''.join(current).strip()
    if tok:
        tokens.append(tok)

    if len(tokens) < 2:
        return None

    # Step 5: pointer/reference stripping from last token (the candidate identifier)
    identifier = tokens[-1]
    ptr_chars  = ''
    while identifier and identifier[0] in ('*', '&'):
        ptr_chars  += identifier[0]
        identifier  = identifier[1:]

    if not identifier:
        return None

    # Step 6: build type string — preceding tokens + any stripped pointer/ref chars
    type_str = ' '.join(tokens[:-1])
    if ptr_chars:
        type_str = type_str + ptr_chars

    return type_str.strip(), identifier


def scan(path: Path) -> dict[str, dict]:
    """Return {TypeName: {qualified_name, metaclass, properties, functions}} for all IG_CLASS types."""
    result: dict[str, dict]            = {}
    state                              = _State.IDLE
    current_type: str | None           = None
    _pending_metaclass: str            = ''
    pending_prop_specifiers: list[str] = []
    pending_prop_line_no: int          = 0
    pending_func: str | None           = None
    class_entry_depth: int             = 0

    current_brace_depth: int                   = 0
    namespace_stack: list[tuple[str, int]]     = []
    pending_namespace: list[str]               = []

    try:
        lines = path.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError:
        return result

    for line_idx, raw_line in enumerate(lines):
        line_no  = line_idx + 1
        line     = raw_line.rstrip()
        stripped = line.strip()

        opens  = line.count('{')
        closes = line.count('}')

        # Namespace segments to push after brace count (same-line brace style)
        ns_push_this_line: list[str] = []

        # ------------------------------------------------------------------ #
        # State machine                                                        #
        # ------------------------------------------------------------------ #

        if state == _State.IDLE:
            m_ns = _RE_NAMESPACE.match(line)
            if m_ns and not stripped.startswith('//'):
                segments = [s for s in m_ns.group(1).split('::') if s]
                if '{' in line:
                    ns_push_this_line = segments
                else:
                    pending_namespace.extend(segments)

            m = _RE_IG_CLASS.match(line)
            if m:
                _pending_metaclass = m.group(1).strip().strip('"')
                state              = _State.AWAIT_TYPE
                current_type       = None

        elif state == _State.AWAIT_TYPE:
            if stripped:
                m = _RE_TYPE_DECL.match(line)
                if m:
                    current_type = m.group(1)
                    qname        = (
                        '::'.join(seg for seg, _ in namespace_stack) + '::' + current_type
                        if namespace_stack else current_type
                    )
                    result[current_type] = {
                        'qualified_name': qname,
                        'metaclass':      _pending_metaclass,
                        'properties':     [],
                        'functions':      [],
                    }
                    class_entry_depth = current_brace_depth
                    state             = _State.IN_CLASS
                else:
                    state = _State.IDLE

        elif state == _State.IN_CLASS:
            m = _RE_IG_PROPERTY.match(line)
            if m:
                pending_prop_specifiers = _parse_specifiers(m.group(1))
                pending_prop_line_no    = line_no
                state                   = _State.AWAIT_FIELD
            else:
                m = _RE_IG_FUNCTION.match(line)
                if m:
                    pending_func = m.group(1).strip()
                    state        = _State.AWAIT_FUNC

        elif state == _State.AWAIT_FIELD:
            if stripped:
                if (_RE_IG_CLASS.match(line) or
                        _RE_IG_PROPERTY.match(line) or
                        _RE_IG_FUNCTION.match(line)):
                    state                   = _State.IN_CLASS
                    pending_prop_specifiers = []
                    m = _RE_IG_PROPERTY.match(line)
                    if m:
                        pending_prop_specifiers = _parse_specifiers(m.group(1))
                        pending_prop_line_no    = line_no
                        state                   = _State.AWAIT_FIELD
                    else:
                        m = _RE_IG_FUNCTION.match(line)
                        if m:
                            pending_func = m.group(1).strip()
                            state        = _State.AWAIT_FUNC
                else:
                    try:
                        parsed = _parse_field_decl(line)
                    except IHTParseError:
                        msg = (
                            f"{path}:{pending_prop_line_no}: parenthesized initializer after "
                            f"IG_PROPERTY is prohibited — use '=' or '{{}}' initialization"
                        )
                        raise IHTParseError(msg)

                    if parsed is None:
                        msg = (
                            f"{path}:{pending_prop_line_no}: IG_PROPERTY — next line "
                            f"(line {line_no}) is not a parseable field declaration"
                        )
                        raise IHTParseError(msg)

                    field_type, field_name = parsed
                    result[current_type]['properties'].append({
                        'specifiers': pending_prop_specifiers,
                        'field_type': field_type,
                        'field_name': field_name,
                    })
                    state                   = _State.IN_CLASS
                    pending_prop_specifiers = []

        elif state == _State.AWAIT_FUNC:
            if stripped:
                result[current_type]['functions'].append(pending_func)
                pending_func = None
                state        = _State.IN_CLASS

        # ------------------------------------------------------------------ #
        # Brace depth update + namespace bookkeeping                          #
        # ------------------------------------------------------------------ #

        new_depth = current_brace_depth + opens - closes

        # Commit pending_namespace when we encounter a { (handles `namespace Foo\n{`)
        if opens > 0 and pending_namespace:
            for seg in pending_namespace:
                namespace_stack.append((seg, new_depth))
            pending_namespace.clear()

        # Push namespace segments declared with brace on the same line (`namespace Foo {`)
        for seg in ns_push_this_line:
            namespace_stack.append((seg, new_depth))

        current_brace_depth = new_depth

        # Pop namespace segments whose scope has closed
        while namespace_stack and namespace_stack[-1][1] > current_brace_depth:
            namespace_stack.pop()

        # IN_CLASS exit: struct/class body closed
        if state == _State.IN_CLASS and closes > 0 and current_brace_depth <= class_entry_depth:
            state        = _State.IDLE
            current_type = None

    return result
