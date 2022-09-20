#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ucmd.h"

/********************************************************************
 * 
 * Parser
 * 
 *******************************************************************/

void ucmd_parse(Command_t cmd_list[], const char *delim, const char *str)
{
	if (!str || strlen(str) == 0) 
		return;    	// return 0 for empty commands

	char *s = malloc(strlen(str)+1);    // just in case in is const
	strcpy(s, str);

	int argc = 0;
	char *argv = NULL;
		
	char *arg = strtok(s, delim);
	char *command = malloc(strlen(arg)+1);
	strcpy(command, arg);
	
	arg = strtok(NULL, "\0");
	if(arg)
	{
		argv = malloc(strlen(arg)+1);
		strcpy(argv, arg);
	}
	
	Command_t *c = NULL;
	for (Command_t *p = cmd_list; strcmp(p->cmd, "") /*p->cmd*/; p++) 
		if (strcmp(p->cmd, command) == 0) 
			c = p;
	if (c)
		c->func(argv);
	

	free(command);
	free(argv);
	free(s);
}