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

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
		
	error (1, 0, "command execution not yet implemented");

}

void
exec_simple(command_t cmd)
{
	pid_t child = fork();
	
	if (child == 0)
	{
		/*if (execvp() == -1)
			error(1,0, "command not successful");*/
	}
	else if (child > 0)
	{
		waitpid(child, &cmd->status, 0);
	}
}
