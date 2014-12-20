/*
 * rpn - Mycroft <mycroft@datasphere.net>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <assert.h>
#include "rpn.h"

int base = DEFBASE, stop = 0;
struct metastack *M = NULL;
int stackmode = 0;
int padcount = 0;

static void process(char *);

extern int repeat;

struct object *
top(void) {
	return(M->t);
}

static void
push(struct object *obj)
{
	if (M->t == NULL) {
		M->t = M->b = obj;
		M->t->prev = M->t->next = NULL;
	} else {
		M->t->prev = obj;
		obj->next = M->t;
		M->t = obj;
		M->t->prev = NULL;
	}

	M->d++;
}

void
pushnum(double num)
{
	struct object *obj;

	if ((obj = malloc(sizeof *obj)) == NULL) {
		perror("Error: malloc");
		exit(1);
	}
	obj->num = num;
	push(obj);
}

unsigned
countstack(void)
{
	unsigned cnt = 0;
	struct object *o = NULL;

	for(o = top(); o; o = o->next)
		cnt += 1;

	return cnt;
}

struct object *
pop(void)
{
	struct object *obj;

	obj = M->t;
	if ((M->t = M->t->next) != NULL)
		M->t->prev = NULL;
	else
		M->b = NULL;
	M->d--;
	return obj;
}

double
peeknthnum(unsigned off)
{
	struct object *o = top();

	while(off--)
		o = o->next;

	return(o->num);
}

double
popnum(void)
{
	double num;
	struct object *obj;

	obj = pop();
	num = obj->num;
	free(obj);
	return num;
}

void
popobj(struct object *obj)
{
	if (obj == top())
		pop();
	else {
		if (obj == M->b)
			M->b = obj->prev;
		else
			obj->next->prev = obj->prev;
		obj->prev->next = obj->next;

		M->d--;
	}
	free(obj);
}

static void
printnum(unsigned long num, int base, int padto)
{
	static char str[sizeof num * CHAR_BIT], *ptr = str;
	static char nums[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	int padc = 0;

	do
		*ptr++ = nums[num % base];
	while ((num /= base) != 0);

	padc = padto - (ptr - str);
	while(padc > 0)
		padc--, putchar('0');

	while (ptr > str) {
		putchar(*--ptr);
		if(base == 2 && ((unsigned long)ptr % 4) == 0)
			putchar('.');
	}

	putchar(' ');
}

static void
printstk(char *prompt)
{
	struct object *obj;

	for (obj = M->b; obj != NULL; obj = obj->prev) {
		if (base == 10)
			printf("%.12g ", obj->num);
		else
			printnum(obj->num, base, padcount);

		if(stackmode && obj->prev)
			putchar('\n');
	}

	fputs(prompt, stdout);
}

static void
eval(char *cmd)
{
	char *operation;
	static int doingmacro = 0;
	long numargs;
	static char prevcmd[MAXSIZE] = { '\0' };
	struct command *cmdptr;

	if (!doingmacro) {
		if (strcmp(cmd, ".") == 0 && prevcmd[0])
			cmd = prevcmd;
		else {
			strncpy(prevcmd, cmd, MAXSIZE-1);
			cmd[MAXSIZE-1] = 0;
		}
	}

	if ((operation = findmacro(cmd)) != NULL) {
		doingmacro = 1;
		process(operation);
		doingmacro = 0;
	} else if ((cmdptr = findcmd(cmd)) != NULL) {
		if (cmdptr->numargs == -1) {
			if (top() == NULL)
				numargs = 1;
			else if (top()->num < 0)
				numargs = -1;
			else
				numargs = top()->num + 1;
		} else
			numargs = cmdptr->numargs;
		if (numargs == -1 || M->d < numargs)
			error(ERR_ARGC);
		else
			cmdptr->function();
	} else
		error(ERR_UNKNOWNCMD);
}

#define isnum(s) (isdigit(s[0])						\
		  || ((s[0] == '-' || s[0] == '.') && isdigit(s[1]))	\
		  || (s[0] == '-' && s[1] == '.' && isdigit(s[2])))
#define isnotfloat(s) ((s[0] == '0' && s[1] != '.')			\
		       || (s[0] == '-' && s[1] == '0' && s[2] != '.'))

static void
process(char *str)
{
	int x;
	char *suffix, word[100];
        char *tmp, *tmp2;

	while (*str != '\0') {
		while (isspace(*str))
			str++;
		if (*str == '\0')
			break;
		for (x = 0; *str != '\0' && !isspace(*str); x++)
			word[x] = *str++;
		word[x] = '\0';
		if ((suffix = strchr(word, BASECHAR)) != NULL) {
			*suffix++ = '\0';
                        tmp = tmp2 = word;
                        while (*tmp2 != '\0') {
                            if (*tmp2 == ',') tmp2++;
                            else *tmp++ = *tmp2++;
                        }
                        *tmp++ = *tmp2++;
			if (word[0] == '-')
				pushnum(strtol(word, NULL, atoi(suffix)));
			else
				pushnum(strtoul(word, NULL, atoi(suffix)));
		} else if (isnum(word)) {
                        tmp = tmp2 = word;
                        while (*tmp2 != '\0') {
                            if (*tmp2 == ',') tmp2++;
                            else *tmp++ = *tmp2++;
                        }
                        *tmp++ = *tmp2++;
			if (isnotfloat(word)) {
				if (word[0] == '-')
					pushnum(strtol(word, &suffix, 0));
				else
					pushnum(strtoul(word, &suffix, 0));
			} else
				pushnum(strtod(word, &suffix));
			if (*suffix != '\0')
				process(suffix);
		} else {
			for (x = repeat, repeat = 1; x > 0; x--) {
				eval(word);
				if (stop) {
					stop = 0;
					return;
				}
			}
		}
	}
}

int isatty(int);

static void
pushstack(void) {
	struct metastack *m = malloc(sizeof *m);
	struct object *o = top();
	m->n = M;
	M = m;
	if(o)
		pushnum(o->num);
}

static void
freestack(struct metastack *m) {
	struct object *o = NULL;
	for(o = m->b; o; o = o->next)
		free(o);
	free(m);
}

static void
popstack(void) {
	struct object *o = top();
	if(M->n) {
		struct metastack *m = M;
		M = M->n;
		if(o)
			pushnum(o->num);

		freestack(m);
	}
}

static void
init(void) {
	struct command pushs = { "pushs", 0, pushstack },
   	 	       pops = { "pops", 0, popstack };
	addcommand(&pushs);
	addcommand(&pops);
	srand(time(NULL));
	init_macros();
	M = malloc(sizeof(*M));
	M->t = NULL;
	M->b = NULL;
	M->d = 0;
	M->n = NULL;
}

int
main(int argc, char *argv[])
{
	init();

	if (argc > 1) {
		int x;
		for (x = 1; x < argc; x++)
			process(argv[x]);
		printstk("\n");
	} else {
		int interactive = isatty(0);
		char buf[1000];
		if (interactive)
			printstk("> ");
		while (fgets(buf, sizeof buf, stdin) != NULL) {
			process(buf);
			if (interactive)
				printstk("> ");
		}
		if (!interactive)
			printstk("\n");
	}

	return 0;
}
