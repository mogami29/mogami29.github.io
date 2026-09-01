/*	EVAL, 2002 Tsuguo Mogami  */
#include "ciph.h"
#include "value.h"
#include "list.h"
#include "lib.h"
#include "eval.h"
#include "vector.h"
#include "y.tab.h"
#include <string.h>

static obj env = nil;

static obj  subs(obj v, obj * vars);
static obj enclose(obj v);

int any(int (*func)(obj), list l){
	for(; l; l=rest(l)){
		if(func(first(l))) return true;
	}
	return false;
}

obj pop(obj* v){
	obj lt = retain(ult(*v));
	obj rt = retain(urt(*v));
	release(*v);
	*v = rt;
	return lt;
}


//-----------------------------------

struct Interpreter_ {
	obj	gl_vars;
	obj	types;
};

static Interpreter curr_interp;

Interpreter create_interpreter(void){
//	myPrintf("%d %d %d", sizeof(value0), sizeof(value), sizeof(node));
	Interpreter in = new Interpreter_();
	in->gl_vars = Assoc();
	in->types = Assoc();
	return in;
}

/*int hoge(int i, int j){return i+j;}*/

void interpret(Interpreter interp, list line_list){

//	for(int i=0; i<1000000000; ){i=hoge(i,1);}
/*	for(int i=0; i<1000000*100; i++){
		if(!pool) fill_pool();
		obj rr=pool;
		pool = cdr(pool);
		rr->refcount = 1;
		rr->type = INT;
		if(rr->type == tNull) continue;
		rr->refcount--;
		cdr(rr) = pool;
		pool = rr;
	}/**/
/*	for(int i=0; i<1000000*100; i++){
		obj rr=alloc();
		rr->type = INT;
		release(rr);	
	}/**/

/*
obj u=Int(1);
obj e=Int(10000000);
obj x=Int(0);
//while(vrInt(compare('<', x, e))){
while(vrInt(LT(x, e))){
	obj r = add(x, u);
	release(x);
	x=r;
}
/**/
	curr_interp = interp;
	env = nil;
	obj stat = parse(line_list);

	obj rr = nil;
	int t = TickCount();
	if(stat) {
		rr = eval(stat);
		release(stat);
	}
//	myPrintf(" %f sec.", (TickCount()-t)/60.);
	if(rr){
		myPrintf(">> ");
		print(rr);
		release(rr);
	}
}

void dispose_interpreter(Interpreter interpreter){
}

//-------------

inline infn* tag(ValueType t, obj(*fn)(obj)){
	infn* rr = (infn*)alloc();
	rr->type = t;
	ufn(rr) = fn;
	return rr;
}

inline infn* tag(obj(*fn)(obj)){
	return tag(tInternalFn, fn);
}

static obj find_var(obj id){
	for(obj e=env; e; e = cdr(e)){
		obj v = search_assoc(car(e), id);
		if (v) return v;
	}
	obj rr = search_assoc(curr_interp->gl_vars, id);	//global
	if(rr) return rr;
	
	obj (*func)(obj) = searchFunc(id, infnbind);
	if(!func) return nil;
	return tag(func);
}

/*static obj lfind_local(obj e, obj id, obj* *rv){	// assume e!=nil
// rvは見つかった時はset, 見つからなければnil
// returnは見つかってかつコピーが必要な時のみnon-nil
	obj* v = left_search_assoc(car(e), id);
	if (v) {
		if(e->refcount ==1) {*rv=v; return nil;}
		obj asc = copy(car(e));
		*rv = left_search_assoc(car(e), id);
		return op(asc, retain(cdr(e)));
	}
	if(cdr(e)){
		obj re = lfind_local(cdr(e), id, rv);
		if(re) return op(retain(car(e)), re);
	}
	*rv = nil;
	return nil;
}*/
static obj left_search_pair(list l, obj key) {
	for (; l; l=rest(l)) {
		assert(l->refcount ==1);
		obj pair = first(l);
		if(equal(car(pair), key)) return pair;
	}
	return nil;
}

static obj* left_search(obj vars, obj key) {
	assert(vars->type==tAssoc);
	obj pair = left_search_pair(ul(vars), key);
	if(pair) return &cdr(pair);
	else return nil;
}

static obj* lfind_var(obj id){
	if(id->type==tRef) return &(uref(id));
	if(env){
		for(obj e=env; e; e=cdr(e)){
			if(e->refcount !=1) break;
			obj* v = left_search(car(e), id);
			if (v) return v;
		}
/*/		obj *v;
		obj e = lfind_local(env, id, &v);
		if(e) {release(env); env = e; return v;}
		if(v) return v;
/**/		return add_assoc(&car(env), id, nil);			//local
	} else {	// when in global space
		obj* v = left_search(curr_interp->gl_vars, id);//global
		if(v) return v;
		return add_assoc(&(curr_interp->gl_vars), id, nil);	//global
	}
}

obj* let(obj* lt, obj rt){
	release(*lt);
	*lt=rt;
	return lt;
}

static obj func_def(obj name, obj params, obj expr) {
	assert(type(name)==tSymbol);
	obj* func = lfind_var(name);
	if(! *func) {
		obj (*fn)(obj) = searchFunc(name, infnbind);
		if(fn) let(func, tag(fn));
	}
	list lam = list3(retain(params), retain(expr), nil);
	if(type(*func)==tClosure){			// free if complete overload, in the future
		lam = merge(lam, retain(ul(*func)));
	} else if(type(*func)==tInternalFn){
		lam = merge(lam, list3(retain(*func), nil, nil));
	}
	return retain(*let(func, render(tClosure, lam)));
}

static obj do_assign(obj lt, obj rt){
	switch(type(lt)) {
	case tRef:
		return retain(*let(&(uref(lt)), rt));
	case tSymbol:
		return retain(*let(lfind_var(lt),rt));
	case tInd:
		obj *var;
		var = lfind_var(ult(lt));
		if((*var)->refcount > 1){
			obj nv = copy(*var);
			release(*var);
			*var = nv;
			myPrintf("performance alert: copy");
		}
		obj inds = eval(urt(lt));
		doLInd(var, ul(inds), rt);
		release(inds);
		return retain(rt);
	case LIST:
		return applyCC(do_assign, lt, rt);
		
		if(type(rt)!=LIST) error("list<-nonlist");
		list s = ul(rt);
		for(list l = ul(lt); l; l=rest(l), s=rest(s)){
			if(! s) error("number is not enough for rhs.");
			do_assign(first(l),first(s));
		}
		if(s) error("too much for rhs.");
		return nil;
	}
	print(lt);
	assert(0);
	return nil;
}

static bool bind_vars(obj* vars, obj lt, obj rt){	//だめならfalseをかえす。
	obj utype;
	switch(lt->type){
	case tSymbol:
		if(vars) add_assoc(vars, lt, rt); 
		return true;
	case tRef:
		let(&(uref(lt)), rt);
		return true;
	case INT:
		return equal(lt, rt);
	case tOp:
		utype = search_assoc(curr_interp->types, ult(lt));
		if(utype){
			if(vrInt(utype) != rt->type) return false;
			return bind_vars(vars, urt(lt), uref(rt));
		}
		if(rt->type!=tOp) return false;
		if(! bind_vars(vars, ult(lt), ult(rt))) return false;
		return bind_vars(vars, urt(lt), urt(rt));
	case LIST:
		if(rt->type!=LIST) return false;
		list x=ul(lt), a=ul(rt);
		for(; (x && a); x=rest(x),a=rest(a)){
			if(!bind_vars(vars, first(x), first(a))) return false;
		}
		if(x||a) return false;
		return true;
	}
	print(lt);
	assert(0);
	return nil;
}

static bool pbind_vars(obj* vars, obj lt){	//だめならfalseをかえす。
	obj utype;
	switch(lt->type){
	case tSymbol:
		if(vars) add_assoc(vars, lt, nil); 
		return true;
	case tRef:
		assert(0);
		let(&(uref(lt)), nil);
		return true;
	case INT:
		assert(0);
	//	return equal(lt,rt);
	case tOp:
		utype = search_assoc(curr_interp->types, ult(lt));
		if(utype){
			return pbind_vars(vars, urt(lt));
		}
		pbind_vars(vars, ult(lt));
		return pbind_vars(vars, urt(lt));
	case LIST:
		list x=ul(lt);
		for(; (x); x=rest(x)){
			pbind_vars(vars, first(x));
		}
		return true;
	}
	print(lt);
	assert(0);
	return nil;
}

static
list seek_lamb(list ll, obj rt){
	for(; ll; ll=rest(rest(rest(ll)))){
		obj params = first(ll);
		if(type(params)==tInternalFn) return ll;
		int suc = bind_vars(nil, params, rt);
		if(suc) return ll;
	}
	return nil;
}
static
obj strip_return(obj r){
	if(!r) return r;
	if(type(r)!=tSigRet) return r;
	obj rr = retain(uref(r));
	release(r);
	return rr;
}

static obj is = nil;

#define push(v) (is=op(v, is))
/*obj pop(){
	obj lt = retain(car(is));
	obj rt = retain(cdr(is));
	release(is);
	is = rt;
	return lt;
}*/
//-----------------------------

obj exec(obj v){
	obj rr=nil;
	if(type(v)!=LIST && type(v)!=tExec) return eval(v);
	for(list l = ul(v); l; ){
		if(rr) release(rr);
		rr = eval(fpp(l));
		if(rr && type(rr)==tSigRet) break;
		if(rr && type(rr)==tBreak) break;
	}
	return rr;
}

obj udef_op0(obj ope, obj v){
	assert(type(ope)==tSymbol);
	obj lamb = find_var(ope);
	if(!lamb) return nil;
	assert(type(lamb)==tClosure);
	list ll = seek_lamb(ul(lamb), v);
	if(! ll) {
		release(v);
		return nil;
	}
	obj vars = Assoc();
	bind_vars(&vars, first(ll), v);
	push(env);
	env = op(vars, retain(third(ll)));
	release(lamb);	//execのなかでlambが削除される可能性あり
	obj rr = exec(second(ll));
	release(env);
	env = pop(&is);
	return strip_return(rr);
}

obj eval_function(ref lt, rel rt) {//lt をスタック積みに
	if(type(lt)== tInternalFn) goto ci;
	if(type(lt)!=tClosure) {print((obj)lt);  assert(0);}
	list ll = seek_lamb(ul(lt), rt);
	if(ll && type(first(ll))==tInternalFn) {
		lt = first(ll);
		goto ci;
	}
	if(! ll) error("no appropriate function.");
	push(env);
	obj vars = Assoc();
	env = op(vars, retain(third(ll)));
	bind_vars(&vars, first(ll), rt);
	release(rt);
	obj rr = exec(second(ll));
	release(env);
	env = pop(&is);
	return strip_return(rr);

ci:	try {
		obj rr=(ufn(lt))(rt);
		release(rt);
		return rr;
	} catch(eval_error){
		error("not defined for that value.");
		return nil;
	}
}
obj eval_curry(obj exp, obj vars) {	// envはいま実行中の
/*	push(env);
	env = op(vars, nil);
	obj rr = exec(em1(exp));
	pop(&env);
	env = pop(&is);
	return strip_return(rr);
/*/	env = op(vars, env);
	obj rr = exec(em1(exp));
	pop(&env);
	return strip_return(rr);
/**/}

static bool macromode = 0;
static obj macro_env = nil;

obj macro_exec(obj lt, obj rt) {		// not yet
	assert(type(lt)==tSyntaxLam);
	list ll = ul(lt);
	obj vars = Assoc();
	int suc = bind_vars(&vars, first(ll), rt);
	if(! suc) {release(vars); error("no appropriate macro.");}

	macro_env = op(vars,  macro_env);
	macromode = true;
	env = op(Assoc(), env);
	obj rr = exec(second(ll));
	release(pop(&env));
	macromode = false;
	release(pop(&macro_env));
	return rr;
}
obj macro_exec0(obj lt, obj rt) {
	assert(type(lt)==tSyntaxLam);
	list ll = ul(lt);
	obj vars = Assoc();
	int suc = bind_vars(&vars, first(ll), rt);
	if(! suc) {release(vars); error("no appropriate macro.");}

	push(env);
	env = nil;
	obj el =  subs(second(ll), &vars);
	print(el);  scroll();
	env = pop(&is);
	release(vars);	// ちょっと不安
	obj rr = exec(el);
	release(el);
	return rr;
}

void newType(obj identifier){
	static int uniq=tLast+1;
	add_assoc(&(curr_interp->types), identifier, Int(uniq));
	uniq++;
}

obj applyV(double (*func)(double), obj v, obj name){
	if(type(v)==tDouble)	return Double(func(udbl(v)));
	if(type(v)==INT)	return Double(func(uint(v)));
	if(type(v) == tDblArray) {
		DblArray* a;
		a = &(udar(v));
		obj r = dblArray(a->size);
//		obj r = new dblarr(a->size);
		for(int i=0; i<(a->size); i++) udar(r).v[i] = func(a->v[i]);
		return r;
	}
	if(isVec(type(v))){
		int len = size(v);
		obj r = aArray(len);
		for(int i=0; i<len; i++){
			obj lt = ind(v,i);
			uar(r).v[i] = applyV(func, lt, name); 
			release(lt);
		}
		return r;
	}
	if(type(v)==LIST){
		list l = nil;
		for(list l1 = ul(v); l1; l1=rest(l1)){
			l = cons(applyV(func,first(l1), name), l);
		}
		return List2v(reverse(l));
	}
	obj rr=nil;
	if(name){
		rr = udef_op0(name, v);
	}
	if(!rr) error("func: argument must be a scalar or an array.");
	return rr;
}

obj udef_op2(obj ope, obj lt, obj rt){
	obj v = List2v(list2(retain(lt), retain(rt)));
	obj rr = udef_op0(ope, v);
	release(v);
	return rr;
}

obj call_fn(obj (*func)(obj, obj), obj lt, obj rt){
	obj rr = func(lt,rt);
	if(rr) return rr;
	if(isCon(type(lt)) && isCon(type(rt))) return applyCC(func, lt,rt);
	if(isCon(type(rt))) return applySC(func, lt,rt);
	if(isCon(type(lt))) return applyCS(func, lt,rt);
	// user defined operations
	if(func==add){
		static obj symadd = Symbol("add");
		rr = udef_op2(symadd, lt,rt);
	} else if(func==mult){
		static obj symmul = Symbol("mul");
		rr = udef_op2(symmul, lt,rt);
	} else if(func==divide){
		static obj symdiv = Symbol("div");
		rr = udef_op2(symdiv, lt,rt);
	} else if(func==power){
		static obj sympow = Symbol("pow");
		rr = udef_op2(sympow, lt,rt);
	} else assert(0);
	if(!rr) error(": operation not defined");
	return rr;
}

inline obj call_fnr(obj (*func)(obj, obj), obj lt, obj rt){
	obj rr = call_fn(func, lt, rt);
	release(lt);
	release(rt);
	return rr;
}

obj prod(obj v, obj (*func)(obj, obj)){
	obj rr;
	switch(type(v)){
	case LIST:
		assert(!! ul(v));
		rr=retain(first(ul(v)));
		for(list ll=rest(ul(v)); ll; ll=rest(ll)){ 
			obj lt = rr;
			rr = call_fn(func, lt, first(ll));
			release(lt);
		}
		return rr;
	case tLAVec:
	case tDblArray:
	case tDblAr2:
	case tArray:
		int len = size(v);
		if(len==0) return nil;
		rr = ind(v,0);
		for(int i=1; i<len; i++){
			obj lt = rr;
			obj rt = ind(v,i);
			rr = call_fnr(func, lt, rt);
		//	release(lt);
		//	release(rt);
		}	
		return rr;
	default:
		error("not defined for that type.");
		return nil;
	}
}

obj applyCC( obj (*func)(obj, obj), obj v1, obj v2){
	if(type(v1)==LIST && type(v2)==LIST) {
		list l1=ul(v1), l2=ul(v2);
		list l=nil;
		for(; l1 && l2; l1=rest(l1), l2=rest(l2)){
			l= cons(call_fn(func, first(l1), first(l2)), l); 
		}
		if(l1 || l2) error("unmatched num. of elems. in the lists");
		return List2v(reverse(l));
	}
	obj lt,rt;
	if(type(v1)==tDblArray && type(v2)==tDblArray){
		int len = udar(v1).size;
		if(len != udar(v2).size) error("num mismatch");
		obj rr = dblArray(len);
//		obj rr = new dblarr(len);
		double* v = udar(rr).v;
		for(int i=0; i<len; i++){
			lt = Double(udar(v1).v[i]);//遅い
			rt = Double(udar(v2).v[i]);
			obj rx = call_fnr(func, lt,rt);
		//	release(lt);
		//	release(rt);
			if(type(rx)!=tDouble) error("array: type mismatch");//kore mondai
			v[i] = udbl(rx);
			release(rx);
		}
		return rr;
	}
	if(isVec(type(v1)) && isVec(type(v2))){
		int len=size(v1);
		if(len!=size(v2)) error("num mismatch");
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			lt = ind(v1,i);
			rt = ind(v2,i);
			uar(rr).v[i] = call_fnr(func, lt, rt); 
		//	release(lt);
		//	release(rt);
		}
		return rr;
	}
	if( type(v1)==LIST && isVec(type(v2))){
		list l=nil, l1=ul(v1);
		int len=size(v2);
		for(int i=0; i<len; i++,l1=rest(l1)){
			if(! l1) error("num mismatch");
			rt = ind(v2,i);
			l = cons(call_fn(func, first(l1), rt), l);
			release(rt);
		}
		return List2v(reverse(l));
	}
	if( isVec(type(v1)) && type(v2)==LIST){
		list l=nil, l2=ul(v2);
		int len=size(v1);
		for(int i=0; i<len; i++,l2=rest(l2)){
			if(! l2) error("num mismatch");
			lt=ind(v1,i);
			l=cons(call_fn(func, lt, first(l2)), l);
			release(lt);
		}
		return List2v(reverse(l));
	}
	error("operation not defined.");
	return nil;
}

obj applySC( obj (*func)(obj, obj), obj v1, obj v2){
	assert(!isVec(type(v1)));
	if(isVec(type(v2))){
		int len=size(v2);
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			obj rt=ind(v2,i);
			uar(rr).v[i] = call_fn(func, v1, rt); 
			release(rt);
		}
		return rr;
	}
	if(type(v2)==LIST){
		list l=phi();
		for(list l2=ul(v2); l2; l2=rest(l2)){
			l = cons(call_fn(func, v1, first(l2)), l);
		}
		return List2v(reverse(l));
	}
	assert(0);
	return nil;
}

obj applyCS(obj (*func)(obj, obj), obj v1, obj v2){
	assert(!isVec(type(v2)));
	if(isVec(type(v1))){
		int len=size(v1);
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			obj lt=ind(v1,i);
			uar(rr).v[i] = call_fn(func, lt, v2); 
			release(lt);
		}
		return rr;
	}
	if(type(v1)==LIST){
		list l=phi();
		for(list l1=ul(v1); l1; l1=rest(l1)){
			l = cons(call_fn(func, first(l1), v2), l);
		}
		return List2v(reverse(l));
	}
	assert(0);
	return nil;
}

#define sp0 car(is)
#define sp1 car(cdr(is))
#define sp2 car(cdr(cdr(is)))
/*
obj prod_eval0(list l, obj (*func)(obj, obj)){
	obj lt,rt,rr;
	assert(!! l);
	lt = eval(fpp(l));
	rr = lt;
	for(; l; ){
		rt = eval(fpp(l));
		rr = call_fn(func, lt, rt);
		release(lt);
		release(rt);
		lt = rr;
	}
	return rr;
}
*/
obj prod_eval(list l, obj (*func)(obj, obj)){
	assert(!! l);
	obj rr = eval(fpp(l));
	for(; l; ){
		push(rr);
		push(eval(fpp(l)));
		rr = call_fn(func, sp1, sp0);
		release(pop(&is));
		release(pop(&is));
	}
	return rr;
}

obj Map(obj vi){
	if(type(vi)!=LIST) error("map: needs two arguments.");
	obj fn = em0(vi);
	obj v = em1(vi);
	if(!(type(fn)==tClosure || type(fn)==tInternalFn)) error("map: first arg must be a function");
	switch(type(v)){
	case LIST:
	case ARITH:
		list r = nil;
		for(list l=ul(v); l; l=rest(l)){
			r = cons(eval_function(fn, retain(first(l))), r);
		}
		return render(type(v), reverse(r));
	case tDblArray:
	case tIntArr:
	case tArray: {
		int len = size(v);
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			obj arg = ind(v,i);
			uar(rr).v[i] = eval_function(fn, arg);
		//	release(arg);
		}
		return rr;
	} default:
		error("map: second arg must be a list or an array.");
		return nil;
	}
}

/*inline list evalList(list l){
	list rl = phi();
	for(;l;) rl = cons(eval(fpp(l)), rl);
	return reverse(rl);
}*/
inline list strip2list(obj r){
	list l = ul(r);
	ul(r)=nil;
	release(r);
	return l;
}
inline list evalList(list l){
	push(List2v((list)nil));
	for(;l;) ul(sp0) = cons(eval(fpp(l)), ul(sp0));
	obj rr = pop(&is);
	return reverse(strip2list(rr));
}

static int curr_operator;
obj compare(int op, obj lt, obj rt);

obj compare0(obj lt, obj rt){
	return compare(curr_operator, lt,rt);
}

obj compare(int op, obj lt, obj rt){
	if(op==EQ) return Int( equal(lt, rt));
	if(op==NE) return Int(!equal(lt, rt));
	obj (*fn)(obj, obj);
	ValueType lty=lt->type, rty=rt->type;
	switch(op){
	case '>': 	fn = ccgt;	break;
	case '<': 	fn = cclt; 	break;
	case GE: 	fn = ccge; 	break;
	case LE: 	fn = ccle; 	break;
	default: 	assert(0);
	}
	obj rr = fn(lt, rt);
	if(rr) return rr;
	// vector
	curr_operator = op;
	if(isCon(lty) && isCon(rty)) return applyCC(compare0, lt,rt);
	if(isCon(lty)) return applyCS(compare0, lt,rt);
	if(isCon(rty)) return applySC(compare0, lt,rt);
	error("compare: type undefined.");
	return nil;
}

/*inline obj evalCond(obj exp){
	obj rr,lt,rt;
	list l = ul(exp);
	lt = eval(fpp(l));
	rt = eval(fpp(l));
	int op = uint(fpp(l));
	rr = compare(op, lt, rt);
	release(lt);
	lt=rt;
	for(; l;){
		rt = eval(fpp(l));
		int op = uint(fpp(l));
		rr = and1(rr, compare(op, lt, rt)); // and1 releases both
		release(lt);
		lt = rt;
	}
	release(lt);
	return rr;
}*/

inline obj evalCond(obj exp){
	list l = ul(exp);
	push(eval(fpp(l)));
	push(eval(fpp(l)));
	int c = uint(fpp(l));
	obj rr = compare(c, sp1, sp0);
	release(sp1);
	sp1 = rr;
	for(; l;){
		push(eval(fpp(l)));
		c = uint(fpp(l));
		sp2 = and1(sp2, compare(c, sp1, sp0)); // and1 releases both
		release(sp1);
		sp1 = sp0;
		pop(&is);	// should be nil
	}
	release(pop(&is));
	return pop(&is);
}

obj global(obj rt){
	if(!env) error("global assign in global env");
	assert(type(rt)==tSymbol);
	obj gv = search_pair(curr_interp->gl_vars, rt);
	if(!gv) error("no such global");
	list *arg_bind = &(ul(car(env)));
	*arg_bind = cons(retain(gv), *arg_bind);
	return nil;
}

obj qquote(obj rt){
	return subs(rt, nil);
}

obj typeDef(obj rt){
	obj rr=search_assoc(curr_interp->types, rt);	// get typenum
	if(rr!=nil) return nil;
	newType(rt);
	return nil;
}
static
obj Catch(obj rt){		// not in function now
	assert(rt->type==LIST);
//	push(retain(env));
//	let(lfind_var(em0(rt)), encap(tCont, retain(is)));
	obj rr = eval(rt);
//	let(&env, pop());
	return rr;
}
static
obj syntax(obj rt){
	let(lfind_var(car(car(rt))),  render(tSyntaxLam, list3(cdr(car(rt)), cdr(rt), nil)));
	return nil;
}

struct funcbind specials[] = {	//special operators
	{"'",		retain},
	{"quote",	retain},
	{"qquote",	qquote},
	{"exec",	exec},
	{"global",	global},
	{"catch",	Catch},
	{"typedef",	typeDef},
	{"syntax",	syntax},
	{"",nil}
};
inline obj eval_symbol(obj exp){	//assuming a symbol
	obj rr = find_var(exp);
	if(rr) return rr;
	rr = search_assoc(curr_interp->types, exp);	//type id
	if(rr) return rr;
	if(strcmp(ustr(exp), "glist")==0) return retain(curr_interp->gl_vars);
	obj (*func)(obj) = searchFunc(exp, specials);
	if(func) return tag(tSpecial, func);
	print(exp);
	myPrintf(" ");
	error(":undefined identifer");
	return nil;
}

obj eval(obj exp){
ev:	assert(!! exp);
	switch (exp->type) {
	case tInd:
		return doInd(eval(ult(exp)), ul(eval(urt(exp))));
	case LIST:
		return List2v(evalList(ul(exp)));
	case tArray:
		return map_obj(eval, exp);
	case tAnd:
		return  prod_eval(ul(exp), mult);
	case MULT:
		return  prod_eval(ul(exp), mult);
	case ARITH:
		return prod_eval(ul(exp), add);
	case POW:
		return prod_eval(ul(exp), power);
	case DIVIDE:
		return prod_eval(ul(exp), divide);
	case tRef:
		return retain(uref(exp));
	case tSymbol:
		if( macromode) {
			if(obj rr = search_assoc(car(macro_env), exp)){
				macromode = false;
				rr = exec(rr);
			//	rr = eval(rr);
				macromode = true;
				return rr;
			}
		}
		return eval_symbol(exp);
	case tMinus:
		return  uMinus(eval(uref(exp)));
	case tReturn:
		if(! uref(exp)) return encap(tSigRet, nil);
		return  encap(tSigRet, eval(uref(exp)));
	case tBreak:
		return retain(exp);
	case CONDITION:
		return evalCond(exp);
	case tOp:
		obj rr,lt;
		if(type(ult(exp)) ==tSymbol) {
			lt = search_assoc(curr_interp->types, ult(exp));
			if(lt) return encap((ValueType)vrInt(lt), eval(urt(exp)));}
		lt = eval(ult(exp));
		push(lt);
		switch(lt->type){
		case tCont:
			assert(0);
		case tSpecial:
			rr = ufn(lt)(urt(exp));
			break;
		case tSyntaxLam:
			rr = macro_exec(lt, urt(exp));
			break;
		case tInternalFn:
		case tClosure:
			obj rt = eval(urt(exp));
//for(int i=0; i<1e+6; i++){
			rr = eval_function(lt, rt);
		//	release(rt);
//release(rr); } rr=nil;
			break;
		default:
			rt = eval(urt(exp));
			rr = call_fn(mult, lt, rt);
			release(rt);
		}
		release(pop(&is));
		return rr;
	case tClosure:
		assert(0);
	case tCurry:
		return eval_curry(exp, em0(exp));
/*		obj vars = Assoc();
		bind_vars(&vars, em0(exp), em2(exp));
		rr = eval_curry(exp, vars);
		release(vars);
		return rr;
*/	case tArrow:
//		return enclose(exp);
/*		if(macromode){
			if(obj rr = search_assoc(car(macro_env), exp)){
			}
		}
*/		return render(tClosure, list3(retain(em0(exp)), retain(em1(exp)), retain(env)));
	case tDefine:
		return func_def(em0(exp), em1(exp), em2(exp));
	case tSyntaxDef:
		let(lfind_var(em0(exp)),  render(tSyntaxLam, list3(em1(exp), em2(exp), nil)));
		return nil;
	case tExec:
		return exec(exp);
	case tAssign:
		lt = car(exp);
		if(type(lt)==tOp){
			return func_def(ult(lt), urt(lt), cdr(exp));
		} else return do_assign(lt, eval(cdr(exp)));
	case tIf:
		rr = eval(em0(exp));
		if (type(rr) != INT) error("if: Boolean Expected");
		if (vrInt(rr)) {
			rr = em1(exp);
		} else {
			rr = em2(exp);
		}
		return exec(rr);
	case tWhile:
		for(;;) {
			rr = eval(car(exp));
			if (type(rr) != INT) error("while: Boolean expected");
			if(!vrInt(rr)) break;
			rr = exec(cdr(exp));
			if(rr && type(rr)==tSigRet) return rr;
			if(rr && type(rr)==tBreak) {release(rr); break;}
			if(rr) release(rr);
		}
		return nil;
	default:
		return retain(exp);
	}
}
/*obj eval(obj exp){
//	print(exp);  scroll();
	obj rr=eval0(exp);
//	myPrintf("   "); print(rr);
	return rr;
}*/

inline obj ref2var(obj var){
	obj r = alloc();
	r->type = tRef;
	uref(r) = var;
	return r;
}

static obj new_assign;

obj subs0(obj v, obj * vars){
	assert(!! v);
	switch(v->type){
	case tSymbol:
		if(vars){		// macro
			obj vp = search_assoc(*vars, v);
			if(vp) return vp;
		//	vp = searchFunc(v, specials);
		//	if(vp) {release(vp); return retain(v);}
			obj (*func)(obj) = searchFunc(v, specials);
			if(func) return retain(v);
			vp = find_var(v);
			if(vp) {release(vp); return retain(v);}
			assert(0);
		//	return ref2var(add_assoc(*vars, v, Null()));
		} else {		// quasi-quote
			obj vp = find_var(v);
			if(vp) return vp;
			return retain(v);
		}
	case tAssign:
		obj vp = search_assoc(*vars, car(v));	//macro-locals
		if(vp) goto nex;
	/*	vp = searchFunc(car(v), specials);		// not needed because cant assign to global
		if(vp) {release(vp); vp = retain(v); return vp;}
		vp = find_var(v);
		if(vp) {release(vp); vp = retain(v); return vp;}
	*/	vp = ref2var(nil);
		add_assoc(vars, car(v), vp);
nex:		return operate(tAssign, vp, subs0(cdr(v), vars));
	case tArray:
		obj r = aArray(uar(v).size);
		for(int i=0; i < uar(v).size; i++) uar(r).v[i] = subs0(uar(v).v[i], vars);
		return r;
	case LIST:		//list
	case POW:
	case MULT:
	case DIVIDE:
	case ARITH:
	case CONDITION:
	case tIf:
	case tExec:
		list l = phi();
		for(list s=ul(v); s; s=rest(s))  l = cons(subs0(first(s), vars), l);
		return render(type(v), reverse(l));
	case tReturn:
		if(!uref(v)) return retain(v);
	case tMinus:
		return encap(v->type, subs0(uref(v), vars));
	case tClosure:
	case tArrow:
		return render(type(v), list3(subs0(em0(v),vars), subs0(em1(v), vars), nil));
	case tDefine:
	case tSyntaxDef:
		assert(0);
	case tInd:
	case tWhile:
	case tOp:
		return operate(v->type, subs0(car(v), vars), subs0(cdr(v), vars));
	case INT:
	case tDouble:
	case TOKEN:
	case tNull:
	case tLAVec:
	case tDblArray:
	case tIntArr:
	case tDblAr2:
	case IMAGE:
	case STRING:
	case tBreak:
		return retain(v);
	}
	print(v);
	assert(0);
	return v;
}

obj subs(obj v, obj * vars){
	new_assign = Assoc();
	obj rr = subs0(v, vars);
	release(new_assign);
	return rr;
}

inline obj is_in(obj e, obj id){
	for(; e; e = cdr(e)){
		obj p = search_pair(car(e), id);
		if (p) return p;
	}
	return nil;
}

static obj penv;

static obj vto_close;

static
void enclose0(obj v){
	assert(!! v);
	switch(v->type){
	case tSymbol:
/*		if( macromode) {
			for(obj e = macro_env; e; e = cdr(e)){
				obj rr = search_assoc(car(e), v);
				if (v) { rr = v; break;}
			}
			//if(obj rr = search_assoc(car(macro_env), v)){ v=rr;}
		}
/**/		if(is_in(penv, v)) return;
		if(search_pair(vto_close, car(v))) return;
		add_assoc(&vto_close, v, nil); 
		return;
	case tAssign:
		enclose0(cdr(v));
		if(is_in(penv, car(v))) return;
		if(search_pair(vto_close, car(v))) return;
		if(is_in(env, car(v))) {
			add_assoc(&vto_close, car(v), nil);
			return;
		}
		add_assoc(&car(penv), car(v), nil);	// new assignment
		return;
	case tClosure:
		assert(0);
	case tArrow:
		obj vs = Assoc();
		pbind_vars(&vs, em0(v));
		penv = op(vs, penv);
		enclose0(em1(v));
		release(pop(&penv));
		return;
	case tDefine:
	case tSyntaxDef:
		assert(0);
	case tArray:
		for(int i=0; i < uar(v).size; i++) enclose0(uar(v).v[i]);
		return;
	case LIST:		//list
	case POW:
	case MULT:
	case DIVIDE:
	case ARITH:
	case CONDITION:
	case tIf:
	case tExec:
	case tAnd:
		for(list s=ul(v); s; s=rest(s)) enclose0(first(s));
		return;
	case tReturn:
		if(!uref(v)) return;
	case tMinus:
		enclose0(uref(v));
		return;
	case tInd:
	case tWhile:
	case tOp:
		enclose0(car(v));
		enclose0(cdr(v));
		return;
	case INT:
	case tDouble:
	case TOKEN:
	case tNull:
	case tLAVec:
	case tDblArray:
	case tIntArr:
	case tDblAr2:
	case IMAGE:
	case STRING:
	case tBreak:
		return;
	}
	print(v);
	assert(0);
	return;
}

inline obj curry(obj var, obj val, obj code){
	obj vars = Assoc();
	bind_vars(&vars, var, val);		// retainは適切？
	return render(tCurry, list3(vars, code,  nil));
	return render(tCurry, list3(var, code,  val));
}

static
obj enclose(obj v){
	vto_close = Assoc();
	assert(v->type == tArrow);
	obj vs = Assoc();
	pbind_vars(&vs, em0(v));
	penv = op(vs, nil);
	enclose0(em1(v));
	release(penv);

	assert(vto_close->type == tAssoc);
	if(! ul(vto_close)) return render(tClosure, list3(retain(em0(v)), retain(em1(v)), nil));
	list varlist = nil, vallist = nil;
	for(list l = ul(vto_close); l; l=rest(l)){
		varlist = cons(retain(car(first(l))), varlist);
		vallist = cons(find_var(car(first(l))), vallist);
	}
	release(vto_close);
	obj rr = curry(List2v(varlist), List2v(vallist), retain(em1(v)));
	rr = render(tClosure, list3(retain(em0(v)), rr, nil));
	return rr;
}


/*
obj eval_meth(obj  exp){	// call by reference
	obj lt,rt,rr,fn;
	lt=em0(exp);
	rt=em1(exp);
	list args;
	switch(rt->type){
	case tOp:
		fn=ult(rt);
		assert(urt(rt)->type==LIST);
		args=map(eval, ul(urt(rt)));
		break;
	case Identifier:
		fn=rt;
		args = nil;
		break;
	default:
		error("non-funcall in rhs of a bar.");
	}
	fn=eval(fn);
	obj object = alloc();
	uref(object)2 = lfind_var(lt);
	switch(fn->type){
	case tClosure:
	case tInternalFn:{
		obj args1=List2v(cons(object, args));
		rr = eval_function(fn, args1);
//		let(uref(object)2, rr);
		break;}
	default:
		error("no such method.");
	}
	return rr;
}
*/

#define EVAL(ex, ia)  exp=ex; ra=ia; goto ev; a##ia:

