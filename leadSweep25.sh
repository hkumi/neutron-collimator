#!/bin/bash
# STEP 2 at 2.5 MeV: lead brick sweep (same as leadSweep.sh at 2 MeV).
# Give the moderator thickness chosen in step 1, e.g.:   bash ../leadSweep25.sh 6
# Judged at the exit plane (back of the lead brick).

MODERATOR=${1:-6}             # cm (first number after the command; 6 if none given)
THICKNESSES="0 2 4 6 8 10"    # cm of lead
EVENTS=100000

for t in $THICKNESSES; do
  name="lead25_${t}cm_mod${MODERATOR}"
  cat > "${name}.mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 16
/collimator/sideShielding true
/collimator/hdpeThickness ${MODERATOR} cm
/collimator/leadThickness ${t} cm
/collimator/boratedThickness 3 cm
/collimator/pinholeDiameter 1 cm
/collimator/screenGap 80 cm
/collimator/screenSize 100 cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source25.mac
/analysis/setFileName ${name}
/run/printProgress 20000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting lead ${t} cm (moderator ${MODERATOR} cm, 2.5 MeV) ..."
  ./collimator "${name}.mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else echo "    PROBLEM: no output file. Look at ${name}.log"; fi
done
echo "=== Step 2 done. Now run:  root -l './leadAnalysis.C(${MODERATOR}, \"lead25\")'"
