#!/bin/sh
# Test end-to-end szablonów: pimpex init --template X, check, a potem gra bez okna
# z zaplanowanymi klawiszami i sprawdzenie .pimpex/state.txt.
# Użycie: templates.sh <ścieżka do pimpex> <katalog roboczy>
set -eu
PIMPEX="$1"
WORK="$2/e2e_templates"
rm -rf "$WORK"
mkdir -p "$WORK"
FAILED=0

play() { # szablon, klatki, klawisze, oczekiwany fragment state.txt...
  template="$1"; frames="$2"; input="$3"; shift 3
  "$PIMPEX" init "$template" --dir "$WORK" --template "$template" >/dev/null
  "$PIMPEX" check "$WORK/$template" >/dev/null
  "$PIMPEX" play "$WORK/$template" --headless --frames "$frames" --input "$input" --quiet
  for expected in "$@"; do
    if ! grep -qF -- "$expected" "$WORK/$template/.pimpex/state.txt"; then
      echo "BŁĄD [$template]: brak '$expected' w state.txt:"
      cat "$WORK/$template/.pimpex/state.txt"
      FAILED=1
    fi
  done
  if grep -q "BŁĄD\|błąd" "$WORK/$template/.pimpex/runtime.log"; then
    echo "BŁĄD [$template]: błędy w runtime.log:"
    cat "$WORK/$template/.pimpex/runtime.log"
    FAILED=1
  fi
  echo "ok: $template"
}

play starter2d 120 "LEFT:0-49 SPACE:5" "procedury: ruch_gracza granice_ekranu zbieranie_monet iskry turbo_wlacznik turbo"
play empty2d 10 "" "Title x=20 y=20"
play topdown2d 30 "RIGHT:0-29" "Player x=320 y=200"
play platformer2d 60 "" "Player x=100 y=544" "grounded=1"
play empty3d 10 "" "Box x=0 y=1 z=0"
play arena3d 60 "W:0-29" "Player x=0 y=0.50 z=-4.50" "grounded=1"
exit $FAILED
