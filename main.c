/*
	Brandon Wu		UID: 603-859-458
*/
// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <dirent.h>	// readdir()
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>	// for testing if directory exists
#include <stdlib.h>
#include <readline/readline.h>	// for interctive subshell
#include <readline/history.h>

#include "command.h"
#include "alloc.h"

#define DIRLEN 256

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
ishell(); 

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  int interactive = 0;
  program_name = argv[0];

  depend_node_t depend_table = checked_malloc(sizeof(struct depend_node));
  //Dependency Table

  for (;;)
    switch (getopt (argc, argv, "pti"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
		case 'i': interactive = 1; break;	// Interactive Shell
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (!interactive && optind != argc - 1) 
    usage ();
  if (interactive) {
  	 printf("Interactive Shell\n");
	 ishell();
	 return 0;
  }

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;
  while ((command = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
      else
	{
	  if (last_command !=NULL)
			free(last_command);
	  last_command = command;
	  execute_command (command, time_travel, depend_table);
	}
    }
  // Deallocate the dependency table
  if (time_travel) // time_travel
  {
		depend_node_t temp = depend_table;
		depend_table = depend_table->nxt;
		free(temp);
		while (depend_table != NULL)
		{
			temp = depend_table;
			depend_table = depend_table->nxt;
			free(temp->handle);
			free(temp);
		}
  }
  return print_tree || !last_command ? 0 : command_status (last_command);
}


/* =================================================================
				INTERACTIVE SUBSHELL SUBROUTINES
	=================================================================	*/
char** parse_path(char** list, int* len, int* pos, char* input);
char** read_dir(char** list, int* len, int* pos, char *path);
char** resize(char** list, int *len);
int test_readline();

int
ishell() {
	char *buffer;
	command_t cmd;

	while (strcmp((buffer=readline("")), "exit")) {
		cmd = make_command(get_next_byte, NULL, buffer, strlen(buffer), 0, 0);
		print_command(cmd);
		if (buffer)
			free(buffer);
	}
	return 0;
}

int
test_readline()
{
	int i;
	int len = 1;
	int pos = 0;
	char** list = malloc(sizeof(char**)*len);

	char* c = getenv("PATH");
	list = parse_path(list, &len, &pos, c);

	for (i=0; i<pos; ++i)
		printf("%s\n", list[i]);
	return 0;
}

char**
parse_path(char** list, int* len, int* pos, char* input) {
	/* Given $PATH as string, process each dir
		@list	= list to store names of files in path
		@len 	= length of buffer to store files
		@pos	= current pos in the buffer
		@input	= string of $PATH	*/
	char* buffer = input;
	int size = 0;
	char c;
	while ((c=*(buffer++)) && c!=':')
		size++;
	if (size) {
		char* dir = malloc(size+1);
		memcpy(dir, input, size);
		dir[size] = '\0';
		// Do something to dir
		printf("%s\n", dir);
		list = read_dir(list, len, pos, dir);
		free(dir);
		if (c == ':')
			list = parse_path(list, len, pos, (input+size+1));
	}
	return list;
}
char**
read_dir(char** list, int *len, int *pos, char *path) {
	/*	Read contents of a directory and store each entry into
		the list.
		@list	= list to store entries
		@len	= length of the list
		@pos	= current position in list
		@path	= string containing absolute path to explore */

	// Check if path is a valid directory
	struct stat* s = malloc(sizeof(struct stat));;
	int err = stat(path, s);
	if (err == -1 || !S_ISDIR(s->st_mode))
		return list;
	
	// Open the directory for reading
	DIR *dir = opendir(path);
	struct dirent *current;
	while ((current=readdir(dir)) != NULL) {
		if (*pos==*len)
			list = resize(list, len);
		if (current->d_name[0] != '.') {
			list[*pos] = malloc(DIRLEN);
			memcpy(list[*pos],current->d_name, DIRLEN);
			(*pos)++;
		}
	}
	closedir(dir);
	return list;
}

char** 
resize(char** list, int *len) {
	/* Double the size of the list 
		@list = pointer to the list to double
		@len	= current size of the list */
	char** newlist = malloc(2*sizeof(char**)*(*len));
	memcpy(newlist, list, sizeof(char**)*(*len));
	free(list);
	*len *= 2;
	return newlist;
}

