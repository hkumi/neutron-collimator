#ifndef RUN_HH
#define RUN_HH

#include "G4UserRunAction.hh"

class G4Run;

class MyRunAction : public G4UserRunAction
{
public:
    MyRunAction();
    ~MyRunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;
};

#endif
