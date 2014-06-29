// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include <stdlib.h>
#include <stdio.h>
#define BUFFER_SIZE 1024
//make_command_stream (int (*get_next_byte) (void *),
//		     void *get_next_byte_argument)
//
/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
	command_t 			cmd;
	struct command_stream* nxt;
};

/* Read line from stream into buffer and return its length */
int
read_line(int (*get_next_byte) (void *), void* fp, char* line_buffer)
{
	int len = 0;
	char c;
	while((c = (*get_next_byte)(fp)) != '\n')
	{
		if (c < 0)
		{
			return -1;
		}
		line_buffer[len] = c;		
		len++;
	}	
	return len;
}
command_t
make_command(int (*get_next_byte)(void *), void *fp)
{
	struct command *cmd = malloc(sizeof(command_t));
	char *line_buffer = malloc(sizeof(char)*BUFFER_SIZE);
	int len = read_line(get_next_byte,fp, line_buffer);
	if (len < 0) 
	{
		free(line_buffer);
		return NULL;
	}
	cmd->type = SIMPLE_COMMAND;
	cmd->status = -1;
	cmd->u.word = malloc(sizeof(void *));
	*(cmd->u.word) =  malloc(len*sizeof(char));
	int i;
	for(i = 0; i < len; i++)
	{
		*((*(cmd->u.word))+i) = line_buffer[i];
	}
//	free(line_buffer);
	return cmd;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
	command_stream_t stream = malloc(sizeof(struct command_stream));
 	if ((stream->cmd = make_command(get_next_byte,
				get_next_byte_argument)) != NULL) 
	{
			stream->nxt=NULL;
	}
	//error (1, 0, "command make command stream not yet implemented");
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
	command_t cmd = s->cmd;
	s->cmd = NULL;
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
  return cmd;
}
