#!/bin/bash
# STEP 3 at 2.5 MeV: lead plate behind the borated sheet (same as rearLeadSweep.sh at 2 MeV).
# Give the moderator and lead brick chosen in steps 1 and 2, e.g.:   bash ../rearSweep25.sh 6 4
# Judged at the screen, 80 cm behind the borated sheet (76 cm behind a 4 cm rear plate).
# The screen is far away, so this step needs 20 million neutrons per run (~8 min each on 16 threads).

MODERATOR=${1:-6}     # cm
LEAD=${2:-4}          # cm
REAR="0 1 2 3 4"      # cm of rear lead
EVENTS=20000000

for t in $REAR; do
  name="rear25_${t}cm"
  cat > "${name}.mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 16
/collimator/sideShielding true
/collimator/hdpeThickness ${MODERATOR} cm
/collimator/leadThickness ${LEAD} cm
/collimator/rearLeadThickness ${t} cm
/collimator/boratedThickness 3 cm
/collimator/pinholeDiameter 1 cm
/collimator/screenGap 80 cm
/collimator/screenSize 100 cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source25.mac
/analysis/setFileName ${name}
/run/printProgress 2000000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting rear lead ${t} cm (moderator ${MODERATOR}, lead ${LEAD}, 2.5 MeV) ..."
  ./collimator "${name}.mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else echo "    PROBLEM: no output file. Look at ${name}.log"; fi
done
echo "=== Step 3 done. Now run:  root -l './rearAnalysis.C(\"rear25\", 800, 1000, ${MODERATOR}, ${LEAD})'"
