#include "generator.hh"

#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"

// The source is fully set in source.mac (particle, energy, position, direction),
// so you can change it without recompiling.
MyPrimaryGenerator::MyPrimaryGenerator()
{
    fGPS = new G4GeneralParticleSource();
}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fGPS;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event* event)
{
    fGPS->GeneratePrimaryVertex(event);
}
