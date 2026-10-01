#include "stepping.hh"
#include "trackflags.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Box.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    // 0) NEW - path flags: remember which parts of the geometry this track has crossed
    {
        const G4String& volName = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
        G4int bit = 0;
        if      (volName == "HDPE")                                   bit = 1;
        else if (volName == "Lead")                                   bit = 2;
        else if (volName == "ShieldHDPE" || volName == "SecondLayerHDPE") bit = 4;
        else if (volName == "ChannelGraphite")                        bit = 32;
        else if (volName == "BoratedHDPE")                            bit = 8;
        else if (volName == "RearLead")                               bit = 16;
        if (bit) TrackFlags::Map()[step->GetTrack()->GetTrackID()] |= bit;
    }

    // 1) Only steps that end on a volume boundary
    G4StepPoint* post = step->GetPostStepPoint();
    if (post->GetStepStatus() != fGeomBoundary) return;

    // 2) Only steps that start inside the central moderator ("HDPE") or the lead brick ("Lead")
    G4VPhysicalVolume* preVol = step->GetPreStepPoint()->GetPhysicalVolume();
    const G4String& preName = preVol->GetName();
    if (preName != "HDPE" && preName != "Lead") return;

    // 3) Only if the particle leaves through the BACK face (towards borated sheet / pinhole)
    auto box = static_cast<const G4Box*>(preVol->GetLogicalVolume()->GetSolid());
    const G4double zBack = preVol->GetTranslation().z() + box->GetZHalfLength();
    const G4ThreeVector pos = post->GetPosition();
    if (std::abs(pos.z() - zBack) > 1.*um) return;

    // 3b) Going from the moderator INTO the lead is not the exit yet: skip it.
    //     So this always records the last surface before the pinhole layer:
    //     the moderator back face without lead, the lead back face with lead.
    G4VPhysicalVolume* postVol = post->GetPhysicalVolume();
    if (postVol && postVol->GetName() == "Lead") return;

    // 4) Only neutrons and gammas
    G4int pdg = step->GetTrack()->GetDefinition()->GetPDGEncoding();
    if (pdg != 2112 && pdg != 22) return;

    G4int evt = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    auto man = G4AnalysisManager::Instance();
    man->FillNtupleIColumn(1, 0, evt);
    man->FillNtupleIColumn(1, 1, pdg);
    man->FillNtupleDColumn(1, 2, post->GetKineticEnergy()/eV);
    man->FillNtupleDColumn(1, 3, pos.x()/mm);
    man->FillNtupleDColumn(1, 4, pos.y()/mm);
    man->FillNtupleDColumn(1, 5, post->GetMomentumDirection().z());  // cos(angle to beam axis)
    man->AddNtupleRow(1);
}
