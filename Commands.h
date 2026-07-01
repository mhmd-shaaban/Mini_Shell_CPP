// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <map>
#include <vector>
#include <string> //
#include <string.h>
#include <regex>
using namespace std;
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class JobEntry;
class Command {
    // TODO: Add your data members
protected:
    std::string  cmd_line;
    std::string original_cmd;
    //commands remember what we type
public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;
    std::string getCmdLine() const { return original_cmd; }
   void setOriginalCmd(const std::string& orig) {
    original_cmd = orig;
}


    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    static int builtin_counter;

    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ChangePromptCommand : public BuiltInCommand {
public:
    static int builtin_counter;

    ChangePromptCommand(const char *cmd_line);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};



class ExternalCommand : public Command {
    bool m_background;
    pid_t m_pid;
public:
    static int execution_counter;
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {
    }

    void execute() override;
};


class RedirectionCommand : public Command {
    // TODO: Add your data members
    int x;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members

public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class USBInfoCommand : public Command {
    // TODO: Add your data members **BONUS: 10 Points**

public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    char**  lastPwd;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    JobsList* m_list;
public:
    // TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {
    }

    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        int m_id;
        pid_t m_pid;
        Command* orig_com;

        // TODO: Add your data members
    public:
        JobEntry(int jobId, pid_t pid, Command* cmd_line)
            : m_id(jobId), m_pid(pid), orig_com(cmd_line) {}
        pid_t get_pid() {
            return m_pid;
        }
        int get_id() {
            return m_id;
        }
        Command* getCom() {
            return orig_com;
        }
    };
private:
    std::vector<JobEntry> jobs_list;
    int max_id;
    // TODO: Add your data members
public:
    JobsList();
    ~JobsList();

    void addJob(Command *cmd,pid_t pid) {
        max_id++;
        JobEntry toAdd = JobEntry(max_id, pid,cmd);
        jobs_list.push_back(toAdd);
    }
    std::vector<JobEntry>& get_vector() {   // we want to modify later thats why &
        return jobs_list;
    }
    void setMax(int max) {
        max_id = max;
    }
    int* getMax() {
        return &max_id;
    }
    void printJobsList() {
        removeFinishedJobs();
        for (size_t i = 0; i < jobs_list.size(); i++) {
            std::cout << "[" << jobs_list[i].get_id() << "] " << jobs_list[i].getCom()->getCmdLine() << std::endl;
        }
    }
    void killAllJobs() {
        removeFinishedJobs();
        for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it) {
            // 1. Send the SIGKILL signal to the process
            if (kill(it->get_pid(), SIGKILL) == -1) {
                perror("smash error: kill failed");
            }
            if (it->getCom() != nullptr) {
                delete it->getCom();
            }
        }
        jobs_list.clear();
        max_id = 0;
    }

    void removeFinishedJobs();

    JobEntry* getJobById(int jobId) {
        for (auto& entry : jobs_list) {
            if (entry.get_id() == jobId) {
                return &entry;
            }
        }
        return nullptr;
    }

    void removeJobById(int jobId) {
        for (auto it = jobs_list.begin(); it != jobs_list.end(); ++it) {
            if (it->get_id() == jobId) {
                delete it->getCom();
                // Remove the entry from the vector
                jobs_list.erase(it);
                return;
            }
        }
    }

    JobEntry *getLastJob(int *lastJobId) {
        if (jobs_list.empty()) {
            return nullptr;
        }
        return &jobs_list.back();
    }
    JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    JobsList* m_list;
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList * m_list;
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }
    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class SysInfoCommand : public BuiltInCommand {
public:
    SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() {
    }

    void execute() override;
};

class SmallShell {
private:
    // TODO: Add your data members
    time_t shell_start_time;
    double system_uptime_at_start;
    std::string prompt;
    char* last_dir;
    JobsList* jobs_list;
    vector<string> To_Print;
    map<string ,Command*> Alaising_Com;
    pid_t m_pid;
    pid_t m_curCmd;

    SmallShell();
public:
    static int totalExecutions;
    time_t getBootTime();
    string getPrompt() const { return prompt; }
    char* getLastDir() const { return last_dir; }
    JobsList* getJobsList() { return jobs_list; }
    pid_t getcurrPID() const {
        return m_curCmd;
    }
    pid_t getShellPid() const {
        return m_pid;
    }
    void setCurrPID(pid_t newCurr) {
        m_curCmd = newCurr;
    }

    void setPrompt(const std::string& new_prompt) { prompt = new_prompt; }
    void setLastDir(const char* new_last_dir) {
        if (last_dir != nullptr) {
            free(last_dir);
            last_dir = nullptr;
        }
        last_dir = strdup(new_last_dir);
    }
    map<string , Command*>& getMap() {
        return Alaising_Com;
    }
    vector<string>& getToPrint() {
        return To_Print;
    }
    string findName(string str) {
        size_t equal_pos = str.find('=');
        std::string alias_name = str.substr(6, equal_pos - 6);
        return alias_name;
    }
    string findCommand(string str) {
        size_t start_pos = str.find('=');
        std::string target_command = str.substr(start_pos + 2, str.length() - (start_pos + 2) - 1);
        return target_command;
    }
    void print_alias() {
        for (size_t i = 0; i < this->getToPrint().size(); ++i) {
            string alias_name = this->getToPrint()[i];
            Command* alias_cmd = this->getMap()[alias_name];
            std::cout << alias_name << "='" << alias_cmd->getCmdLine() << "'" << std::endl;
        }
    }

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator     because it is a singleton
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_