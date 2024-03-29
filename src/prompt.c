#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

// for windows, mock interpreter prompt
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1)
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0'
	return (cpy)
}

void add_history(char* unused) {}


#else
// for Macs (and linux decomment "history.h")
#include <editline/readline.h>
// #include <editline/history.h>
#endif

/*--------------------------------------------------------*/
// DATA STRUCTURES

// New lval ("lisp value") struct
typedef struct lval
{
	int type;
	long num;
	int err;
} lval;

// create enumeratino of lval types 
// (assigns constant integer values)
// e.g. enum { LVAL_NUM: 0, LVAL_ERR: 1}
enum { LVAL_NUM, LVAL_ERR };
//  errors
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/*--------------------------------------------------------*/
// FUNCITON DECLARATIONS

// main
int main(int argc, char** argv);
// recursive evaluation of input
lval eval(mpc_ast_t* t);
// evaluation of operators
lval eval_op(lval x, char* op, lval y);

// lval num and err type functions
lval lval_num(long x);
lval lval_err(int x);
// lval printing functions
void lval_print(lval v);
void lval_println(lval v);

/*--------------------------------------------------------*/
// MAIN
int main(int argc, char** argv) {

	// create some parsers (mpc lib)
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	// define the language (mpc lib)
	mpca_lang(MPCA_LANG_DEFAULT,
		" \
		number: /-?[0-9]+/ ; \
		operator: '+' | '-' | '*' | '/' ; \
		expr: <number> | '(' <operator> <expr>+ ')' ; \
		lispy: /^/ <operator> <expr>+ /$/ ; \
		",
		Number, Operator, Expr, Lispy);


	puts("Lispy Version 0.0.2");
	puts("Press Ctrl+c to exit\n");

	while (1) {

		char* input = readline("lispy> ");

		add_history(input);

		// attempt to parse user input
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			// on success print AST
			// mpc_ast_print(r.output);

			// on success, evaluate AST
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);


		}
		else {
			// otherwise print error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, Lispy);

}

/*--------------------------------------------------------*/
// LVAL DATA FUNCTIONS (FOR NUM AND ERROR)

// create new number type lval
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

// create new error type lval
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

// print lval based on type
void lval_print(lval v) {
	switch (v.type) {
		// case number
		case LVAL_NUM:
			printf("%li", v.num); 
		break;

		// case error
		case LVAL_ERR:
			// type of error
			if (v.err == LERR_DIV_ZERO) {
				printf("Error: division by zero");
			}
			if (v.err == LERR_BAD_OP) {
				printf("Error: invalid operator");
			}
			if (v.err == LERR_BAD_NUM) {
				printf("Error: invalid number");
			}
		break;
	}
}

void lval_println(lval v) { 
	lval_print(v); 
	putchar('\n'); 
}

/*--------------------------------------------------------*/
// EVALUATION FUNCTIONS (and errors)
lval eval(mpc_ast_t* t) {

	/* If tagged as number return it directly. */
	if (strstr(t->tag, "number")) {
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	/* The operator is always second child. */
	char* op = t->children[1]->contents;

	/* We store the third child in `x` */
	lval x = eval(t->children[2]);

	/* Iterate the remaining children and combining. */
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

lval eval_op(lval x, char* op, lval y) {
	
	// if err, return value
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	// otherwise evaluate
	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) { 
		// if second operand is zero return error
		return y.num == 0 
		? lval_err(LERR_DIV_ZERO) 
		: lval_num(x.num / y.num);
	}
	
	return lval_err(LERR_BAD_OP);
}

















