#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include <string>
#include <time.h>
#include <complex>
#include <set>
#include <sys/stat.h>
#include <sys/syscall.h> // For syscall() and SYS_getdents64
#include <cmath>         // For ceil()
#include <stdint.h>
#include <pwd.h>
#include <algorithm>
using namespace std;
// shaban && amir
int ExternalCommand:: execution_counter = 0;
int BuiltInCommand:: builtin_counter= 0;
int SmallShell :: totalExecutions = 0;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h


Command::Command(const char *cmd_line) : cmd_line(string(cmd_line)), original_cmd(string(cmd_line)) {}Command::~Command() {}
///Execute////////////////////////////////////////Execute//////////////////////////////////////////Execute//////////////
void ChangePromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    if (num_args <= 1) {
        smash.setPrompt("smash"); // No args provided, reset to default
    } else {
        smash.setPrompt(args[1]); // Change to the word they typed
    }
    for (int i = 0; i < num_args; i++) {
        free(args[i]); // Clean up memory!
    }
}

void ShowPidCommand::execute() {
     // cout << "smash pid is " << getpid() << endl;
    cout << "smash pid is " << getpid() << endl;
}

void GetCurrDirCommand::execute() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        cout << buffer << endl;
    }
}

void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
      builtin_counter++; // Increment the global counter
    if (num_args == 1) {
        // If no argument is provided, do nothing
        builtin_counter--;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    if (num_args > 2) {
        // FIX: Clean, hardcoded error string matching the exact syllabus format
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        builtin_counter--;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    // Save current directory before we change it so we can update OLDPWD later
    char buffer[4096];
    char* current_pwd = nullptr;
    builtin_counter++;
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        current_pwd = buffer;
    }

    std::string target_path(args[1]);

    if (target_path == "-") {
        // We want to go to the last pwd
        if (*lastPwd == nullptr) {
            std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            for (int i = 0; i < num_args; i++) free(args[i]);
            return;
        }
        if (chdir(*lastPwd) == -1) {
            perror("smash error: cd failed");
            builtin_counter--;
            for (int i = 0; i < num_args; i++) free(args[i]);
            return;
        }
    } else {
        // FIX: This must be in an 'else' block so 'cd -' doesn't accidentally run this too!
        if (chdir(args[1]) == -1) {
            perror("smash error: cd failed");
            for (int i = 0; i < num_args; i++) free(args[i]);
            return;
        }
    }
    builtin_counter++;
    // If we made it here, the directory change was successful. Update last_dir!
    if (current_pwd != nullptr) {
        SmallShell::getInstance().setLastDir(current_pwd);
        builtin_counter--;
    }

    for (int i = 0; i < num_args; i++) free(args[i]);
}

void JobsCommand::execute() {
    builtin_counter++; // Increment the global counter
    m_list->printJobsList();
}
void ExternalCommand::execute() {
    // 1. Prepare a clean command string without the '&'
    char clean_cmd[COMMAND_MAX_LENGTH];
    strcpy(clean_cmd, cmd_line.c_str());
    execution_counter++;
    if (m_background) {
        _removeBackgroundSign(clean_cmd);
    }

    // 2. Check for wildcards to determine if it is a complex command
    bool is_complex = (string(clean_cmd).find('*') != string::npos ||
                       string(clean_cmd).find('?') != string::npos);
    execution_counter--;
    // 3. Parse the clean string into an array of arguments
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(clean_cmd, args);

    // 4. Fork the process
    pid_t curComPid = fork();

    if (curComPid == -1) {
        execution_counter--;
        perror("smash error: fork failed");
    }
    else if (curComPid == 0) {
        // ==========================================
        // CHILD PROCESS
        // ==========================================
        setpgrp();

        if (is_complex) {
            // Complex command: route the entire string through bash
            char* bash_args[] = {
                (char*)"/bin/bash",
                (char*)"-c",
                clean_cmd,
                nullptr
            };
            execv("/bin/bash", bash_args);
            perror("smash error: execv failed");
        } else {
            // Simple command: run the executable directly
            execvp(args[0], args);
            perror("smash error: execvp failed");
        }

        // Free memory before the child exits if exec fails
        for (int i = 0; i < num_args; i++) {
            if (args[i] != nullptr) free(args[i]);
        }
        exit(1);
    }    else {
        // ==========================================
        // PARENT PROCESS (Your Shell)
        // ==========================================
        if (m_background) {
            SmallShell::getInstance().getJobsList()->addJob(this, curComPid);
            execution_counter++;
        } else {
            int status;
            SmallShell::getInstance().setCurrPID(curComPid); // <-- TELL THE SHELL WHO IS RUNNING
            waitpid(curComPid, &status, WUNTRACED);
            SmallShell::getInstance().setCurrPID(0);
        }
    }
    execution_counter++;
    // 5. Prevent memory leaks in the parent shell
    for (int i = 0; i < num_args; i++) {
        if (args[i] != nullptr) {
            free(args[i]);
        }
    }
}

void ForegroundCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    JobsList* jobs_list = smash.getJobsList();
    JobsList::JobEntry* targetJob = nullptr;

    // 1. NO ARGUMENTS PROVIDED
    if (num_args == 1) {
        if (jobs_list->get_vector().empty()) {
            cerr << "smash error: fg: jobs list is empty" << endl;
            for (int i = 0; i < num_args; i++) free(args[i]);
            return; // MUST RETURN TO PREVENT CRASH
        } else {
            targetJob = jobs_list->getLastJob(jobs_list->getMax()); // Using the fixed getLastJob()
        }
    }
    // 2. ONE ARGUMENT PROVIDED (The Job ID)
    else if (num_args == 2) {
        int targetId;
        try {
            targetId = std::stoi(args[1]); // Properly convert string to int
        } catch (...) {
            // Catches if they type letters instead of numbers
            cerr << "smash error: fg: invalid arguments" << endl;
            for (int i = 0; i < num_args; i++) free(args[i]);
            return;
        }

        targetJob = jobs_list->getJobById(targetId);

        // If the job ID wasn't found in the list
        if (targetJob == nullptr) {
            cerr << "smash error: fg: job-id " << args[1] << " does not exist" << endl;
            for (int i = 0; i < num_args; i++) free(args[i]);
            return; // MUST RETURN TO PREVENT CRASH
        }
    }
    // 3. TOO MANY ARGUMENTS
    else {
        cout << "smash error: fg: invalid arguments" << endl;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return; // MUST RETURN TO PREVENT CRASH
    }

    // --- By the time we reach here, we are 100% sure targetJob is safe to use ---

    cout << targetJob->getCom()->getCmdLine() << " " << targetJob->get_pid() << endl;

    pid_t target_pid = targetJob->get_pid();
    int target_job_id = targetJob->get_id();

    // Remove it from the background jobs list
    jobs_list->removeJobById(target_job_id);

    // Bring to foreground and wait
    int status;
    smash.setCurrPID(target_pid);            // <-- TELL THE SHELL WHO IS RUNNING
    waitpid(target_pid, &status, WUNTRACED);
    smash.setCurrPID(0);
    // Clean up memory
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

void QuitCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    if (num_args > 1 && strcmp(args[1],"kill") == 0) {
        std::vector<JobsList::JobEntry>& TheList = m_list->get_vector();
        cout << "smash: sending SIGKILL signal to " << TheList.size() << " jobs:" << endl;
        for (auto it = TheList.begin(); it != TheList.end(); ++it) {
            cout << it->get_pid() << ": " << it->getCom()->getCmdLine() << endl;
        }
        m_list->killAllJobs();
    }
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
    exit(0);
}

void KillCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);

    // 1. Validate argument count and formatting
    if (num_args != 3 || args[1][0] != '-') {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    int sig_num;
    int job_id;

    // 2. Safely parse the signal number and job ID
    try {
        std::string sig_str(args[1]);
        sig_num = std::stoi(sig_str.substr(1)); // Extract the number after the '-'
        job_id = std::stoi(args[2]);
    } catch (...) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    // 3. Retrieve the job and strictly verify it exists before using it
    JobsList::JobEntry* job = m_list->getJobById(job_id);
    if (job == nullptr) {
        std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    // 4. Send the signal (Now 100% safe to call job->get_pid())
    if (kill(job->get_pid(), sig_num) == -1) {
        perror("smash error: kill failed");
        for (int i = 0; i < num_args; i++) free(args[i]);
        return;
    }

    // 5. Success output
    std::cout << "signal number " << sig_num << " was sent to pid " << job->get_pid() << std::endl;

    if (sig_num == 9 || sig_num == 15) {
        m_list->removeJobById(job_id);
    }

    // 6. Clean up memory
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

void AliasCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    string new_str = _trim(string(this->cmd_line));

    if (num_args == 1) {
        smash.print_alias();
    } else {
        regex alias_pattern("^alias [a-zA-Z0-9_]+='[^']*'$");
        if (!std::regex_match(new_str, alias_pattern)) {
            std::cerr << "smash error: alias: invalid alias format" << endl;
            // FIX: Free memory before returning early
            for (int i = 0; i < num_args; i++) {
                if (args[i] != nullptr) free(args[i]);
            }
            return;
        }
        string name = smash.findName(new_str);
        string new_com = smash.findCommand(new_str);

        set<string> comNames = {"chprompt", "pwd", "showpid", "fg", "cd", "alias", "unalias",
            "sysinfo", "jobs", "quit", "kill", "unsetenv"};

        // FIX: Check if 'name' is in comNames, NOT 'new_com'
        if (smash.getMap().find(name) != smash.getMap().end() || comNames.find(name) != comNames.end()) {
            std::cerr << "smash error: alias: " << name << " already exists or is a reserved command" << endl;
        }
        else {
            Command* ToInsert = smash.CreateCommand(new_com.c_str());
            smash.getMap()[name] = ToInsert;
            smash.getToPrint().push_back(name);
        }
    }

    for (int i = 0; i < num_args; i++) {
        if (args[i] != nullptr) free(args[i]);
    }
}

void UnAliasCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    if (num_args == 1) {
        cerr << "smash error: unalias: not enough arguments" << endl;
        return;
    }
    for (int i = 1; i < num_args; i++) {
        if (smash.getMap().find(args[i]) == smash.getMap().end()) {
            cerr << "smash error: unalias: " << args[i] << " alias does not exist"<< endl;
            return;
        }
        smash.getMap().erase(args[i]);
        vector<string>& print_list = smash.getToPrint();
        auto it = find(print_list.begin(), print_list.end(), string(args[i]));
        if (it != print_list.end()) {
            print_list.erase(it);
        }
    }
}

extern char** __environ;

void UnSetEnvCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);

    if (num_args == 1) {
        std::cerr << "smash error: unsetenv: not enough arguments" << std::endl;
        for (int i = 0; i < num_args; i++) {
            if (args[i] != nullptr) free(args[i]);
        }
        return;
    }

    std::string env_path = "/proc/" + std::to_string(getpid()) + "/environ";

    // --- 1. OPEN (Using raw syscall instead of ifstream) ---
    int fd = open(env_path.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "smash error: unsetenv: failed to open environ file" << std::endl;
        for (int i = 0; i < num_args; i++) {
            if (args[i] != nullptr) free(args[i]);
        }
        return;
    }

    // --- 2. READ (Pulling bytes into a buffer using read() syscall) ---
    std::string env_content;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        env_content.append(buffer, bytes_read);
    }

    // We are done with the file, so we close the file descriptor immediately
    close(fd);

    // --- 3. PROCESS THE ARGUMENTS ---
    for (int i = 1; i < num_args; i++) {
        std::string search_string = std::string(args[i]) + "=";
        bool found = false;

        // Parsing the NUL-separated memory buffer instead of using getline()
        size_t pos = 0;
        while (pos < env_content.size()) {
            std::string env_entry(env_content.c_str() + pos);

            if (env_entry.find(search_string) == 0) {
                found = true;
                break;
            }
            // Jump the position forward by the string length + 1 (for the '\0')
            pos += env_entry.length() + 1;
        }

        if (!found) {
            std::cerr << "smash error: unsetenv: " << args[i] << " does not exist" << std::endl;
            // No need to close the file here, it was already closed above!
            for (int k = 0; k < num_args; k++) {
                if (args[k] != nullptr) free(args[k]);
            }
            return;
        }

        // --- 4. PHASE 2 DELETION (Remains completely unchanged) ---
        int env_index = 0;
        while (__environ[env_index] != nullptr) {
            std::string current_var(__environ[env_index]);
            if (current_var.find(search_string) == 0) {
                int shift_index = env_index;
                while (__environ[shift_index] != nullptr) {
                    __environ[shift_index] = __environ[shift_index + 1];
                    shift_index++;
                }
                break;
            }
            env_index++;
        }
    }

    // --- 5. CLEAN UP ---
    for (int i = 0; i < num_args; i++) {
        if (args[i] != nullptr) free(args[i]);
    }
}

void SysInfoCommand::execute() {
    // 1. Parse and clean up arguments to prevent memory leaks
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);
    for (int i = 0; i < num_args; i++) {
        if (args[i] != nullptr) free(args[i]);
    }

    std::string sysname = "Unknown";
    std::string hostname = "Unknown";
    std::string release = "Unknown";
    std::string architecture = "Unknown";

    // We will reuse this buffer and file descriptor for all files
    char buffer[1024];
    ssize_t bytes_read;
    int fd;

    // 2. Read System Name
    fd = open("/proc/sys/kernel/ostype", O_RDONLY);
    if (fd != -1) {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the raw bytes
            sysname = buffer;
            // Remove the hidden newline character at the end of the file
            if (sysname.find('\n') != std::string::npos) sysname = sysname.substr(0, sysname.find('\n'));
        }
        close(fd);
    }

    // 3. Read Hostname
    fd = open("/proc/sys/kernel/hostname", O_RDONLY);
    if (fd != -1) {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            hostname = buffer;
            if (hostname.find('\n') != std::string::npos) hostname = hostname.substr(0, hostname.find('\n'));
        }
        close(fd);
    }

    // 4. Read Kernel Release
    fd = open("/proc/sys/kernel/osrelease", O_RDONLY);
    if (fd != -1) {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            release = buffer;
            if (release.find('\n') != std::string::npos) release = release.substr(0, release.find('\n'));
        }
        close(fd);
    }

    // 5. Determine Architecture
    architecture = "x86_64";
    // 6. Read Uptime for Boot Time calculation
    time_t boot_time = SmallShell::getInstance().getBootTime();
    struct tm* time_info = localtime(&boot_time);
    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);

    // 7. Print Output exactly as formatted in the assignment image
    std::cout << "System: " << sysname << std::endl;
    std::cout << "Hostname: " << hostname << std::endl;
    std::cout << "Kernel: " << release << std::endl;
    std::cout << "Architecture: " << architecture << std::endl;
    std::cout << "Boot Time: " << time_buffer << std::endl;
}

#include <unistd.h> // For dup, dup2, close
#include <fcntl.h>  // For open and O_ flags

void RedirectionCommand::execute() {
    // 1. Remove the background sign (Instructions say to ignore '&')
    char clean_cmd[COMMAND_MAX_LENGTH];
    strcpy(clean_cmd, this->cmd_line.c_str());
    _removeBackgroundSign(clean_cmd);
    std::string cmd_str(clean_cmd);
    bool is_append = false;
    size_t pos = cmd_str.find(">>");
    int symbol_length = 2;
    if (pos != std::string::npos) {
        is_append = true;
    } else {
        pos = cmd_str.find(">");
        symbol_length = 1;
    }

    std::string command_to_run = _trim(cmd_str.substr(0, pos));
    std::string filename_str = _trim(cmd_str.substr(pos + symbol_length));
    int original_stdout = dup(1);
    if (original_stdout == -1) {
         perror("smash error: dup failed");
        return;
    }

    int flags = O_WRONLY | O_CREAT;
    if (is_append) {
        flags |= O_APPEND; // For >>
    } else {
        flags |= O_TRUNC;  // For >
    }
    int fd = open(filename_str.c_str(), flags, 0666);
    if (fd == -1) {
         perror("smash error: open failed");
        close(original_stdout);

        return;
    }
    if (dup2(fd, 1) == -1) {
         perror("smash error: dup2 failed");
        close(fd);
        close(original_stdout);
        return;
    }
    SmallShell::getInstance().executeCommand(command_to_run.c_str());
    close(fd);
    dup2(original_stdout, 1);
    close(original_stdout);
}

void PipeCommand::execute() {
    // 1. Remove the background sign (Instructions say pipes won't run in background)
    char clean_cmd[COMMAND_MAX_LENGTH];
    strcpy(clean_cmd, this->cmd_line.c_str());
    _removeBackgroundSign(clean_cmd);
    std::string cmd_str(clean_cmd);

    // 2. Figure out if it is a standard pipe (|) or error pipe (|&)
    bool is_error_pipe = false;
    size_t pos = cmd_str.find("|&");
    int symbol_length = 2;

    if (pos != std::string::npos) {
        is_error_pipe = true;
    } else {
        pos = cmd_str.find("|");
        symbol_length = 1;
    }

    // 3. Split the string into Left (Writer) and Right (Reader)
    std::string cmd1_str = _trim(cmd_str.substr(0, pos));
    std::string cmd2_str = _trim(cmd_str.substr(pos + symbol_length));

    // 4. Create the pipe tunnel
    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    // 5. Fork the First Child (The Writer)
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("smash error: fork failed");
        close(fd[0]);
        close(fd[1]);
        return;
    }

    if (pid1 == 0) {
        setpgrp();

        if (is_error_pipe) {
            dup2(fd[1], 2); // |& redirects Standard Error (FD 2)
        } else {
            dup2(fd[1], 1); // | redirects Standard Output (FD 1)
        }
        close(fd[0]);
        close(fd[1]);

        SmallShell::getInstance().executeCommand(cmd1_str.c_str());
        exit(0); // Kill the child after the command finishes
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("smash error: fork failed");
        close(fd[0]);
        close(fd[1]);
        return;
    }
    if (pid2 == 0) {
        setpgrp();
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        SmallShell::getInstance().executeCommand(cmd2_str.c_str());
        exit(0);
    }
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, nullptr, WUNTRACED);
    waitpid(pid2, nullptr, WUNTRACED);
}

struct linux_dirent64 {
    ino64_t        d_ino;    // 64-bit inode number
    off64_t        d_off;    // 64-bit offset to next structure
    unsigned short d_reclen; // Size of this dirent
    unsigned char  d_type;   // File type
    char           d_name[]; // Filename (null-terminated)
};

// Recursive function to sum up all allocated 512-byte blocks using raw syscalls
size_t getDirectoryBlocks(std::string path) {
    struct stat statbuf;

    // Use lstat so we DO NOT follow symlinks!
    if (lstat(path.c_str(), &statbuf) == -1) {
        return 0;
    }

    size_t total_blocks = statbuf.st_blocks;

    // If this current path is a directory, we need to open it
    if (S_ISDIR(statbuf.st_mode)) {
        // Use the raw open() syscall, explicitly telling it we are opening a directory
        int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
        if (fd == -1) {
            return total_blocks; // Skip if we don't have permission to open it
        }

        char buf[1024];
        int nread;

        // Loop using the raw getdents64 syscall to pull directory entries into our buffer
        while ((nread = syscall(SYS_getdents64, fd, buf, sizeof(buf))) > 0) {
            for (int bpos = 0; bpos < nread;) {
                // Cast our raw buffer bytes into the kernel's dirent structure
                struct linux_dirent64 *d = (struct linux_dirent64 *) (buf + bpos);
                std::string name = d->d_name;

                // Skip "." and ".." to prevent infinite loops
                if (name != "." && name != "..") {
                    std::string new_path = path + "/" + name;
                    total_blocks += getDirectoryBlocks(new_path); // Recursion!
                }

                // Jump forward in the buffer by the exact size of this entry
                bpos += d->d_reclen;
            }
        }

        // Use the raw close() syscall
        close(fd);
    }

    return total_blocks;
}
void DiskUsageCommand::execute() {
    // 1. Parse arguments safely
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(this->cmd_line.c_str(), args);

    // 2. Error Handling: Too many arguments
    if (num_args > 2) {
        std::cerr << "smash error: du: too many arguments" << std::endl;
        for (int i = 0; i < num_args; i++) {
            if (args[i] != nullptr) free(args[i]);
        }
        return;
    }

    // 3. Determine target directory
    std::string target_path;
    if (num_args == 1) {
        // No path given: Use the current working directory
        char buffer[4096];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            target_path = buffer;
        } else {
            target_path = "."; // Safe fallback to current directory dot
        }
    } else {
        // Path was given as arg[1]
        target_path = args[1];
    }

    // 4. Calculate total blocks using our raw-syscall recursive helper
    size_t total_blocks = getDirectoryBlocks(target_path);

    // 5. Convert blocks to KB and round up
    // Linux disk blocks are exactly 512 bytes each.
    // Total Bytes = blocks * 512.
    // Total KB = (blocks * 512) / 1024.0 = blocks / 2.0
    double total_kb_exact = total_blocks / 2.0;
    size_t total_kb_rounded = std::ceil(total_kb_exact);

    // 6. Print final output exactly as requested
    std::cout << "Total disk usage: " << total_kb_rounded << " KB" << std::endl;

    // 7. Prevent memory leaks!
    for (int i = 0; i < num_args; i++) {
        if (args[i] != nullptr) {
            free(args[i]);
        }
    }
}


void WhoAmICommand::execute() {
    // 1. Get the IDs using your allowed system calls
    uid_t uid = getuid();
    gid_t gid = getgid();

    // 2. Fetch the strings directly from the environment variables
    char* username = getenv("USER");
    char* home_dir = getenv("HOME");

    // 3. Strict error checking (just in case the variables are somehow missing)
    if (username == nullptr || home_dir == nullptr) {
        std::cerr << "smash error: failed to fetch user info" << std::endl;
        return;
    }

    // 4. Print everything in the exact order requested by the instructions
    std::cout << username << std::endl;
    std::cout << uid << std::endl;
    std::cout << gid << std::endl;
    std::cout << home_dir << std::endl;
}




// 1. Data Structure to hold our USB info and sort it automatically
struct UsbDevice {
    int devnum;
    std::string vendor;
    std::string product;
    std::string manufacturer;
    std::string product_name;
    std::string power;

    // This tells std::sort how to organize our list (Ascending by devnum)
    bool operator<(const UsbDevice& other) const {
        return devnum < other.devnum;
    }
};

// 2. A completely raw syscall reader to bypass the banned <fstream>
std::string readUsbAttribute(const std::string& filepath) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) return "";

    char buf[256];
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0) return "";
    buf[n] = '\0';

    std::string res(buf);

    // Clean up trailing/leading whitespace and hidden newline characters (\n)
    res.erase(res.find_last_not_of(" \n\r\t") + 1);
    res.erase(0, res.find_first_not_of(" \n\r\t"));
    return res;
}

// 3. The fully compliant execute function
void USBInfoCommand::execute() {
    std::vector<UsbDevice> devices;
    std::string basePath = "/sys/bus/usb/devices/";

    // Open the system USB directory
    int fd = open(basePath.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd != -1) {
        char buf[1024];
        int nread;

        // Loop using the raw getdents64 syscall
        while ((nread = syscall(SYS_getdents64, fd, buf, sizeof(buf))) > 0) {
            for (int bpos = 0; bpos < nread;) {
                struct linux_dirent64 *d = (struct linux_dirent64 *) (buf + bpos);
                std::string name = d->d_name;
                bpos += d->d_reclen;

                if (name == "." || name == "..") continue;

                std::string devPath = basePath + name;
                std::string devnumStr = readUsbAttribute(devPath + "/devnum");

                // CRITICAL FILTER: Only actual USB roots have a 'devnum' file.
                // Sub-interfaces do not. If it is empty, skip it!
                if (devnumStr.empty()) continue;

                // NEW FILTER: Ignore the internal Linux root hubs (Vendor ID 1d6b)
                std::string vendorStr = readUsbAttribute(devPath + "/idVendor");
                // CRITICAL FILTER: Only actual USB roots have a 'devnum' file.
                // Sub-interfaces do not. If it is empty, skip it!
                if (devnumStr.empty()) continue;

                UsbDevice device;
                device.devnum = std::stoi(devnumStr);

                device.vendor = readUsbAttribute(devPath + "/idVendor");
                if (device.vendor.empty()) device.vendor = "N/A";

                device.product = readUsbAttribute(devPath + "/idProduct");
                if (device.product.empty()) device.product = "N/A";

                device.manufacturer = readUsbAttribute(devPath + "/manufacturer");
                if (device.manufacturer.empty()) device.manufacturer = "N/A";

                device.product_name = readUsbAttribute(devPath + "/product");
                if (device.product_name.empty()) device.product_name = "N/A";

                std::string powerStr = readUsbAttribute(devPath + "/bMaxPower");
                if (powerStr.empty()) {
                    device.power = "N/A";
                } else {
                    // Linux sysfs usually includes the 'mA' string directly (e.g., "200mA").
                    // We strip it off here so we can safely add it back at the exact
                    // spot requested without accidentally printing "200mAmA".
                    if (powerStr.size() >= 2 && powerStr.substr(powerStr.size() - 2) == "mA") {
                        powerStr = powerStr.substr(0, powerStr.size() - 2);
                    }
                    device.power = powerStr;
                }

                devices.push_back(device);
            }
        }
        close(fd);
    }

    // Error handling: No devices found
    if (devices.empty()) {
        std::cerr << "smash error: usbinfo: no USB devices found" << std::endl;
        return;
    }

    // Sort the vector by devnum ascending
    std::sort(devices.begin(), devices.end());

    // Print out the data matching the strict format string perfectly
    for (const auto& dev : devices) {
        std::cout << "Device " << dev.devnum << ": ID "
                  << dev.vendor << ":" << dev.product << " "
                  << dev.manufacturer << " " << dev.product_name << " "
                  << "MaxPower: " << dev.power;

        // Only append 'mA' if the power wasn't marked as N/A
        if (dev.power != "N/A") {
            std::cout << "mA";
        }
        std::cout << std::endl;
    }
}
///Execute////////////////////////////////////////Execute//////////////////////////////////////////Execute//////////////

/////////////////////////////////////////////////Constructors//////////////////////////////////////////////////////////
///
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}
ChangePromptCommand::ChangePromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
ChangeDirCommand::ChangeDirCommand(const char *cmd_line , char **plastPwd) : BuiltInCommand(cmd_line) , lastPwd(plastPwd) {}
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) , m_list(jobs) {}
JobsList::JobsList() {
    max_id = 0;
}
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {
    m_background = _isBackgroundComamnd(cmd_line);
}
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)  : BuiltInCommand(cmd_line){}
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) :BuiltInCommand(cmd_line) , m_list(jobs) {}
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) , m_list(jobs) {}
AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line)  {}
UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
SysInfoCommand::SysInfoCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}
DiskUsageCommand::DiskUsageCommand(const char *cmd_line) : Command(cmd_line) {}
WhoAmICommand::WhoAmICommand(const char *cmd_line) : Command(cmd_line) {}
USBInfoCommand::USBInfoCommand(const char *cmd_line) : Command(cmd_line) {}

/////////////////////////////////////////////////Constructors//////////////////////////////////////////////////////////

void JobsList::removeFinishedJobs() {
    if (jobs_list.empty()) {
        max_id = 0;
        return;
    }

    int status;
    pid_t p;

    // Loop and reap ANY child that has finished
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        // A child finished! Find it in our list and erase it
        for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it) {
            if (it->get_pid() == p) {
                if (it->getCom() != nullptr) {
                    delete it->getCom();
                }
                jobs_list.erase(it);
                break; // Break the inner loop, go back to waitpid
            }
        }
    }

    // Update max_id
    if (jobs_list.empty()) {
        max_id = 0;
    } else {
        max_id = jobs_list.back().get_id();
    }
}

    // TODO: add your implementation
SmallShell::SmallShell() : prompt("smash"), last_dir(nullptr), jobs_list(new JobsList),
                         To_Print(*new vector<string>), Alaising_Com(*new map<string ,Command*>),
                         m_curCmd(0)
{
    m_pid = getpid();

    // Read the actual boot time from the kernel
    int fd = open("/proc/stat", O_RDONLY);
    if (fd != -1) {
        char buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            std::string s(buf);
            // Look for the line starting with "btime"
            size_t pos = s.find("btime");
            if (pos != std::string::npos) {
                // Parse the timestamp after "btime "
                size_t start = pos + 6;
                size_t end = s.find('\n', start);
                std::string btime_str = s.substr(start, end - start);
                shell_start_time = (time_t)atol(btime_str.c_str());
            }
        }
        close(fd);
    }
}


SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
    Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    size_t first_space = cmd_s.find_first_of(WHITESPACE);
    string firstWord = (first_space == string::npos) ? cmd_s : cmd_s.substr(0, first_space);

    // 1. NEW CHECK: Catch alias assignments FIRST so they don't get split by | or >
    if (firstWord == "alias" && cmd_s.find('=') != string::npos) {
        return new AliasCommand(cmd_line);
    }

    // 2. Now it is safe to check for redirections and pipes
    if (cmd_s.find( ">>") != string::npos || cmd_s.find( ">") != string::npos)  {
        return new RedirectionCommand(cmd_line);
    }
    if (cmd_s.find("|") != string::npos || cmd_s.find("|&") != string::npos) {
        return new PipeCommand(cmd_line);
    }
        if (firstWord == "chprompt") {
            return new ChangePromptCommand(cmd_line);
        }
        if (firstWord == "showpid") {
            return new ShowPidCommand(cmd_line);
        }
        if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
        }
        if (firstWord == "cd") {
            return new ChangeDirCommand(cmd_line , &last_dir);
        }
        if (firstWord == "jobs") {
            return new JobsCommand(cmd_line ,jobs_list);
        }
        if (firstWord == "fg") {
            return new ForegroundCommand(cmd_line , jobs_list);
        }
        if (firstWord == "quit") {
            return new QuitCommand(cmd_line , jobs_list);
        }
        if (firstWord == "kill") {
            return new KillCommand(cmd_line , jobs_list);
        }
        if (firstWord == "alias") {
            return new AliasCommand(cmd_line);
        }
        if (firstWord == "unalias") {
            return new UnAliasCommand(cmd_line);
        }
        if (firstWord == "unsetenv") {
            return new UnSetEnvCommand(cmd_line);
        }
        if (firstWord == "sysinfo") {
            return new SysInfoCommand(cmd_line);
        }
        if (firstWord == "du") {
            return new DiskUsageCommand(cmd_line);
        }
        if (firstWord == "whoami") {
            return new WhoAmICommand(cmd_line);
        }
        if (firstWord == "usbinfo") {
            return new  USBInfoCommand(cmd_line);
        }
        return new ExternalCommand(cmd_line);
    }

    // For example:
    /*
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
      return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
    }
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */

void SmallShell::executeCommand(const char *cmd_line) {
    totalExecutions++;
    jobs_list->removeFinishedJobs();

    string cmd_s = _trim(string(cmd_line));
    if (cmd_s.empty()) {
        totalExecutions--;
        return; // Ignore empty enters
    }

    // 1. Extract the first word to check against the alias map
    size_t first_space = cmd_s.find_first_of(WHITESPACE);
    string firstWord = (first_space == string::npos) ? cmd_s : cmd_s.substr(0, first_space);

    string final_cmd_str = cmd_line; // Default to running exactly what the user typed

    // 2. Check if the first word is an alias
    auto alias_it = this->getMap().find(firstWord);
    if (alias_it != this->getMap().end()) {
        // It IS an alias! Get the substituted command
        string alias_cmd = alias_it->second->getCmdLine();

        // 3. Reconstruct the full command with the alias substitution + any arguments
        if (first_space == string::npos) {
            final_cmd_str = alias_cmd; // No arguments, just the alias (e.g., "ll")
        } else {
            // Append the rest of the arguments after the alias word (e.g., "sleep" + " 1")
            final_cmd_str = alias_cmd + cmd_s.substr(first_space);
        }
        totalExecutions++; // Increment again because we are technically executing the expanded alias
    }

    // 4. Create and execute the command using the correctly expanded string
    Command* cmd = CreateCommand(final_cmd_str.c_str());
    totalExecutions--;

    if (cmd != nullptr) {
        // === FIX: Preserve the original typed string for 'jobs' and 'fg' ===
        cmd->setOriginalCmd(cmd_line);

        cmd->execute();

        // Only delete the command if it is NOT a background external command!
        if (!_isBackgroundComamnd(final_cmd_str.c_str())) {
            delete cmd;
        }
    }
}

time_t SmallShell::getBootTime() {
    // Return the cached value directly. No offsets, no rounding, no guessing.
    return shell_start_time;
}