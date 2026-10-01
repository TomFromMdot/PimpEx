#!/bin/sh
# Test end-to-end przykładu Wolf 3D: ruch, kolizje ze ścianą, drzwi, strzał.
# Użycie: wolf3d.sh <ścieżka do pimpex> <katalog przykładu> <katalog roboczy>
set -eu
PIMPEX="$1"
GAME="$2"
RUNTIME="$3/e2e_wolf3d"
FAILED=0

run() { # klatki, klawisze, oczekiwane fragmenty state.txt...
  frames="$1"; input="$2"; shift 2
  rm -rf "$RUNTIME"
  "$PIMPEX" play "$GAME" --headless --frames "$frames" --input "$input" --quiet \
    --runtime-dir "$RUNTIME"
  for expected in "$@"; do
    if ! grep -qF -- "$expected" "$RUNTIME/state.txt"; then
      echo "BŁĄD: brak '$expected' po --input '$input':"
      grep -E "^(Player|Game|Door_7_3)" "$RUNTIME/state.txt"
      FAILED=1
    fi
  done
}

"$PIMPEX" check "$GAME" >/dev/null
# Idzie na wschód i zatrzymuje się na ścianie (x = 13 - 0.3).
run 130 "UP:0-120" "Player x=12.70 y=0.60 z=2 "
# Strzał zabiera nabój.
run 20 "SPACE:5" "ammo=7"
# Obrót, podejście do drzwi, E, drzwi wjeżdżają w sufit, przejście do pokoju strażnika.
run 300 "RIGHT:0-29 UP:30-63 LEFT:64-93 UP:94-190 E:195 UP:230-280" \
  "Player x=18.82 y=0.60 z=6.08" "Door_7_3 x=14 y=2.92 z=6" "ammo=8"
# ESC: pauza z gui/pauza.xml – gracz stoi mimo strzałki.
run 30 "ESCAPE:2 UP:3-29" "gui: pauza (pauza)" "Player x=2 y=0.60 z=2 "
# Klik w „Graj dalej” chowa pauzę i gra rusza dalej.
run 30 "ESCAPE:2 CLICK@400/315:6 UP:10-29" "Player x=4.40 y=0.60 z=2 "
[ $FAILED -eq 0 ] && echo "ok: wolf3d"
exit $FAILED
