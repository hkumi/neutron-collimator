#!/bin/bash
# Same graphite channel sweep as channelSweep.sh, but with
#   - a 2.5 MeV source (source25.mac)
#   - the screen 76 cm behind the collimator (back face of the rear lead plate)
#     -> screenGap = 4 cm rear lead + 76 cm = 80 cm behind the borated sheet
#   - a 1 x 1 m screen, because the beam is much wider so far away
# STEP 4 at 2.5 MeV. Give the moderator, lead brick and rear lead chosen in steps 1-3, e.g.:
#   bash ../channelSweep76.sh 6 4 4
# Run from the BUILD folder.

MODERATOR=${1:-6}  # cm
LEAD=${2:-4}       # cm, lead brick
REAR=${3:-4}       # cm, rear lead plate
GAP=80             # cm, back of the borated sheet -> screen (= 76 cm behind a 4 cm rear plate),
                   #     kept fixed in all 2.5 MeV runs
SCREEN=100         # cm, screen width
CHANNEL="0 2 4 6"  # cm of graphite
EVENTS=20000000

for c in $CHANNEL; do
  name="chan76_${c}cm"
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
/collimator/screenGap ${GAP} cm
/collimator/screenSize ${SCREEN} cm
/collimator/boronMassFraction 0.05
/run/initialize
/control/execute source25.mac
/analysis/setFileName ${name}
/run/printProgress 2000000
/run/beamOn ${EVENTS}
EOF
  echo "=== $(date +%H:%M:%S)  starting graphite channel ${c} cm (moderator ${MODERATOR}, lead ${LEAD}, rear ${REAR}, 2.5 MeV) ..."
  ./collimator "$mac" > "${name}.log" 2>&1
  if [ -f "${name}.root" ]; then
    echo "    $(date +%H:%M:%S)  finished -> ${name}.root"
  else
    echo "    PROBLEM: no output file. Look at ${name}.log"
  fi
done
echo "=== Step 4 done. Now run:  root -l './channelAnalysis.C(\"chan76\", 800, 1000, $((REAR*10)), ${MODERATOR}, ${LEAD})'"
