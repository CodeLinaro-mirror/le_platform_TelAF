#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -Eeuo pipefail
IFS=$'\n\t'
umask 022

: "${OBJCOPY:=objcopy}"
: "${STRIP:=strip}"
CP_FLAGS=( -a --no-preserve=ownership --force )

log() {
    local file="${BASH_SOURCE[2]##*/}"
    local line="${BASH_LINENO[1]}"
    printf '[%s:%s] %s\n' "$file" "$line" "$*" >&2
}

info() { log "INFO: $*"; }
warn() { log "WARN: $*"; }
die()  { log "ERROR: $*"; exit 1; }

on_error() {
  local rc=$?
  local cmd="${BASH_COMMAND}"
  local src="${BASH_SOURCE[1]:-unknown}"
  local line="${BASH_LINENO[0]:-0}"

  if [ "$rc" -ne 0 ]; then
    printf '[%(%F %T)T] ERROR: rc=%d, cmd=%s, at %s:%s\n' -1 "$rc" "$cmd" "$src" "$line" >&2
  fi
  return "$rc"
}

set -o errtrace
trap on_error ERR
trap 'rc=$?; [ $rc -ne 0 ] && on_error; exit $rc' EXIT

# -----------------------------
# Usage
# -----------------------------
usage() {
  cat <<'EOF'
Usage: mkimg.sh -t <target> -o <output_dir> [options]

Required:
  -t  TARGET
  -o  OUTPUT
  -r  TELAF_ROOT

Optional:
  -s  TARGET_BASIC_DIR
  -a  PA_BUILD_DIR
  -p  PROP_BUILD_DIR
  -n  NOSHIP_BUILD_DIR
  -w  TARGET_PA_BUILD_DIR
  -d  DEFAULT_PA_BUILD_DIR
  -v  VENDOR
  -N  NO_STRIP (do not strip .so)
  -h  help
EOF
}

TARGET="" OUTPUT="" TELAF=""
TARGET_BASIC_DIR="" PROP_BUILD_DIR="" NOSHIP_BUILD_DIR="" PA_BUILD_DIR="" TARGET_PA_BUILD_DIR="" VENDOR="" DEFAULT_PA_BUILD_DIR=""
NO_STRIP=0
while getopts ":t:o:r:s:a:p:n:w:d:v:Nh" opt; do
  case "$opt" in
    t) TARGET=$OPTARG ;;
    o) OUTPUT=$OPTARG ;;
    r) TELAF=$OPTARG ;;
    s) TARGET_BASIC_DIR=$OPTARG ;;
    a) PA_BUILD_DIR=$OPTARG ;;
    p) PROP_BUILD_DIR=$OPTARG ;;
    n) NOSHIP_BUILD_DIR=$OPTARG ;;
    w) TARGET_PA_BUILD_DIR=$OPTARG ;;
    d) DEFAULT_PA_BUILD_DIR=$OPTARG ;;
    v) VENDOR=$OPTARG ;;
    N) NO_STRIP=1 ;;
    h) usage; exit 0 ;;
    \?) usage; die "unknown: -$OPTARG" ;;
    :)  usage; die "opt -$OPTARG is missing arg" ;;
  esac
done

[[ -n "$TARGET" && -n "$OUTPUT" ]] || { usage; die "needs -t and -o"; }
[[ -n "$TELAF"  ]] || die "needs -r TELAF_ROOT"

if [[ -z "$TARGET_BASIC_DIR" || ! -d "$TARGET_BASIC_DIR" ]]; then
    warn "No valid base filesystem provided (-s). Will build from empty root."
fi

# Allow VENDOR to fallback to env if not explicitly set
[[ -z "$VENDOR" && -n "${VENDOR_ROOT:-}" ]] && VENDOR="$VENDOR_ROOT"

run() {
  local _out _err rc
  _out="$(mktemp)" || { echo "[run] mktemp failed" >&2; return 1; }
  _err="$(mktemp)" || { echo "[run] mktemp failed" >&2; rm -f "$_out"; return 1; }

  set +e
  "$@" 1>"$_out" 2>"$_err"
  rc=$?
  set -e

  if [ -s "$_out" ]; then cat "$_out"; fi
  if [ -s "$_err" ]; then cat "$_err" >&2; fi

  rm -f "$_out" "$_err"

  if (( rc != 0 )); then
    echo "[FAIL:$rc] $*" >&2
  fi
  return $rc
}

ensure_dir() { run install -d -m 0755 "$1"; }

# Replace .so files
replace_libs_in_stage() {
  local module="$1" src_dir="$2" dst_root="$3"
  [[ -d "$src_dir" ]] || { info "[$module] skip source dir not found -> $src_dir"; return; }
  [[ -d "$dst_root" ]] || { info "[$module] skip dst root not found -> $dst_root"; return; }

  info "[$module] replace in combined stage: from $src_dir -> $dst_root"

  local replaced=0 skipped=0 notfound=0

  while IFS= read -r -d '' so; do
    local base="$(basename "$so")"
    local matched=0
    while IFS= read -r -d '' dst; do
      # don't replace a symlink target path itself; only real files in dst tree
      if [[ -L "$dst" ]]; then
        info "[$module] skip: symbol link -> $dst"
        ((skipped++))
        continue
      fi

      if (( matched == 0 )); then
        info "[$module] replace: $base"
        info "  source: $src_dir/$base"
        info "  target: $dst"
      fi

      ensure_dir "$(dirname "$dst")"
      # keep debug file
      run "$OBJCOPY" --only-keep-debug "$so" "$OUTPUT/${base}.debug"
      (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
      run rm -f "$dst"
      run cp "${CP_FLAGS[@]}" "$so" "$dst"
      matched=1
    done < <(find "$dst_root" -type f -name "$base" -print0) || true

    if (( matched == 0 )); then
      info "[$module] not found: no target for $base"
      ((notfound++))
    else
      ((replaced++))
    fi

  done < <(find "$src_dir" -maxdepth 2 -type f -name '*.so*' -print0) || true

  info "[$module] summary: replaced=$replaced, skipped=$skipped, not_found=$notfound"
}

install_libs_to_runtime() {
  local module="$1" dir="$2" dst="$3"

  if [[ ! -d "$dir" ]]; then
    info "[$module] skip: source dir not found -> $dir"
    return
  fi

  ensure_dir "$dst"
  info "[$module] install: $dir -> $dst"

  local count=0
  while IFS= read -r -d '' so; do
    local base="$(basename "$so")"
    local target="$dst/$base"

    if [[ -e "$target" || -L "$target" ]]; then
      info "[$module] skip: already exists -> $target"
      continue
    fi

    if [[ ! -L "$so" ]]; then
      if [[ -n "$OBJCOPY" && -n "$OUTPUT" ]]; then
        # Only extract debug symbols if the binary has not already been stripped.
        # Binaries pre-processed by split_debug_symbols_strip_binaries.cmake are
        # already stripped; running objcopy --only-keep-debug on them produces a
        # near-empty (~2 KB) placeholder instead of real DWARF data.  The real
        # debug symbols for those binaries are in the .build-id tree inside their
        # staging directory and are collected separately below.
        if file "$so" 2>/dev/null | grep -q "not stripped"; then
          ensure_dir "$OUTPUT"
          local dbgfile="$OUTPUT/${base}.debug"
          info "[$module] keep debug: $dbgfile"
          run "$OBJCOPY" --only-keep-debug "$so" "$dbgfile"
          (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
        else
          info "[$module] skip debug extract: already stripped -> $base"
          (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
        fi
      else
        (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
      fi
    fi

    run cp "${CP_FLAGS[@]}" "$so" "$dst/"
    ((count++))
  done < <(find "$dir" -maxdepth 2 \( -type f -o -type l \) -name '*.so*' -print0) || true

  if (( count == 0 )); then
    info "[$module] no files: no matching files in $dir"
  else
    info "[$module] done: installed $count file(s)"
  fi
}

# generate TelAF image from combined stage
build_image() {
  local src_dir="$1"
  local out_stage="$OUTPUT/mkimg"
  ensure_dir "$out_stage"
  info "*** generating TelAF image from: $src_dir"

  local squashfs_img="$out_stage/telaf.squashfs"
  local squashfs_ubi="$out_stage/telaf_ro.squashfs.ubi"

  run mksquashfs "$src_dir" "$squashfs_img" \
    -context-file "${SELINUX_FILE_CONTEXTS}" \
    -noappend \
    -comp xz -Xdict-size 32K \
    -noI -Xbcj arm \
    -b 65536 \
    -processors 1 \
    -all-root \
    -xattrs

  ensure_dir "$OUTPUT"
  for f in telaf.squashfs telaf.squashfs.ubi; do
    if [[ -f "$out_stage/$f" ]]; then
      local out_name="${f/telaf/telaf_ro}"
      info "output $f -> $OUTPUT/$out_name"
      run cp "${CP_FLAGS[@]}" "$out_stage/$f" "$OUTPUT/$out_name"
    else
      warn "cannot find: $out_stage/$f"
    fi
  done
}

PA_API_PREFIX_REGEX="${PA_API_PREFIX_REGEX:-^taf_pa_}"

extract_pa_api_sets() {
  local so="$1" out_set="$2" out_list="$3"
  [[ -f "$so" || -L "$so" ]] || { warn "[API-CHECK] skip missing so: $so"; : >"$out_set"; : >"$out_list"; return; }

  command -v readelf >/dev/null || die "cannot find readelf"
  if [[ ! -x /usr/bin/c++filt ]]; then
    warn "c++filt not found; C++ API demangle unavailable.."
  fi

  local tmp_syms tmp_dm
  tmp_syms="$(mktemp)" || die "[API-CHECK] mktemp failed"
  tmp_dm="$(mktemp)"   || die "[API-CHECK] mktemp failed"

  readelf -Ws "$so" \
  | awk '$4=="FUNC" && ($5=="GLOBAL" || $5=="WEAK") && $6=="DEFAULT" && $7!="UND" {print $8}' \
  | LC_ALL=C sort -u > "$tmp_syms"

  : > "$tmp_dm"
  while IFS= read -r sym; do
    local dm sig
    if [[ -x /usr/bin/c++filt ]]; then
      dm="$(/usr/bin/c++filt "$sym" 2>/dev/null || echo "$sym")"
    else
      dm="$sym"
    fi

    sig="$(awk '
      {
        s=$0
        while (match(s,/<[^<>]*>/)) {
          s = substr(s,1,RSTART-1) "" substr(s,RSTART+RLENGTH)
        }
        print s
      }
    ' <<< "$dm")"

    local func_name="${sig%%(*}"
    func_name="${func_name##*::}"

    if [[ -n "${PA_API_PREFIX_REGEX:-}" ]]; then
      [[ "$func_name" =~ $PA_API_PREFIX_REGEX ]] || continue
    else
      [[ "$func_name" =~ ^taf_pa_ ]] || continue
    fi

    printf '%s\n' "$sig" >> "$tmp_dm"
  done < "$tmp_syms"

  LC_ALL=C sort -u "$tmp_dm" > "$out_set"
  cp "$out_set" "$out_list"

  rm -f "$tmp_syms" "$tmp_dm"
}

verify_one_pair() {
  local strong="$1" weak="$2" base strong_set weak_set strong_dm weak_dm
  base="$(basename "$strong")"
  strong_set="$(mktemp)" || die "[API-CHECK] mktemp failed"
  weak_set="$(mktemp)"   || die "[API-CHECK] mktemp failed"
  strong_dm="$(mktemp)"  || die "[API-CHECK] mktemp failed"
  weak_dm="$(mktemp)"    || die "[API-CHECK] mktemp failed"

  info "[API-CHECK] Pair: strong=$strong"
  info "[API-CHECK]       weak  =$weak"

  extract_pa_api_sets "$strong" "$strong_set" "$strong_dm"
  extract_pa_api_sets "$weak"   "$weak_set"   "$weak_dm"

  info "[API-CHECK] Strong API list:"
  if [[ -s "$strong_dm" ]]; then
    sed 's/^/    /' "$strong_dm"
  else
    info "    (none)"
  fi

  info "[API-CHECK] Weak API list:"
  if [[ -s "$weak_dm" ]]; then
    sed 's/^/    /' "$weak_dm"
  else
    info "    (none)"
  fi

  local _strong_f _weak_f
  _strong_f="$(mktemp)" || die "[API-CHECK] mktemp failed"
  _weak_f="$(mktemp)"   || die "[API-CHECK] mktemp failed"

  cp "$strong_dm" "$_strong_f"
  cp "$weak_dm" "$_weak_f"

  local miss_in_weak extra_in_weak rc=0
  miss_in_weak="$(LC_ALL=C comm -23 "$_strong_f" "$_weak_f")"
  if [[ -n "$miss_in_weak" ]]; then
    rc=1
    warn "[API-CHECK] Strong-only API:"
    printf '%s\n' "$miss_in_weak" | sed 's/^/    /'
  fi

  extra_in_weak="$(LC_ALL=C comm -13 "$_strong_f" "$_weak_f")"
  if [[ -n "$extra_in_weak" ]]; then
    info "[API-CHECK] Weak-only API:"
    printf '%s\n' "$extra_in_weak" | sed 's/^/    /'
  fi

  rm -f "$_strong_f" "$_weak_f"
  return "$rc"
}

verify_pa_strong_vs_weak() {
  local strong_so weak_pattern weak_so strong_base prefix
  [[ -d "$TARGET_PA_BUILD_DIR" ]] || { info "[API-CHECK] skip: no TARGET_PA_BUILD_DIR"; return 0; }
  [[ -d "$DEFAULT_PA_BUILD_DIR" ]] || { info "[API-CHECK] skip: no DEFAULT_PA_BUILD_DIR"; return 0; }

  local any_pair=0 rc_all=0
  while IFS= read -r -d '' strong_so; do
    any_pair=1
    strong_base="$(basename "$strong_so")"
    prefix="${strong_base%%.so*}"                     # e.g. libComponent_taf_pa_voicecall
    weak_pattern="${prefix}Def.so*"                   # e.g. libComponent_taf_pa_voicecallDef.so*
    weak_so="$(find "$DEFAULT_PA_BUILD_DIR" -type f -name "$weak_pattern" -print -quit)"
    if [[ -z "$weak_so" ]]; then
      die "[API-CHECK] missing weak library for strong: $strong_base (expect pattern: $weak_pattern under DEFAULT_PA_BUILD_DIR)"
    fi
    if ! verify_one_pair "$strong_so" "$weak_so"; then
      rc_all=1
    fi
  done < <(find "$TARGET_PA_BUILD_DIR" -maxdepth 2 -type f -name 'libComponent_taf_pa_*.so*' -print0)

  if (( any_pair == 0 )); then
    warn "[API-CHECK] no strong PA libraries found under TARGET_PA_BUILD_DIR=$TARGET_PA_BUILD_DIR"
  fi

  return "$rc_all"
}

# -----------------------------
# main
# -----------------------------
SELINUX_FILE_CONTEXTS="${TELAF}/security/selinux/sepolicy/files/file_contexts"
if [[ ! -f "$SELINUX_FILE_CONTEXTS" ]]; then
  warn "SELINUX file_contexts missing, creating empty fallback"
  SELINUX_FILE_CONTEXTS="$OUTPUT/file_contexts.empty"
  touch "$SELINUX_FILE_CONTEXTS"
fi
export SELINUX_FILE_CONTEXTS

info "target: $TARGET"
info "base (TARGET_BASIC_DIR): $TARGET_BASIC_DIR"
ensure_dir "$OUTPUT"

STAGE_DIR_COMBINED="$OUTPUT/staging_combined"
info "combined stage: $STAGE_DIR_COMBINED"
run rm -rf "$STAGE_DIR_COMBINED"
ensure_dir "$STAGE_DIR_COMBINED"

if [[ -d "$TARGET_BASIC_DIR" ]]; then
  info "Syncing base filesystem from $TARGET_BASIC_DIR"
  run cp -a "$TARGET_BASIC_DIR/." "$STAGE_DIR_COMBINED/"
else
  info "No base TARGET_BASIC_DIR found, starting from empty"
fi

ensure_dir "$STAGE_DIR_COMBINED/systems/current/lib"
ensure_dir "$STAGE_DIR_COMBINED/systems/current/modules"

info "running PA strong/weak API compatibility check"
verify_pa_strong_vs_weak
info "API check passed"

replace_libs_in_stage "PA"     "$PA_BUILD_DIR"     "$STAGE_DIR_COMBINED"
# replace_libs_in_stage "PA_RW" "$TARGET_PA_BUILD_DIR" "$STAGE_DIR_COMBINED" # usually RW libs are new, not replace

install_libs_to_runtime "PROP"   "$PROP_BUILD_DIR"    "$STAGE_DIR_COMBINED/systems/current/lib"
install_libs_to_runtime "NOSHIP" "$NOSHIP_BUILD_DIR"  "$STAGE_DIR_COMBINED/systems/current/lib"
install_libs_to_runtime "VENDOR" "$VENDOR"            "$STAGE_DIR_COMBINED/systems/current/modules"
# For TARGET PA and DEFAULT, there is an installation order requirement: install TARGET PA first, then DEFAULT.
# This ensures that when TARGET PA exists, it won't be overwritten by DEFAULT PA.
install_libs_to_runtime "TARGET_PA"  "$TARGET_PA_BUILD_DIR"   "$STAGE_DIR_COMBINED/systems/current/lib"
install_libs_to_runtime "DEFAULT_PA"  "$DEFAULT_PA_BUILD_DIR"   "$STAGE_DIR_COMBINED/systems/current/lib"

replace_libs_in_stage "PROP"   "$PROP_BUILD_DIR"   "$STAGE_DIR_COMBINED"
replace_libs_in_stage "NOSHIP" "$NOSHIP_BUILD_DIR" "$STAGE_DIR_COMBINED"

# Collect .build-id debug symbol trees produced by split_debug_symbols_strip_binaries.cmake
# from every PA/PROP/NOSHIP staging directory into $OUTPUT/debug/.build-id/.
# These trees are NOT copied by install_libs_to_runtime (which only handles *.so* files),
# so without this step the telaf-pa debug symbols are entirely absent from the build output.
collect_build_id_debug_symbols() {
  local debug_out="${OUTPUT}/debug/.build-id"
  ensure_dir "${debug_out}"
  local collected=0
  for staging_dir in \
      "${TARGET_PA_BUILD_DIR}" \
      "${DEFAULT_PA_BUILD_DIR}" \
      "${PROP_BUILD_DIR}" \
      "${NOSHIP_BUILD_DIR}"; do
    local src="${staging_dir}/.build-id"
    if [[ -d "${src}" ]]; then
      info "[.build-id] merging: ${src} -> ${debug_out}"
      cp -a "${src}/." "${debug_out}/"
      collected=$((collected + 1))
    fi
  done
  if [[ "${collected}" -eq 0 ]]; then
    info "[.build-id] no .build-id trees found in staging directories"
  else
    info "[.build-id] merged ${collected} .build-id tree(s) into ${debug_out}"
  fi
}
collect_build_id_debug_symbols

info "install/replace done"

build_image "$STAGE_DIR_COMBINED"
info "mkimg done"
