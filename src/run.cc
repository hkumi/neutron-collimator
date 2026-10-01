#include "run.hh"

#include "G4Run.hh"
#include "G4AnalysisManager.hh"

MyRunAction::MyRunAction()
{
    auto man = G4AnalysisManager::Instance();
    man->SetDefaultFileType("root");
    man->SetVerboseLevel(1);
    man->SetNtupleMerging(true);     // one output file even with several threads
    man->SetFileName("collimator");  // change in macro with /analysis/setFileName

    // Histogram 0: one entry per primary -> used to normalise "per primary event"
    man->CreateH1("nPrimaries", "Number of primary events", 1, 0., 1.);

    // Ntuple 0: every particle entering the screen
    man->CreateNtuple("Screen", "Particles entering the screen");
    man->CreateNtupleIColumn("event");
    man->CreateNtupleIColumn("pdg");
    man->CreateNtupleDColumn("E_eV");
    man->CreateNtupleDColumn("x_mm");
    man->CreateNtupleDColumn("y_mm");
    man->CreateNtupleDColumn("cosTheta");
    man->CreateNtupleDColumn("dirX");      // NEW: direction x component
    man->CreateNtupleDColumn("dirY");      // NEW: direction y component
    man->CreateNtupleIColumn("path");      // NEW: parts crossed (bits, see trackflags.hh)
    man->CreateNtupleIColumn("isPrimary"); // NEW: 1 = the source neutron itself
    man->FinishNtuple();

    // Ntuple 1: neutrons and gammas leaving the BACK face of the moderator
    man->CreateNtuple("ModExit", "Particles leaving the back face of the moderator");
    man->CreateNtupleIColumn("event");
    man->CreateNtupleIColumn("pdg");
    man->CreateNtupleDColumn("E_eV");
    man->CreateNtupleDColumn("x_mm");
    man->CreateNtupleDColumn("y_mm");
    man->CreateNtupleDColumn("cosTheta");
    man->FinishNtuple();
}

void MyRunAction::BeginOfRunAction(const G4Run*)
{
    G4AnalysisManager::Instance()->OpenFile();
}

void MyRunAction::EndOfRunAction(const G4Run* run)
{
    auto man = G4AnalysisManager::Instance();
    if (IsMaster()) {
        G4cout << "\n>>> Run finished: " << run->GetNumberOfEvent()
               << " primaries. Output file: " << man->GetFileName() << "\n" << G4endl;
    }
    man->Write();
    man->CloseFile();
}
