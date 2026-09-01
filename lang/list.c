/*	LIST LIB 2002 Tsuguo Mogami  */
#include "ciph.h"
#include "list.h"
//#include <stdlib.h>


//--------リストハンドル系-----------

static L pool2 = nil;

#define NODESIZE 16

//template <class T>
L node_alloc(){
	L nn;
	if(pool2){
		nn = pool2;
		pool2 = pool2->d;
	//	nn->refcount = 1;
	//	assert(nn->refcount==1);
	} else	{
		nn = (L)malloc(NODESIZE);
		if(nn==nil) {
			DrawString("\pmemory shortage");
			exit2shell();
		}
		nn->refcount = 1;
	}
	return nn;
}

void node_free(L p){
	p->d = pool2;
	pool2 = p;
}
/*
template <class T> void* node<T>::operator new(size_t size){
	assert(size <= NODESIZE);
	return node_alloc();
}

template <class T> void node<T>::operator delete(void* n, size_t size){
	assert(size <= NODESIZE);
	((node*)n)->d = pool2;
	pool2 = (node*)n;
}
*/
L cons(obj v, L l){
	L nn = node_alloc();
//	L nn = new node<T>();
	nn->a = v;
	nn->d = l;
	return nn;
}

L last0(L l){		//最後のノードを指すリストを返す
	while(l->d != nil) l = l->d;
	return l;
}

L merge(L l1, L l2){		//破壊的
	if(l1==nil) return l2;
	last0(l1)->d = l2;
	return l1;
}

void release(L p){
	L next;
	for( ; p; p=next){
		next = rest(p);
		if((p->refcount)-1) {--(p->refcount); return;}
		release(p->a);
		node_free(p);
	}
}

void append(L* lis, obj v){
	L nn = cons(v, nil);
	if(*lis==nil){
		*lis= nn;
	} else {
		last0(*lis)->d = nn;
	}
}

int length(L l){
	int i=0;
	for(; l; l=rest(l)) i++;
	return i;
}

L rest(L l, int n){
	for(int i=0; i<n; l=rest(l),i++) if(! l) assert(0);
	return l;
}
L* rest(L* l, int n){
	for(int i=0; i<n; l=&rest(*l),i++) if(! *l) assert(0);
	return l;
}

//node生成破壊に関わるのは、cons, free, surface_free, copyList, take
//template <class T>
L copy(L s){	//surface copy
	L dest;
	L* prev;

	prev = &dest;
	for(; s; s=rest(s)){
		L nn = node_alloc();
	//	L nn = new node();
		nn->a = retain(s->a);
		*prev = nn;
		prev = &(nn->d);
	}
	*prev = nil;
	return dest;
}

obj* last(L l){
	if(! l) return nil;
	l = last0(l);
	return &(l->a);
}

obj take(L* lp, int n){	//最初の要素は０番
	lp = rest(lp, n);
	return pop(lp);
}

obj pop(L*lp){
	assert(!! *lp);
	L p = *lp;
	if((p->refcount)-1) {
		--(p->refcount);
		*lp = retain(rest(p));
		return retain(first(p));
	}
	*lp = rest(p);
	obj rv = first(p);
	node_free(p);
	return rv;
}

L reverse(L l){//破壊的
	L dl=nil;
	L next;
	for(;  l; l=next) {
		next = l->d;
		l->d = dl;
		dl = l;
	}
	return dl;
}

//------これより下ではリストの実装に依存していないことを確認。

list list3(obj v1, obj v2, obj v3){
	return cons(v1, cons(v2, cons(v3, nil)));
}
list list2(obj v1, obj v2){
	return cons(v1, cons(v2, nil));
}
list list1(obj v){
	return cons(v, nil);
}

list apply2(L l1, L l2, obj (*func)(obj, obj)){
	L l=nil;
	for(; l1 && l2; l1=rest(l1), l2=rest(l2)){
		l= cons(func(first(l1), first(l2)), l); 
	}
	if(l1 || l2) error("unmatched num. of elems. in the lists");
	return reverse(l);
}
list map(obj (*func)(obj), L l){
	L rl=nil;
	for(; l; l=rest(l)){
		rl= cons(func(first(l)), rl); 
	}
	return reverse(rl);
}
list mapSL(obj (*func)(obj, obj), obj v, L l1){
	L l=nil;
	for(; l1; l1=rest(l1)){
		l= cons(func(v, first(l1)), l); 
	}
	return reverse(l);
}
list applyVL(obj v, list l1, obj (*func)(obj, obj)){
	L l=nil;
	for(;l1; l1=rest(l1)){
		l=cons(func(v, first(l1)), l); 
	}
	return reverse(l);
}
