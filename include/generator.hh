#ifndef GENERATOR_HH
#define GENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"

class G4GeneralParticleSource;
class G4Event;

class MyPrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
public:
    MyPrimaryGenerator();
    ~MyPrimaryGenerator() override;

    void GeneratePrimaries(G4Event* event) override;

private:
    G4GeneralParticleSource* fGPS = nullptr;
};

#endif
