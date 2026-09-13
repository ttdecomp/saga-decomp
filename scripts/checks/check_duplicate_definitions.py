#!/usr/bin/env python3
"""Repository check for duplicate struct/class/union/enum definitions,
and (optionally, when given a build directory) duplicate object symbols.

A type is a DEFINITION (as opposed to a forward declaration) when its tag is
followed by a body, e.g.:

    struct Foo { ... };              -- definition
    struct Foo {};                   -- definition (empty placeholder)
    struct Foo { };                  -- definition (empty placeholder)
    typedef struct Foo { ... } Foo;  -- definition
    typedef struct { ... } Foo;      -- definition (anonymous, name = alias)
    struct Foo;                      -- forward declaration (ONLY thing ignored)

An empty body (``struct Foo {};``) is a closed definition, not a declaration:
two of them in different files are a duplicate definition (a TU including both
fails with a redefinition), and one shadowing a real ``struct Foo {...}`` is a
hazard. Only a true forward declaration (``struct Foo;``) is ignored.

Defining the same tag in two different translation units is not a compiler
error (they never see each other), but it is a maintenance hazard: a future
file that includes both definitions will fail to compile with an ODR
redefinition, and an opaque placeholder shadowing a real type hides the
canonical definition. This script flags every tag that has more than one
definition anywhere in the tree, so those duplicates can be consolidated.

When a build directory is supplied (``--build``), the script also runs the
archiver ``nm`` over every compiled object file and reports any *defined*
symbol (local and global, text and data) that is present in more than one
object. The linker already rejects duplicate *global* strong symbols, but it
silently accepts duplicate *local/static* symbols (they are private to each
translation unit) -- those are the ones a source scan cannot see and a link
never complains about. Weak symbols (inline functions / templates) are
excluded, since multiple weak definitions are normal and intended.

Exit code is 0 when no duplicate definitions are found, 1 otherwise.
"""

import argparse
import os
import re
import subprocess
import sys

from scripts.lib.ndk_tools import find_ndk_tool

SRC_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"}

# Strong (allocated) symbol types we consider real definitions. Weak (W/V)
# and global (G) are excluded because multiple weak definitions (inline
# functions, templates, COMDAT) are normal; undefined (U), debug (N), absolute
# (A) and special (I) are not definitions.
_SYMBOL_TYPES = set("TtDdBbRrCcSs")

# Tag kinds and their keywords. enum needs care: "enum Foo : int { ... }".
KEYWORDS = ("struct", "class", "union")

# Matches "typedef struct Foo {" or "typedef struct Foo Foo;" (named typedef).
_TYPEDEF_OPEN = re.compile(r"\btypedef\s+(struct|class|union|enum)\b")

# Matches a type/namespace scope opener that opens a body with '{':
#   struct Foo { | class Foo : public Bar { | union Foo { | enum Foo { |
#   namespace Foo { | namespace Foo::Bar { | namespace { | enum class Foo {
# SAGA_HOST_PACKED_ALIGN4 is an annotation between the keyword and tag.
# The tag is captured; it may be empty for an anonymous namespace.
_SCOPE_OPEN = re.compile(
    r"\b(?P<kw>namespace|struct|class|union|enum)\b"
    r"\s*(?:\bclass\b\s*)?"
    r"(?:SAGA_HOST_PACKED_ALIGN4\s*)?"
    r"(?P<tag>[A-Za-z_][A-Za-z0-9_:]*|)"
    r"\s*(?::\s*[^{;]*)?\s*\{"
)

# Keywords that open a scope we track (tag lookup is by struct/class/union/enum;
# namespace is tracked for qualification only).
_TYPE_KW = {"struct", "class", "union", "enum"}


def strip_comments(text):
    """Remove // and /* */ comments and string literals (naive but adequate
    for scanning tag definitions; does not touch real code)."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "/" and nxt == "/":
            end = text.find("\n", i)
            i = end if end != -1 else n
        elif c == "/" and nxt == "*":
            end = text.find("*/", i + 2)
            i = end + 2 if end != -1 else n
        elif c == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    break
                j += 1
            out.append(" " * (j - i + 1))
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def find_definitions(text):
    """Yield (kind, qualified_tag, is_empty) for every definition found.

    ``is_empty`` is True when the body is empty (``struct X {};``), which is an
    empty placeholder definition. Only a forward declaration (``struct X;``) is
    not yielded. Tracks namespace/class/union/enum scope via brace depth so
    that, e.g. ``NuMemoryPool::IVisitor`` and ``NuMemoryManager::IVisitor`` are
    treated as distinct types instead of a duplicate.
    """
    text = strip_comments(text)
    n = len(text)
    depth = 0  # total brace depth
    # stack of (tag, close_depth) for each opened type/namespace scope
    scopes = []  # list of path components
    closes = []  # parallel: depth at which that scope's body closes
    defs = []
    i = 0
    while i < n:
        c = text[i]
        if c == "{":
            depth += 1
            i += 1
            continue
        if c == "}":
            depth -= 1
            # pop any scopes whose body closes at this depth
            while closes and closes[-1] == depth:
                closes.pop()
                scopes.pop()
            i += 1
            continue
        if c == ";" or c.isspace():
            i += 1
            continue

        m = _SCOPE_OPEN.match(text, i)
        if m:
            kw = m.group("kw")
            tag = m.group("tag")
            i = m.end()
            if kw in _TYPE_KW and tag:
                # Scan the body between '{' and its matching '}'. An empty body
                # (``struct X {};``) is an opaque placeholder: it is a closed
                # definition, not a forward declaration, so it can still clash
                # with a real definition -- report it as a placeholder.
                j = m.end()
                bd = 1
                while j < n:
                    if text[j] == "{":
                        bd += 1
                    elif text[j] == "}":
                        bd -= 1
                        if bd == 0:
                            break
                    j += 1
                qname = "::".join(scopes + [tag])
                defs.append((kw, qname, text[m.end() : j].strip() == ""))
                scopes.append(tag)
                closes.append(depth)  # this scope closes when depth returns here
            elif kw == "namespace":
                # anonymous namespace has an empty tag
                scopes.append(tag if tag else "(anonymous)")
                closes.append(depth)
            else:
                # a type with no tag. For an anonymous ``typedef struct {...} N;``
                # the trailing typedef alias is the real name -- record it so a
                # ``struct N {...}`` definition elsewhere is caught as a dup.
                if kw in _TYPE_KW:
                    prefix = text[max(0, m.start() - 16) : m.start()]
                    if re.search(r"\btypedef\s*$", prefix):
                        j = m.end()
                        bd = 1
                        while j < n:
                            if text[j] == "{":
                                bd += 1
                            elif text[j] == "}":
                                bd -= 1
                                if bd == 0:
                                    break
                            j += 1
                        k = j + 1
                        tail = ""
                        while k < n and text[k] != ";":
                            tail += text[k]
                            k += 1
                        for alias in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", tail):
                            defs.append(
                                (kw, "::".join(scopes + [alias]), text[m.end() : j].strip() == "")
                            )
                # keep scopes/closes parallel so pop stays in sync
                scopes.append("(anon)")
                closes.append(depth)
            depth += 1  # we just consumed '{'
            continue

        i += 1
    return defs


def is_noise_symbol(name):
    """True for symbols that are legitimately defined once per translation unit
    and are not meaningful duplicates:
      * assembler-internal labels (.L##, .LC## constant pools) -- C identifiers
        cannot start with '.';
      * compiler-generated PIC thunks (__x86.get_pc_thunk.*);
      * static-init/destruction trampolines and tcf cleanups.
    """
    if name.startswith(".L"):
        return True
    if "__x86.get_pc_thunk." in name:
        return True
    if "__static_initialization_and_destruction_" in name:
        return True
    if name.startswith("_GLOBAL__sub_I_") or name.startswith("_GLOBAL__sub_D_"):
        return True
    if "__tcf_" in name:
        return True
    return False


def find_nm():
    """Locate nm, preferring the $NM override."""
    return find_ndk_tool("nm")


def collect_defined_symbols(object_file, nm):
    """Return the set of defined symbol names (strong, non-weak) in ``object_file``."""
    try:
        proc = subprocess.run(
            [nm, "--defined-only", object_file],
            capture_output=True,
            text=True,
            errors="replace",
        )
    except OSError as e:
        print(f"warning: cannot run nm on {object_file}: {e}", file=sys.stderr)
        return set()
    if proc.returncode != 0:
        print(f"warning: nm failed on {object_file}", file=sys.stderr)
        return set()
    out = set()
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[-2] in _SYMBOL_TYPES:
            name = parts[-1]
            if is_noise_symbol(name):
                continue
            out.add(name)
    return out


def check_duplicate_symbols(build_dir, nm):
    """Find symbols defined in more than one object file under ``build_dir``.

    Returns (symbol -> [object_file, ...]) for every duplicated symbol.
    """
    objects = []
    for dirpath, _dirnames, filenames in os.walk(build_dir):
        # Skip third-party external builds and compiler probes.
        if os.sep + "external" + os.sep in dirpath + os.sep:
            continue
        if "CompilerId" in dirpath:
            continue
        for fn in filenames:
            if fn.endswith(".o"):
                objects.append(os.path.join(dirpath, fn))
    if not objects:
        return {}

    symbol_to_obj = {}
    for obj in objects:
        for sym in collect_defined_symbols(obj, nm):
            symbol_to_obj.setdefault(sym, set()).add(obj)

    return {sym: objs for sym, objs in symbol_to_obj.items() if len(objs) > 1}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "src",
        nargs="?",
        default="src",
        help="source root directory to scan (default: src/)",
    )
    ap.add_argument(
        "--build",
        default=None,
        help="build directory to scan for duplicate symbols in compiled objects "
        "(default: 'build' if present)",
    )
    ap.add_argument(
        "--verbose",
        action="store_true",
        help="print every definition found, not just duplicates",
    )
    ap.add_argument(
        "--fail-on-symbols",
        action="store_true",
        help="also return a non-zero exit code when duplicate object symbols are "
        "found (by default they are reported but not treated as an error, since "
        "static-inline/static-const header functions are legitimately repeated "
        "in every translation unit)",
    )
    args = ap.parse_args()

    invocation_dir = os.environ.get("BUILD_WORKING_DIRECTORY", os.getcwd())

    def argument_path(value):
        return value if os.path.isabs(value) else os.path.join(invocation_dir, value)

    root = os.path.abspath(argument_path(args.src))
    if not os.path.isdir(root):
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2

    # tag -> dict(file -> count)
    by_tag = {}
    total = 0
    empty = {}  # tag -> True if any definition has an empty body
    for dirpath, _dirnames, filenames in os.walk(root):
        for fn in filenames:
            ext = os.path.splitext(fn)[1].lower()
            if ext not in SRC_EXTENSIONS:
                continue
            path = os.path.join(dirpath, fn)
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as fh:
                    text = fh.read()
            except OSError as e:
                print(f"warning: cannot read {path}: {e}", file=sys.stderr)
                continue
            defs = find_definitions(text)
            for kw, tag, is_empty in defs:
                total += 1
                key = (kw, tag)
                if is_empty:
                    empty[key] = True
                entry = by_tag.setdefault(key, {})
                entry[path] = entry.get(path, 0) + 1

    if args.verbose:
        print(f"scanned {total} definitions\n")

    duplicates = {
        key: files for key, files in by_tag.items() if sum(files.values()) > 1
    }

    code = 0

    if duplicates:
        code = 1
        print("Found type(s) defined in more than one place:\n")
        for kw, tag in sorted(duplicates, key=lambda k: k[1]):
            files = duplicates[(kw, tag)]
            n = sum(files.values())
            empty_mark = "  [empty placeholder(s)]" if (kw, tag) in empty else ""
            print(f"  {kw} {tag}  ({n} definitions){empty_mark}")
            for path, count in sorted(files.items()):
                rel = os.path.relpath(path, os.path.dirname(root))
                print(f"      {rel}:{count}")
            print()

    # Duplicate object symbols (local + global, text + data).
    build_dir = args.build
    if build_dir is None:
        default_build = os.path.join(os.path.dirname(root), "build")
        if os.path.isdir(default_build):
            build_dir = default_build
    if build_dir is not None:
        build_dir = os.path.abspath(argument_path(build_dir))
        if not os.path.isdir(build_dir):
            print(f"error: --build is not a directory: {build_dir}", file=sys.stderr)
            return 2
        nm = find_nm()
        if nm is None:
            print("warning: could not find NDK nm; skipping duplicate-symbol check",
                  file=sys.stderr)
        else:
            dup_symbols = check_duplicate_symbols(build_dir, nm)
            if dup_symbols:
                if args.fail_on_symbols:
                    code = 1
                print("\nFound symbol(s) defined in more than one object file:\n")
                for sym in sorted(dup_symbols):
                    objs = sorted(dup_symbols[sym])
                    print(f"  {sym}  ({len(objs)} objects)")
                    for obj in objs:
                        rel = os.path.relpath(obj, os.path.dirname(build_dir))
                        print(f"      {rel}")
                    print()
                print(
                    "note: symbols repeated across many objects are usually "
                    "static-inline/static-const header functions (expected per "
                    "translation unit); a symbol in just two different .cpp "
                    "objects is a real duplicate definition."
                )

    if code == 0:
        print("OK: no duplicate definitions found")
    return code


if __name__ == "__main__":
    sys.exit(main())
