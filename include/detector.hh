#ifndef DETECTOR_HH
#define DETECTOR_HH

#include "G4VSensitiveDetector.hh"

class MySensitiveDetector : public G4VSensitiveDetector
{
public:
    explicit MySensitiveDetector(const G4String& name);
    ~MySensitiveDetector() override = default;

protected:
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
};

#endif
