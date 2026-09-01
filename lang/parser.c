/*	PARSER 2001 Tsuguo Mogami  */
#include "ciph.h"
#include "y.tab.h"

#include "value.h"
#include "list.h"
#include <string.h>
#include <setjmp.h>

//-----------------------------------------------------
static int numToken=0;		//0 or 1
static int next_token;		//buffer
static obj next_value;

static
int valueToToken(obj vp){
	switch (vp->type) {
	case TOKEN:
		return uint(vp);
	case tSymbol:
		return IDENTIFIER;
	case STRING:
		return STRING;
	case INT:
		return INT_LITERAL;
	case tDouble:
		return DBL_LITERAL;
	}
	assert(0);
	return 0;
}

static
obj getVal(){		//これはreadToken()の後にしか使えないことに注意。
	obj r = next_value;
	r->refcount = 1;
	numToken = 0;
	return r;
}

static
int readToken(){
	if(numToken==0) {
		next_value = yylex();
		next_token=valueToToken(next_value);
		numToken=1;	
	}
	return next_token;
}

static
obj peek(){
	if(numToken==0) {
		next_value = yylex();
		next_token=valueToToken(next_value);
		numToken=1;	
	}
	return next_value;
}

static
int getToken(){
	int r=readToken();
	numToken=0;
	release(next_value);
	return r;
}

static 
int get(int token){
	if(readToken()!=token) return false;
	numToken=0;
	release(next_value);
	return true;
}
static 
int get(char* sym){	// not tested yet
	if(readToken()!=IDENTIFIER) return false;
	if(strcmp(ustr(next_value), sym)==0) {
		release(next_value);
		numToken=0;
		return true;
	}
	return false;
}
//------------------------------------------------------------------
static list paramList();
static list exprSequence();
static obj exprList();

static obj topExp();
static obj lamExp();
static obj ifExp();
static obj andExp();
static obj conditionExp();
static obj dotsExp();
static obj aexp();
static obj mExp();
static obj barExp();	//type: left recursion, bi-list
static obj operatorExp();
static obj powerExp();
static obj dotExp();
static obj term();

static obj statement();
static obj assignExp();

extern char *ptr;	//in tokenizer.c
//static char * start_of_this_line;

#define parse_error(string) \
 (parse_error_func(string,__FILE__, __LINE__))

volatile jmp_buf parse_jmp_buf;

void parse_error_func(char *str, char* file, int line){
	myPrintf("parse error: %s occured in line %d of file %s\n",str, line, file);
	scroll();
//	myPrintf("%s", ptr);
	longjmp(parse_jmp_buf, 1);
}

obj parseString(char** str){
	if(*str==nil) return nil;
	ptr=*str;
	numToken=0;

	obj rr;
	if(setjmp(parse_jmp_buf)==0){	//try
		rr = statement();
	} else {					//catch
		rr = nil;
	}
	*str=ptr;
	return rr;
}	//解釈されずに残ったトークンが捨てられるバグがあるが無害なので放っておく。

obj parse(list line){
	obj stat;
	obj tl = listToCString(line);
	ptr = ustr(tl);
	numToken = 0;
	if(setjmp(parse_jmp_buf)==0){	//try
		stat = statement();
	} else {					//catch
		return nil;
	}
	release(tl);
	if(getToken()!=NUL) myPrintf("unrecognized token '%d' .",next_token);
	return stat;
}

list statementSeq(){
	obj rt;
	list stats=phi();
	while(readToken()){
		while(get(CR)||get(','));
		rt = statement();
		if(rt==nil) break;
		stats=cons(rt, stats);
	}
	return reverse(stats);
}

list statementList(){
	if(!get('{')) parse_error("{ expected");
	list stats=statementSeq();
	if(!get('}')) parse_error("stat_list: } expected");
	return stats;
}

obj statementPseudoList(){
	if(readToken()=='{') {
		return render(tExec, statementList());
	} else  return statement();
}

obj define(){
	obj name,params,exps;
	if(readToken()!=IDENTIFIER) parse_error("identifier required after define");
	name = getVal();
	params = term();
	if(get('=')) {
		exps = topExp();
		if(!exps) parse_error("define: no funcbody");
	} else {
		exps = List2v(statementList());
	}
	return render(tDefine, list3(name, params, exps));
}

obj syntax(){
	obj name,params, exps;
	if(readToken()!=IDENTIFIER) parse_error("identifier required after define");
	name = getVal();
	if(!get('(')) parse_error("( expected");
//	params = term();
	params = topExp();
	if(!get(')')) parse_error(") expected");
	exps = List2v(statementList());
	return render(tSyntaxDef, list3(name, params, exps));
}


obj statement(){
	obj lt, rt;

	if(get("define")) return define();
	if(get("syntax")) return syntax();
	if(get("while")){
		if(!get('(')) parse_error("( expected");
		lt = andExp();
		if(!get(')')) parse_error(") expected");
		rt = statementPseudoList();
		return operate(tWhile, lt, rt);
	} if(get(RETURN)){
		lt = topExp();
//		if(!lt) lt = Null();
		return encap(tReturn, lt);
	} if(get(BREAK)){
		obj rr = alloc();
		rr->type = tBreak;
		return rr;
	}
	return assignExp();
}

obj exprList(){
	if(!get('{')) parse_error("exp list");
	list exprs=exprSequence();
	if(!get('}')) parse_error("exp list");
	return render(LIST, exprs);
}

list paramList(){
	list l=phi();
	if(readToken()!=IDENTIFIER) return l;
	l=cons(getVal(), l);
	while(get(',')){
		if(readToken()!=IDENTIFIER) parse_error("paramlist, identifier expected");
		l = cons(getVal(), l);
	}
	return reverse(l);
}

list exprSequence(){
	list r = phi();	
	obj rt=topExp();
	if(! rt) return r;
	r = cons(rt, r);
	while(get(',')){
		rt=topExp();
//		assert(rt !=nil);
		if(! rt) parse_error("tail comma");
		r= cons(rt, r);
	}
	return reverse(r);
}

static obj parseLR( obj (*LHS)(), int op, obj (*RHS)(),ValueType type){	//left-recursive (((a)b)c)
	obj lt,rt,rr;
	lt=LHS(); 
	if(!get(op)) return lt;
	list r=list1(lt);
	while(1){
		rt=RHS();
		r= cons(rt, r);
		if (!get(op)) break;
	}
	return render(type, reverse(r));
}

obj topExp(){
	return assignExp();
}

obj assignExp(){
	obj lt=lamExp();
	if(!get('=')) return lt;
	obj rt=lamExp();
	if(!rt) parse_error("no rhs.");
	return operate(tAssign, lt, rt);
}

obj lamExp(){	//type non-recursive, bi-list
	obj lt,rt;
	int op = kArrow;

	lt = ifExp();
	if(!get(op)) return lt;
	rt = ifExp();
	return render(tArrow, list2(lt, rt));
}

obj ifExp(){
	if (!get(IF)) return andExp();
	obj thenc,elsec;
	if(!get('(')) parse_error("if:");
	obj cond=andExp();
	if(!get(')')) parse_error("if:");
	thenc= statement();
	get(CR) ;
	if(get(ELSE)){
		elsec = statementPseudoList();
	}else{
		elsec = List2v(nil);
	}
	return render(tIf, list3(cond, thenc, elsec));
}

obj andExp(){
	return parseLR(conditionExp, '&', conditionExp, tAnd);	
}

obj conditionExp(){
	obj lt,rt;
	list rl;
	
	lt=dotsExp();
	int op=readToken();
	if (!(op==EQ || op==NE || op=='>' || op==GE || op=='<' || op==LE)) return lt ;
	rl = list1(lt);
	while(1){
		getToken();	//flush op
		rt=dotsExp();
		rl=cons(rt, rl);
		rl=cons(Int(op), rl);

		op=readToken();
		if (!(op==EQ || op==NE || op=='>' || op==GE || op=='<' || op==LE)) break;
	}
	return render(CONDITION, reverse(rl));
}

obj dotsExp(){
	obj lt = aexp();
	if(get(kDots)) {
		obj rt = aexp();
		if(!rt) parse_error("no rhs.");
		return operate(tOp, Symbol(".."), List2v(list2(lt, rt)));
	} if(get(kDots3)) {
		obj rt = aexp();
		if(!rt) parse_error("no rhs.");
		return operate(tOp, Symbol("..."), List2v(list2(lt, rt)));
	}
	return lt;
}

obj aexp(){
	obj rt;
	int sign=1;
	
	int op=readToken();
	if(get('+'))sign = +1;
	else if(get('-')) sign = -1;
	rt=mExp();
	op=readToken();
	if( op!='+' && op!='-' && sign == 1) return rt;
	list rl=phi();
	while(1){
		if(sign==-1) rt=encap(tMinus, rt);
		rl=cons(rt, rl);
		if(get('+')) sign = +1; 
		else if(get('-')) sign = -1;
		else break;

		rt=mExp();
		if(! rt) parse_error("rhs missing for a [+-].");
	}
	return render(ARITH, reverse(rl));
}

obj mExp(){
	obj lt,rt;
	
	lt=barExp(); 
	while(1){
		int op=readToken();
		if(op!='*' && op!='/') break;
		getToken();
		rt = barExp();
		if(! rt) parse_error("rhs missing for a [*/].");
		lt=render(MULT,list2(lt,rt));
		if(op=='*') lt->type=MULT;
		if(op=='/') lt->type=DIVIDE;
	}
	return lt;
}

obj barExp(){	// actuall not bar but colon
	obj lt,rt;
	
	lt=operatorExp(); 
	return lt;

	if(!get(':')) return lt;
	rt = operatorExp();
	if(! rt) parse_error("rhs missing for a ':'.");
	return operate(tType, lt, rt);
}

obj operatorExp(){	//type: right recursion, bi-list
	obj lt,rt;
	
	lt = powerExp();
	if(! lt) return nil;
	rt = operatorExp();
	if(! rt) return lt;
	return operate(tOp,lt,rt);
}

obj powerExp(){
	obj lt,rt;

	lt = dotExp();
	if(!get('^')) return lt;
	rt = dotExp();
	return render(POW, list2(lt,rt));
}

obj indExp();

obj dotExp(){
	return indExp();
}

obj indExp(){
	obj lt=term(); 
	if(! lt) return nil;
	list l=nil;
	for(;;){
		if(get('[')) {
			l = merge(l, exprSequence());
			if (!get(']')) parse_error("']'expected");
		} else if(get('.')) {
			append(&l, op(Symbol("'"), indExp()));
		} else break;
	}
	if(! l) return lt;
	return operate(tInd, lt, List2v(l));
}

obj term(){
	if(get(LP)){
		list seq=exprSequence();
		if(!get(RP)) parse_error("imbalanced ()");
		if(length(seq)==1) return pop(&seq);
		else return List2v(seq);
	}
	if(get('{')){
		list l = statementSeq();
		if(get('|')){	// list内包
			obj lt =  first(l);
			list rt = statementSeq();
			if(!get('}')) parse_error("term: '}' expected.");
			return op(Symbol("listc"), List2v(cons(lt, rt)));
		}
		if(!get('}')) parse_error("term: '}' expected.");
		return render(LIST, l);
	}
	switch (type(peek())) {
	case tSymbol:
	case INT:
	case tDouble:
	case STRING:
		return getVal();
	}
//	if(readToken()=='@') return getVal();
	if(get('@')) return Token('@');
	return nil;
}


//--- EOF