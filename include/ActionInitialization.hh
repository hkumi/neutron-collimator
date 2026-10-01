#ifndef ACTIONINITIALIZATION_HH
#define ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class MyActionInitialization : public G4VUserActionInitialization
{
public:
    MyActionInitialization() = default;
    ~MyActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

#endif
