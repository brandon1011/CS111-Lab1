/*
	Brandon Wu		UID: 603-859-458
*/
// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include <error.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WORD_SIZE 100
#define TEMP0 ".temp0.out"
#define TEMP1 ".temp1.out"

// Delete the dynamic content of a command
inline void delete_command(command_t c);

/* Determine if 2 strings are equal*/
inline int
equals(char* s1, char* s2)
{
	do {
		if (*(s1++) != *(s2++))
			return 0;
	} while(*s1!='\0' || *s2!='\0');
	return 1;
}
depend_node_t
add_dependency(command_t c, depend_node_t list)
{	
	char *resource = checked_malloc(WORD_SIZE);
	depend_node_t prev = list;
	list = list->nxt;

	if (c->output != NULL)
		memcpy(resource, c->output, WORD_SIZE);
	else memcpy(resource, c->input, WORD_SIZE);//resource = c->input;
	
	while(list != NULL)
	{
		if (equals(resource, list->handle))
		{
			//printf("Dependency on %s\n", resource);
			free(resource);
			return list;
		}
		prev=list;
		list=list->nxt;
	}
	//printf("No depdency for %s\n", resource);
	list = checked_malloc(sizeof(struct depend_node));
	list->handle = resource;
	list->pid = -1;
	list->nxt = NULL;
	prev->nxt = list;
	return list;
}
int
command_status (command_t c)
{
  return c->status;
}

void exec_pipe(command_t cmd);
void exec_simple(command_t c, int time_travel, depend_node_t dpn_list);
void exec_and(command_t cmd, int time_travel, depend_node_t dpn_list);
void exec_or(command_t cmd, int time_travel, depend_node_t dpn_list);

void
execute_command (command_t c, int time_travel, depend_node_t dpn_list)
{
    if( time_travel)
    {
    }
     
	switch (c->type)
	{
		case SIMPLE_COMMAND:
			exec_simple(c, time_travel, dpn_list);
			break;
		case SEQUENCE_COMMAND:
			execute_command(c->u.command[0],time_travel, dpn_list);
			execute_command(c->u.command[1],time_travel, dpn_list);
			c->status = (c->u.command[0]->status)&&(c->u.command[1]->status);
			//printf("Exit status: %d\n",c->status);
			break;
		case PIPE_COMMAND:
			exec_pipe(c);
			break;
		case AND_COMMAND:
			exec_and(c,time_travel, dpn_list);
			break;
		case OR_COMMAND:
			exec_or(c, time_travel, dpn_list);
			break;
		case SUBSHELL_COMMAND:
			execute_command(c->u.subshell_command,0, dpn_list);
			c->status = c->u.subshell_command->status;
		default:;
	}
	delete_command(c);
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
	execute_command(cmd->u.command[0],0, NULL);
	execute_command(cmd->u.command[1],0, NULL);
}

/* Executes a simple command*/
void
exec_simple(command_t cmd, int time_travel, depend_node_t dpn_list)
{
	FILE *fin = stdin;
	FILE *fout = stdout;
	pid_t blocked_on = 0;
	if(time_travel && (cmd->output!=NULL || cmd->input!=NULL))
	{
		dpn_list = add_dependency(cmd, dpn_list);
		pid_t blocked_on = dpn_list->pid;
	}

	if (time_travel && blocked_on >0)
	{
		/*printf("Blocked on %d Simple cmd: %s\n", 
				(int) blocked_on, cmd->u.word[0]);*/
		waitpid(blocked_on, &(cmd->status), 0);
	}
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
		//printf("Simple cmd: %s, PID: %d\n", cmd->u.word[0], child);
		if (time_travel == 0)
			waitpid(child, &cmd->status, 0);
		else if (dpn_list )
			dpn_list->pid = child;
	}
}

/* Executes AND_TYPE command */
inline void
exec_and(command_t cmd, int time_travel, depend_node_t dpn_list)
{
	execute_command( cmd->u.command[0], time_travel, dpn_list);
	if((cmd->u.command[0]->status) == 0)
	{
		execute_command(cmd->u.command[1], time_travel, dpn_list);
		cmd->status = cmd->u.command[1]->status;
	}
	else
		cmd->status = cmd->u.command[0]->status;
}

inline void
exec_or(command_t cmd, int time_travel, depend_node_t dpn_list)
{
	execute_command(cmd->u.command[0],time_travel, dpn_list);
	if (cmd->u.command[0]->status != 0)
	{
		execute_command(cmd->u.command[1],time_travel, dpn_list);
		cmd->status = cmd->u.command[1]->status;
	}
	else
		cmd->status = cmd->u.command[0]->status;
}

inline void
delete_command(command_t c)
{
	char** walk;
	char** temp;
	switch(c->type)
	{
		case SIMPLE_COMMAND:
			walk = c->u.word;
			while (*walk)
			{
				temp = walk;
				++walk;
				if (*temp != NULL)
					free(*temp);
			}
			free(c->u.word);
			if (c->output != NULL)
				free(c->output);
			if (c->input != NULL)
				free(c->input);
			return;
			case AND_COMMAND:
			case OR_COMMAND:
			case SEQUENCE_COMMAND:
			case PIPE_COMMAND:
				free(c->u.command[0]);
				free(c->u.command[1]);
				break;
			case SUBSHELL_COMMAND:
				free(c->u.subshell_command);
	}
}


