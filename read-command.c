// UCLA CS 111 Lab 1 command reading
#include "command.h"
#include "command-internals.h"
#include <error.h>
/* TODO: 	
					1. check syntax errors
					2. syntax error when sequence, and or, etc followed by single word
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "alloc.h"
#define BUFFER_SIZE 1024
#define WORD_COUNT 	10
#define WORD_SIZE		100
typedef enum
{
	TRUE,
	FALSE,
} boolean;

typedef enum
{
	WORD,				// A word not including whitespace or other token
	OPAREN,			// Left paren  (
	CPAREN,			// Right paren )
	AND,				// &&
	OR,					// ||
	PIPE,				// |
	SEMI,				// ;
	IN,					// <
	OUT,				// >
	COMMENT,		// #
	INVALID,		// Invalid token (e.g. <<<)
} token_type;

typedef struct
{
	token_type type;
	char* word;
} token;

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
		case '#':
			return TRUE;
		default: return FALSE;
	}
};

inline
token_type
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
		else return INVALID;		// Invalid if single &
	}
	if (c == '|')
	{
		if (pos<len-1 && line_buffer[pos+1] == '|')
			return OR;
		else return PIPE;
	}
	return INVALID;
}

int
get_token(char* line_buffer, int len, int pos, token* t)
{
	char c;
	//int pos = 0;
	int word_len = 0;
	if (pos >= len)
	{
		t->type = INVALID;
		return pos;
	}
	while((pos<len) && is_space(c=line_buffer[pos])==TRUE)
		++pos;

	//printf("%c HERE\n", c);
	if ((pos<len) && is_special(c) == FALSE)	// if it is a word token
	{
		t->type = WORD;
		t->word = malloc(WORD_SIZE);

		//t->word[0] = c;
		while (pos<len && is_space(c=line_buffer[pos]) == FALSE  
						&& is_special(c) == FALSE && word_len < WORD_SIZE)
		{
			t->word[word_len] = c;
			word_len++;
			pos++;
		}
	}
	else	// if it is a special token
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
	if (len >= BUFFER_SIZE)
		error(1,0, "Buffer Overflow");
	return len;
}

int count_words(char* line_buffer, int len);

int
make_cmd_alt_aux(int (*get_next_byte) (void *), void* fp, char* line_buffer,
								int len, command_t cmd, int flag, boolean subshell)
{
	boolean done = FALSE;	// Signifies incomplete cmd
	token* t = malloc(sizeof(token));
	int word_count, wnum=0, i=0;
	char** word_ptr;
	
	cmd->status = -1;
	cmd->input = NULL;
	cmd->output = NULL;
	cmd->type = SIMPLE_COMMAND;

	while(done == FALSE || subshell == TRUE)
	{

		word_count = count_words(line_buffer, len);	

		if (word_count > 0 && wnum == 0) 
		{
			word_ptr = malloc((word_count+2)*sizeof(void*));	
			for (i=0; i<word_count+1; ++i)
				word_ptr[i] = malloc(WORD_SIZE);
			cmd->u.word = word_ptr;
		}
	
		i =0;
		while(i < len)	// while current line is not done
		{
			// if only word tokens, iterate til end of line
			// and create normal token
			
			done = TRUE;
			i = get_token(line_buffer, len, i, t);
			token_type type = t->type;	
		
			if(type == WORD)
			{
				memcpy(word_ptr[wnum], t->word, WORD_SIZE);
				free(t->word);
				wnum++;
			}
			else if (type == OPAREN)
			{
				if (wnum != 0)
					error(1,0, "Subshell error");	
				//free(word_ptr);	
				cmd->u.subshell_command = checked_malloc(sizeof(struct command));
				i+= make_cmd_alt_aux(get_next_byte, fp, line_buffer+i,
							len-i, cmd->u.subshell_command, 1, TRUE);
				i = get_token(line_buffer, len, i, t);
				if (t->type != CPAREN)
					error(1,0, "Need close paren");
				cmd->type = SUBSHELL_COMMAND;
				done = TRUE;
			}
			else if (type == CPAREN)
			{
				subshell = FALSE;
				return i-1;
			}
			// if {AND, OR, PIPE, SEQUENCE}_COMMAND
			else if ((type==AND) || (t->type==OR) || (t->type==PIPE)
					|| (type == SEMI))
			{
				cmd->u.command[0] = malloc(sizeof(struct command));
				cmd->u.command[1] = malloc(sizeof(struct command));
				memcpy(cmd->u.command[0], cmd, sizeof(struct command));
				cmd->u.command[0]->u.word = word_ptr;
				cmd->input = NULL;
				cmd->output = NULL;
				switch(type)
				{
					case AND:
						cmd->type = AND_COMMAND;
						break;
					case OR:
						cmd->type = OR_COMMAND;
						break;
					case PIPE:
						cmd->type = PIPE_COMMAND;
						break;
					case SEMI:
						cmd->type = SEQUENCE_COMMAND;
						break;
					default:;
				}

				i += make_cmd_alt_aux(get_next_byte, fp, line_buffer+i,
							 len-i, cmd->u.command[1], 1, FALSE);	
			}
			else if (type== IN)
			{
				if (cmd->input != NULL)
					error(1,0, "multiple input redirect");
				word_count = count_words(line_buffer+i, len-i);
				if (word_count > 1)
					error(1,0, "multiple symbols after <");
				else if (word_count < 1)
					error(1,0, "no symbols after <");
				i = get_token(line_buffer, len, i, t);
				cmd->input = malloc(WORD_SIZE);
				memcpy(cmd->input, t->word, WORD_SIZE);
			
			}
			else if (type == OUT)
			{
				if (cmd->output != NULL)
					error(1,0, "multiple output redirect");
				word_count = count_words(line_buffer+i, len-i);
				if (word_count > 1)
					error(1,0, "multiple symbols after >");
				else if (word_count < 1)
					error(1,0, "no symbols after >");
				i = get_token(line_buffer, len, i, t);
				cmd->output = checked_malloc(WORD_SIZE);
				memcpy(cmd->output, t->word, WORD_SIZE);
				
			}
			else if (type == COMMENT)
			{
				len = get_line(get_next_byte, fp, line_buffer);
				i=0;
				break;
			}
		}	
		// When the current line buffer is complete and more is needed
		if ((i >= len) && ((done == FALSE) || (subshell == TRUE)))
		{
			// Get a new line_buffer and reset i
			len = get_line(get_next_byte, fp, line_buffer);
			i = 0;
			if (len == 0 && flag == 0)
				return -1;
			else if (len == 0 && flag == 1)
				error(1,0, "Syntax Error");
			else if (len==0 && subshell == TRUE)
				error(1,0, "Subshell error");
			//TODO: if len=0 then file is EOF so output error.
		} 	
	}
	if (cmd->type == SIMPLE_COMMAND)
		cmd->u.word = word_ptr;
	return i;
}

command_t
make_command_alt(int (*get_next_byte) (void *), void* fp, char* line_buffer,
								int len)
{
	int val;
	command_t command = malloc(sizeof(command));
	val = make_cmd_alt_aux(get_next_byte, fp, line_buffer, 0, 
						command, 0, FALSE);
	if (val == -1)
		return NULL;
	return command;
}

inline
void
syntax_chk(int retval, int line)
{
	if (retval < 0)
		error (1, 0, "Error at line:%d\n", line);
		//printf("Error at line: %d\n",line);
}


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
		++i;
	}	
	if (i ==0)
		return -1;
	return pos;		// Track current position in the line_buffer
}

/* Helper function for make_command given line_buffer */
int
make_cmd_aux(char* line_buffer, int len, command_t cmd, int cmd_num)
{
	/* Returns number of words in cmd if type == SIMPLE_COMMAND*/
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

		if (word_count < num_words)
			i = make_word(line_buffer, len, i,word_ptr[word_count]);

		word_count++;
		if (i >= len)
			break;

		/* Detect I/O redirects */
		if ((c=line_buffer[i]) == '<')	// Detect Input redirect
		{
			if (i < len-1 && line_buffer[i+1] == '<')
				i++;
			cmd->input = malloc(WORD_SIZE*sizeof(char));
			i++;
			while(is_space(line_buffer[i]) == TRUE)
				i++;

			syntax_chk((i = make_word(line_buffer, len, i, cmd->input)),cmd_num);
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
			syntax_chk((i = make_word(line_buffer, len, i, cmd->output)),cmd_num);
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
				syntax_chk((make_cmd_aux(line_buffer+i+2,
						len-i-2,cmd->u.command[1], cmd_num)),cmd_num-100);

			else if (type==PIPE_COMMAND || type == SEQUENCE_COMMAND)
				syntax_chk((make_cmd_aux(line_buffer+i+1,
					len-i-1,cmd->u.command[1],cmd_num)),cmd_num-100);

			cmd->type = type;
			return 0;
		}
		else if (type == SUBSHELL_COMMAND)
		{
			cmd->u.subshell_command = malloc(sizeof(struct command));
			make_cmd_aux(line_buffer+i+1,len-1, cmd->u.subshell_command, cmd_num);
			cmd->type = type;
			return 0;
		}
			
	}
	cmd->u.word = word_ptr;
	cmd->type = SIMPLE_COMMAND;
	if (word_count == 0)
		return -1;
	return word_count;
}

command_t
make_command(int (*get_next_byte)(void *), void *fp, char *line_buffer, int cmd_num)
{
	command_t cmd = malloc(sizeof(struct command));
	int len = read_line(get_next_byte,fp, line_buffer);
	if (len < 0) 
	{
		free(line_buffer);
		return NULL;
	}
	make_cmd_aux(line_buffer,len,cmd, cmd_num);
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

 //	while ((walk->cmd = make_command(get_next_byte,
	//			get_next_byte_argument, line_buffer, num_cmds+1)) != NULL)
	while ((walk->cmd = make_command_alt(get_next_byte,
				get_next_byte_argument, line_buffer, BUFFER_SIZE)) != NULL)  
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
