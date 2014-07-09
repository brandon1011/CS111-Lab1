/*
	Jordi Burbano UID: 204-076-325
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

#define TEMP "temp.out"

int
command_status (command_t c)
{
  return c->status;
}

void exec_pipe(command_t cmd);
void exec_simple(command_t c);
void exec_and(command_t cmd, int time_travel);

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
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
		case AND_COMMAND:
		          exec_and(c,0);
		          break;
		default:;
	}
	//error (1, 0, "command execution not yet implemented");

}

/* Execute a pipe command */
void
exec_pipe(command_t cmd)
{
	cmd->u.command[0]->output = malloc((int)sizeof(TEMP));
	cmd->u.command[1]->input = malloc((int)sizeof(TEMP));
	//cmd->u.command[1]->input = cmd->u.command[0]->output;
	
	memcpy(cmd->u.command[0]->output, TEMP, (int)sizeof(TEMP));
	memcpy(cmd->u.command[1]->input, TEMP, (int)sizeof(TEMP));
	if (cmd->output != NULL)
		cmd->u.command[1]->output = cmd->output;
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
		//printf("Exit Status: %d\n",cmd->status);
	}
}

/* Executes AND_TYPE command */
void exec_and(command_t cmd, int time_travel)
{


      execute_command( cmd->u.command[0], time_travel );

  if( (cmd->u.command[0]->status) == 0)
    {
      execute_command( cmd->u.command[1], time_travel );
    }

}
