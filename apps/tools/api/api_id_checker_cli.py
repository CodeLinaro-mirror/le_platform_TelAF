# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. 
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os
import re
import sys
import argparse
import codecs

# ---------------------------------------------------------------------
# Phase 1: mask comments and string literals by replacing with spaces
#          (preserving length) so we do not match inside them.
# Phase 2: single-pass regex over the masked text:
#   - match up to the parameter closing ')'
#   - allow whitespace (masked from comments/strings)
#   - either '=' + IDs + optional whitespace + ';'
#     or direct ';' (missing '=').
# Rebuild replacements using ORIGINAL content slices to preserve formatting.
# New option: --reset-ids : force re-numbering from 0 per file,
#   ignoring existing numbers entirely.
# ---------------------------------------------------------------------

BLOCK_PATTERN = re.compile(
    r'(?P<type>EVENT|FUNCTION)\s+.*?\)'      # up to and including the parameter ')'
    r'(?P<gap>\s*)'                          # whitespace after ')'
    r'(?:='                                  # with '=' case
    r'\s*'
    r'(?:(?P<ids_paren>\((?P<ids_inner>[^)]*?)\))|(?P<id_single>\d+))'
    r'(?P<gap2>\s*)'
    r';'
    r'|'
    r'(?P<semi>;))',                         # or missing '=': direct ';'
    re.DOTALL
)

def mask_comments_and_strings(text):
    """Replace characters in comments (//, /*...*/) and string literals with spaces, preserving length."""
    n = len(text)
    out = list(text)

    i = 0
    IN_NONE, IN_LINE, IN_BLOCK, IN_STRING = 0, 1, 2, 3
    state = IN_NONE
    string_quote = None

    while i < n:
        ch = text[i]
        nxt = text[i+1] if i+1 < n else ''

        if state == IN_NONE:
            if ch == '/' and nxt == '/':
                out[i] = ' '; out[i+1] = ' '
                i += 2; state = IN_LINE; continue
            if ch == '/' and nxt == '*':
                out[i] = ' '; out[i+1] = ' '
                i += 2; state = IN_BLOCK; continue
            if ch == '"':
                out[i] = ' '
                i += 1; state = IN_STRING; string_quote = '"'; continue
            i += 1
            continue

        if state == IN_LINE:
            if ch in ('\n', '\r'):
                i += 1; state = IN_NONE
            else:
                out[i] = ' '; i += 1
            continue

        if state == IN_BLOCK:
            if ch == '*' and nxt == '/':
                out[i] = ' '; out[i+1] = ' '
                i += 2; state = IN_NONE
            else:
                if ch not in ('\n', '\r'):
                    out[i] = ' '
                i += 1
            continue

        if state == IN_STRING:
            if ch == '\\':
                out[i] = ' '
                if i+1 < n:
                    if out[i+1] not in ('\n', '\r'):
                        out[i+1] = ' '
                    i += 2
                else:
                    i += 1
                continue
            if ch == string_quote:
                out[i] = ' '
                i += 1; state = IN_NONE; string_quote = None; continue
            if ch not in ('\n', '\r'):
                out[i] = ' '
            i += 1
            continue

    return ''.join(out)

def parse_ids(text):
    """Extract integer IDs from arbitrary text."""
    if not text:
        return []
    return [int(x) for x in re.findall(r'\d+', text)]

def fix_ids_in_content(content, reset_ids=False, start_id=0):
    safe = mask_comments_and_strings(content)

    used_ids = set()
    next_id = int(start_id)
    out_parts = []
    last = 0
    modified = False

    for m in BLOCK_PATTERN.finditer(safe):
        start, end = m.start(), m.end()

        typ = m.group('type')
        required = 2 if typ == 'EVENT' else 1

        prefix_start = start
        prefix_end = m.start('gap')
        prefix_orig = content[prefix_start:prefix_end]
        gap_orig = content[m.start('gap'):m.end('gap')]

        has_eq = (m.group('ids_paren') is not None) or (m.group('id_single') is not None)

        out_parts.append(content[last:start])

        if reset_ids:
            new_ids = [next_id + i for i in range(required)]
            next_id += required
            for i in new_ids:
                used_ids.add(i)

            if typ == 'EVENT':
                id_text = '(' + ','.join(str(x) for x in new_ids) + ')'
            else:
                has_paren_style = (m.group('ids_paren') is not None)
                id_text = '(' + ','.join(str(x) for x in new_ids) + ')' if has_paren_style else str(new_ids[0])

            if has_eq:
                gap2_orig = content[m.start('gap2'):m.end('gap2')] if m.group('gap2') is not None else ''
                new_seg = prefix_orig + gap_orig + '=' + id_text + gap2_orig + ';'
            else:
                semi_orig = content[m.start('semi'):m.end('semi')]
                new_seg = prefix_orig + " = " + id_text + gap_orig + semi_orig

            out_parts.append(new_seg)
            modified = True
        else:
            if has_eq:
                ids_inner = m.group('ids_inner')
                id_single = m.group('id_single')
                gap2_orig = content[m.start('gap2'):m.end('gap2')] if m.group('gap2') is not None else ''

                if ids_inner is not None:
                    existing = parse_ids(ids_inner)
                    has_paren_style = True
                elif id_single is not None:
                    existing = parse_ids(id_single)
                    has_paren_style = False
                else:
                    existing = []
                    has_paren_style = False

                is_valid = (
                    len(existing) == required and
                    len(set(existing)) == required and
                    all(i not in used_ids for i in existing)
                )

                if is_valid:
                    for i in existing:
                        used_ids.add(i)
                    out_parts.append(content[start:end])
                else:
                    new_ids = []
                    while len(new_ids) < required:
                        if next_id not in used_ids:
                            new_ids.append(next_id)
                            used_ids.add(next_id)
                        next_id += 1

                    if typ == 'EVENT':
                        id_text = '(' + ','.join(str(x) for x in new_ids) + ')'
                    else:
                        id_text = '(' + ','.join(str(x) for x in new_ids) + ')' if has_paren_style else str(new_ids[0])

                    new_seg = prefix_orig + gap_orig + '=' + id_text + gap2_orig + ';'
                    out_parts.append(new_seg)
                    modified = True
            else:
                semi_orig = content[m.start('semi'):m.end('semi')]
                new_ids = []
                while len(new_ids) < required:
                    if next_id not in used_ids:
                        new_ids.append(next_id)
                        used_ids.add(next_id)
                    next_id += 1

                if typ == 'EVENT':
                    id_text = " = (" + ','.join(str(x) for x in new_ids) + ")"
                else:
                    id_text = " = " + str(new_ids[0])

                new_seg = prefix_orig + id_text + gap_orig + semi_orig
                out_parts.append(new_seg)
                modified = True

        last = end

    out_parts.append(content[last:])
    return ''.join(out_parts), modified

def _read_text_best_effort(path):
    for enc in ('utf-8', 'utf-8-sig', 'latin-1'):
        try:
            with codecs.open(path, 'r', encoding=enc) as f:
                return f.read(), enc
        except Exception:
            pass
    with open(path, 'rb') as f:
        raw = f.read()
    try:
        return raw.decode('utf-8'), 'utf-8'
    except Exception:
        return raw.decode('utf-8', 'ignore'), 'utf-8'

def _write_text(path, text, encoding):
    with codecs.open(path, 'w', encoding=encoding) as f:
        f.write(text)

def insert_version_headers(text):
    if 'API_VERSION' in text and 'SERVING_VERSION' in text:
        return text, False

    copyright_pattern = re.compile(r'/\*.*?Copyright.*?\*/', re.DOTALL)
    match = copyright_pattern.search(text)
    insert_pos = match.end() if match else 0

    version_text = '\nAPI_VERSION = 1;\nSERVING_VERSION = 1;\n'
    new_text = text[:insert_pos] + version_text + text[insert_pos:]
    return new_text, True

def process_file(path, make_backup, dry_run, reset_ids, start_id):
    text, enc = _read_text_best_effort(path)
    updated_text, version_modified = insert_version_headers(text)
    fixed_text, id_modified = fix_ids_in_content(updated_text, reset_ids=reset_ids, start_id=start_id)

    if version_modified or id_modified:
        if dry_run:
            print("[DRY]  would update: %s" % path)
            return True
        if make_backup:
            bak = path + '.bak'
            if not os.path.exists(bak):
                _write_text(bak, text, enc)
        _write_text(path, fixed_text, enc)
        print("[UPDATED] %s" % path)
        return True
    else:
        print("[OK]      %s" % path)
        return False

def scan_path(root, no_backup=False, dry_run=False, reset_ids=False, start_id=0):
    modified = 0
    total = 0

    if os.path.isfile(root) and root.endswith('.api'):
        total = 1
        if process_file(root, make_backup=(not no_backup), dry_run=dry_run,
                        reset_ids=reset_ids, start_id=start_id):
            modified += 1
    elif os.path.isdir(root):
        for dirpath, _, files in os.walk(root):
            for fn in files:
                if fn.endswith('.api'):
                    total += 1
                    fp = os.path.join(dirpath, fn)
                    if process_file(fp, make_backup=(not no_backup), dry_run=dry_run,
                                    reset_ids=reset_ids, start_id=start_id):
                        modified += 1
    else:
        print("Invalid path: %s" % root)
        return

    print("\n==== Summary ====")
    print("Files scanned : %d" % total)
    print("Files updated : %d" % modified)

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Check/fix or reset IDs for EVENT/FUNCTION in .api files (per-file uniqueness).\n"
            "- EVENT: two IDs; FUNCTION: one ID.\n"
            "- IDs are unique only within each file.\n"
            "- Use --reset-ids to renumber from 0 (or --start-id N) ignoring existing numbers.\n"
            "- If '=' is missing, it will be inserted right after ')', before whitespace/comments.\n"
            "- Only the ID segment changes; formatting/comments remain intact.\n"
            "- Also ensures API_VERSION and SERVING_VERSION are present after copyright block."
        )
    )
    parser.add_argument("path", help="File or directory to scan")
    parser.add_argument("--no-backup", action="store_true", help="Do not create .bak backups")
    parser.add_argument("--dry-run", action="store_true", help="Preview changes without writing files")
    parser.add_argument("--reset-ids", action="store_true", help="Force renumbering from start-id for each file")
    parser.add_argument("--start-id", type=int, default=0, help="Starting ID when --reset-ids is used (default: 0)")
    args = parser.parse_args()

    scan_path(args.path, no_backup=args.no_backup, dry_run=args.dry_run,
              reset_ids=args.reset_ids, start_id=args.start_id)

if __name__ == "__main__":
    main()

