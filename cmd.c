/*
 * rpn - Mycroft <mycroft@datasphere.net>
 */

/*
 * Things to do:
 *	- Arbitrary-precision math
 *	- Variables
 *	- Programming control statements:
 *	  <expression> if <commands> [else] <commands> end
 *	  begin <expression> while <commands> end
 *	  <lower> <upper> for <variable> <commands> next
 *	  <lower> <upper> for <variable> <commands> <n> step
 *	  <lower> <upper> start <commands> next
 *	  <lower> <upper> start <commands> <n> step
 *	  Some sort of do ... until loop
 *	- Complex numbers
 *	- Different word sizes; better integer/real number support
 *	- Polar/rectangular support
 *	- Radian/degrees mode
 *	- Strings (needed to do macro definition stuff)
 *	- Arrays/vectors/matrices
 *	- xroot, combinatorics, time access
 *	- shell escape
 *	- $RPNINIT, ~/.rpnrc, ~/.rpnstk support
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "rpn.h"

int repeat = 1;
extern int stackmode;
extern int padcount;

static char *thiscmd;
static struct macro *macrohead = NULL;
extern int base, stop;
extern struct metastack *M;

void
error(char *msg)
{
	printf("Error: %s: %s\n", thiscmd, msg);
	stop = 1;
}

/*
 * Built-in commands
 */

static void
cmd_not(void)
{
	top()->num = !top()->num;
}

static void
cmd_ne(void)
{
	double tmpnum = popnum();
	top()->num = top()->num != tmpnum;
}

static void
cmd_mod(void)
{
	if (top()->num == 0)
		error(ERR_DIVBYZERO);
	else {
		double tmpnum = popnum();
		top()->num = fmod(top()->num, tmpnum);
	}
}

static void
cmd_bitand(void)
{
	double tmpnum = popnum();
	top()->num = (unsigned long)top()->num & (unsigned long)tmpnum;
}

static void
cmd_and(void)
{
	double tmpnum = popnum();
	top()->num = top()->num && tmpnum;
}

static void
cmd_mul(void)
{
	double tmpnum = popnum();
	top()->num *= tmpnum;
}

static void
cmd_add(void)
{
	double tmpnum = popnum();
	top()->num += tmpnum;
}

static void
cmd_inc(void)
{
	top()->num++;
}

static void
cmd_sub(void)
{
	double tmpnum = popnum();
	top()->num -= tmpnum;
}

static void
cmd_dec(void)
{
	top()->num--;
}

static void
cmd_div(void)
{
	if (top()->num == 0)
		error(ERR_DIVBYZERO);
	else {
		double tmpnum = popnum();
		top()->num /= tmpnum;
	}
}

static void
cmd_lt(void)
{
	double tmpnum = popnum();
	top()->num = top()->num < tmpnum;
}

static void
cmd_bitshl(void)
{
	double tmpnum = popnum();
	top()->num = (unsigned long)top()->num << (unsigned long)tmpnum;
}

static void
cmd_le(void)
{
	double tmpnum = popnum();
	top()->num = top()->num <= tmpnum;
}

static void
cmd_eq(void)
{
	double tmpnum = popnum();
	top()->num = top()->num == tmpnum;
}

static void
cmd_gt(void)
{
	double tmpnum = popnum();
	top()->num = top()->num > tmpnum;
}

static void
cmd_ge(void)
{
	double tmpnum = popnum();
	top()->num = top()->num >= tmpnum;
}

static void
cmd_bitshr(void)
{
	double tmpnum = popnum();
	top()->num = (unsigned long)top()->num >> (unsigned long)tmpnum;
}

static void
cmd_bitxor(void)
{
	double tmpnum = popnum();
	top()->num = (unsigned long)top()->num ^ (unsigned long)tmpnum;
}

/* Someday, this will be a macro */
static void
cmd_abs(void)
{
	if (top()->num < 0)
		top()->num = -top()->num;
}

static void
cmd_acos(void)
{
	if (top()->num < -1 || top()->num > 1)
		error(ERR_DOMAIN);
	else
		top()->num = acos(top()->num);
}

static void
cmd_asin(void)
{
	if (top()->num < -1 || top()->num > 1)
		error(ERR_DOMAIN);
	else
		top()->num = asin(top()->num);
}

static void
cmd_atan(void)
{
	top()->num = atan(top()->num);
}

static void
cmd_ceil(void)
{
	top()->num = ceil(top()->num);
}

static void
cmd_cos(void)
{
	top()->num = cos(top()->num);
}

static void
cmd_cosh(void)
{
	top()->num = cosh(top()->num);
}

static void
cmd_depth(void)
{
	pushnum(M->d);
}

static void
cmd_ntohs(void) {
	unsigned short s = (unsigned short) popnum();
	pushnum(ntohs(s));
}

static void
cmd_htons(void) {
	unsigned short s = (unsigned short) popnum();
	pushnum(htons(s));
}

static void
cmd_ntohl(void) {
	unsigned s = (unsigned) popnum();
	pushnum(ntohl(s));
}

static void
cmd_htonl(void) {
	unsigned s = (unsigned) popnum();
	pushnum(htonl(s));
}

static void
cmd_stack(void) {
	stackmode = stackmode ? 0 : 1 ;
}

static void
cmd_pad(void) {
	unsigned s = (unsigned) popnum();
	padcount = s;
}

static void
cmd_ipaddr(void) {
	unsigned addr = top()->num;
	pushnum(((u_char *)&addr)[0]);
	pushnum(((u_char *)&addr)[1]);
	pushnum(((u_char *)&addr)[2]);
	pushnum(((u_char *)&addr)[3]);
}

static void
cmd_drop(void)
{
	free(pop());
}

/* Someday, this will be a macro */
static void
cmd_dropn(void)
{
	double tmpnum;
	for (tmpnum = popnum(); tmpnum > 0; tmpnum--)
		cmd_drop();
}

static void
cmd_dup(void)
{
	pushnum(top()->num);
}

/* Someday, this will be a macro */
static void
cmd_dupn(void)
{
	double tmpnum, tmpnum2;
	struct object *obj;

	tmpnum = tmpnum2 = popnum();
	for (obj = top(); tmpnum > 1; obj = obj->next, tmpnum--)
		;
	for (; tmpnum2 > 0; obj = obj->prev, tmpnum2--)
		pushnum(obj->num);
}

static void
cmd_e(void)
{
	pushnum(2.7182818284590452354);
}

static void
cmd_exp(void)
{
	top()->num = exp(top()->num);
}

/* Someday, this will be a macro */
static void
cmd_fact(void)
{
	double tmpnum;
	if (top()->num < 0 || modf(top()->num, &tmpnum) != 0)
		error(ERR_DOMAIN);
	else if (top()->num == 0)
		top()->num = 1;
	else
		for (tmpnum = top()->num, top()->num = 1; tmpnum > 1; tmpnum--)
			top()->num *= tmpnum;
}

static void
cmd_floor(void)
{
	top()->num = floor(top()->num);
}

static void
cmd_fp(void)
{
	double tmpnum;
	top()->num = modf(top()->num, &tmpnum);
}

static void
cmd_getbase(void)
{
	pushnum(base);
}

static void
cmd_ip(void)
{
	modf(top()->num, &top()->num);
}

static void
cmd_ln(void)
{
	if (top()->num < 0)
		error(ERR_DOMAIN);
	else
		top()->num = log(top()->num);
}

static void
cmd_log(void)
{
	if (top()->num < 0)
		error(ERR_DOMAIN);
	else
		top()->num = log10(top()->num);
}

/* Someday, this will be a macro */
static void
cmd_max(void)
{
	popobj(top()->num > top()->next->num ? top()->next : top());
}

/* Someday, this will be a macro */
static void
cmd_min(void)
{
	popobj(top()->num < top()->next->num ? top()->next : top());
}

static void
cmd_pi(void)
{
	pushnum(3.14159265358979323846);
}

static void
cmd_pick_roll(void)
{
	double tmpnum;
	struct object *obj;

	if ((tmpnum = popnum()) == 0)
		return;

	for (obj = top(); tmpnum > 1; obj = obj->next, tmpnum--)
		;
	pushnum(obj->num);

	if (strcmp(thiscmd, "roll") == 0)
		popobj(obj);
}

static void
cmd_pow(void)
{
	double tmpnum;
	if ((top()->next->num == 0 && top()->num <= 0) ||
	    (top()->next->num < 0 && modf(top()->num, &tmpnum) != 0))
		error(ERR_DOMAIN);
	else {
		tmpnum = popnum();
		top()->num = pow(top()->num, tmpnum);
	}
}

static void
cmd_quit(void)
{
	exit(0);
}

static void
cmd_rand(void)
{
	pushnum(rand());
}

static void
cmd_repeat(void)
{
	if (top()->num <= 0)
		error(ERR_DOMAIN);
	else
		repeat = popnum();
}

static void
cmd_rolld(void)
{
	double tmpnum;
	struct object *obj, *obj2;

	if ((tmpnum = popnum()) == 1)
		return;

	for (obj = pop(), obj2 = top(); tmpnum > 2; obj2 = obj2->next, tmpnum--)
		;
	if (obj2 == M->b) {
		M->b = obj;
		obj->next = NULL;
	} else {
		obj2->next->prev = obj;
		obj->next = obj2->next;
	}
	obj2->next = obj;
	obj->prev = obj2;
	M->d++;
}

static void
cmd_setbase(void)
{
	int num = popnum();
	if (num < 2 || num > 36)
		error(ERR_DOMAIN);
	else
		base = num;
}

/* Someday, this will be a macro */
static void
cmd_sign(void)
{
	if (top()->num)
		top()->num = top()->num < 0 ? -1 : 1;
}

static void
cmd_sin(void)
{
	top()->num = sin(top()->num);
}

static void
cmd_sinh(void)
{
	top()->num = sinh(top()->num);
}

static void
cmd_sqrt(void)
{
	if (top()->num < 0)
		error(ERR_DOMAIN);
	else
		top()->num = sqrt(top()->num);
}

static void
cmd_swap(void)
{
	double tmpnum = top()->next->num;
	top()->next->num = top()->num;
	top()->num = tmpnum;
}

static void
cmd_tanh(void)
{
	top()->num = tanh(top()->num);
}

static void
cmd_version(void)
{
	pushnum(VERSION);
}

static void
cmd_bitor(void)
{
	double tmpnum = popnum();
	top()->num = (unsigned long)top()->num | (unsigned long)tmpnum;
}

static void
cmd_or(void)
{
	double tmpnum = popnum();
	top()->num = top()->num || tmpnum;
}

static void
cmd_bitcmpl(void)
{
	top()->num = ~(unsigned long)top()->num;
}

static void
addmacro(char *name, char *operation)
{
	struct macro *macro, *ptrtmp;

	for (ptrtmp = macrohead; ptrtmp != NULL; ptrtmp = ptrtmp->next) {
		if (strcmp(name, ptrtmp->name) == 0) {
			ptrtmp->operation = operation;
			return;
		}
	}

	if ((macro = malloc(sizeof *macro)) == NULL) {
		perror("Error: malloc");
		exit(1);
	}
	macro->name = name;
	macro->operation = operation;

	if (macrohead == NULL) {
		macrohead = macro;
		macrohead->prev = macrohead->next = NULL;
	} else {
		macrohead->prev = macro;
		macro->next = macrohead;
		macrohead = macro;
		macrohead->prev = NULL;
	}
}

char *
findmacro(char *name)
{
	struct macro *macro;

	for (macro = macrohead; macro != NULL; macro = macro->next)
		if (strcmp(macro->name, name) == 0)
			return macro->operation;

	return NULL;
}

void
init_macros(void)
{
	char *env = getenv("HOME");

	addmacro("total", "depth -- repeat +");
	addmacro("tan", "dup sin swap cos /");
	addmacro("sq", "2 pow");
	addmacro("sec", "cos inv");
	addmacro("rot", "3 roll");
	addmacro("rep", "repeat");
	addmacro("over", "2 pick");
	addmacro("oct", "8 setbase");
	addmacro("inv", "1 swap /");
	addmacro("hex", "16 setbase");
	addmacro("exit", "quit");
	addmacro("q", "quit");
	addmacro("dec", "10 setbase");
	addmacro("csc", "sin inv");
	addmacro("cot", "tan inv");
	addmacro("clr", "depth dropn");
	addmacro("chs", "-1 *");
	addmacro("bin", "2 setbase");
	addmacro("aven", "dup -- swap depth rolld repeat + depth roll /");
	addmacro("ave", "depth aven");
	addmacro("alog", "10 swap pow");
	addmacro("?", "help");

	if (env) {
	    char buf[10240];
	    FILE *fp;
	    sprintf(buf, "%s/.rpn_macros", env);

	    fp = fopen(buf, "r");
	    if (fp) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
		    if(buf[0] == '#')
			continue;

		    char *p = buf;
		    while (*p && *p != ' ') p++;
		    if (*p) {
			*p++ = 0;
			addmacro(strdup(buf), strdup(p));
		    }
		}
	    }
	}
}

/*
 * commands[] is sorted for the binary search in findcmd().
 */

static void cmd_help(void);
static struct command _commands[] = {
	{ "!",		1,	cmd_not		},
	{ "!=",		2,	cmd_ne		},
	{ "%",		2,	cmd_mod		},
	{ "&",		2,	cmd_bitand	},
	{ "&&",		2,	cmd_and		},
	{ "*",		2,	cmd_mul		},
	{ "+",		2,	cmd_add		},
	{ "++",		1,	cmd_inc		},
	{ "-",		2,	cmd_sub		},
	{ "--",		1,	cmd_dec		},
	{ "/",		2,	cmd_div		},
	{ "<",		2,	cmd_lt		},
	{ "<<",		2,	cmd_bitshl	},
	{ "<=",		2,	cmd_le		},
	{ "==",		2,	cmd_eq		},
	{ ">",		2,	cmd_gt		},
	{ ">=",		2,	cmd_ge		},
	{ ">>",		2,	cmd_bitshr	},
	{ "^",		2,	cmd_bitxor	},
	{ "abs",	1,	cmd_abs		},
	{ "acos",	1,	cmd_acos	},
	{ "asin",	1,	cmd_asin	},
	{ "atan",	1,	cmd_atan	},
	{ "ceil",	1,	cmd_ceil	},
	{ "cos",	1,	cmd_cos		},
	{ "cosh",	1,	cmd_cosh	},
	{ "depth",	0,	cmd_depth	},
	{ "drop",	1,	cmd_drop	},
	{ "dropn",	-1,	cmd_dropn	},
	{ "dup",	1,	cmd_dup		},
	{ "dupn",	-1,	cmd_dupn	},
	{ "e",		0,	cmd_e		},
	{ "exp",	1,	cmd_exp		},
	{ "fact",	1,	cmd_fact	},
	{ "floor",	1,	cmd_floor	},
	{ "fp",		1,	cmd_fp		},
	{ "getbase",	0,	cmd_getbase	},
	{ "help",	0,	cmd_help	},
	{ "hnl",	1, 	cmd_htonl	},
	{ "hns",	1, 	cmd_htons	},
	{ "ip",		1,	cmd_ip		},
	{ "ipaddr",	1,	cmd_ipaddr	},
	{ "ln",		1,	cmd_ln		},
	{ "log",	1,	cmd_log		},
	{ "max",	2,	cmd_max		},
	{ "min",	2,	cmd_min		},
	{ "nhl",	1, 	cmd_ntohl	},
	{ "nhs",	1, 	cmd_ntohs	},
	{ "pad",	1,	cmd_pad		},
	{ "pi",		0,	cmd_pi		},
	{ "pick",	-1,	cmd_pick_roll	},
	{ "pow",	2,	cmd_pow		},
	{ "quit",	0,	cmd_quit	},
	{ "rand",	0,	cmd_rand	},
	{ "repeat",	1,	cmd_repeat	},
	{ "roll",	-1,	cmd_pick_roll	},
	{ "rolld",	-1,	cmd_rolld	},
	{ "setbase",	1,	cmd_setbase	},
	{ "sign",	1,	cmd_sign	},
	{ "sin",	1,	cmd_sin		},
	{ "sinh",	1,	cmd_sinh	},
	{ "sqrt",	1,	cmd_sqrt	},
	{ "stack",	0,      cmd_stack	},
	{ "swap",	2,	cmd_swap	},
	{ "tanh",	1,	cmd_tanh	},
	{ "version",	0,	cmd_version	},
	{ "|",		2,	cmd_bitor	},
	{ "||",		2,	cmd_or		},
	{ "~",		1,	cmd_bitcmpl	}
};
#define NUMCMDS (sizeof _commands / sizeof *_commands)
static struct command *commands = _commands;
static size_t numcmds = NUMCMDS;
static size_t roomcmds = 0;

static void
cmd_help(void)
{
	int x;
	struct macro *macro;

	puts("\nStandard commands:");
	for (x = 0; x < NUMCMDS; x++) {
		printf("%8s", commands[x].name);
		if (x % 9 == 8)
			putchar('\n');
	}
	if (x % 9)
		puts("\n");
	else
		putchar('\n');

	if (macrohead != NULL) {
		puts("Macros:");
		for (macro = macrohead, x = 0; macro != NULL; macro = macro->next) {
			if(macro->name[0] == '$')
				continue;

			printf("%8s", macro->name);
			if (x++ % 9 == 8)
				putchar('\n');
		}
		if (x % 9)
			puts("\n");
		else
			putchar('\n');
	}
}

static int
cmdcmp(const void *cmd, const void *cmdptr)
{
	return strcmp(((struct command *)cmd)->name, ((struct command *)cmdptr)->name);
}


static void
cmdrefresh(void) {
	qsort(commands, numcmds, sizeof *commands, cmdcmp);
}

void
addcommand(struct command *c) {
	if(!roomcmds) {
		struct command *ca = malloc((numcmds * 2) * sizeof *commands);
		memcpy(ca, commands, numcmds * sizeof *commands);
		if(commands != _commands)
			free(commands);

		commands = ca;
		roomcmds = numcmds;
	}

	commands[numcmds] = *c;
	numcmds++;
	roomcmds--;
	cmdrefresh();
}

struct command *
findcmd(char *cmd)
{
	struct command c;
	c.name = cmd;
	thiscmd = cmd;

	return bsearch(&c, commands, numcmds, sizeof *commands, cmdcmp);
}
