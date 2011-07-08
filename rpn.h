/*
 * rpn - Mycroft <mycroft@datasphere.net>
 */

#define VERSION		0.51

#define MAXSIZE		10
#define DEFBASE		10
#define BASECHAR	'#'

#define ERR_DIVBYZERO	"Division by zero."
#define ERR_DOMAIN	"Argument is outside of function domain."
#define ERR_UNKNOWNCMD	"Unknown command."
#define ERR_ARGC	"Too few arguments."

struct metastack {
	struct object *t;
	struct object *b;
	struct metastack *n;
	size_t d;
};

struct object {
	double num;
	struct object *prev, *next;
};

struct command {
	char *name;
	long numargs;
	void (*function)(void);
};

struct macro {
	char *name, *operation;
	struct macro *prev, *next;
};

void addcommand(struct command *c);
struct object *top(void);
char *findmacro(char *);
double popnum(void);
struct object *pop(void);
struct command *findcmd(char *);
void popobj(struct object *), pushnum(double), init_macros(void), error(char *);
unsigned countstack(void);
double peeknthnum(unsigned off);
