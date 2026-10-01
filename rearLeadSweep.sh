#!/bin/bash
# Rear lead plate sweep (lead BEHIND the borated sheet, conical hole).
# Run from the BUILD folder with:   bash ../rearLeadSweep.sh

MODERATOR=6          # cm
LEAD=4               # cm, lead brick behind the moderator
REAR="0 1 2 3 4"     # cm, rear lead plate (must be < screen gap of 5 cm)
EVENTS=100000

for t in $REAR; do
  name="rear_${t}cm"
  mac="${name}.mac"
  cat > "$mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 4
/collimator/sideShielding true
/collimator/hdpeThickness ${MODERATOR} cm
/collimator/leadThickness ${LEAD} cm
/collimator/rearLeadThickness ${t} cm
/collimator/boratedThickness 3 cm
/collimator/pinholeDiameter 1 cm
/collimator/screenGap 5 cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source.mac
/analysis/setFileName ${name}
/run/printProgress 20000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting rear lead ${t} cm ..."
  ./collimator "$mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then
    echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else
    echo "    PROBLEM: no output file. Look at ${name}.log"
  fi
done
echo "=== Sweep done. Now run:  root -l './rearAnalysis.C'"
