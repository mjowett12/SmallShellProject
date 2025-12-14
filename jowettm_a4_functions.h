/*  Assignment 4: SMALLSH
 *  Mallory Jowett
 *  Function headers needed for main program
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_JOBS 100
#define MAX_ARGS 512

struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};

struct job
{
	pid_t pid;				// process ID
	int status;				// waitpid status
	int job_state;			// 0=empty (unused struct), 1=alive, 2=done
};

void initialize_job_array(struct job job_array[]);
struct command_line *parse_input();
void exit_command();
void cd_command(struct command_line *curr_command);
void status_command();
void other_command(struct command_line *curr_command, struct job job_array[]);
void check_job_statuses_nohang(struct job job_array[]);

#endif