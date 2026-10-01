#!/bin/bash
# Second layer sweep: graphite block with the beam channel behind the lead brick.
# The 8 HDPE blocks around the centre are as thick as lead brick + graphite block.
# Run from the BUILD folder with:   bash ../channelSweep.sh

MODERATOR=6        # cm
LEAD=4             # cm, lead brick
REAR=4             # cm, rear lead plate
CHANNEL="0 2 4 6"  # cm of graphite (6 = Juan's full 10 cm second column)
EVENTS=20000000

for c in $CHANNEL; do
  name="chan20M_${c}cm"
  mac="${name}.mac"
  cat > "$mac" << EOF
/control/verbose 1
/run/verbose 1
/run/numberOfThreads 16
/collimator/sideShielding true
/collimator/hdpeThickness ${MODERATOR} cm
/collimator/leadThickness ${LEAD} cm
/collimator/channelLength ${c} cm
/collimator/rearLeadThickness ${REAR} cm
/collimator/boratedThickness 3 cm
/collimator/pinholeDiameter 1 cm
/collimator/screenGap 5 cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source.mac
/analysis/setFileName ${name}
/run/printProgress 2000000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting graphite channel ${c} cm ..."
  ./collimator "$mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then
    echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else
    echo "    PROBLEM: no output file. Look at ${name}.log"
  fi
done
echo "=== Sweep done. Now run:  root -l './channelAnalysis.C'"
