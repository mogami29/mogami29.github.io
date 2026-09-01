/*	OBJECT LIB 
	Copyright(C) 2002- Tsuguo Mogami  (mogami@yugiri.bnf.brain.riken.go.jp)*/
#include "ciph.h"
#include <string.h>
#include <math.h>
#include "value.h"
#include "list.h"
#include "vector.h"
#include "app.h"
#include "eval.h"

#define dVal	3	// mask
#define idInt	1
#define idChar	3
 //-----------------------------------------------------------------
#define MINALLOC sizeof(dbl_)

obj pool = nil;

void fill_pool(){
	char* tt = (char*)malloc(MINALLOC*1000);
	if(!tt) {
		DrawString("\pmemory shortage");
		exit2shell();
	}
	for(int i=0; i<1000; i++) {
		obj p = (obj)(tt + i*MINALLOC);
		p->refcount = 1;
		cdr(p) = pool;
		pool = p;
	}
}

obj alloc(){
	if(!pool) fill_pool();
	obj r = pool;
	pool = cdr(pool);
//	r->refcount = 1;
//	assert(r->refcount==1);
	return r;
}

void* value::operator new(size_t size){
	assert(size == MINALLOC);
	return alloc();
}

void value::operator delete(void* v, size_t size){
	assert(size == MINALLOC);
	cdr((obj)v) = pool;
	pool = (obj)v;
}

void* my_malloc(size_t n){
	if(n > MINALLOC) {
		void* tt = malloc(n);
		if(tt) return tt;
		DrawString("\pmemory shortage");
		exit2shell();
	}
	if(!pool) fill_pool();
	obj r = pool;
	pool = cdr(pool);
	return (void*) r;
}

void my_free(void* p, size_t n){
	if(n <= MINALLOC) {
//		((obj)p)->refcount = 1;
		cdr((obj)p) = pool;
		pool = (obj)p;
		return;
	}
	free(p);
}

double v2Double(obj v){		//cast to double
	switch(type(v)){
	case tDouble:return udbl(v);
	case INT:	 return uint(v);	
	}
	error("cast to double not defined.");
	return 0;
}
obj dblArray(int n){
	obj v = alloc();
	v->type = tDblArray;
	udar(v).size = n;
	udar(v).v = (double *)my_malloc(sizeof(double)*n);
	return v;
}
/*dblarr::dblarr(int n) {
	type = tDblArray; 
	dbl_arr.size = n;
	dbl_arr.v = (double *)my_malloc(sizeof(double)*n);
}*/
obj intArray(int n){
	obj v = alloc();
	v->type = tIntArr;
	uiar(v).size = n;
	uiar(v).v = (int *)my_malloc(sizeof(int)*n);
	return v;
}
obj vector(int n){
	obj v = alloc();
	v->type = tLAVec;
	udar(v).size = n;
	udar(v).v = (double *)my_malloc(sizeof(double)*n);
	return v;
}
obj Null(){
	obj v = alloc();
	v->type = tNull;
	return v;
}
obj Assoc(){
	list_* v = (list_*)alloc();
	v->type = tAssoc;
	ul(v) = nil;
	return (obj)v;
}
obj Token(int i){
	obj v = alloc();
	v->type=TOKEN;
	uint(v) = i;
	return v;
}
obj Int(int i){
	obj v = alloc();
	v->type = INT;
	uint(v) = i;
	return v;
}
static dbl_ Double2Value(double d){
	dbl_ v;
	v.type = tDouble;
	v.dblv = d;
	v.refcount = 1;
	return v;
}
obj Double(double d){
	obj v = alloc();
	v->type = tDouble;
	udbl(v) = d;
	return v;
}
inline void* m_malloc(size_t n){
	return malloc(n);
}
inline void m_free(void*p, size_t n){
	free(p);
}
char* copyString(const char* str){
	char* ns = (char*)m_malloc(strlen(str)+1);
	strcpy(ns, str);
	return ns;
}
str_* val(char* str){
	str_* v = (str_*) alloc();
	v->type = STRING;
	v->string = str;
	return v;
}
obj aString(int n){
	return val((char *)m_malloc(n));
}
obj cString(const char* st, const char* en){
	char *p, *ns=(char*) m_malloc(en-st+1);
	for(p=ns; st<en; p++,st++) *p=*st;
	*p=NULL;

	return val(ns);
}
obj Symbol(const char* s){
	str_* v = (str_*)alloc();
	v->type = tSymbol;
	v->string = copyString(s);
	return v;
}
arr* aArray(int n){
	arr* r = (arr*) alloc();
	r->type = tArray;
	r->u.array.size = n;
	r->u.array.v = (obj*)my_malloc(n*sizeof(obj));
	return r;
}
arr* cArray(obj v[], int n){
	arr* r = (arr*) alloc();
	r->type = tArray;
	r->u.array.size = n;
	r->u.array.v = v;
	return r;
}
list_* render(ValueType type, list l){
	list_* r = (list_*)alloc();
	r->type = type;
	ul(r) = l;
	return r;
}
obj encap(ValueType t, obj v){
	obj rr = alloc();
	rr->type = t;
	uref(rr) = v;
	return rr;
}
list_* List2v(list l){
	return render(LIST, l);
}
bool isToken(obj v, int tk){
	if(type(v) !=TOKEN) return false;
	if(uint(v)!= tk) return false;
	return true;
}

inline void free_v(obj v){
	switch(type(v)) {
	case INT:
	case tDouble:
	case tChar:
	case tNull:
	case TOKEN:
	case tInternalFn:
	case tSpecial:
	case tDel:
		break;
	case tArray:
	case IMAGE:
	case tCImg:
		for(int i=0; i < uar(v).size; i++) release(uar(v).v[i]);
		my_free(uar(v).v, sizeof(obj)*uar(v).size);
		break;
	case tIntArr:
		my_free(uiar(v).v, sizeof(int)*uiar(v).size);
		break;
	case tLAVec:
	case tDblArray:
		my_free(udar(v).v, sizeof(double)*udar(v).size);
		break;
	case tDblAr2:
		my_free(uda2(v).v, sizeof(double)*uda2(v).size1*uda2(v).size2);
		break;
	case LIST:
	case POW:
	case MULT:
	case DIVIDE:
	case ARITH:	
	case CONDITION:
	case tDefine:
	case tIf:
	case tAssoc:
	case tExec:
	case tIns:
	case tMove:
	case tSyntaxLam:
	case tClosure:
	case tCurry:
	case tArrow:
	case tAnd:
	case FRACTION:
	case SubScript:
	case SuperScript:
	case tShow:
	case tHide:
		release(ul(v));
		break;
	case tAssign:
	case tInd:
	case tWhile:
	case tScope:
	case tOp:
		release(car(v));
		release(cdr(v));
		break;
	case tReturn:
	case tSigRet:
	case tMinus:
	case tRef:
		release(uref(v));
		break;
	case STRING:
	case tSymbol:
		free(ustr(v));
		break;
	case tSpVec:
		delete uspv(v);
		break;
	case tHash:
		delete (hash*)uhash(v);
		break;
	default:
		if(v->type>tLast){ //user defined types
			release(uref(v));
			return;
		}
		print(v);
		assert(0);
	}
}

void release(obj v){
	if(! v) return;
	if(v->refcount -1) {v->refcount--; return;}
//	if(v->refcount -1) {v->refcount = v->refcount -1; return;}
	free_v(v);
	cdr(v) = pool;
	pool = v;
}/**/

obj retain(obj v){
	if(v==nil) return v;
//	if((int)v & dVal) return v;
	(v->refcount)++;
	return v;
}

int vrInt(obj v){	// value-release to Int
	assert(type(v)==INT);
	int i = uint(v);
	release(v);
	return i;
}

DblArr2 copyM(DblArr2 m){
	double* v = (double*)malloc(sizeof(double)*m.size2*m.size1);
	for(int i=0; i<m.size2*m.size1; i++){
		v[i] = m.v[i];
	}
	m.v=v;
	return m;
}

DblArray copyV(DblArray v1){
	double* v = (double*)malloc(sizeof(double)*v1.size);
	for(int i=0; i<v1.size; i++){
		v[i] = v1.v[i];	
	}
	v1.v=v;
	return v1;
}
obj copy(obj v){		//surface copy
	if(v==nil) assert(0);
	obj r = alloc();
	*r = *v;
	r->refcount = 1;
	switch(type(v)){
	case INT:
	case tDouble:
	case TOKEN:
	case tNull:
		return r;
	case tLAVec:
	case tDblArray:
		udar(r) = copyV(udar(v));
		return r;
	case tDblAr2:
		uda2(r) = copyM(uda2(v));
		return r;
	case tArray:
		uar(r).v = (obj*)my_malloc(uar(r).size*sizeof(obj));
		for(int i=0; i < uar(r).size; i++) uar(r).v[i] = retain(uar(v).v[i]);
		return r;
	case tIntArr:
		uiar(r).v = (int*)my_malloc(uiar(r).size*sizeof(int));
		for(int i=0; i < uiar(r).size; i++) uiar(r).v[i] = uiar(v).v[i];
		return r;
	case LIST:
	case POW:		//list-2
	case MULT:		//list
	case DIVIDE:		//list
	case ARITH:		//list
	case CONDITION:	//list
	case tDefine:	//list-3
	case tIf:		//list-3
	case tAssoc:
		ul(r) = copy(ul(v));
		return r;
	case tAssign:
	case tInd:
	case tWhile:
	case tOp:
		car(r) = retain(car(v));
		cdr(r) = retain(cdr(v));
		return r;
	case tReturn:
	case tMinus:
		uref(r) = copy(uref(v));
		return r;
	case STRING:
		return String2v(ustr(v));
	case tSymbol:
		r = String2v(ustr(v));
		r->type = tSymbol;
		return r;
	}
	print(v);
	assert(0);
	return nil;
}

void print_vector(DblArray v){
	double *p = v.v;
	for(int i=0; i<v.size; i++){
		myPrintf("%g ",*p++);
	}
}

void print_intarr(IntArr v){
	int *p = v.v;
	for(int i=0; i<v.size; i++){
		myPrintf("%d ",*p++);
	}
}

void print_array(Array v){
	for(int i=0; i<v.size;){
		print(v.v[i]);
		myPrintf(" ");
		i++;
	}
}

void print_matrix(DblArr2 m){
	double *p=m.v;
	myPrintf("%d,%d",m.size1,m.size2);
	scroll();
	for(int i=0; i < m.size1; i++){
		for(int j=0; j<m.size2; j++){
			myPrintf("%f ",*p++);
		}
//		scroll();
	}
}

void print_image(obj v){
	Point pt;
	GetPen(&pt);

	int baseLine=pt.v;
	RGBColor color;
	if(type(v)==IMAGE){
		for(int i=0; i< uar(v).size; i++){
			obj row = uar(v).v[i];
			assert(type(row)==tDblArray);
			for(int j=0; j < udar(row).size; j++){
				color.red=color.green=color.blue=0xffff*(larger(0,smaller(1,udar(row).v[j])));
				SetCPixel(LEFTMARGIN+j, baseLine-i, &color);
			}
		}
	} else if(type(v)==tCImg){
		for(int i=0; i< uar(uar(v).v[0]).size; i++){
			assert(uar(v).v[0]->type==tArray);
			obj rowr = uar(uar(v).v[0]).v[i];
			obj rowg = uar(uar(v).v[1]).v[i];
			obj rowb = uar(uar(v).v[2]).v[i];
			assert(type(rowr)==tDblArray);
			for(int j=0; j< udar(rowr).size; j++){
				color.red	=0xffff*(udar(rowr).v[j]);
				color.green	=0xffff*(udar(rowg).v[j]);
				color.blue	=0xffff*(udar(rowb).v[j]);
				SetCPixel(LEFTMARGIN+j, baseLine-i, &color);
			}
		}
	} else assert(0);
}

void print_seq(list l, char* delimeter){
	if(! l) return;
	for(; ; ){ 
		print(first(l));
		l=rest(l);
		if(! l) break;
		myPrintf(delimeter);
	}
}

void print_list(list list){
	myPrintf("{");
	print_seq(list,",");
	myPrintf("}");
}

void print(obj v){
/*	Point pt;
	GetPen(&pt);
	if(pt.h>LEFTMARGIN+colWidth) return;
*/	if(v==nil){
		myPrintf("<nil>");
		return;
	}
	if((int)v & dVal==idInt){
		myPrintf("%d", (int)v>>2);
		return;
	}
	if((int)v & dVal==idChar){
		myPrintf("%c", (int)v>>2);
		return;
	}
	switch(type(v)){
	case tNull:		myPrintf("<void>");		return;
	case INT:		myPrintf("%d", uint(v));	return;
	case tDouble:	myPrintf("%.5g", udbl(v));	return;
	case tChar:
		char buf[4];
		*(int*)buf = 0;
		if(uint(v) > 256) *(unsigned short*)buf = uint(v);
			else *buf = uint(v);
		myPrintf("%s", buf);		return;
	case tInternalFn:
		myPrintf("<internalFn>");	return;
	case tSpecial:
		myPrintf("<specialOp>");	return;
	case tLAVec:
		myPrintf("Vector ");
		print_vector(udar(v));
		return;
	case tDblArray:
		myPrintf("dbl_arr{");
		print_vector(udar(v));
		myPrintf("}");
		return;
	case tIntArr:
		myPrintf("int_arr{");
		print_intarr(uiar(v));
		myPrintf("}");
		return;
	case tArray:
		myPrintf("array{");
		print_array(uar(v));
		myPrintf("}");
		return;
	case tDblAr2:
		myPrintf("Matrix");
		print_matrix(uda2(v));
		return;
	case tSpVec:
		uspv(v)->print();
		return;
	case tHash:
		uhash(v)->print();
		return;
	case IMAGE:
	case tCImg:
		myPrintf("Image");
		if(type(v)==IMAGE){
			scrollBy(uar(v).size);
		} else if(type(v)==tCImg){
			scrollBy(uar(uar(v).v[0]).size);
		}
		addObjToText(retain(v));
		print_image(v);
		return;
	case STRING:
		if(strlen(ustr(v))<255) myPrintf("string %s",ustr(v));
			else myPrintf("string <too long to print>");
		return;
	case tSymbol:
		if(strlen(ustr(v))<255) myPrintf("%s",ustr(v));
			else myPrintf("sym<too long to print>");
		return;
	case tInd:		myPrintf("ind ");		goto prop;
	case tWhile:	myPrintf("while ");	goto prop;
	case tOp:
//		myPrintf("(");
prop:		print(car(v));
		myPrintf(" ");
		print(cdr(v));
//		myPrintf(")");
		return;
	case tMinus:
		myPrintf("(-");
		print(uref(v));
		myPrintf(")");
		return;
	case tCurry:
		print(em0(v));
		myPrintf(">>>");
		print(em1(v));
		return;
	case tClosure:
		print(em0(v));
		myPrintf(">>");
		print(em1(v));
		return;
	case tArrow:
		print(em0(v));
		myPrintf("->");
		print(em1(v));
		return;
	case MULT:		//list
//		myPrintf("(");
		print_seq(ul(v), "*");
//		myPrintf(")");
		return;
	case ARITH:		print_seq(ul(v), "+");return;
	case tAssign:
		print(car(v));
		myPrintf("=");
		print(cdr(v));
		return;
	case LIST:						break;
	case SubScript:	myPrintf("sub");		break;	//list-1
	case POW:		myPrintf("pow");	break;
	case DIVIDE:		myPrintf("div");		break;	//list
	case CONDITION:	myPrintf("cond");	break;	//list
	case tAssoc:	myPrintf("assoc");	break;	//list-2
	case tDefine:	myPrintf("define");	break;	//list-3
	case tIf:		myPrintf("if");		break;	//list-3
	case tSyntaxLam:	myPrintf("syntax");	break;	//list-1
	case tReturn:	myPrintf("return");	return;	//ref
	case tBreak:	myPrintf("break");	return;
	case tRef:		myPrintf("ref");		print((uref(v)));return;
	case tCont:		myPrintf("continuation");return;
	default:
		if(v->type>tLast){
			myPrintf("<%d>",v->type);
			print(uref(v));
			return;
		}
		myPrintf("<%d unknown>",v->type);
		return;
	}
	print_list(ul(v));
}


//------------------------------実演算系----------------------------


void set(DblArray* a, int i, double d){
	a->v[i]=d;
}
inline DblArr2 addMM(DblArr2 *m1, DblArr2 *m2, int plusminus){
	DblArr2 m;
	if(m1->size2 != m2->size2  || m1->size1 != m2->size1) error("unequal number matrix:add");		
	m.size1 = m1->size1;
	m.size2 = m1->size2;
	m.v = (double*)malloc(sizeof(double)*m.size2*m.size1);
	for(int i=0; i < m.size2*m.size1; i++){
		m.v[i] = m1->v[i] + plusminus*m2->v[i];
	}
	return m;
}
inline DblArray addVV(DblArray v1, DblArray v2, int plusminus){
	DblArray rv;	
	if(v1.size != v2.size) error ("add: unequal number of element in vec");	
	rv.size = v1.size;
	double* v = (double*)malloc(sizeof(double)*rv.size);
	for(int i=0; i < v1.size; i++){
		v[i] = v1.v[i] + plusminus*v2.v[i];
	}
	rv.v = v;	
	return rv;
}
inline DblArray addDA(double d1, DblArray v2, int plusminus){
	DblArray rv;
	rv.size = v2.size;
	double* v = (double*)malloc(sizeof(double)*rv.size);
	for(int i=0; i < v2.size; i++){
		v[i] = d1 + plusminus*v2.v[i];
	}
	rv.v = v;
	return rv;
}
static DblArray multDV(double d1, DblArray v2){
	DblArray rv;
	rv.size = v2.size;
	double *v = (double*)malloc(sizeof(double)*rv.size);														
	for(int i=0; i < v2.size; i++){
		v[i] = d1 * v2.v[i];
	}
	rv.v = v;
	return rv;
}
static DblArr2 multDM(double d, DblArr2 m){
	double *v = (double*)malloc(sizeof(double)*m.size1*m.size2);
	for(int i=0; i < m.size1*m.size2; i++){
		v[i] = d * m.v[i];
	}
	m.v=v;
	return m;
}
static DblArray multAA(DblArray *v1, DblArray *v2){
	DblArray rv;
	double *v = (double*)malloc(sizeof(double)*v1->size);												
	for(int i=0; i < v1->size; i++){
		v[i] = v1->v[i] * v2->v[i];
	}
	rv.size=v1->size;
	rv.v=v;
	return rv;
}

//--------------------------------------------------


obj and1(obj lt, obj rt){
	obj rr;
	if(lt->type==INT && rt->type==INT){
		rr  = Int(uint(lt) & uint(rt));
	} else if(isCon(type(lt)) && isCon(type(rt))) {
		rr = applyCC(and1, lt,rt);
	} else error("and: non Boolean.");
	release(lt);
	release(rt);
	return rr;
}

obj uMinus0(obj v){	//non-releasing
	obj rr=alloc();
	rr->type = v->type;
	switch(type(v)){
	case tDouble:
		udbl(rr) = -udbl(v);	break;
	case INT:
		uint(rr) = - uint(v);	break;
	case LIST:
		ul(rr) = map(uMinus0, ul(v));break;
	case tLAVec:
	case tDblArray:
		udar(rr) = multDV(-1, udar(v));	break;
	case tSpVec:
		uspv(rr) = mul(-1, uspv(v));break;
	case tArray:
		rr->type=tNull;
		release(rr);
		rr = map_obj(uMinus0, v);				break;
	case tDblAr2:
		uda2(rr) = multDM(-1, uda2(v));	break;
	default:
		print(v);
		error("uMinus: not defined to that type");
	}
	return rr;
}

obj uMinus(obj v){	//releasing
	obj rr=uMinus0(v);
	release(v);
	return rr;
}

obj add(obj lt, obj rt){
	ValueType lty=type(lt), rty=type(rt);
//	if(lty== rty && lty==INT) return new_int(uint(lt) + uint(rt));
	if(lty== rty && lty==INT) return Int(uint(lt) + uint(rt));
	if(lty== rty && lty==tDouble) return Double(udbl(lt) + udbl(rt));
	if(lty==INT && rty==tDouble) return Double(uint(lt) + udbl(rt));
	if(lty==tDouble && rty==INT) return Double(udbl(lt) + uint(rt));
	// vector
	obj rr = alloc();
	if(lty == rty && lty==tDblArray){
		rr->type = tDblArray;
		udar(rr) = addVV(udar(lt), udar(rt), +1);
		return rr;
	}
	// linear algebra
	if(lty==rty && lty==tLAVec){
		assert(0);
		udar(rr) = addVV(udar(lt), udar(rt), +1);
		rr->type = tLAVec;
		return rr;
	} else if(lty==rty && lty==tSpVec){
		uspv(rr) = add(uspv(lt), uspv(rt));
		rr->type = tSpVec;
		return rr;
	} else if(lty==rty &&rty==tDblAr2){
		uda2(rr) = addMM(&uda2(lt),&uda2(rt), +1);
		rr->type = tDblAr2;
		return rr;
	} else if(lty==tDouble && rty==tDblArray){
		rr->type = tDblArray;
		udar(rr) = addDA(udbl(lt), udar(rt),+1);
		return rr;
	} else if(lt->type==tDblArray && rt->type==tDouble){
		rr->type = tDblArray;
		udar(rr) = addDA(udbl(rt), udar(lt),1);
		return rr;
	}
	rr->type = tNull;
	release(rr);
	return nil;
}

obj mult(obj lt, obj rt){
	ValueType lty=type(lt), rty=type(rt);
	if(lty==INT && rty==INT) return Int(uint(lt) * uint(rt));
	if(lty==INT && rty==tDouble) return Double(uint(lt) * udbl(rt));
	if(lty==tDouble && rty==INT) return Double(udbl(lt) * uint(rt));
	if(lty==tDouble && rty==tDouble) return Double(udbl(lt) * udbl(rt));
	// vector
	obj rr = alloc();
	if(lty==tDblArray && rty==tDblArray){
		udar(rr) = multAA(&udar(lt), &udar(rt));
		rr->type = tDblArray;
		return rr;
	} if(lty==tDouble && rty==tDblAr2){
		rr->type = tDblAr2;
		uda2(rr) = multDM(udbl(lt), uda2(rt));
		return rr;
	} else if(lty==tDouble && rty==tLAVec){
		udar(rr) = multDV(udbl(lt), udar(rt));
		rr->type = tLAVec;
		return rr;
	} else if(lty==tDouble && rty==tSpVec){
		uspv(rr) = mul(udbl(lt), uspv(rt));
		rr->type = tSpVec;
		return rr;
	} else if(lty==rty && lty==tSpVec){
		uspv(rr) = mul(uspv(lt), uspv(rt));
		rr->type = tSpVec;
		return rr;
/*	}else if(lty==tDblAr2 && rty==tLAVec){
		rr->u.dbl_arr = multMV(uda2(lt), rt->u.dbl_arr);
		rr->type = tLAVec;
		return rr;		*/
/*	}else if(lty==tDblAr2 && rty==tDblAr2){
		uda2(rr)=multMM(uda2(lt), uda2(rt));
		rr->type = tDblAr2;
		return rr;		*/
	// half external and arguables
	} else if(lty==tDouble && rty==tDblArray){
		udar(rr) = multDV(udbl(lt), udar(rt));
		rr->type = tDblArray;
		return rr;
	} else if(lty==tDouble && rty==LIST){
		ul(rr) = applyVL(lt, ul(rt), mult);
		rr->type = LIST;
		return rr;
	}
	rr->type = tNull;
	release(rr);
//	if(rty==tDouble) return mult(rt, lt);
	return nil;
}

obj divide(obj lt, obj rt){
	if(type(lt)==tDouble && type(rt)==tDouble) return Double(udbl(lt) / udbl(rt));
	if(type(lt)==INT && type(rt)==tDouble) return Double(uint(lt) / udbl(rt));
	if(type(lt)==tDouble && type(rt)==INT) return Double(udbl(lt) / uint(rt));
	if(type(lt)==INT && type(rt)==INT) return Double((double)uint(lt) / (double)uint(rt));

	if(type(rt)==tDouble) return mult(lt, (obj)&Double2Value(1/udbl(rt)));
	if(type(rt)==INT) return mult(lt, (obj)&Double2Value(1.0/uint(rt)));
	return nil;
}

obj power(obj lt, obj rt) {
	if(type(lt)==tDouble && type(rt)==tDouble) return Double(pow(udbl(lt), udbl(rt)));
	if(type(lt)==INT && type(rt)==INT) 	return Int(pow(uint(lt), uint(rt)));
	if(type(lt)==tDouble && type(rt)==INT) return Double(pow(udbl(lt), uint(rt)));
	if(type(lt)==INT && type(rt)==tDouble) return Double(pow(uint(lt), udbl(rt)));
	return nil;
}

int equal(L l1, L l2){
a1:	if(l1==l2) return true;		//nilのときも一致
	if(! l1 || ! l2) return false;
	if(!equal(first(l1), first(l2))) return false;
	l1 = rest(l1);
	l2 = rest(l2);
	goto a1;
}

int equal(obj v1, obj v2){
	if(v1==v2) return true;
	if(type(v1) != type(v2)) return false;
	switch(type(v1)){
	case INT:
	case TOKEN:
	case tChar:
		return uint(v1) == uint(v2);
	case tSymbol:
	case STRING:
		return !strcmp(ustr(v1), ustr(v2));
	case LIST:
		return equal(ul(v1), ul(v2));
	case tAssign:
	case tInd:
	case tWhile:
	case tOp:
		return equal(ult(v1), ult(v2)) && equal(urt(v1), urt(v2));
	}
	assert(0);
	return false;
}

obj ccgt(obj lt, obj rt){
	ValueType lty=lt->type, rty=rt->type;
	if(lty==INT && rty==INT) return Int(uint(lt) > uint(rt));
	if(lty==INT && rty==tDouble) return Int(uint(lt) > udbl(rt));
	if(lty==tDouble && rty==INT) return Int(udbl(lt) > uint(rt));
	if(lty==tDouble && rty==tDouble) return Int(udbl(lt) > udbl(rt));
	return nil;
}
obj cclt(obj lt, obj rt){
	ValueType lty=lt->type, rty=rt->type;
	if(lty==INT && rty==INT) return Int(uint(lt) < uint(rt));
	if(lty==INT && rty==tDouble) return Int(uint(lt) < udbl(rt));
	if(lty==tDouble && rty==INT) return Int(udbl(lt) < uint(rt));
	if(lty==tDouble && rty==tDouble) return Int(udbl(lt) < udbl(rt));
	return nil;
}
obj ccge(obj lt, obj rt){
	ValueType lty=lt->type, rty=rt->type;
	if(lty==INT && rty==INT) return Int(uint(lt) >= uint(rt));
	if(lty==INT && rty==tDouble) return Int(uint(lt) >= udbl(rt));
	if(lty==tDouble && rty==INT) return Int(udbl(lt) >= uint(rt));
	if(lty==tDouble && rty==tDouble) return Int(udbl(lt) >= udbl(rt));
	return nil;
}
obj ccle(obj lt, obj rt){
	ValueType lty=lt->type, rty=rt->type;
	if(lty==INT && rty==INT) return Int(uint(lt) <= uint(rt));
	if(lty==INT && rty==tDouble) return Int(uint(lt) <= udbl(rt));
	if(lty==tDouble && rty==INT) return Int(udbl(lt) <= uint(rt));
	if(lty==tDouble && rty==tDouble) return Int(udbl(lt) <= udbl(rt));
	return nil;
}

