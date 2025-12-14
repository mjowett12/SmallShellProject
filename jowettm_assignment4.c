/*  Assignment 4: SMALLSH
 *  Mallory Jowett
 *  Main program
 *  Uses code adapted from sample_parser.c on Assignment page and Canvas explorations
*/
#include "jowettm_a4_functions.h"

int last_fg_status = 0;          // initialize last foreground process status to 0
struct job job_array[MAX_JOBS];  // used to store background processes for tracking
int foreground_only_mode = 0;    // 0=off, 1=on

// signal handler for SIGTSTP
void handle_SIGTSTP(int signo)
{
    static int toggle_count = 0;
    toggle_count++;
    if(toggle_count % 2 == 1) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        foreground_only_mode = 1;
    } else {
        char* message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        foreground_only_mode = 0;
    }
}

int main()
{
    // SIGINT handling
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // SIGTSTP handling
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // initialize all job states to 0 (empty) in the background process array
    initialize_job_array(job_array);

	while(true)
	{
        check_job_statuses_nohang(job_array);
        
		struct command_line *curr_command = parse_input();

        // If no arguments were passed (blank line or #comment line), ignore
        if(curr_command->argc == 0) {
            free(curr_command);
            check_job_statuses_nohang(job_array);
            continue;
        }

        // Check first argv element for command
        if(!strcmp(curr_command->argv[0], "exit")) {                    // exit
            exit_command();
            free(curr_command);
            break;
        } else if(!strcmp(curr_command->argv[0], "cd")) {               // cd
            cd_command(curr_command);
        } else if(!strcmp(curr_command->argv[0], "status")) {           // status
            status_command();
        } else {                 
            other_command(curr_command, job_array);                     // other commands
        }

        // Free memory for curr_command
        free(curr_command);
	}
	return EXIT_SUCCESS;
}