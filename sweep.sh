#!/bin/bash
# Moderator thickness sweep.
# Run from the BUILD folder with:   bash ../sweep.sh
# Each thickness gets its own macro (sweep_Xcm.mac), log (sweep_Xcm.log) and output (sweep_Xcm.root).

THICKNESSES="2 4 6 8 10 12"   # cm
EVENTS=100000

for t in $THICKNESSES; do
  mac="sweep_${t}cm.mac"
  cat > "$mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 4
/collimator/sideShielding true
/collimator/hdpeThickness ${t} cm
/collimator/boratedThickness 3 cm
/collimator/pinholeDiameter 1 cm
/collimator/screenGap 5 cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source.mac
/analysis/setFileName sweep_${t}cm
/run/printProgress 20000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting ${t} cm ..."
  ./collimator "$mac" > "sweep_${t}cm.log" 2>&1
  if [ -f "sweep_${t}cm.root" ]; then
    echo "    $(date +%H:%M:%S)  finished ${t} cm -> sweep_${t}cm.root"
  else
    echo "    PROBLEM with ${t} cm: no output file. Look at sweep_${t}cm.log"
  fi
done
echo "=== Sweep done. Now run:  root -l './sweepAnalysis.C'"
