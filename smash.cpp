// #include <iostream>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <signal.h>
// #include "Commands.h"
// #include "signals.h"
//
// int main(int argc, char *argv[]) {
//     if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
//         perror("smash error: failed to set ctrl-C handler");
//     }
//
//
//     SmallShell &smash = SmallShell::getInstance();
//     while (true) {
//         std::cout << "smash> ";
//         std::string cmd_line;
//         std::getline(std::cin, cmd_line);
//         smash.executeCommand(cmd_line.c_str());
//     }
//     return 0;
// }
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }


    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getPrompt() << "> ";
        std::flush(std::cout); // Forces the prompt to show on the screen instantly!

        std::string cmd_line;
        // Check if getline actually successfully read a line.
        // If it reaches the end of a test file, break out of the infinite loop.
        if (!std::getline(std::cin, cmd_line)) {
            break;
        }

        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}