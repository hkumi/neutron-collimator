#ifndef EVENT_HH
#define EVENT_HH

#include "G4UserEventAction.hh"

class G4Event;

class MyEventAction : public G4UserEventAction
{
public:
    MyEventAction() = default;
    ~MyEventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;
};

#endif
