/*	VECTOR 2004 Tsuguo Mogami  */
#include "ciph.h"
#include "value.h"
#include "list.h"
#include "vector.h"
#include <string.h>


int isVec(ValueType t){
	return t==tDblArray || t==tIntArr || t==tArray;
}
int isCon(ValueType t){
	return t==LIST || t==tDblArray || t==tIntArr ||t==tArray;
}

int size(obj v){
	if(type(v)==tDblArray || type(v)==tLAVec) return udar(v).size;
	if(type(v)==tDblAr2   || type(v)==IMAGE) return uda2(v).size1;
	if(type(v)==tArray)	return uar(v).size;
	if(type(v)==tIntArr)	return uiar(v).size;
	if(type(v)==LIST)	return length(ul(v));
	if(type(v)==tHash)	return uhash(v)->size();
	print(v);
	error("size: not defined");
	return 0;
}

static DblArray applyVD(double func(double, double),DblArray v, double d){
	DblArray rv;
	rv.size = v.size;
	rv.v = (double*)malloc(sizeof(double)*rv.size);														
	for(int i=0; i < v.size; i++){
		rv.v[i] = func(v.v[i], d);
	}
	return rv;
}
/*static obj prod_list(list l, obj (*func)(obj, obj)){
	assert(!! l);
	obj rr=retain(first(l));
	for(list ll=rest(l); ll; ll=rest(ll)){ 
		obj lt = rr;
		obj rt = first(ll);
		rr = func(lt, rt);
		release(lt);
	}
	return rr;
}*/

int find(list l, obj v){
	for(int i=0 ;!!(l); l=rest(l), i++) {
		if( equal(first(l), v)) return i;
	}
	return -1;
}

//#define fn2(fn) obj(*fn)(obj,obj)

//typedef obj(*)(obj,obj) fn;

/*obj map2(obj v){	// to be closure map
	assert(type(v)==LIST);
	obj fn = em0(v); 
	obj v1 = em1(v);
	obj v2 = em2(v);
	if(isVec(type(v1)) && isVec(type(v2))){
		int len=size(v1);
		if(len!=size(v2)) error("num mismatch");
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			obj rt = List2v(list2(ind(v1,i), ind(v2,i)));
			uar(rr).v[i] = eval_function(fn, rt);
			release(rt);
		}
		return rr;
	}
	error("operation not defined.");
	return nil;
}*/

obj toDblArr(obj v){
	int len;
	obj rr;
	if(isVec(type(v))){
		len = size(v);
		rr = dblArray(len);
		double *w = udar(rr).v;
		for(int i=0; i<len; i++){
			obj rt = ind(v,i);
			w[i] = v2Double(rt);
			release(rt);
		}
		return rr;
	}
	if(type(v)==LIST){
		len = length(ul(v));
		rr = dblArray(len);
		double *w = udar(rr).v;
		int i=0;
		for(list l=ul(v); l; l=rest(l), i++){
			w[i] = v2Double(first(l));
		}
		return rr;
	}
	print(v);
	error("not defined");
	return nil;
}
/*ValueType vec_type(obj v){
	int len;
	ValueType vt = (ValueType)0;
	if(isVec(type(v))){
		len = size(v);
		obj rt = ind(v,0);
		vt = type(rt);
		release(rt);
		for(int i=1; i<len; i++){
			rt = ind(v,i);
			if(vt != type(rt)) return (ValueType)0;
			release(rt);
		}
		return vt;
	}
	if(type(v)==LIST){
		list l=ul(v);
		vt=type(fpp(l));
		for(; l;){
			if(vt != type(fpp(l))) return (ValueType)0;
		}
		return vt;
	}
	print(v);
	error("not defined");
	return vt;
}*/

obj map_obj(obj (*func)(obj), obj v){	// not tested yet
	switch(type(v)){
	case LIST:
	case ARITH: {
		list r=phi();
		for(list l=ul(v); l; l=rest(l)){
			r = cons(func(first(l)), r);
		}
		return render(type(v), reverse(r));
	}
	case tDblArray:
	case tArray:
	case tIntArr: {
		int len=size(v);
		obj rr = aArray(len);
		for(int i=0; i<len; i++){
			obj rt = ind(v,i);
			uar(rr).v[i] = func(rt);
			release(rt);
		}
		return rr;
	} default:
		error("map: not defined.");
		return nil;
	}
}

void do_obj(void (*func)(obj), obj v){	// not tested yet
	switch(type(v)){
	case LIST:
	case ARITH:
		for(list l=ul(v); l; l=rest(l)){
			func(first(l));
		}
		return;
	case tDblArray:
	case tArray:
	case tIntArr: {
		int len=size(v);
		for(int i=0; i<len; i++){
			obj rt = ind(v,i);
			func(rt);
			release(rt);
		}
		return;
	} default:
		error("map: not defined.");
	}
}

obj map_arr( obj (*func)(obj), obj v){	// map and the result is an array
	int n;
	obj rr;
	switch(type(v)){
	case LIST: {
		n = size(v);
		rr = aArray(n);
		int i=0;
		for(list l=ul(v); l; l=rest(l), i++){
			uar(rr).v[i] = func(first(l));
		}
		return rr;
	}
	case tDblArray:
	case tArray:
		n = size(v);
		rr = aArray(n);
		for(int i=0; i<n; i++){
			obj rt = ind(v,i);
			uar(rr).v[i] = func(rt);
			release(rt);
		}
		return rr;
	default:
		error("map: not defined.");
		return nil;
	}
}

obj list2arr(obj v){	// 
	int n;
	obj rr;
	switch(type(v)){
	case LIST: {
		n = size(v);
		rr = aArray(n);
		int i=0;
		for(list l=ul(v); l; l=rest(l), i++){
			uar(rr).v[i] = retain(first(l));
		}
		return rr;
	}
	case tDblArray:
	case tArray:
		n = size(v);
		rr = aArray(n);
		for(int i=0; i<n; i++){
			obj rt = ind(v,i);
			uar(rr).v[i] = (rt);
			release(rt);
		}
		return rr;
	default:
		error("map: not defined.");
		return nil;
	}
}

//-----------------------------------
obj search_pair(list l, obj key) {
	for (; l; l=rest(l)) {
		obj pair = first(l);
		if(equal(car(pair), key)) return pair;
	}
	return nil;
}

obj search_pair(obj vars, obj key) {
	if(vars->type==tAssoc) return search_pair(ul(vars), key);
//	if(vars->type==tHash) ;
//	print(vars);
	assert(0);
	return nil;
}

obj search_assoc(obj vars, obj key) {
	obj pair = search_pair(vars, key);
	if(pair) return retain(cdr(pair));
	else return nil;
}

obj* left_search_assoc(obj vars, obj key) {
	obj pair = search_pair(vars, key);
	if(pair) return &cdr(pair);
	else return nil;
}

obj* add_assoc(obj* vars, obj key, obj val){
	assert((*vars)->type==tAssoc);
	obj pair = op(retain(key), retain(val));
	ul(*vars) = cons(pair, ul(*vars));
	return &cdr(pair);
}

obj* left_assoc(obj *var, obj id){
	if(type(*var) !=tAssoc) {
		release(*var);
		*var = Assoc();
	}
	obj* vp = left_search_assoc(*var, id);
	if(vp) return vp;
	return add_assoc(var, id, nil);
}

typedef struct {
	void *var;
	obj (*func)(obj, void*); 
} lambda;

obj map(lambda lam, obj v){
	if(isVec(type(v))){
		int len=size(v);
		obj rr=aArray(len);
		for(int i=0; i<len; i++){
			obj lt=ind(v,i);
			uar(rr).v[i] = lam.func(lt, lam.var);
			release(lt);
		}
		return rr;
	}
	if(type(v)==LIST){
		list l=phi();
		for(list l1=ul(v); l1; l1=rest(l1)){
			l = cons(lam.func(v, lam.var), l);
		}
		return List2v(reverse(l));
	}
	assert(0);
	return nil;
}

//------------ vector accessors---------------


obj ind1(obj lt, obj ix){
	assert(ix->type==INT);
	return ind(lt, uint(ix));
}

static obj nthOf(L p, int i){	//一番最初の要素は０
	for(int j=0; j<i; j++){
		if(p==nil) error("index overrun.");
		p=rest(p);
	}
	if(p==nil) error("index overrun.");
	return p->a;
}

obj ind(obj lt, int ix){
	obj rr;
	switch(type(lt)){
	case LIST:
	case MULT:
		rr = nthOf(ul(lt), ix);
	//	if(rr==nil) error("index overrun.");
		return retain(rr);
	case tDblAr2: {
		if(ix >= uda2(lt).size1) error("index overrun.");
		rr = dblArray(uda2(lt).size2);
		double* vs=(uda2(lt).v)+ix*(uda2(lt).size2);
		for(int i=0; i<udar(rr).size; i++) udar(rr).v[i] = vs[i];
		rr->type = tLAVec;
		break;
	}
	case tLAVec:
	case tDblArray:
		if(ix >= udar(lt).size) error("index overrun.");
		return Double(udar(lt).v[ix]);
	case tIntArr:
		if(ix >= uiar(lt).size) error("index overrun.");
		return Int(uiar(lt).v[ix]);
	case tArray:
		if(ix >= uar(lt).size) error("index overrun.");
		return retain(uar(lt).v[ix]);
	case STRING:
		if(ix >= strlen(ustr(lt))) error("index overrun.");
		rr = Int(ustr(lt)[ix]);
		rr->type=tChar;
		break;
	default:
		error("dot: operation not defined");
	}
	return rr;
}

obj doInd0(obj lt, list ixs){	//release lt, non-release ixs
	obj ix, x, y, rr;
	if(! ixs) return lt;

	ix = first(ixs);
	if(lt->type==tAssoc){
		 x = search_assoc(lt, ix);
		if(! x) error("no such assoc.");
		rr = doInd0(x, rest(ixs));
	} else if(lt->type==tHash){
		 x = (*uhash(lt))[ix];
		if(! x) error("no such assoc.");
		rr = doInd0(x, rest(ixs));
	} else if( isToken(ix,'@')){
		int n = size(lt);
		rr = aArray(n);
		for(int i=0; i<n; i++){
			x = ind(lt, i);
			uar(rr).v[i] = doInd0(x, rest(ixs));
		}
	} else if(type(ix)==LIST){
		list rl=phi();
		for(list l=ul(ix); l; l=rest(l)){
			if(type(first(l))!=INT) error("ind: index should be int.");
			x = ind(lt, uint(first(l)));
			if(! x) error("index overrun.");
			y = doInd0(x, rest(ixs));
			rl = cons(y, rl);
		}
		rr = List2v(reverse(rl));
	} else if(type(ix)==tArray){
		rr = aArray(uar(ix).size);
		for(int i=0; i < uar(ix).size; i++){
			y = ind(ix,i);
			if(type(y)!=INT) error("ind: index should be int.");
			x = ind(lt, vrInt(y));
			if(! x) error("index overrun.");
			uar(rr).v[i] = doInd0(x, rest(ixs));
		}
	}else{
		x = ind(lt, uint(ix));
		if(! x) error("index overrun.");
		rr = doInd0(x, rest(ixs));
	}
	release(lt);
	return rr;
}

obj doInd(obj lt, list indices){		//releasing
	obj rr=doInd0(lt, indices);
	release(indices);
	return rr;
}

static void doLIndDA(double *v, int size, obj index, obj rt){
	if( isToken(index,'@')){
		for(int i=0; ;i++){
			if(size<=i) break;
			obj rr=ind(rt, i);
			if(rr==nil) break;
			assert(rr->type==tDouble);
			*(v+i) = udbl(rr);
		}
	} else if(type(index)==INT){
		int i=uint(index);
		if(size<=i) error("index overrun");
		assert(rt->type==tDouble);
		*(v+i) = udbl(rt);
	} else error("index must be a int.");
}
static void doLIndM(obj lt, list inds, obj rt){	// assume double_array_2
	obj ix=first(inds);
	if( isToken(ix,'@')){
		for(int i=0; ;i++){
			if(i>=uda2(lt).size1) break;
			double * v = (uda2(lt).v)+i*(uda2(lt).size2);
			obj rr=ind(rt, i);
			if(rr==nil) break;
			doLIndDA(v, uda2(lt).size2, second(inds), rr);
		}
	} else if(type(ix)==INT){
		int i = uint(ix);
		if(i >= uda2(lt).size1) error("index overrun");
		double * v = (uda2(lt).v) + i*(uda2(lt).size2);
		inds = rest(inds);
		assert(! rest(inds));
		doLIndDA(v, uda2(lt).size2, first(inds), rt);
	} else error("index must be a int.");
}
/*
static void doLIndV(obj lt, list inds, obj rt){	//expect double_array
	obj ix=first(inds);
	assert(! rest(inds));
	doLIndDA(lt->u.dbl_arr.v, lt->u.dbl_arr.size, ix, rt);
}
*/
double toDouble(obj v){
	if(type(v)==tDouble) return udbl(v);
	if(type(v)==INT) return uint(v);
	error("a double or int expected.");
	return 0;
}

static obj* left_nthOf(L p, int i){	//一番最初の要素は０
	for(int j=0; j<i; j++){
		if(p==nil) error("index overrun.");
		p=rest(p);
	}
	if(p==nil) error("index overrun.");
	return &(p->a);
}

void doLInd(obj* lt, list inds, obj rt){
	assert(!! lt);
	if(! inds){
		release(*lt);
		*lt=rt;
		return;
	}
	if((*lt)->refcount !=1) {assert(0);}
	if(type(*lt)==tDblAr2||(*lt)->type==IMAGE) {
		doLIndM(*lt, inds,rt);
		return;
	}
	obj ix = first(inds);
	if(type(*lt)==tLAVec||(*lt)->type==tDblArray) {
	//	doLIndV(*lt, inds,rt);
	//	obj ix=first(inds);
		assert(! rest(inds));
		doLIndDA(udar(*lt).v, udar(*lt).size, ix, rt);
		return;
	}
	if(type(*lt)==tAssoc){
		doLInd(left_assoc(lt,ix), rest(inds), rt);
		return;
	}
	if(type(*lt)==tHash){
		obj* hr = (uhash(*lt))->reference(ix);
		if(! hr){
			(uhash(*lt))->add(ix, nil);
			hr = (uhash(*lt))->reference(ix);
		}
		doLInd(hr, rest(inds), rt);
		return;
	}
	if(type(*lt)==tArray){
		doLInd(uar(*lt).v + uint(ix), rest(inds), rt);
		return;
	}
	if(type(*lt)==tIntArr) {	// it should be like doLIndDA().
		assert(! rest(inds));
		assert(type(ix)==INT);
		assert(type(rt)==INT);
		uiar(*lt).v[uint(ix)] = uint(rt);
		return;
	}
	if(type(*lt)==tSpVec) {
		(uspv(*lt))->reference(uint(ix)) = toDouble(rt);
		return;
	}
	if(type(*lt)==LIST){
		obj *rx = left_nthOf(ul(*lt), uint(ix));
	//	if(!rx) error("index overrun.");
		doLInd(rx,rest(inds), rt);
		return;
	}
	if( isToken(ix,'@')){
		assert(type(*lt)==LIST);
		for(int i=0; ;i++){
			obj * rx = left_nthOf(ul(*lt), i);
	//		if(!rx) break;
			obj rr = ind(rt, i);
			if(!rr) break;
			doLInd(rx, rest(inds),rr);
		}
		return;
	}
	assert(0);
	return;
}


//--------sparse vector-------------------


#define MIN_ALLOC 16


spvec::spvec(int n){
	room = MIN_ALLOC;
	len = 0;
	size = n;
	ix = new int[room];
	v = new double[room];
}
spvec::spvec(int n, int rm){
	room = rm;
	len = 0;
	size = n;
	ix = new int[room];
	v = new double[room];
}

spvec::~spvec(){
	delete [] ix;
	delete [] v;
}
//spvec& spvec::operator=(const spvec& rhs){}

spvec::spvec(obj vv){
	int n=0;
	switch(type(vv)){
	case LIST: {
		size = ::size(vv);
		int j=0;
		int i=0;
		for(list l=ul(vv); l; l=rest(l), i++){
			if(v2Double(first(l)) != 0) {
//				ix[j] = i;
//				v[j] = vv->u.dbl_arr.v[i];
				j++;
			}
		}
		break;
	}
	case tDblArray:
		size = ::size(vv);
		for(int i=0; i<size; i++){
			if(udar(vv).v[i] != 0) n++; 
		}
		ix = new int[n];
		v = new double[n];
		for(int i=0, j=0; i<size; i++){
			if(udar(vv).v[i] != 0) {
				ix[j] = i;
				v[j] = udar(vv).v[i];
				j++;
			}
		}
		break;
	default:
		error("spvec: not defined.");
	}
	room = n;
	len = n;
}

void spvec::print(){
	int p=0;
	for(int i=0; i<size; i++) {
		if(p<len && ix[p] == i) {
			myPrintf("%f ", v[p]);
			p++;
		} else myPrintf("0 ");
	}
}

double spvec::operator[](int i){
	for(int p=0; p<len; p++) {
		if(ix[p] == i) return v[p];
	}
	return 0;
}

spvec::spvec(spvec& source){		// copy construntor
	room = source.room;
	len = source.len;
	size = source.size;
	ix = new int[room];
	v = new double[room];
	for(int i=0; i<len; i++){
		ix[i] = source.ix[i];
		v[i] = source.v[i];	
	}
}

spvec* mul(double lt, spvec* rt){
//	spvec* r = (spvec*)Malloc(sizeof(spvec));
	spvec* r = new spvec(rt->size, rt->room);
	r->len = rt->len;
	for(int i=0; i<r->len; i++){
		r->ix[i] = rt->ix[i];
		r->v[i] = lt * rt->v[i];	
	}
	return r;
}

/*int n_intersect(int m, int* a, int n, int *b){	//配列aとbの共通する成分の数。
	return 0;
}
*/
inline void heapize(int n, int a[], int i, double b[]){
	int rra = a[i];
	double rrb = b[i];
	int j = i*2;
	while(j <= n){
		if(j+1<= n && a[j] < a[j+1]) j++;	// jを大きな方に照準
		if(rra < a[j]) {
			a[i] = a[j];
			b[i] = b[j];
			i = j;
			j = i*2;
		} else break;
	}
	a[i] = rra;
	b[i] = rrb;	
}

void sort0(int n, int a[], double b[]){	// heap sort, 1-based array, can be non-stable, ascending order
	if(n==1) return;
	for(int l = n>>1; l > 0; l--){
		heapize(n, a, l, b);
	}
	for(int ir = n;;){
		int rra = a[ir]; a[ir] = a[1]; a[1] = rra;
		double rrb = b[ir]; b[ir] = b[1]; b[1] = rrb;
		ir--;
		if(ir ==1) return;
		heapize(ir, a, 1, b);
	}
}

void spvec::sort(){
	sort0(len, ix-1, v-1);
}

spvec* add(spvec* lhs, spvec* rhs){
	if(lhs->size != rhs->size) error("vec:add: unequl number of elements.");
	lhs->sort();
	rhs->sort();
	spvec* rr = new spvec(lhs->size, lhs->len + rhs->len);
	int len = 0;
	int i1=0, i2=0, i=0;
	while(i1 < lhs->len && i2 < rhs->len){
		if(lhs->ix[i1] == rhs->ix[i2]) {
			rr->ix[i] = lhs->ix[i1];
			rr->v[i++] = lhs->v[i1++] + rhs->v[i2++];
			continue;
		}
		if(lhs->ix[i1] < rhs->ix[i2]) {
			rr->ix[i] = lhs->ix[i1];
			rr->v[i++] = lhs->v[i1++];
			continue;
		}
		if(lhs->ix[i1] > rhs->ix[i2]) {
			rr->ix[i] = rhs->ix[i2];
			rr->v[i++] = rhs->v[i2++];
			continue;
		}
	}
	while(i1 < lhs->len){
		rr->ix[i] = lhs->ix[i1];
		rr->v[i++] = lhs->v[i1++];
	}
	while(i2 < rhs->len){
		rr->ix[i] = rhs->ix[i2];
		rr->v[i++] = rhs->v[i2++];
	}
	rr->len = i;
	return rr;
}

spvec* mul(spvec* lhs, spvec* rhs){
	if(lhs->size != rhs->size) error("vec:add: unequl number of elements.");
	lhs->sort();
	rhs->sort();
	spvec* rr = new spvec(lhs->size, lhs->len + rhs->len);
	int len = 0;
	int i1=0, i2=0, i=0;
	while(i1 < lhs->len && i2 < rhs->len){
		if(lhs->ix[i1] == rhs->ix[i2]) {
			rr->ix[i] = lhs->ix[i1];
			rr->v[i++] = lhs->v[i1++] * rhs->v[i2++];
			continue;
		}
		if(lhs->ix[i1] < rhs->ix[i2]) {
			i1++;
			continue;
		}
		if(lhs->ix[i1] > rhs->ix[i2]) {
			i2++;
			continue;
		}
	}
	rr->len = i;
	return rr;
}

/*
spvec& spvec::operator=(const spvec& rhs){
	if (this == &rhs) return *this;
	if(room != rhs.room){
		delete [] v;
		v = new double[rhs.room];
	}
	size=rhs.size;
	room=rhs.room;
	for(int i=0; i<room; i++) v[i]=rhs.v[i];
	return *this;
}
spvec::spvec(spvec& source){	// copy constructor
	v = new double[source.size];
	size=source.size;
	room=source.room;
	for(int i=0; i<room; i++) v[i] = source.v[i];
}
*/
double& spvec::reference(int i){
	for(int p=0; p<len; p++) {
		if(ix[p] == i) return v[p];
	}
	int p=len;
	len++;
	if(len > room) {
		room = room*2;
		int* nix = new int[room];
		double* nv = new double[room];
		for(int i=0; i < p; i++) {
			nix[i] = ix[i];
			nv[i] = v[i];
		}
	}
	ix[p] = i;
//	for(int p=0; p<len; p++) {
//		if(ix[p] == i) return v[p];
//	}
	return v[p];
}

//--------------------------


hash::hash(){
	for(int i=0; i < HASH_SIZE; i++) table[i] = nil;
}

hash::~hash(){
	for(int i=0; i < HASH_SIZE; i++) release(table[i]);
}

static unsigned int hashfun(char *s){
	unsigned int h=0;
	for (int i = 0; s[i] && i<99; i++) {
		h = h * 37 + ((unsigned char*)s)[i];
	}
	return h;
}

static unsigned int hashfun(obj key){
	switch(type(key)){
	case STRING:
	case tSymbol:
		return hashfun(ustr(key));	
	case INT:
	case tChar:
		return uint(key);
	case tOp:
		return hashfun(ult(key));
	}
	return key->type;
}

obj hash::operator[](obj key){
	int i = hashfun(key) % HASH_SIZE;
	if(! table[i]) return nil;
	obj p = search_pair(table[i], key);
	if(!p) return nil;
	return retain(cdr(p));
}

obj* hash::reference(obj key){
	int i = hashfun(key) % HASH_SIZE;
	if(! table[i]) return nil;
	obj p = search_pair(table[i], key);
	if(!p) return nil;
	return &cdr(p);
}

void hash::add(obj key, obj val){
	int i = hashfun(key) % HASH_SIZE;
	table[i] = cons(op(retain(key), retain(val)), table[i]);
}

hash::hash(obj v){
	for(int i=0; i < HASH_SIZE; i++) table[i] = nil;
//	*this = hash();
//	map(hash::add2(this, *), v);
	assert(type(v)==LIST);
	for(list l=ul(v); l; l=rest(l)){
		obj t=first(l);
		assert(t->type==LIST);
		obj key = em0(t);
		obj val = em1(t);
		int i = hashfun(key) % HASH_SIZE;
		table[i] = cons(op(retain(key), retain(val)), table[i]);
	}
}

void hash::print(){
	for(int i=0; i<HASH_SIZE; i++) if(table[i]) print_list(table[i]);
}

int hash::size(){
	int len=0;
	for(int i=0; i<HASH_SIZE; i++) len += length(table[i]);
	return len;
}

obj hash::array(){
	int n = size();
	obj r = aArray(n);
	int j=0;
	for(int i=0; i<HASH_SIZE; i++) 
		for(list l=table[i]; l; l=rest(l)) uar(r).v[j++] = retain(first(l));
	return r;
}
