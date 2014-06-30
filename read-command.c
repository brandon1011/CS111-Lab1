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

/*************** Function Prototypes ***************/
command_t
make_command(int (*get_next_byte)(void *), void *fp);
/***************************************************/

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

/* Given position i determine if current command 
	is AND, OR, PIPE or SEQUENCE or Neither*/
inline
enum command_type
categorize_cmd(char* line, int pos, int len) 
{
	if (line[pos] == ';')
		return SEQUENCE_COMMAND;
	if (line[pos] == '&' && pos < len-1 && line[pos+1] == '&')
		return AND_COMMAND;
	if (line[pos] == '|')
	{
		if(pos < len-1 && line[pos+1] == '|')
				return OR_COMMAND;
		else return PIPE_COMMAND;
	}
	return SIMPLE_COMMAND;
}

/*	Helper function for make_command given line_buffer */
command_t
make_cmd_aux(char* line_buffer, int len, command_t cmd)
{
	int i;
	char** word_ptr = malloc(sizeof(void*));
	enum command_type type;

	*word_ptr = malloc(len*sizeof(char));
	cmd->status = -1;

	for(i = 0; i < len; i++)
	{
		// If SEQUENCE_COMMAND
		if ((type = categorize_cmd(line_buffer, i, len)) != SIMPLE_COMMAND)
		{
			// First cmd is SIMPLE_COMMAND
			cmd->u.command[0] = malloc(sizeof(struct command));
			cmd->u.command[0]->status = -1;
			cmd->u.command[0]->u.word = word_ptr;
			cmd->u.command[0]->type = SIMPLE_COMMAND;
			// Form second cmd recursively
			cmd->u.command[1] = malloc(sizeof(struct command));
			if (type==AND_COMMAND || type==OR_COMMAND)
				make_cmd_aux(line_buffer+i+2,len-i-2,cmd->u.command[1]);
			else
				make_cmd_aux(line_buffer+i+1,len-i-1,cmd->u.command[1]);
			cmd->type = type;
			return cmd;
		}
		*((*word_ptr)+i) = line_buffer[i];
	}
	//	free(line_buffer);
	cmd->u.word = word_ptr;
	cmd->type = SIMPLE_COMMAND;
	printf("SIMPLE_COMMAND: %s\n", *word_ptr);
	return cmd;
}

command_t
make_command(int (*get_next_byte)(void *), void *fp)
{
	command_t cmd = malloc(sizeof(struct command));
	char *line_buffer = malloc(sizeof(char)*BUFFER_SIZE);
	int len = read_line(get_next_byte,fp, line_buffer);
	if (len < 0) 
	{
		free(line_buffer);
		return NULL;
	}
	return make_cmd_aux(line_buffer,len,cmd);
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
