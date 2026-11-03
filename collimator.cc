// Main program: builds the run manager, geometry, physics and actions.
#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

#include "QGSP_BIC_HP.hh"        // standard physics list with high-precision neutrons (< 20 MeV)
#include "G4ThermalNeutrons.hh"  // adds S(alpha,beta) thermal scattering for "TS_" elements

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

int main(int argc, char** argv)
{
    // Interactive mode with visualisation:
    //   ./collimator               -> opens the window with vis.mac
    //   ./collimator -v vis76.mac  -> opens the window with another view macro
    // Batch mode:
    //   ./collimator run1.mac
    G4String visMacro = "vis.mac";
    G4UIExecutive* ui = nullptr;
    if (argc == 1) {
        ui = new G4UIExecutive(argc, argv);
    } else if (argc == 3 && G4String(argv[1]) == "-v") {
        visMacro = argv[2];
        ui = new G4UIExecutive(1, argv);
    }

    auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

    runManager->SetUserInitialization(new DetectorConstruction());

    auto physicsList = new QGSP_BIC_HP();
    physicsList->RegisterPhysics(new G4ThermalNeutrons());
    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new MyActionInitialization());

    auto visManager = new G4VisExecutive();
    visManager->Initialize();

    auto UImanager = G4UImanager::GetUIpointer();

    if (!ui) {
        // batch mode:  ./collimator run1.mac
        UImanager->ApplyCommand("/control/execute " + G4String(argv[1]));
    } else {
        // interactive mode
        UImanager->ApplyCommand("/control/execute " + visMacro);
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
    return 0;
}
