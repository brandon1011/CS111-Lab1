// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include <stdlib.h>
#include <stdio.h>
#define BUFFER_SIZE 1024

typedef enum
{
	TRUE,
	FALSE,
} boolean;

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
	command_t 			cmd;
	struct command_stream* nxt;
};
/* Determine if character is a special character */

boolean is_space(char c)
{
	switch(c)
	{
		case ' ':
		case '\t':
		case '\n':
		case '\v':
		case	'\f': return TRUE;
		default: return FALSE;
	}
};

/* Read line from stream into buffer and return its length */
int
read_line(int (*get_next_byte) (void *), void* fp, char* line_buffer)
{
	int len = 0;
	boolean expected_word = FALSE;
	boolean term_set = FALSE;
	char term = '\n';
	char c;

	while((c = (*get_next_byte)(fp)) != term || expected_word == TRUE)
	{
		if (c < 0)
			return -1;

		if (c == '\n')
			continue;

		if (term_set == FALSE) 
			if (c == '(')
			{
				term = ')';
				term_set = TRUE;
			}

		if (expected_word == TRUE) 
			if (is_space(c) == FALSE && c != ';' && c != '|' && c != '&')
				expected_word = FALSE;

		if (c == '&' || c== '|' || c == ';')
		{
			term_set = TRUE;
			expected_word = TRUE;
		}

		line_buffer[len] = c;
		len++;
	}
	if (term == ')')
	{
		line_buffer[len] = term;
		len++;
	}
	return len;
}

/* Given string determine command type and if command is complete */
enum command_type
categorize_command(char* line, int len) 
{
	int i;
	for (i = 0; i < len-1; ++i)
	{
		if (line[i] == '(')
			return SUBSHELL_COMMAND;
		if (line[i] == ';')
			return SEQUENCE_COMMAND;
		if (line[i] == '&' && line[i+1] == '&')
			return AND_COMMAND;
		if (line[i] == '|')
		{
			if (line[i+1] == '|')
				return OR_COMMAND;
			return PIPE_COMMAND;
		}
	}
	return SIMPLE_COMMAND;
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
