#ifndef STEPPING_HH
#define STEPPING_HH

#include "G4UserSteppingAction.hh"

class G4Step;

// Records neutrons and gammas leaving the BACK face of the central moderator block
// (the face the pinhole looks at).
class MySteppingAction : public G4UserSteppingAction
{
public:
    MySteppingAction() = default;
    ~MySteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;
};

#endif
