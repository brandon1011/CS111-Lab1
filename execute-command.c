/*
	Jordi Burbano	UID: 204-076-325
	Brandon Wu		UID: 603-859-458
*/
// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TEMP0 ".temp0.out"
#define TEMP1 ".temp1.out"

int
command_status (command_t c)
{
  return c->status;
}

void exec_pipe(command_t cmd);
void exec_simple(command_t c);
void exec_and(command_t cmd, int time_travel);
void exec_or(command_t cmd);

void
execute_command (command_t c, int time_travel)
{
    if( time_travel == 0 )
    {
    }
     
	switch (c->type)
	{
		case SIMPLE_COMMAND:
			exec_simple(c);
			break;
		case SEQUENCE_COMMAND:
			execute_command(c->u.command[0],0);
			execute_command(c->u.command[1],0);
			c->status = (c->u.command[0]->status)&&(c->u.command[1]->status);
			//printf("Exit status: %d\n",c->status);
			break;
		case PIPE_COMMAND:
			exec_pipe(c);
			break;
		case AND_COMMAND:
			exec_and(c,0);
			break;
		case OR_COMMAND:
			exec_or(c);
			break;
		case SUBSHELL_COMMAND:
			execute_command(c->u.subshell_command,0);
			c->status = c->u.subshell_command->status;
		default:;
	}
}

/* Execute a pipe command */
void
exec_pipe(command_t cmd)
{
	/* Implements a | b as a > tempfile; b < tempfile */
	if (cmd->output == NULL)
	{
		cmd->u.command[0]->output = malloc((int)sizeof(TEMP0));
		cmd->u.command[1]->input = malloc((int)sizeof(TEMP0));
		memcpy(cmd->u.command[0]->output, TEMP0, (int)sizeof(TEMP0));
		memcpy(cmd->u.command[1]->input, TEMP0, (int)sizeof(TEMP0));
	}
	else if (strcmp(cmd->output,TEMP0) == 0)
	{
		cmd->u.command[1]->output = cmd->output;

		cmd->u.command[0]->output = malloc((int)sizeof(TEMP1));
		cmd->u.command[1]->input = malloc((int)sizeof(TEMP1));
		memcpy(cmd->u.command[0]->output, TEMP1, (int)sizeof(TEMP1));
		memcpy(cmd->u.command[1]->input, TEMP1, (int)sizeof(TEMP1));

	}
	else if (strcmp(cmd->output,TEMP1) == 0)
	{
		cmd->u.command[1]->output = cmd->output;

		cmd->u.command[0]->output = malloc((int)sizeof(TEMP0));
		cmd->u.command[1]->input = malloc((int)sizeof(TEMP0));
		memcpy(cmd->u.command[0]->output, TEMP0, (int)sizeof(TEMP0));
		memcpy(cmd->u.command[1]->input, TEMP0, (int)sizeof(TEMP0));
	}
	execute_command(cmd->u.command[0],0);
	execute_command(cmd->u.command[1],0);
}

/* Executes a simple command*/
void
exec_simple(command_t cmd)
{
	FILE *fin = stdin;
	FILE *fout = stdout;
	pid_t child = fork();	// Call fork and store pid into child
	
	if (child == 0)	// If we are the child
	{
		// Set up I/O redirects
		if (cmd->input != NULL)
			fin = freopen(cmd->input, "r", stdin);
		if (cmd->output != NULL)
			fout = freopen(cmd->output, "w", stdout);

		// exec system call with simple command parameters
		if (execvp(cmd->u.word[0], cmd->u.word) == -1)
			error(1,0, "command not successful");
		fclose(fin);
		fclose(fout);
	}
	else if (child > 0)	// If we are the parent, wait for child to exit
	{
		waitpid(child, &cmd->status, 0);
	}
}

/* Executes AND_TYPE command */
inline void
exec_and(command_t cmd, int time_travel)
{
	execute_command( cmd->u.command[0], time_travel );
	if((cmd->u.command[0]->status) == 0)
	{
		execute_command(cmd->u.command[1], time_travel );
		cmd->status = cmd->u.command[1]->status;
	}
	else
		cmd->status = cmd->u.command[0]->status;
}

inline void
exec_or(command_t cmd)
{
	execute_command(cmd->u.command[0],0);
	if (cmd->u.command[0]->status != 0)
	{
		execute_command(cmd->u.command[1],0);
		cmd->status = cmd->u.command[1]->status;
	}
	else
		cmd->status = cmd->u.command[0]->status;
}
