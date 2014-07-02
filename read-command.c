// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include <error.h>
/* TODO: 	
					1. check syntax errors
					2. Ignore # comments
					3. Allow for >> and <<
					4. Fix | bug for a<b>c|d<e>f|g<h>i 
*/
#include <stdlib.h>
#include <stdio.h>
#define BUFFER_SIZE 1024
#define WORD_COUNT 	10
#define WORD_SIZE		100
typedef enum
{
	TRUE,
	FALSE,
} boolean;

struct command_node
{
	command_t 						cmd;
	struct command_node* 	nxt;
};

struct command_stream
{
	/* command_stream implemented by linked list of commands*/
	struct command_node* list;
};

/* Determine if character is white space */
inline
boolean 
is_space(char c)
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
inline
boolean
is_special(char c)
{
	switch(c)
	{
		case ';':
		case '|':
		case '&':
		case '<':
		case '>':
		case '(':
		case ')':
			return TRUE;
		default: return FALSE;
	}
};
/* Read line from stream into buffer and return its length */
// TODO Add check for first char, if first char is whitespace
int
read_line(int (*get_next_byte) (void *), void* fp, char* line_buffer)
{
	/*TODO: Add function to return current line-num and remove ignores to \n*/
	int len = 0;
	boolean expected_word = FALSE;
	boolean term_set = FALSE;
	boolean first_char = FALSE;
	char term = '\n';
	char c;
	while(is_space(c=(*get_next_byte)(fp)) == TRUE && (c>=0));
	if (c == '#')
	{
		while((c=(*get_next_byte)(fp)) != '\n');
		return read_line(get_next_byte, fp, line_buffer);
	}
	while((c != term || expected_word == TRUE)
				&& (len < BUFFER_SIZE))
	{
		if (c < 0)
			return -1;

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

		c = (*get_next_byte)(fp);
	}
	if (term == ')')
	{
		line_buffer[len] = term;
		len++;
		while((c = (*get_next_byte)(fp)) != '\n');
	}
	return len;
}

/* Count the number of words in buffer up to (not including)
	special tokens (i.e. |, &, etc) */
int
count_words(char* line_buffer, int len)
{
	int i=0, num=0;
	char c;
	while (i < len)
	{
		while(i < len && is_space(c=line_buffer[i]) == TRUE
						&& is_special(c) == FALSE)
			i++;
		if (i < len && is_special(c) == FALSE)
			num++;
		while(i < len && is_space(c=line_buffer[i]) == FALSE
						&& is_special(c) == FALSE)
			i++;
		if(is_special(c) == TRUE)
		{
			break;
		}
	}
	return num;
}
/* Given position i determine if current command 
	is AND, OR, PIPE or SEQUENCE or None*/
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
	if (line[pos] == '(' && line[len-1] == ')')
		return SUBSHELL_COMMAND;
	return SIMPLE_COMMAND;
}

/* Generate word from line_buffer copy non-white space into word */
inline
int
make_word(char* line_buffer, int len, int pos, char* word)
{
	int i=0;
	char c;
	while ((pos<len) && (is_space(c=line_buffer[pos])==FALSE)
					&& is_special(c) == FALSE)
	{
		if (i > WORD_SIZE)
			return pos;
		word[i] = line_buffer[pos];
		pos++;
		i++;
	}	
	//if (i ==0)
		//return -1;
	return pos;		// Track current position in the line_buffer
}

/* Helper function for make_command given line_buffer */
void
make_cmd_aux(char* line_buffer, int len, command_t cmd)
{
	int i, word_count = 0;
	int num_words = count_words(line_buffer, len);
	//int num_words = WORD_COUNT;
	char c;
	char** word_ptr = malloc((num_words+1)*sizeof(void*));
	enum command_type type;
	
	for (i=0; i<num_words; i++)
		word_ptr[i] = malloc(WORD_SIZE*sizeof(char));
	cmd->status = -1;

	for(i = 0; i < len; i++)
	{
		if (is_space(line_buffer[i]) == TRUE)
			continue;

		i = make_word(line_buffer, len, i, word_ptr[word_count]);
		if (i >= len)
			break;
		word_count++;

		/* Detect I/O redirects */
		if ((c=line_buffer[i]) == '<')	// Detect Input redirect
		{
			if (i < len-1 && line_buffer[i+1] == '<')
				i++;
			cmd->input = malloc(WORD_SIZE*sizeof(char));
			i++;
			while(is_space(line_buffer[i]) == TRUE)
				i++;

			i = make_word(line_buffer, len, i, cmd->input);
			if (i >= len)
				break;
		}
		if ((c=line_buffer[i]) == '>')	// Detect Output redirect
		{
			if (i < len-1 && line_buffer[i+1] == '>')
				i++;
			cmd->output = malloc(WORD_SIZE*sizeof(char));
			i++;
			while(is_space(line_buffer[i]) == TRUE)
				i++;
			i = make_word(line_buffer, len, i, cmd->output);
			if (i >= len)
				break;
		}

		/* If {SEQUENCE, OR, AND, PIPE}_COMMAND */
		if ((type = categorize_cmd(line_buffer, i, len)) != SIMPLE_COMMAND
				&& type != SUBSHELL_COMMAND)
		{
			// First cmd is SIMPLE_COMMAND
			cmd->u.command[0] = malloc(sizeof(struct command));
			cmd->u.command[0]->status = -1;
			cmd->u.command[0]->u.word = word_ptr;
			cmd->u.command[0]->type = SIMPLE_COMMAND;
			cmd->u.command[0]->input = cmd->input;
			cmd->u.command[0]->output = cmd->output;
			cmd->input = NULL;
			cmd->output = NULL;
			// Form second cmd recursively
			cmd->u.command[1] = malloc(sizeof(struct command));
			cmd->u.command[1]->status = -1;
			if (type==AND_COMMAND || type==OR_COMMAND)
				make_cmd_aux(line_buffer+i+2,len-i-2,cmd->u.command[1]);
			else if (type==PIPE_COMMAND || type == SEQUENCE_COMMAND)
				make_cmd_aux(line_buffer+i+1,len-i-1,cmd->u.command[1]);
			cmd->type = type;
			return;
		}
		else if (type == SUBSHELL_COMMAND)
		{
			cmd->u.subshell_command = malloc(sizeof(struct command));
			make_cmd_aux(line_buffer+i+1,len-1, cmd->u.subshell_command);
			cmd->type = type;
			return;
		}
			
	}
	cmd->u.word = word_ptr;
	cmd->type = SIMPLE_COMMAND;
	return;
}

command_t
make_command(int (*get_next_byte)(void *), void *fp, char *line_buffer)
{
	command_t cmd = malloc(sizeof(struct command));
	int len = read_line(get_next_byte,fp, line_buffer);
	if (len < 0) 
	{
		free(line_buffer);
		return NULL;
	}
	make_cmd_aux(line_buffer,len,cmd);
	return cmd;
}
/* Make a command stream given file handler */
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	int num_cmds = 0;
	char *line_buffer = malloc(BUFFER_SIZE);
	command_stream_t stream = malloc(sizeof(struct command_stream));
	stream->list = malloc(sizeof(struct command_node));
	struct command_node* walk = stream->list;

 	while ((walk->cmd = make_command(get_next_byte,
				get_next_byte_argument, line_buffer)) != NULL) 
	{
			walk->nxt = malloc(sizeof(struct command_node));
			walk = walk->nxt;
			num_cmds++;
	}
	//error (1, 0, "command make command stream not yet implemented");
  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
	command_t cmd = s->list->cmd;
	struct command_node* temp = s->list;
	if (s->list != NULL)
		s->list = s->list->nxt;
  //error (1, 0, "command reading not yet implemented");
	free(temp);
  return cmd;
}
