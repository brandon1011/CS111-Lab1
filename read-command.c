/*
	Jordi Burbano 	UID: 204-076-325
	Brandon Wu		UID: 603-859-458
*/

// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include <error.h>
/* TODO: 	
		1. Proper error msg format
		2.	Better documentation
		3. Detect single & as an ERROR
	FIXME:
		1. Subshell commands across multiple lines only picks up last word	
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "alloc.h"
#define BUFFER_SIZE 1024
#define WORD_SIZE		100

typedef enum
{
	TRUE,
	FALSE,
} boolean;

typedef enum
{
	WORD,			// A word not including whitespace or other token
	OPAREN,		// Left paren  (
	CPAREN,		// Right paren )
	AND,			// &&
	OR,			// ||
	PIPE,			// |
	SEMI,			// ;
	IN,			// <
	OUT,			// >
	COMMENT,		// #
	TICK,			// `
	INVALID,		// Invalid token (e.g. <<<)
} token_type;

typedef struct
{
	token_type type;
	char* word;
} token;

struct command_node
{
	command_t 					cmd;
	struct command_node* 	nxt;
};

struct command_stream
{
	/* command_stream implemented by linked list of commands*/
	struct command_node* list;
};

/* Determine if character is white space */
inline boolean 
is_space(char c)
{
	switch(c)
	{
		case ' ':
		case '\t':
		case '\n':
		case '\v':
		case	'\f': return TRUE;
		default:    return FALSE;
	}
};
/* Determine if argument c is special character */
inline boolean
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
		case '#':
		case '`':
			return TRUE;
		default: return FALSE;
	}
};

/* Returns the type of the token at position pos in line_buffer of length
	len. RETURNS INVALID if no valid token (e.g. white space) */
inline token_type
get_token_type(char* line_buffer, int pos, int len)
{
	char c = line_buffer[pos];	
	if (c=='(')
		return OPAREN;
	if (c==')')
		return CPAREN;
  if (c==';')
		return SEMI;
	if (c=='<')
		return IN;
	if (c=='>')
		return OUT;
	if (c=='#')
	{
		return COMMENT;
	}
	if (c=='&')
	{
		if(pos<len-1 && line_buffer[pos+1] == '&')
			return AND;
		else error(1, 0, "Single ampersand is invalid\n");;		// Invalid if single &
	}
	if (c == '|')
	{
		if (pos<len-1 && line_buffer[pos+1] == '|')
			return OR;
		else return PIPE;
	}
	else if (c == '`')
		return TICK;
	return INVALID;
}

/* GET_TOKEN
	Given line buffer and current pos within buffer, get the next token 
	in that line Return position of next char in buffer after end
	of token t. If no token exists, report INVALID token.
	RETURNS next unvisited pos in line_buffer, or len if EOL */
int
get_token(char* line_buffer, int len, int pos, token* t)
{
	char c;
	int word_len = 0;
	if (pos >= len)
	{
		t->type = INVALID;
		return pos;
	}
	while((pos<len) && is_space(c=line_buffer[pos])==TRUE)
		++pos;

	if ((pos<len) && is_special(c) == FALSE)	// if it is a word token
	{
		t->type = WORD;
		t->word = checked_malloc(WORD_SIZE);

		while (pos<len && is_space(c=line_buffer[pos]) == FALSE  
				&& is_special(c) == FALSE && word_len < WORD_SIZE)
		{
			t->word[word_len] = c;
			word_len++;
			pos++;
		}
		t->word[word_len] = '\0';
	}
	else	// If it is a valid special token
	{
		t->type = get_token_type(line_buffer, pos, len);
		if (t->type == COMMENT)
			return len;
		switch((t->type = get_token_type(line_buffer, pos, len)))
		{
			case AND:
			case OR:
				pos++;
			default: pos++;
		}
	}
	if (t->type == IN || t->type == OUT)
		if (line_buffer[pos-1] == line_buffer[pos])
	pos++;
	return pos;
}

/* 	Read a single line terminated by \n or EOF from file 
		RETURN len of line_buffer */
int
get_line(int (*get_next_byte) (void *), void* fp, char* line_buffer)
{
	char c;
	int len=0;
	c=(*get_next_byte)(fp);
	while((len<BUFFER_SIZE) && c != '\n' && (c>0)) 
	{
		line_buffer[len] = c;	
		len++;
		c=(*get_next_byte)(fp);
	}
	if (c < 0)		// If c is negative, return EOF
		return -1;
	if (len >= BUFFER_SIZE)
		error(1,0, "Buffer Overflow");
	return len;
}

/* Count num of words in line_buffer up to \n or special token */
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

/* Get I/O redirect file beginning at pos, store the word into command_t */
int
get_io(char* line_buffer, int len, int pos, command_t cmd, int input,
	int* line_num)
{
	/* if input = 1, the redirect is input
		if input = 0, the redirect is an output	*/
	int i;
	char* word = checked_malloc(WORD_SIZE);
	token* t = checked_malloc(sizeof(token));
	if (input)
	{
		if (cmd->input != NULL)
			error(1,0, "Multiple I/O redirect :%d", *line_num);
		cmd->input = word;
	}
	else
	{
		if (cmd->output != NULL)
			error(1,0, "Multiple I/O Redirect :%d", *line_num);
		cmd->output = word;
	}
	i = get_token(line_buffer,len, pos, t);
	if (t->type != WORD)
		error(1,0, "I/O redirect needs to be followed by a word :%d", *line_num);
	memcpy(word, t->word, WORD_SIZE);
	free(t->word);
	get_token(line_buffer,len,i,t);
	if (t->type == WORD)
		error(1,0,"I/O redirect cannot be followed by > 1 word :%d", *line_num);
	return i;
}

/* If next cmd is a simple command, create the cmd starting at pos until the
end of the simple cmd (i.e newline, pipe, sequence, etc). RETURN last
visited position in line_buffer */
int
make_simple_cmd(int (*get_next_byte) (void*), void* fp, char* line_buffer,
		int len, int pos, command_t cmd, int subshell, int* line_num)
{
	int num_words = count_words(line_buffer+pos, len-pos);
	int i = 0, wnum = 0;
	token* t = checked_malloc(sizeof(token));
	
	if( get_next_byte == 0 && fp == 0)
	{
	}
	
	if (num_words == 0)
	{
		return -1;
	}
	if (subshell && (cmd->type == SIMPLE_COMMAND))
	{
		// If there is another simple command, it should be treated as
		// ( cmd1; cmd2)
		char** temp = cmd->u.word;
		cmd->type = SEQUENCE_COMMAND;
		cmd->u.command[0] = checked_malloc(sizeof(struct command));
		cmd->u.command[0]->status = -1;
		cmd->u.command[0]->type = SIMPLE_COMMAND;
		cmd->u.command[0]->u.word = temp;
		cmd->u.command[0]->input = cmd->input;
		cmd->u.command[0]->output = cmd->output;
		cmd->input = NULL;
		cmd->output = NULL;

		cmd->u.command[1] = checked_malloc(sizeof(struct command));
		cmd->u.command[1]->status = -1;
		return make_simple_cmd(get_next_byte, fp, line_buffer, len, pos,
			cmd->u.command[1], subshell, line_num);
	}
	else if (subshell && cmd->type == SEQUENCE_COMMAND)
	{
		/* For nested, multi-line subshell commands
			Example:	( a\n b\n c) should be treated as (a; b; c) */
		command_t tmp_cmd = cmd->u.command[1];
		cmd->u.command[1] = checked_malloc(sizeof(struct command));
		cmd->u.command[1]->type = SEQUENCE_COMMAND;
		cmd->u.command[1]->status=-1;
		cmd->u.command[1]->u.command[0] = tmp_cmd;
		cmd->u.command[1]->u.command[1] = checked_malloc(sizeof(struct command));
		cmd->u.command[1]->u.command[1]->status = -1;
		return make_simple_cmd(get_next_byte, fp, line_buffer, len, pos,
			cmd->u.command[1]->u.command[1], subshell, line_num);
	}
	else // For a new simple cmd
	{
		cmd->type = SIMPLE_COMMAND;
		cmd->u.word = checked_malloc(sizeof(void*)*num_words+1);
	}
	// Allocate mem for each word
	for (i=wnum; i<num_words; ++i)
		cmd->u.word[i] = checked_malloc(WORD_SIZE);

	i = get_token(line_buffer, len, pos, t);
	do
	{
		if (t->type==IN || t->type==OUT)
		{
			i = get_io(line_buffer, len, i, cmd, (t->type==IN),line_num);
		}
		else
			memcpy(cmd->u.word[wnum],t->word, WORD_SIZE);
		wnum++;
		i = get_token(line_buffer, len, i, t);
	}
	while ((wnum<num_words && t->type==WORD) || 
		t->type==IN || t->type==OUT);
	
	token_type type = t->type;
	free(t->word);
	switch(type)
	{
		case AND:
		case OR:
			return i-2;
		case IN:
		case OUT:
		case INVALID:
			return i;
		default: return i-1;
	}
}	

/* Parse the next command not yet read from the file stream and return it */
command_t
make_command(int (*get_next_byte) (void*), void* fp, char* line_buffer,
		int len, int* line_num, int subshell)
{
	/* subshell = 1 if current command is a SUBSHELL_COMMAND and should only
		terminate on a close paren 
		line_num tracks the current line number in the filestream 
		len stores the length of the currently active line buffer, len=0
		when there is no valid line stored in line_buffer (start of read) */
	boolean done = FALSE;
	command_t cmd = NULL;
	token* t = checked_malloc(sizeof(token));
	int pos = 0;

	int temp;
	
	while(done == FALSE || subshell==1)
	{
		while(pos>=len)
		{
			len = get_line(get_next_byte, fp, line_buffer);
			if (len < 0)
			{
				if (subshell)
					error(1,0, "%d: Subshell not terminated", *line_num);
				else return NULL;	
			}
			
			pos = 0;
			*line_num += 1;
		}
		
		done = TRUE;
		
		while(pos < len)
		{
			temp = get_token(line_buffer, len, pos, t);	
			token_type type = t->type;
			if (type!=WORD)
				pos= temp;
			if (type==WORD)
			{
				if (cmd == NULL)
					cmd = checked_malloc(sizeof(struct command));
				pos = make_simple_cmd(get_next_byte, fp, 
					line_buffer, len, pos, cmd, subshell, line_num);
			}
			else if (type==IN || type==OUT)
			{
				error(1,0, "%d: I/O not affiliated with simple command", *line_num);
			} 
			else if (type==AND || type==OR || type==PIPE || type == SEMI)
			{
				if (cmd == NULL)
					error(1,0, "%d: No left-hand operand", *line_num);
	
				command_t tmp_cmd;
				tmp_cmd = checked_malloc(sizeof(struct command));
				tmp_cmd->u.command[1] = checked_malloc(sizeof(struct command));
				tmp_cmd->status = -1;
				
				if((type==PIPE || type==SEMI)&&(cmd->type==AND_COMMAND ||
					cmd->type == OR_COMMAND))
				{
					tmp_cmd->u.command[0] = cmd->u.command[1];
					cmd->u.command[1] = tmp_cmd;
				}
				else 
				{
					tmp_cmd->u.command[0] = cmd;
					cmd = tmp_cmd;
				}
				
				while((pos = make_simple_cmd(get_next_byte, fp, line_buffer, 
					len, pos,tmp_cmd->u.command[1], subshell, line_num)) == -1)
				{
					len = get_line(get_next_byte, fp, line_buffer);		
					if (len == -1)
						error(1,0, "%d: Syntax Error", *line_num);
					pos = 0;
					*line_num += 1;
				}

				switch(type)
				{
					case AND:
						tmp_cmd->type = AND_COMMAND;
						break;
					case OR:
						tmp_cmd->type = OR_COMMAND;
						break;
					case PIPE:
						tmp_cmd->type = PIPE_COMMAND;
						break;
					case SEMI:
						tmp_cmd->type = SEQUENCE_COMMAND; 
						break;
					default:;
				}	
			}
			else if (type == OPAREN)
			{
				if (cmd != NULL)
					error(1,0,"%d: Non empty cmd preceding '('", *line_num);	
				cmd = checked_malloc(sizeof(struct command));
				cmd->status = -1;
				cmd->type = SUBSHELL_COMMAND;
				cmd->u.subshell_command = make_command(get_next_byte, fp,
					line_buffer+pos, len-pos, line_num, subshell+1);
				return cmd;
			}
			else if (type == CPAREN)
			{
				if (subshell >= 1)
					subshell--;
				else
					error(1,0,"%d: Unmatched ')'", *line_num);
			}
			else if (type == COMMENT)
				done = FALSE;
			else if (type == TICK)
			{
				error(1,0,"%d: Invalid symbol", *line_num);
			}
		}
	}
	return cmd;
}

/* Make a command stream given file handler */
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
	int num_cmds = 0;
	int* line = checked_malloc(sizeof(int));
	*line = 0;
	char *line_buffer = checked_malloc(BUFFER_SIZE);
	command_stream_t stream = checked_malloc(sizeof(struct command_stream));
	stream->list = checked_malloc(sizeof(struct command_node));
	struct command_node* walk = stream->list;

	while ((walk->cmd = make_command(get_next_byte,
				get_next_byte_argument, line_buffer, 0, line, 0)) != NULL)
	{
			walk->nxt = checked_malloc(sizeof(struct command_node));
			walk = walk->nxt;
			num_cmds++;
	}
  	free(line_buffer);
	free(line);
	return stream;
}
/* Read from a command stream, one command at a time until command stream
	is empty (returns NULL) */
command_t
read_command_stream (command_stream_t s)
{
	command_t cmd = s->list->cmd;
	struct command_node* temp = s->list;
	if (s->list != NULL)
		s->list = s->list->nxt;
	free(temp);
  return cmd;
}

