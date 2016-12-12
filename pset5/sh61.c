#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>


// Part 3: Command lists
//    Updated command struct as linked list. Other functions throughout
//    shell code leverage this to traverse command lists (eval, run, etc.)

// struct command
//    Data structure describing a command. Add your own stuff.

typedef struct command command;
struct command {
    int argc;      	  // number of arguments
    char** argv;   	  // arguments, terminated by NULL
    pid_t pid;     	  // process ID running this command, -1 if none
    struct command* next; // next command to be processed - need to traverse
    int control; 	  // control operator
    int conditional;  	  // conditional operator, NULL if none
    int prev_status;      // status of previous process, NULL if not set
    char* redirect_token; // redirect token 
};

// signal handler
void signal_handler(int signal) {
    (void) signal;
    printf("\nsh61[%d]$ ", getpid());
    fflush(stdout);
}

// command_alloc()
//    Allocate and return a new command structure.

static command* command_alloc(void) {
    command* c = (command*) malloc(sizeof(command));
    c->argc = 0;
    c->argv = NULL;
    c->pid = -1;
    c->next = NULL;
    c->control = -1;
    c->conditional = -1;
    c->prev_status = 0;
    c->redirect_token = NULL;
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
    // Part 1: Simple commands
    //    Forks copy of current process, and executes program on child 
    //    process using execvp system call. 
    c->pid = fork();
    setpgid(c->pid, c->pid);
    if (!c->pid) {
        execvp(c->argv[0], c->argv);
    }
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
//    PART 4: Change the loop to handle conditionals.
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* c) {
    pid_t pid = 0;
    int status = 0;
    command* current = c;
    
    while(current && current->argc) {
	//  Part 8: Interruption
	// if the last command was ctrl-c interrupt signal
        if ((WIFSIGNALED(status) != 0) && (WTERMSIG(status) == SIGINT))
            break;
	
	// Part 7: Redirections
        //    Supports three (3) kinds of redirections so the cmd's file descriptors
        //    are sent to disk files (< filename, > filename, 2> filename).
        if (current->control == TOKEN_REDIRECTION) {
    	    // Counts the total of redirections and creates a file descriptor array.
			int num_redirects = 0;
			command* cur = current;
			while (cur && cur->control == TOKEN_REDIRECTION) {
			    num_redirects++;
			    cur = cur->next;
			}
			int fd[num_redirects];
			pid_t cpid;
			cpid = fork();

			// Determines kind of Redirection and duplicates the file descriptor.
			if (!cpid) {
				command* cmd = current;
				for (int i = 0; i < num_redirects; i++) {
					current = current->next;
					if (!current || !current->argv[0]) {
						printf("No such file or directory\n");
						exit(EXIT_FAILURE);
					}
					// First kind of Redirection '<', standard input.
					if (!strcmp(current->redirect_token, "<")) {
		                fd[i] = open(current->argv[0], O_RDONLY);
		                if (fd[i] == -1) {
		                	printf("No such file or directory\n");
		                    exit(EXIT_FAILURE);
		                }
						dup2(fd[i], STDIN_FILENO);
					}
					// Second kind of Redirection '>', standard output.
					else if (!strcmp(current->redirect_token, ">")) {
		            	fd[i] = open(current->argv[0], O_WRONLY | O_CREAT, 0666);
		            	if (fd[i] == -1) {
		                	perror(strerror(errno));
		                	exit(EXIT_FAILURE);
		                }
						dup2(fd[i], STDOUT_FILENO);
					}
					// Third kind of Redirection '2>', standard error. 
					else if (!strcmp(current->redirect_token, "2>")) {
		            	fd[i] = open(current->argv[0], O_WRONLY | O_CREAT, 0666);
		                if (fd[i] == -1) {
		                	perror(strerror(errno));
		                	exit(EXIT_FAILURE);
		                }                        
						dup2(fd[i], STDERR_FILENO);
					}
					close(fd[i]);
				}
			
				// Part 5: Pipelines 
				//    Supports pipelines, which are chaings of commands linked by |.
				if (current->control == TOKEN_PIPE) {
				// Counts totals of Pipes and creates file descriptor array.
				int num_pipes = 0;
				command* cur2 = current;
				while (cur2 && cur2->control == TOKEN_PIPE) {
					num_pipes++;
					cur2 = cur2->next;
				} 
				int fd2[2 * num_pipes];

				// Executes Pipe syscall for total count computed above.
				for (int i = 0; i < num_pipes; i++) {
					if (pipe(fd2 + i*2) == -1) {
						perror("Failed pipe()\n");
						exit(EXIT_FAILURE);
					}
				}
	
				int cmd_count = 0;
	
				// Executes the Pipes.
				while (current && (!cmd_count || current->conditional == TOKEN_PIPE)) {
					cpid = fork();
					if (cpid == -1) {
						perror("Failed fork()\n");
						exit(EXIT_FAILURE);
					}

					// Child Process
					//    Determines if Piped is required, then duplicates the first file
					//    descriptor into the second file descriptor, so both can refer to 
					//    the same object. Closes associated descriptors, then executes.
					if (!cpid) {
						if (cmd_count)
							dup2(fd[cmd_count - 2], STDIN_FILENO);
						if (current->next)
							dup2(fd[cmd_count + 1], STDOUT_FILENO);
						for (int i = 0; i < 2*num_pipes; i++) 
							close(fd[i]);                
						if (cmd_count)
		                	execvp(current->argv[0], current->argv);
						execvp(cmd->argv[0], cmd->argv);
					}
	
					// Traverses cmd list.
					current->pid = cpid;
				    current = current->next;
				    cmd_count += 2;
				}

		    	// Close all associated file descriptors.
				for (int i = 0; i < 2*num_pipes; i++) {
		        	close(fd[i]);
		    	}

			   	// Wait for changes in the child of the calling proceses, then update status.
			   	waitpid(cpid, &status, 0);

			   	// Set the previous status to current status
			   	if (current)
				   current->prev_status = status;
			   	continue;
			}

			// Part 9: The cd command 
			//    Shell supports the cd directory command:
			//    If 'cd' is the 1st provided argument, updates exit status,
			//    and then uses 'chdir' system call to change current working 
			//    directory of the calling process to the directory specified in path.

			if (!strcmp(current->argv[0], "cd")) {
				if(chdir(current->argv[1])) {
					perror(strerror(errno));
				    exit(EXIT_FAILURE);
				}
				exit(EXIT_SUCCESS);
			}
			execvp(cmd->argv[0], cmd->argv);
		}
	
		// Traverse to next command.
		for (int i = 0; i < num_redirects; i++) {
			current = current->next;
		}

		// Part 2: Background commands
		//    If the cmd type does not indicate if should run in background,
		//    wait for changes in the child of the calling process, then update status. 
	
		if (current->control == TOKEN_PIPE) {
			int num_pipes = 0;
			while (cur && cur->control == TOKEN_PIPE) {
				num_pipes++;
				cur = cur->next;
			} 
			for (int i = 0; i < num_pipes; i++) {
		    	current = current->next;
		    }
		}

		if (current && current->control != TOKEN_BACKGROUND) {
			waitpid(current->pid, &status, 0);          
			if (current->next) {
				current->next->prev_status = status;
			}
		}


		// Traverse cmd list.
		current = current->next;
		continue;
  	}

	if (!strcmp(current->argv[0], "cd")) {
        if(chdir(current->argv[1]))
            perror(strerror(errno));
       	current = current->next;
        continue;
    }

	if (current->control == TOKEN_PIPE) {
    	
    	int num_pipes = 0;
		command* cur = current;
		while (cur && cur->control == TOKEN_PIPE) {
			num_pipes++;
			cur = cur->next;
		} 
     	int fd[2 * num_pipes];
      	pid_t cpid;

		for (int i = 0; i < num_pipes; i++) {
        	if (pipe(fd + i*2) == -1) {
            	perror("pipe failure\n");
              	exit(EXIT_FAILURE);
			}
		}

        int cmd_count = 0;

        while (current && (!cmd_count || current->conditional == TOKEN_PIPE)) {
           cpid = fork();

           if (cpid == -1) {
		       	perror("fork");
		      	exit(EXIT_FAILURE);
           	}
			setpgid(current->pid, current->pid);

           if (!cpid) {
            
              if (cmd_count)
               	dup2(fd[cmd_count - 2], STDIN_FILENO);

                if (current->next)
                    dup2(fd[cmd_count + 1], STDOUT_FILENO);

                for (int i = 0; i < 2*num_pipes; i++) {
                	close(fd[i]);
                }                    

                execvp(current->argv[0], current->argv);
            }

            current->pid = cpid;

            current = current->next;
            cmd_count += 2;
        }

        for (int i = 0; i < 2*num_pipes; i++) {
            close(fd[i]);
        }


        set_foreground(cpid);
        
        waitpid(cpid, &status, 0);
        set_foreground(0);

        if (current)
            current->prev_status = status;

        continue;

	}

    // Part 4: Conditionals
    //    Determines if there are chains of commands linked by && and/or ||.
    if (((current->control == TOKEN_AND || (current->control == TOKEN_OR)) && 
    current->conditional != TOKEN_AND && current->conditional != TOKEN_OR)) {
		pid = fork();
    }
    
    // Parent process - Determines if conditionals and then traverses.
    if (pid && (current->control == TOKEN_AND || current->control == TOKEN_OR)) {
        current = current->next;
        continue;
    }
 
    // Child Process - Updates status and traverses
    if (!pid) {
        if (current->next)
            current->next->prev_status = current->prev_status;
        if (!WIFEXITED(current->prev_status)) {
            current = current->next;
            continue;
        }
        if (current->conditional == TOKEN_AND && WEXITSTATUS(current->prev_status)){
            current = current->next;
            continue;
        }
	if (current->conditional == TOKEN_OR && !WEXITSTATUS(current->prev_status)){
            current = current->next;
            continue;
        }
    }
	
    // Determine if end of conditionals count.
    if ((current->conditional == TOKEN_AND || current->conditional == TOKEN_OR) 
        && current->control != TOKEN_AND && current->control != TOKEN_OR) {       
	    // Wait for changes in the child of the calling process.
            if (!pid) {
                start_command(current, 0);
				set_foreground(current->pid);
                waitpid(current->pid, &status, 0);
				set_foreground(0);
                exit(EXIT_SUCCESS);
            }
            if (current->control != TOKEN_BACKGROUND) {           
                waitpid(pid, &status, 0);
            }
            pid = 0;
            current = current->next;
            continue;
    }      
    
    start_command(current, 0);

    // Part 2: Background commands
    //    If the current cmd type does not indicate it should run in background,
    //    wait for changes in the child of the calling process.
    if (current->control != TOKEN_BACKGROUND) {
		set_foreground(current->pid);
		waitpid(current->pid, &status, 0);
		set_foreground(0);          
        if (current->next)
	    current->next->prev_status = status;
    }
    current = current->next;
    
    }
}


// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.

void eval_line(const char* s) {
    int type;
    char* token;

    // Declares new command
    command* c = command_alloc();
    command* current = c;

    while ((s = parse_shell_token(s, &type, &token)) != NULL) {
        // Initializes the command's data types.
	if (type) {
	    current->control = type;
            current->next = command_alloc();
            current = current->next;
            current->conditional = type;
	    if (type == TOKEN_REDIRECTION) {
		current->redirect_token = token;
            }
        }
        else { 
            command_append_arg(current, token);
	}
    }

    // Runs list of evaluated cmds.
    if (c->argc)
        run_list(c);
    
    // Frees all dynamic memory used by cmds.    
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
	signal(SIGINT, signal_handler);

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

	// Part 6: Zombie processes
        //    Reaps all zombies processes using waipid()
	int status;
        int pid = 1;
        while (pid != -1 && pid != 0) {
	    pid = waitpid(-1, &status, WNOHANG);
        }
		
    }
    return 0;
}

