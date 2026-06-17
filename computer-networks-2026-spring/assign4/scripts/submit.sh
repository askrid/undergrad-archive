#!/usr/bin/env bash
# Build the submission tarball.
# Output: submission/<ID>_<Name>_assign4.tar.gz containing
#   sr_router.c, sr_arpcache.c, readme.pdf
set -eu
source "$(dirname "$0")/../env.sh"

STUDENT_ID="${STUDENT_ID:-202017316}"
STUDENT_NAME="${STUDENT_NAME:-JoonwooChoi}"
NAME="${STUDENT_ID}_${STUDENT_NAME}_assign4"

SRC=("$PROJ_DIR/src/router/sr_router.c" "$PROJ_DIR/src/router/sr_arpcache.c")
PDF="$PROJ_DIR/submission/readme.pdf"
OUT_DIR="$PROJ_DIR/submission"
STAGE="$OUT_DIR/$NAME"
TARBALL="$OUT_DIR/$NAME.tar.gz"

# readme.pdf
if [[ -f "$PROJ_DIR/README.md" ]]; then
  echo "[submit] compiling $PDF from README.md..."
  pandoc "$PROJ_DIR/README.md" -o "$PDF" --pdf-engine=typst
else
  echo "[submit] no README.md" >&2
  exit 1
fi

for f in "${SRC[@]}"; do
  [[ -f "$f" ]] || { echo "[submit] missing: $f" >&2; exit 1; }
done

rm -rf "$STAGE" "$TARBALL"
mkdir -p "$STAGE"
cp "${SRC[@]}" "$PDF" "$STAGE/"
tar -C "$OUT_DIR" -zcf "$TARBALL" "$NAME"
rm -rf "$STAGE"

echo "[submit] built: $TARBALL"
tar -tzf "$TARBALL"
