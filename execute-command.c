/*
	Jordi Burbano UID: 204-076-325
	Brandon Wu		UID: 603-859-458
*/
// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int
command_status (command_t c)
{
  return c->status;
}

void exec_simple(command_t c);

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
	switch (c->type)
	{
		case SIMPLE_COMMAND:
			exec_simple(c);
			break;
		case SEQUENCE_COMMAND:
			c->status = 0;
			execute_command(c->u.command[0],0);
			execute_command(c->u.command[1],0);
			break;
		default:;
	}
	//error (1, 0, "command execution not yet implemented");

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
