/* part of cipher language, by Tsuguo Mogami  */
#ifndef PRIVATE_CIPH_H_INCLUDED
#define PRIVATE_CIPH_H_INCLUDED
#endif /* PRIVATE_CIPH_H_INCLUDED */

#define smaller(a, b) ((a) < (b) ? (a) : (b))
#define larger(a, b) ((a) > (b) ? (a) : (b))

//typedef struct value*	obj;
typedef class value*	obj;
//typedef const struct value* ref;
typedef const class value* ref;
typedef struct value*	rel;

/*typedef struct node* list;
/*/template <class T> class node;
typedef node<obj>* list;/**/

typedef struct Interpreter_* Interpreter;
Interpreter create_interpreter(void);
void interpret(Interpreter interpreter, list line);
void dispose_interpreter(Interpreter interpreter);

#ifdef GLOBAL_VARIABLE_DEFINE
#define GLOBAL
#else
#define GLOBAL extern
#endif

void	scroll();			// in app.c
void 	assert_func(char* file, int line);
void 	error_func(char *str, char* file, int line);
void 	exit2shell();
void 	myPrintf(char *fmt,...);// in appSpeci.c
obj 	editline(obj );		// in appSpeci.c
obj 	edit(obj);			// in appSpeci.c
obj 	parse(list line);			//in parser.c
obj 	parseString(char** string);	//in parser.c

#define error(string) 		(error_func(string,__FILE__, __LINE__))
#define assert(condition)	((condition) ? (void)(0) : assert_func(__FILE__, __LINE__))
#define nil	NULL

#ifdef PtoCstr	// a little hack to find CodeWarrior
	typedef int bool;
#endif

typedef struct {
	int size;
	char* s;
} string;

obj	yylex();			// in tokenizer.c
string nullstr();			// in tokenizer.c
void appendS(string* s, int c);// in tokenizer.c
void freestr(string* s);	// in tokenizer.c
char*next(char* st);		// in tokenizer.c
int readchar(char* st);		// in tokenizer.c
int get_pat(unsigned char**pp, char* s);	//get pattern in tokenizer.c

// in value.c
obj retain(obj);	
void release(obj);

/* class obj {
	value * a;
public:
	inline value* operator->(){return a;}; 
	inline value& operator*(){return *a;}
	inline obj(){};
	inline obj(void* p):	a((value*)p){};
	inline obj(int p):		a((value*)p){} //nilからのキャスト
	inline obj(value*p):	a(p){}
	inline operator int()	{return (int)a;}
//	inline operator void*()	{return (void*)a;}
};*/
