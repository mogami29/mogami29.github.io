/*	TOKENIZER 2001 Tsuguo Mogami  */

#include "ciph.h"
#include "y.tab.h"
#include "value.h"
//#include <math.h>
#include <string.h>
#include <stdio.h>
//------appendable string--------------

#define ALLOCUNIT 16

string nullstr(){
	string st;
	st.s = (char*)malloc(ALLOCUNIT);
	*(st.s) = NUL;
	st.size = 0;
	return st;
}

inline int is2n(int x){
	for(int a=1; a; a<<=1) if(a==x) return true;
	return false;
//	while(x & (x-1)) x = x & (x-1);
//	return x;
}
static
void appendB(string* s, int c){
	char* p = s->s;
	int len = s->size;
	if(len+1 >= ALLOCUNIT && is2n(len+1)){
		p = (char*)malloc((len+1)*2);
		strcpy(p, s->s);	//(dest, source)
		free(s->s);
		s->s = p;
	}
	*(p+len) = c;
	*(p+len+1) = NUL;
	s->size = len+1;
	return;
}

void appendS(string* s, int c){
	if(c & 0xff00) appendB(s, c>>8);
	appendB(s, c);
}

void freestr(string* s){
	free(s->s);
}

bool get_pat(unsigned char**pp, char* s){	//get pattern
	unsigned char* pt = *pp;
	for(;*s; s++,pt++) if(*pt != *s) return false;
	*pp = pt;
	return true;
}
//----- multibyte aware

char* next(char* st){
	if(*st & 0x80) return st+2;
	else return st+1;
}

int readchar(char* st){
	if(*st & 0x80) return *((unsigned short*)st);
	else return *(unsigned char*)st;
}

//--------------------


unsigned char *ptr=nil;	// global

inline int isalpha(char c){
	return ('a'<=c && c<='z')||('A'<=c && c<='Z')||(c<0);
}

static inline int read(){ return *ptr;}		//#define read() (*ptr)

static inline void step(){ ptr++;}

/*static obj numeral0(){
    int c,d,sign,exp=0;
	double f;
	
	c = read();
	step();
	d = c-'0';
	while(c= read(), '0'<=c && c<='9') {
		step();
		d = d*10 + (c -'0');
	}
	if(c !='.' && c!='e') return Int(d);
	if(c=='.'){
		if(*(ptr+1)=='.') return Int(d);
		step();
		f = d;
		while(c = read(),'0'<=c && c<='9') {
			f = f*10 + (c -'0');
			exp--;
			step();
		}
	}else{
		f = d;
	}
	if(c!='e') return Double(f*pow(10,exp));
	step();
	c = read();
	if(c!='+' && c!='-') {ptr--; return Double(f*pow(10,exp));}
	if(c=='+') sign=1; else sign=-1;
	step();
	c = read();
	if(!('0'<=c && c<='9')) {ptr-=2; return Double(f*pow(10,exp));}
	step();
	d = c-'0';
	while(c = read(),'0'<=c && c<='9') {
		step();
		d = d*10 + (c -'0');
	}
	return Double(f*pow(10,sign*d+exp));
}*/

static obj numeral(){
    int c,d;
	unsigned char* be = ptr;

	c = read();
	step();
	d = c-'0';
	while(c= read(), '0'<=c && c<='9') {
		step();
		d = d*10 + (c -'0');
	}
	if(c !='.' && c!='e') return Int(d);
	if(c=='.'){
		if(*(ptr+1)=='.') return Int(d);
		step();
	// koko made ha onaji
		while(c = read(),'0'<=c && c<='9') {
			step();
		}
	}else{}
	if(c!='e') goto conv;
	step();
	c = read();
	if(c!='+' && c!='-') {ptr--; goto conv;}
	step();
	c = read();
	if(!('0'<=c && c<='9')) {ptr-=2; goto conv;}
	step();
	while(c = read(),'0'<=c && c<='9') {
		step();
	}

conv:
	double f;
	c = *ptr;
	*ptr = 0;
	sscanf((char*) be, "%lf", &f);	// strtod(str, NULL) is OK also
	*ptr = c;
	return Double(f);
}


static
bool get(char c){
	if(read()!=c) return false;
	step();
	return true; 
}

inline str_* cre_id(char* s){
	str_* lval = (str_*)alloc();
	ustr(lval) = s;
	lval->type = tSymbol;
	return lval;
}

obj yylex(){
	int c;
	obj lval;
	string s;

	while(c=read(),c==' '||c=='\t') step();
	if(get_pat(&ptr, "->")) return Token(kArrow);
	if(get_pat(&ptr, "--")) {
		while(c=read(),c!=NUL && c!=CR) step();
		return Token(c);
	}
	if(c==NUL) return Token(c);	//行の終わりではnullが返される。
	if('0'<=c && c<='9') return numeral();
	if(isalpha(c)) {		// identifier 
		s=nullstr();
		while(c = read(), isalpha(c) || ('0'<=c && c<='9')) {appendS(&s, c); step();}

		if(strcmp(s.s, "if")	==0) {freestr(&s); return Token(IF)	;}
		if(strcmp(s.s, "else")	==0) {freestr(&s); return Token(ELSE);}
		if(strcmp(s.s, "break")	==0) {freestr(&s); return Token(BREAK);}
		if(strcmp(s.s, "return")==0) {freestr(&s); return Token(RETURN);}

		return cre_id(s.s);
	}
	if(c & 0x80){
		s = nullstr();
		appendS(&s, c); step();
		appendS(&s, read()); step();
		return cre_id(s.s);
	}

	if(get(CR)) return Token(CR);
	if(get('"')){			//文字列リテラル
		s=nullstr();
		while(c= read()) {
			step();
	    	if(c == '"') break;
	    	if(c == '\\') {
				c = read();
				step();
				if(c !='\\') c -= 'a';
	    	}
			appendS(&s, c);
		}
		lval = alloc();
		ustr(lval) = s.s;
		lval->type = STRING;
		return lval;
	}
	if(get_pat(&ptr, "==")) return Token(EQ);
	if(get_pat(&ptr, "!=")) return Token(NE);
	if(get_pat(&ptr, ">=")) return Token(GE);
	if(get_pat(&ptr, "<=")) return Token(LE);
	if(get_pat(&ptr, "...")) return Token(kDots3);
	if(get_pat(&ptr, "..")) return Token(kDots);
	step();
	switch(c){
	case '#':
	case '\'':
		s=nullstr();
		appendS(&s, c);
		return cre_id(s.s);
	}
	return Token(c);
}

/* end */
