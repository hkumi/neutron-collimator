#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

class G4GenericMessenger;
class G4Material;
class G4VPhysicalVolume;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

private:
    void DefineMaterials();
    void DefineCommands();

    G4GenericMessenger* fMessenger = nullptr;

    G4Material* fVacuum      = nullptr;
    G4Material* fHDPE        = nullptr;
    G4Material* fBoratedHDPE = nullptr;
    G4Material* fLead        = nullptr;
    G4Material* fGraphite    = nullptr;

    // ---- Geometry parameters (defaults = Juan's first design) ----
    // All can be changed from a macro BEFORE /run/initialize (see run1.mac)
    G4double fBlockSize        = 10.*cm;  // transverse size of HDPE block and borated sheet
    G4double fHDPEThickness    = 10.*cm;  // moderator thickness along the beam
    G4double fBoratedThickness = 3.*cm;   // borated HDPE sheet (lab has 0.3 or 3 cm)
    G4double fPinholeDiameter  = 1.*cm;   // pinhole diameter D
    G4double fScreenGap        = 5.*cm;   // distance from back of borated sheet to screen
    G4double fScreenSize       = 30.*cm;  // screen is 30 x 30 cm
    G4double fBoronFraction    = 0.05;    // mass fraction of natural boron (5%)
    G4bool   fSideShielding    = false;   // true = add 8 HDPE blocks around the moderator (3x3 stack)
    G4double fLeadThickness    = 0.*cm;   // lead brick behind the moderator (0 = no lead)
    G4double fRearLeadThickness = 0.*cm;  // lead plate BEHIND the borated sheet, with a conical hole (0 = none)
    G4double fChannelLength    = 0.*cm;   // graphite block with the beam channel, behind the lead brick (0 = none)
};

#endif
