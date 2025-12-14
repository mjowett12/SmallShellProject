/*  Small Shell Assignment
 *  Operating Systems course
 *  Complete definitions of functions
 *  Uses code adapted from sample_parser.c on Assignment page, Canvas explorations, and The Linux Programming Interface
 */

#include "jowettm_a4_functions.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define INPUT_LENGTH 2048

int last_fg_exit_signal;				// 0 if last fg process exited, 1 if last fg process terminated by signal 
extern int last_fg_status;
extern struct job job_array[MAX_JOBS];
extern int foreground_only_mode;

/* ********************************************************
 * function:  initialize_job_array
 *		Sets all job_states in job_array to 0 for empty
 */
void initialize_job_array(struct job job_array[]) 
{
	for(int i=0; i<MAX_JOBS; i++) {
		job_array[i].job_state = 0;
	}
}

/* ********************************************************
 * function:  parse_input
 *		Parses command entered by user and returns a command_line struct.
 */
struct command_line *parse_input()
{
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));
    char *token_ptr;

	// Get input
	printf(": ");
	fflush(stdout);
	// if this is interrupted by signal, reprompt
	while(fgets(input, INPUT_LENGTH, stdin) == NULL && errno == EINTR) {
		printf(": ");
		fflush(stdout);
	}

	// Tokenize the input
	char *token = strtok_r(input, " \n", &token_ptr);
	while(token){
		if(!strcmp(token,"<")){
			// following token is input file
			curr_command->input_file = strdup(strtok_r(NULL, " \n", &token_ptr));
		} else if(!strcmp(token, ">")){
			// following token is output file
			curr_command->output_file = strdup(strtok_r(NULL, " \n", &token_ptr));
		} else if(!strcmp(token, "&")){
			// specifies running as a background command
			if(!foreground_only_mode) {
				curr_command->is_bg = true;
			}
        } else if(!strncmp(token, "#", 1) && (curr_command->argc)==0) {
            // comment line, ignore whole line and go back to main
            break;
		} else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok_r(NULL," \n", &token_ptr);
	}

	// add null terminator to argv and return 
	curr_command->argv[curr_command->argc] = NULL;
	return curr_command;
}

/* ********************************************************
 * function: exit_command
 *		Terminates any remaining child processes and exits program.
 */
void exit_command()
{
	// check for all remaining open processes, forcefully terminate them
	int pid;
	int child_status;
	while((pid = waitpid(-1, &child_status, 0)) > 0) {
		// forcefully terminate process
		kill(pid, SIGKILL);
	}
}

/* ********************************************************
 * function: cd_command
 *		Changes the working directory, to the path of a directory given,
 *		or if no path is given, to the directory specified in the HOME environment variable.
 */
 void cd_command(struct command_line *curr_command)
 {
	// if no path was given in the command
	if(curr_command->argc < 2) {
		// change directory to HOME directory
		char *home = getenv("HOME");
		if(home != NULL) {
			if(chdir(home) != 0) {
				perror("cd failed");
			}
		}
	} else {
		// path was provided
		char *new_directory = curr_command->argv[1];
		if(chdir(new_directory) != 0) {
			perror("cd failed");
		}
	}
 }

 /* ********************************************************
  * function: status_command
  *		Ignores 3 built-in commands; for other commands, prints a message with either the exit status or 
  *		terminating signal of the last foreground process run.
  *		(External global variable last_fg_status tracks which it is.)
  */
void status_command()
{
	if(last_fg_status == 0) {		// includes if no fg commands have been run yet
		printf("exit value 0\n");
		fflush(stdout);
	} else {						// check whether exit status (other than 0) or signal number and print
		if(last_fg_exit_signal) {
			printf("terminated by signal %d\n", last_fg_status);
			fflush(stdout);
		} else {
			printf("exit value %d\n", last_fg_status);
			fflush(stdout);
		}
	}
}

/* ********************************************************
 * function: interpret_raw_status
 * 		Takes in raw status from waitpid and interprets if the process terminated normally or abnormally.
 *		Uses is_signal (0 for false, 1 for true) to indicate to calling function whether process ended abnormally.
 *		Returns exit code or signal number, as appropriate.
 */
int interpret_raw_status(int raw_status, int *is_signal) 
{
	if(WIFEXITED(raw_status)) {
		// child exited normally - return exit code
		*is_signal = 0;
		return WEXITSTATUS(raw_status);
	} else if(WIFSIGNALED(raw_status)) {
		// child exited abnormally - return signal number that caused termination
		*is_signal = 1;
		return WTERMSIG(raw_status);
	} else {
		return -1;	// this should never happen
	}
}

/* ********************************************************
 * function: print_bg_starting_message
 *		Prints identifying message about new bg process when it's spawned.
 */
void print_bg_starting_message(int pid) 
{
	printf("background pid is %d\n", pid);
	fflush(stdout);
}

/* ********************************************************
 * function: print_bg_exit_message
 *		Prints informative message about process exit status for the given bg pid.
 */
void print_bg_exit_message(int pid, int exit_code) 
{
	printf("background pid %d is done: exit value %d\n", pid, exit_code);
	fflush(stdout);
}

/* ********************************************************
 * function: print_bg_signal_message
 *		Prints informative message about process termination by signal for the given bg pid.
 */
void print_bg_signal_message(int pid, int signal_number) 
{
	printf("background pid %d is done: terminated by signal %d\n", pid, signal_number);
	fflush(stdout);
}

/* ********************************************************
 * function: check_job_statuses_nohang
 *		Checks for any bg child processes in job_array that have changed status, updates them in job_array,
 *		and prints appropriate message (exit message or signal termination message).
 */
void check_job_statuses_nohang(struct job job_array[])
{
	int child_status;
	int pid;
	
	while((pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
		// find matching job in job_array and update its status
		for(int i=0; i<MAX_JOBS; i++) {
			if(job_array[i].pid == pid && job_array[i].job_state == 1) {
				int is_signal = 0;	// will be updated to 1 for true if applicable, by interpret_raw_status
				job_array[i].status = interpret_raw_status(child_status, &is_signal);
				// Check if child process terminated normally or abnormally
				if(is_signal) {
					print_bg_signal_message(job_array[i].pid, job_array[i].status);
					job_array[i].job_state = 2;
				} else {
					print_bg_exit_message(job_array[i].pid, job_array[i].status);
					job_array[i].job_state = 2;
				}
			}
		}
	}
}

/* ********************************************************
 * function: redirect_stdin
 *		Redirects stdin for a child process, to specified input if one is provided (fg and bg processes),
 *		or to "dev/null" if not (only used for bg processes).
 */
void redirect_stdin(struct command_line *curr_command) 
{
	// if no input_file specified
	if(curr_command->input_file == NULL) {
		int null_in = open("/dev/null", O_RDONLY);
		if(null_in == -1) {
			perror("null input open");
			exit(1);
		}
		int result = dup2(null_in, 0);
		if(result == -1) {
			perror("null input dup2");
			exit(2);
		}
	// if input_file is specified
	} else {
		int specified_input = open(curr_command->input_file, O_RDONLY);
		if(specified_input == -1) {
			printf("cannot open %s for input\n", curr_command->input_file);
			fflush(stdout);
			exit(1);
		}
		int result = dup2(specified_input, 0);
		if(result == -1) {
			perror("specified input dup2");
			exit(2);
		}
	}
}

/* ********************************************************
 * function: redirect_stdout
 *		Redirects stdout for a child process, to specified output file if one is provided (fg and bg processes),
 *		or to "dev/null" if not (only used for bg processes).
 */
void redirect_stdout(struct command_line *curr_command) 
{
	// if no output_file specified
	if(curr_command->output_file == NULL) {
		int null_out = open("/dev/null", O_WRONLY);
		if(null_out == -1) {
			perror("null output open");
			exit(1);
		}
		int result = dup2(null_out, 1);
		if(result == -1) {
			perror("null output dup2");
			exit(2);
		}
	// if output_file is specified
	} else {
		int specified_output = open(curr_command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(specified_output == -1) {
			perror("specified output open");
			exit(1);
		}
		int result = dup2(specified_output, 1);
		if(result == -1) {
			perror("specified output dup2");
			exit(2);
		}
	}
}

/* ********************************************************
 * function: record_job
 *		Finds an unused job struct in the job_array and records given job_pid as a new background process spawned.
 */
void record_job(struct job job_array[], int job_pid)
{
	for(int i=0; i<MAX_JOBS; i++) {
		if(job_array[i].job_state != 0) {
			continue;
		} else {
			job_array[i].pid = job_pid;
			job_array[i].job_state = 1;	// 1=alive
			break;
		}
	}
}

/* ********************************************************
 * function: other_command
 *		For any command other than the 3 built-in commands, spawns a child process to execute it using exec().
 *		Differentiated signal handling behavior defined in child branch here, as needed.
 */
void other_command(struct command_line *curr_command, struct job job_array[])
{
    pid_t spawn_pid = fork();
    switch(spawn_pid) {
        case -1:
            perror("fork() failed");
            exit(1);
            break;
        case 0:     /******* CHILD *******/
			if(curr_command->is_bg) {
				redirect_stdin(curr_command);
				redirect_stdout(curr_command);
			} else {	// for foreground processes
				if(curr_command->input_file) {
					redirect_stdin(curr_command);
				}
				if(curr_command->output_file) {
					redirect_stdout(curr_command);
				}
				// SIGINT handling, set to default SIGINT behavior
    			struct sigaction SIGINT_action = {0};
    			SIGINT_action.sa_handler = SIG_DFL;
    			sigaction(SIGINT, &SIGINT_action, NULL);
			}
			// SIGTSTP handling (for fg and bg child processes)
    		struct sigaction SIGTSTP_action = {0};
    		SIGTSTP_action.sa_handler = SIG_IGN;
    		sigaction(SIGTSTP, &SIGTSTP_action, NULL);

            execvp(curr_command->argv[0], curr_command->argv);
			// exec returns only on error
			perror(curr_command->argv[0]);
			_exit(1);
            break;
        default:    /******* PARENT *******/
			int child_status;
			int is_signal = 0;

			// for a background child
			if(curr_command->is_bg){
				print_bg_starting_message(spawn_pid);
				record_job(job_array, spawn_pid);
			// for a foreground child	
			} else {
				waitpid(spawn_pid, &child_status, 0);
				int result = interpret_raw_status(child_status, &is_signal);
				if(is_signal) {
					last_fg_exit_signal = 1;
					printf("terminated by signal %d\n", result);
				} else {
					last_fg_exit_signal = 0;
				}
				last_fg_status = result;
			}
			break;
    }
}
