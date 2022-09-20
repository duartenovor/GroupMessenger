#ifndef __UCMD_H__
#define __UCMD_H__

typedef void (*Command_cb)(char *user_argv); // callback to function

typedef struct Command
{
	const char *cmd;
	Command_cb func; // callback to cmd function
} Command_t;

void ucmd_parse(Command_t cmd_list[], const char *delim, const char *str);

#endif // ! __UCMD_H__