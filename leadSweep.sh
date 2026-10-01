#!/bin/bash
# Lead brick thickness sweep.
# Run from the BUILD folder with:   bash ../leadSweep.sh

MODERATOR=6                  # cm of HDPE moderator (change to 6 to repeat with the thinner moderator)
THICKNESSES="0 2 4 6 8 10"    # cm of lead
EVENTS=100000

for t in $THICKNESSES; do
  name="lead_${t}cm_mod${MODERATOR}"
  mac="${name}.mac"
  cat > "$mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 4
/collimator/sideShielding true
/collimator/hdpeThickness ${MODERATOR} cm
/collimator/leadThickness ${t} cm
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
  echo "=== $(date +%H:%M:%S)  starting lead ${t} cm (moderator ${MODERATOR} cm) ..."
  ./collimator "$mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then
    echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else
    echo "    PROBLEM: no output file. Look at ${name}.log"
  fi
done
echo "=== Sweep done. Now run:  root -l './leadAnalysis.C(${MODERATOR})'"
