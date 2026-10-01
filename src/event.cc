#include "event.hh"
#include "trackflags.hh"

#include "G4AnalysisManager.hh"

void MyEventAction::BeginOfEventAction(const G4Event*)
{
    TrackFlags::Map().clear();   // NEW: start every event with empty path flags
}

void MyEventAction::EndOfEventAction(const G4Event*)
{
    // count this primary (for the "per primary event" normalisation)
    G4AnalysisManager::Instance()->FillH1(0, 0.5);
}
