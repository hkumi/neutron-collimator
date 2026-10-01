#include "detector.hh"
#include "trackflags.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

MySensitiveDetector::MySensitiveDetector(const G4String& name)
: G4VSensitiveDetector(name)
{}

G4bool MySensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4StepPoint* pre = step->GetPreStepPoint();

    // Count each particle ONCE, at the moment it ENTERS the screen
    if (pre->GetStepStatus() != fGeomBoundary) return false;

    G4Track* track = step->GetTrack();
    G4int    pdg   = track->GetDefinition()->GetPDGEncoding();  // 2112 = neutron, 22 = gamma

    G4double      ekin = pre->GetKineticEnergy();
    G4ThreeVector pos  = pre->GetPosition();
    G4ThreeVector dir  = pre->GetMomentumDirection();

    G4int evt = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    auto man = G4AnalysisManager::Instance();
    man->FillNtupleIColumn(0, 0, evt);
    man->FillNtupleIColumn(0, 1, pdg);
    man->FillNtupleDColumn(0, 2, ekin/eV);   // energy in eV
    man->FillNtupleDColumn(0, 3, pos.x()/mm);
    man->FillNtupleDColumn(0, 4, pos.y()/mm);
    man->FillNtupleDColumn(0, 5, dir.z());   // cos(angle to beam axis), for beam divergence later

    // NEW: full direction and the path the particle took
    auto& flags = TrackFlags::Map();
    auto it = flags.find(track->GetTrackID());
    G4int path = (it != flags.end()) ? it->second : 0;
    man->FillNtupleDColumn(0, 6, dir.x());
    man->FillNtupleDColumn(0, 7, dir.y());
    man->FillNtupleIColumn(0, 8, path);
    man->FillNtupleIColumn(0, 9, track->GetParentID() == 0 ? 1 : 0);   // 1 = source neutron itself
    man->AddNtupleRow(0);

    return true;
}
