# Smash Shell

A custom Unix-style command shell implemented in C++, supporting job control, signal handling, I/O redirection, and pipes.

## Overview

Smash ("Small Shell") is a simplified reimplementation of a Unix shell. It parses and executes both built-in and external commands, manages background/foreground jobs, and handles process signals — demonstrating core operating-systems concepts such as process control, signal handling, and inter-process I/O redirection.

## Features

**Built-in commands**
- `chprompt` — change the shell prompt
- `showpid` — display the shell's process ID
- `pwd` — print the current working directory
- `cd <path>` — change directory (supports `-` for previous directory)
- `jobs` — list background/stopped jobs
- `fg [job-id]` — bring a job to the foreground
- `kill -<signal> <job-id>` — send a signal to a job
- `alias` / `unalias` — define and remove command aliases
- `unsetenv <var>` — remove an environment variable
- `sysinfo` — display system information
- `du` — report disk usage
- `whoami` — display the current user
- `usbinfo` — display connected USB device information

**Process & job control**
- Foreground and background (`&`) command execution
- Job tracking and management
- `Ctrl+C` handling: terminates the currently running foreground process

**I/O**
- Output redirection (`>`, `>>`)
- Pipes (`|`)
- External command execution via `fork`/`exec`

## Project Structure

```
.
├── smash.cpp       # Entry point / main shell loop
├── Commands.h      # Command class declarations
├── Commands.cpp    # Command parsing and execution logic
├── signals.h       # Signal handler declarations
├── signals.cpp     # Signal handler implementations
└── Makefile        # Build configuration
```

## Build & Run

Requirements: `g++` with C++11 support, on a Linux/Unix environment.

```bash
make
./smash
```

You'll be greeted with a `smash>` prompt where you can run built-in and external commands.

To clean build artifacts:

```bash
make clean
```

## Example Usage

```
smash> pwd
/home/user/smash-shell
smash> chprompt myshell
myshell> ls -l &
myshell> jobs
[1] ls -l
myshell> fg 1
```

## Author

Built as part of an Operating Systems course project.
