#include "DetectorConstruction.hh"
#include "detector.hh"

#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Element.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4GenericMessenger.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

DetectorConstruction::DetectorConstruction()
{
    DefineCommands();
}

DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger;
}

void DetectorConstruction::DefineMaterials()
{
    auto nist = G4NistManager::Instance();

    // World and pinhole: vacuum (Juan: pinhole filled with the mother material)
    fVacuum = nist->FindOrBuildMaterial("G4_Galactic");

    // Hydrogen bound in polyethylene.
    // The special name "TS_H_of_Polyethylene" makes Geant4 use thermal scattering
    // data for this hydrogen. Without it, H is treated as a free atom and the
    // thermal part of the spectrum is wrong.
    auto elH_PE = new G4Element("TS_H_of_Polyethylene", "H_PE", 1., 1.0079*g/mole);
    auto elC    = nist->FindOrBuildElement("C");
    auto elB    = nist->FindOrBuildElement("B");   // natural boron (~20% B-10)

    // Lead for the gamma-shielding brick
    fLead = nist->FindOrBuildMaterial("G4_Pb");

    // Graphite for the beam-channel block. The name "TS_C_of_Graphite" makes Geant4 use
    // thermal scattering data for carbon bound in graphite (as for H in polyethylene).
    auto elC_gr = new G4Element("TS_C_of_Graphite", "C_GR", 6., 12.011*g/mole);
    fGraphite = new G4Material("Graphite", 1.70*g/cm3, 1, kStateSolid, 293.6*kelvin);
    fGraphite->AddElement(elC_gr, 1);

    // HDPE: (CH2)n
    fHDPE = new G4Material("HDPE", 0.95*g/cm3, 2, kStateSolid, 293.6*kelvin);
    fHDPE->AddElement(elH_PE, 2);
    fHDPE->AddElement(elC, 1);

    // Borated HDPE: HDPE + natural boron (check the % on the lab sheet's datasheet)
    fBoratedHDPE = new G4Material("BoratedHDPE", 1.00*g/cm3, 2, kStateSolid, 293.6*kelvin);
    fBoratedHDPE->AddMaterial(fHDPE, 1.0 - fBoronFraction);
    fBoratedHDPE->AddElement(elB, fBoronFraction);
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    DefineMaterials();
    const G4bool checkOverlaps = true;

    // Beam goes along +z. The FRONT face of the HDPE block is at z = 0.
    // The source (set in source.mac) sits at z = -3 cm.
    const G4double screenThickness = 1.*mm;
    const G4double zHDPE    = 0.5*fHDPEThickness;
    // Order along the beam: moderator -> lead layer (optional) -> borated sheet -> gap -> screen
    // Second layer = lead brick + graphite channel block (centre), surrounded by HDPE
    const G4double secondLayer = fLeadThickness + fChannelLength;
    const G4double zLead    = fHDPEThickness + 0.5*fLeadThickness;
    const G4double zChannel = fHDPEThickness + fLeadThickness + 0.5*fChannelLength;
    const G4double zSecond  = fHDPEThickness + 0.5*secondLayer;
    const G4double zBorated = fHDPEThickness + secondLayer + 0.5*fBoratedThickness;
    const G4double zScreen  = fHDPEThickness + secondLayer + fBoratedThickness + fScreenGap + 0.5*screenThickness;
    // rear lead plate sits directly behind the borated sheet, inside the gap before the screen
    const G4double zRear    = fHDPEThickness + secondLayer + fBoratedThickness + 0.5*fRearLeadThickness;
    // total length of the beam channel (graphite channel + pinhole in the borated sheet)
    const G4double channelTotal = fChannelLength + fBoratedThickness;
    if (fRearLeadThickness >= fScreenGap) {
        G4Exception("DetectorConstruction::Construct()", "Collim001", FatalException,
                    "The rear lead plate must be thinner than the screen gap.");
    }

    // Transverse size of the whole stack: 10 cm without side shielding, 30 cm with it
    const G4double stackSize = fSideShielding ? 3.*fBlockSize : fBlockSize;

    // ---------- World (vacuum, 2 x 2 x 2 m) ----------
    auto solidWorld = new G4Box("World", 1.*m, 1.*m, 1.*m);
    auto logicWorld = new G4LogicalVolume(solidWorld, fVacuum, "World");
    auto physWorld  = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World",
                                        nullptr, false, 0, checkOverlaps);
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    // ---------- HDPE moderator block ----------
    auto solidHDPE = new G4Box("HDPE", 0.5*fBlockSize, 0.5*fBlockSize, 0.5*fHDPEThickness);
    auto logicHDPE = new G4LogicalVolume(solidHDPE, fHDPE, "HDPE");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zHDPE), logicHDPE, "HDPE",
                      logicWorld, false, 0, checkOverlaps);
    logicHDPE->SetVisAttributes(new G4VisAttributes(G4Colour(0.6, 0.8, 1.0, 0.4)));

    // ---------- NEW: 8 surrounding HDPE blocks (3 x 3 stack, moderator in the centre) ----------
    // They use the same box shape as the moderator, so they have the same thickness.
    if (fSideShielding) {
        auto logicShield = new G4LogicalVolume(solidHDPE, fHDPE, "ShieldHDPE");
        logicShield->SetVisAttributes(new G4VisAttributes(G4Colour(0.3, 0.5, 0.9, 0.3)));

        G4int copyNo = 0;
        for (G4int i = -1; i <= 1; ++i) {        // i = column (x)
            for (G4int j = -1; j <= 1; ++j) {    // j = row (y)
                if (i == 0 && j == 0) continue;  // centre position = moderator, already placed
                new G4PVPlacement(nullptr,
                                  G4ThreeVector(i*fBlockSize, j*fBlockSize, zHDPE),
                                  logicShield, "ShieldHDPE", logicWorld, false, copyNo, checkOverlaps);
                ++copyNo;
            }
        }
    }

    // ---------- Second layer (Juan's sketch, second column) ----------
    // Centre: lead brick, then a graphite block with the beam channel (1 cm hole, vacuum).
    // Around it (if side shielding is on): 8 HDPE blocks as thick as the whole second layer.
    if (fLeadThickness > 0.) {
        auto solidLead = new G4Box("LeadBrick", 0.5*fBlockSize, 0.5*fBlockSize, 0.5*fLeadThickness);
        auto logicLead = new G4LogicalVolume(solidLead, fLead, "Lead");
        new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zLead), logicLead, "Lead",
                          logicWorld, false, 0, checkOverlaps);
        logicLead->SetVisAttributes(new G4VisAttributes(G4Colour(0.9, 0.1, 0.1, 0.6)));
    }
    if (fChannelLength > 0.) {
        auto solidBlock = new G4Box("ChannelBlock", 0.5*fBlockSize, 0.5*fBlockSize, 0.5*fChannelLength);
        auto solidHole  = new G4Tubs("ChannelCut", 0., 0.5*fPinholeDiameter,
                                     0.5*fChannelLength + 1.*mm, 0., 360.*deg);
        auto solidChan  = new G4SubtractionSolid("GraphiteWithChannel", solidBlock, solidHole);
        auto logicChan  = new G4LogicalVolume(solidChan, fGraphite, "ChannelGraphite");
        new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zChannel), logicChan, "ChannelGraphite",
                          logicWorld, false, 0, checkOverlaps);
        logicChan->SetVisAttributes(new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.5)));
    }
    if (fSideShielding && secondLayer > 0.) {
        auto solidRing = new G4Box("SecondLayerBlock", 0.5*fBlockSize, 0.5*fBlockSize, 0.5*secondLayer);
        auto logicRing = new G4LogicalVolume(solidRing, fHDPE, "SecondLayerHDPE");
        logicRing->SetVisAttributes(new G4VisAttributes(G4Colour(0.3, 0.5, 0.9, 0.3)));
        G4int copyNo = 0;
        for (G4int i = -1; i <= 1; ++i) {
            for (G4int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) continue;   // centre = lead brick + graphite channel block
                new G4PVPlacement(nullptr, G4ThreeVector(i*fBlockSize, j*fBlockSize, zSecond),
                                  logicRing, "SecondLayerHDPE", logicWorld, false, copyNo, checkOverlaps);
                ++copyNo;
            }
        }
    }

    // ---------- Borated HDPE sheet with a round pinhole ----------
    // CHANGED: the sheet now covers the whole back of the stack
    auto solidSheet = new G4Box("BoratedSheet", 0.5*stackSize, 0.5*stackSize, 0.5*fBoratedThickness);
    // the hole is made a bit longer than the sheet so the cut is clean
    auto solidHole  = new G4Tubs("PinholeCut", 0., 0.5*fPinholeDiameter,
                                 0.5*fBoratedThickness + 1.*mm, 0., 360.*deg);
    auto solidBorated = new G4SubtractionSolid("BoratedWithPinhole", solidSheet, solidHole);
    auto logicBorated = new G4LogicalVolume(solidBorated, fBoratedHDPE, "BoratedHDPE");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zBorated), logicBorated, "BoratedHDPE",
                      logicWorld, false, 0, checkOverlaps);
    logicBorated->SetVisAttributes(new G4VisAttributes(G4Colour(0.2, 0.7, 0.2, 0.6)));

    // ---------- NEW: rear lead plate behind the borated sheet, with a CONICAL hole ----------
    // The hole starts with the pinhole diameter D and widens at the same angle as the
    // neutrons leaving the channel (slope D/L, L = graphite channel + borated sheet).
    // So the borated pinhole still defines the beam, and everything outside the beam
    // has to go through the lead.
    if (fRearLeadThickness > 0.) {
        const G4double slope  = fPinholeDiameter / channelTotal;        // widening per unit length
        const G4double margin = 0.1*mm;                                 // makes the cut clean at both faces
        const G4double rFront = 0.5*fPinholeDiameter - margin*slope;
        const G4double rBack  = 0.5*fPinholeDiameter + (fRearLeadThickness + margin)*slope;

        auto solidPlate = new G4Box("RearLeadPlate", 0.5*stackSize, 0.5*stackSize, 0.5*fRearLeadThickness);
        auto solidCone  = new G4Cons("RearLeadHole", 0., rFront, 0., rBack,
                                     0.5*fRearLeadThickness + margin, 0., 360.*deg);
        auto solidRear  = new G4SubtractionSolid("RearLeadWithHole", solidPlate, solidCone);
        auto logicRear  = new G4LogicalVolume(solidRear, fLead, "RearLead");
        new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zRear), logicRear, "RearLead",
                          logicWorld, false, 0, checkOverlaps);
        logicRear->SetVisAttributes(new G4VisAttributes(G4Colour(0.6, 0.0, 0.0, 0.6)));

        G4cout << " Rear lead hole: diameter " << 2.*(rFront + margin*slope)/cm << " cm at the front, "
               << 2.*(rBack - margin*slope)/cm << " cm at the back" << G4endl;
    }

    // ---------- Diagnostic screen (vacuum, so it does not disturb anything) ----------
    auto solidScreen = new G4Box("Screen", 0.5*fScreenSize, 0.5*fScreenSize, 0.5*screenThickness);
    auto logicScreen = new G4LogicalVolume(solidScreen, fVacuum, "Screen");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., zScreen), logicScreen, "Screen",
                      logicWorld, false, 0, checkOverlaps);
    logicScreen->SetVisAttributes(new G4VisAttributes(G4Colour(1.0, 0.3, 0.3, 0.5)));

    // Print the geometry so every run log records what was simulated
    G4cout << "\n===== Collimator geometry =====" << G4endl
           << " HDPE thickness     : " << fHDPEThickness/cm    << " cm (front face at z = 0)" << G4endl
           << " Lead thickness     : " << fLeadThickness/cm << " cm" << G4endl
           << " Graphite channel   : " << fChannelLength/cm << " cm  (total channel L = "
                                       << channelTotal/cm << " cm, L/D = " << channelTotal/fPinholeDiameter << ")" << G4endl
           << " Rear lead plate    : " << fRearLeadThickness/cm << " cm (behind the borated sheet)" << G4endl
           << " Borated thickness  : " << fBoratedThickness/cm << " cm, boron mass fraction "
                                       << fBoronFraction << G4endl
           << " Pinhole diameter D : " << fPinholeDiameter/cm  << " cm" << G4endl
           << " Side shielding     : " << (fSideShielding ? "ON (3x3 HDPE stack)" : "OFF") << G4endl
           << " Stack / sheet size : " << stackSize/cm << " x " << stackSize/cm << " cm" << G4endl
           << " Screen at z        : " << zScreen/cm           << " cm" << G4endl
           << "===============================\n" << G4endl;

    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    auto screenSD = new MySensitiveDetector("ScreenSD");
    SetSensitiveDetector("Screen", screenSD);   // attach to the logical volume named "Screen"
}

void DetectorConstruction::DefineCommands()
{
    fMessenger = new G4GenericMessenger(this, "/collimator/",
                                        "Collimator geometry (set BEFORE /run/initialize)");

    auto& c1 = fMessenger->DeclarePropertyWithUnit("hdpeThickness", "cm", fHDPEThickness,
                                                   "Thickness of the HDPE moderator block.");
    c1.SetStates(G4State_PreInit);
    c1.SetToBeBroadcasted(false);

    auto& c2 = fMessenger->DeclarePropertyWithUnit("boratedThickness", "cm", fBoratedThickness,
                                                   "Thickness of the borated HDPE sheet.");
    c2.SetStates(G4State_PreInit);
    c2.SetToBeBroadcasted(false);

    auto& c3 = fMessenger->DeclarePropertyWithUnit("pinholeDiameter", "cm", fPinholeDiameter,
                                                   "Diameter of the pinhole.");
    c3.SetStates(G4State_PreInit);
    c3.SetToBeBroadcasted(false);

    auto& c4 = fMessenger->DeclarePropertyWithUnit("screenGap", "cm", fScreenGap,
                                                   "Distance from the borated sheet to the screen.");
    c4.SetStates(G4State_PreInit);
    c4.SetToBeBroadcasted(false);

    auto& c5 = fMessenger->DeclareProperty("boronMassFraction", fBoronFraction,
                                           "Mass fraction of natural boron in borated HDPE (0.05 = 5%).");
    c5.SetStates(G4State_PreInit);
    c5.SetToBeBroadcasted(false);

    auto& c6 = fMessenger->DeclareProperty("sideShielding", fSideShielding,
                                           "Add 8 HDPE blocks around the moderator (true/false).");
    c6.SetStates(G4State_PreInit);
    c6.SetToBeBroadcasted(false);

    auto& c7 = fMessenger->DeclarePropertyWithUnit("leadThickness", "cm", fLeadThickness,
                                                   "Thickness of the lead brick behind the moderator (0 = none).");
    c7.SetStates(G4State_PreInit);
    c7.SetToBeBroadcasted(false);

    auto& c8 = fMessenger->DeclarePropertyWithUnit("rearLeadThickness", "cm", fRearLeadThickness,
                                                   "Lead plate behind the borated sheet, with a conical hole (0 = none).");
    c8.SetStates(G4State_PreInit);
    c8.SetToBeBroadcasted(false);

    auto& c9 = fMessenger->DeclarePropertyWithUnit("channelLength", "cm", fChannelLength,
                                                   "Graphite block with the beam channel, behind the lead brick (0 = none).");
    c9.SetStates(G4State_PreInit);
    c9.SetToBeBroadcasted(false);
}
