#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;
    SmallShell& myShell =  SmallShell::getInstance();
    if(myShell.getcurrPID() == 0){
        return;
    }
    if(myShell.getcurrPID() == -1){
        return;
    }
    if(kill(myShell.getcurrPID() , SIGKILL) == -1){
        perror("smash error: kill failed");
        return;
    }
    cout << "smash: process " << myShell.getcurrPID() << " was killed" << endl;
    myShell.setCurrPID(0);
}
