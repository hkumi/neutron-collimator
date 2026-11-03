#!/bin/bash
# STEP 1 at 2.5 MeV: moderator thickness sweep (same as sweep.sh at 2 MeV).
# 3 x 3 HDPE stack, 3 cm borated sheet, 1 cm pinhole, no lead, no graphite.
# Judged at the exit plane (back of the moderator), so the screen distance does not matter here;
# the screen is still placed 80 cm behind the borated sheet, as in all 2.5 MeV runs.
# Run from the BUILD folder with:   bash ../modSweep25.sh

THICKNESSES="2 4 6 8 10 12"   # cm
EVENTS=100000

for t in $THICKNESSES; do
  name="sweep25_${t}cm"
  cat > "${name}.mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 16
/collimator/sideShielding true
/collimator/hdpeThickness ${t} cm
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
  echo "=== $(date +%H:%M:%S)  starting moderator ${t} cm (2.5 MeV) ..."
  ./collimator "${name}.mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else echo "    PROBLEM: no output file. Look at ${name}.log"; fi
done
echo "=== Step 1 done. Now run:  root -l './sweepAnalysis.C(\"sweep25\")'"
