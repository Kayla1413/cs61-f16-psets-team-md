#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

// For readability purposes, bool type leveraged in cmd struct

typedef enum {false, true} bool;

// struct command
//    Data structure describing a command. Add your own stuff.

typedef struct command command;
struct command {
    int argc;      // number of arguments
    char** argv;   // arguments, terminated by NULL
    pid_t pid;     // process ID running this command, -1 if none
    int status;    // status leveraged for wait sys call
    bool bg_cmd;   // track commands in the background
	command* next; // next command to be processed
    int control;  // control operator
    int conditional;  // conditional operator, NULL if none
    int prev_status;    // status of previous process, NULL if not set
};


// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    c->status = 0;
    c->bg_cmd = false;
	c->next = NULL;
    c->control = -1;
    c->conditional = -1;
    c->prev_status = 0;
    return c;
}


// command_free(c)
//    Free command structure `c`, including all its words.

static void command_free(command* c) {
    for (int i = 0; i != c->argc; ++i)
        free(c->argv[i]);
    free(c->argv);
    free(c);
}


// command_append_arg(c, word)
//    Add `word` as an argument to command `c`. This increments `c->argc`
//    and augments `c->argv`.

static void command_append_arg(command* c, char* word) {
    c->argv = (char**) realloc(c->argv, sizeof(char*) * (c->argc + 2));
    c->argv[c->argc] = word;
    c->argv[c->argc + 1] = NULL;
    ++c->argc;
}


// COMMAND EVALUATION

// start_command(c, pgid)
//    Start the single command indicated by `c`. Sets `c->pid` to the child
//    process running the command, and returns `c->pid`.
//
//    PART 1: Fork a child process and run the command using `execvp`.
//    PART 5: Set up a pipeline if appropriate. This may require creating a
//       new pipe (`pipe` system call), and/or replacing the child process's
//       standard input/output with parts of the pipe (`dup2` and `close`).
//       Draw pictures!
//    PART 7: Handle redirections.
//    PART 8: The child process should be in the process group `pgid`, or
//       its own process group (if `pgid == 0`). To avoid race conditions,
//       this will require TWO calls to `setpgid`.

pid_t start_command(command* c, pid_t pgid) {
    (void) pgid;
    // Your code here!
    c->pid = fork();
    if (!c->pid)
        execvp(c->argv[0], c->argv);
    return c->pid;
}


// run_list(c)
//    Run the command list starting at `c`.
//
//    PART 1: Start the single command `c` with `start_command`,
//        and wait for it to finish using `waitpid`.
//    The remaining parts may require that you change `struct command`
//    (e.g., to track whether a command is in the background)
//    and write code in run_list (or in helper functions!).
//    PART 2: Treat background commands differently.
//    PART 3: Introduce a loop to run all commands in the list.
//    PART 4I: Change the loop to handle conditionals.
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* c) {    
	pid_t pid = 0;

	
	command* current = c;
    while(current && current->argc) {
		if(c->bg_cmd == true){
			pid = fork();
		}

        // run in background
       if (((current->control == TOKEN_AND || (current->control == TOKEN_OR)) && 
            current->conditional != TOKEN_AND && current->conditional != TOKEN_OR)) {
            pid = fork();
        }       

       if (pid && (current->control == TOKEN_AND || current->control == TOKEN_OR
            || current->conditional == TOKEN_AND || current->conditional == TOKEN_OR)) {
            current = current->next;
			continue;
		}

        if (!WIFEXITED(current->prev_status)) {
            current = current->next;
            continue;
        }

        if (current->conditional == TOKEN_AND && WEXITSTATUS(current->prev_status)) {
            current = current->next;
            continue;
        }

        if (current->conditional == TOKEN_OR && !WEXITSTATUS(current->prev_status)) {
            current = current->next;
            continue;
        }                        

        start_command(current, 0);
        int status;
        if (current->control != TOKEN_BACKGROUND) {
            waitpid(current->pid, &status, 0);
            
            if (current->next)
                current->next->prev_status = status;
        }

        if ((current->control != TOKEN_AND && current->conditional == TOKEN_AND) 
            || (current->control != TOKEN_OR && current->conditional == TOKEN_OR)) {
            
            //end child process
            if (!pid)
                exit(EXIT_SUCCESS);

            // run process background if &
            if (current->control != TOKEN_BACKGROUND) {
                waitpid(pid, &status, 0);
                pid = 0;
            }
		}

        current = current->next;
    }
}
   

// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {
    int type;
    char* token;
    // Your code here!
    command* c = command_alloc();
    command* current = c;
    while ((s = parse_shell_token(s, &type, &token)) != NULL) {
        if (type) {
            current->control = type;
            current->next = command_alloc();
            current = current->next;
            
            // Set conditional operator in next command to false
            if (type == TOKEN_AND || type == TOKEN_OR)
                current->conditional = type;
        }
        else
            command_append_arg(current, token);
    }
    if (c->argc)
        run_list(c);

    // free commands
    current = c;
    while (current) {
        command* next = current->next;
        command_free(current);
        current = next; 
    }
}


int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    int quiet = 0;

    // Check for '-q' option: be quiet (print no prompts)
    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        --argc, ++argv;
    }

    // Check for filename option: read commands from file
    if (argc > 1) {
        command_file = fopen(argv[1], "rb");
        if (!command_file) {
            perror(argv[1]);
            exit(1);
        }
    }

    // - Put the shell into the foreground
    // - Ignore the SIGTTOU signal, which is sent when the shell is put back
    //   into the foreground
    set_foreground(0);
    handle_signal(SIGTTOU, SIG_IGN);

    char buf[BUFSIZ];
    int bufpos = 0;
    int needprompt = 1;

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
        if (needprompt && !quiet) {
            printf("sh61[%d]$ ", getpid());
            fflush(stdout);
            needprompt = 0;
        }

        // Read a string, checking for error or EOF
        if (fgets(&buf[bufpos], BUFSIZ - bufpos, command_file) == NULL) {
            if (ferror(command_file) && errno == EINTR) {
                // ignore EINTR errors
                clearerr(command_file);
                buf[bufpos] = 0;
            } else {
                if (ferror(command_file))
                    perror("sh61");
                break;
            }
        }

        // If a complete command line has been provided, run it
        bufpos = strlen(buf);
        if (bufpos == BUFSIZ - 1 || (bufpos > 0 && buf[bufpos - 1] == '\n')) {
            eval_line(buf);
            bufpos = 0;
            needprompt = 1;
        }

        // Handle zombie processes and/or interrupt requests
        // Your code here!
    }

    return 0;
}
