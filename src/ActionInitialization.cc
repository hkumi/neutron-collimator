#include "ActionInitialization.hh"
#include "generator.hh"
#include "run.hh"
#include "event.hh"
#include "stepping.hh"

void MyActionInitialization::BuildForMaster() const
{
    SetUserAction(new MyRunAction());
}

void MyActionInitialization::Build() const
{
    SetUserAction(new MyPrimaryGenerator());
    SetUserAction(new MyRunAction());
    SetUserAction(new MyEventAction());
    SetUserAction(new MySteppingAction());   // NEW: moderator-exit diagnostic
}
