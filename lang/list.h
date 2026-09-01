/* list.h: a part of cipher language, by Tsuguo Mogami  */
#include <stdlib.h>

//#define L node*
//#define L node<class T>*
#define L list

template <class T> class node {
//class node {
public:
	obj	a;
	node* d;
//	node<T>* d;
	int	refcount;
	void*operator new(size_t size);
	void operator delete(void* val, size_t size);
};

//--------リストハンドル系-----------
inline L phi(){return nil;}
inline L&	rest(L l){ return l->d;}
inline obj& 	first(L l){ return l->a;}
inline obj& 	second(L l){return first(rest(l));};
inline obj& 	third(L l){return second(rest(l));};
L 	rest(L l, int n);
L* 	rest(L* l, int n);
int 	length(L l);
//template <class T> int length(node<T>* l);
L	merge(L l1, L l2);		//左は破壊、右はtaking
void 	append(L* lis, obj v);
L	node_alloc();
void 	node_free(L p);
L 	cons(obj v, L l);
inline L retain(L l){ l->refcount++; return l;}
void	release(L l);
L 	copy(L source);
obj* 	last(L l);
obj 	take(L* l, int n);
obj	pop(L* lp);
L 	reverse(L lp);//破壊的

obj 	listToCString(list l);	// in appSpeci.c
list	list1(obj v);
list	list2(obj v1, obj v2);
list	list3(obj v1, obj v2, obj v3);
list	apply2(L l1, L l2, obj (*func)(obj, obj));
list	map(obj (*func)( obj), L l1);
list	mapSL(obj (*func)(obj, obj), obj v, L l1);
list	applyVL(obj v, L l1, obj (*func)(obj, obj));

inline obj fpp(L &l){
	obj v=first(l);
	l=rest(l);
	return v;
}

//inline obj& car(List s){return s->a;}
//inline List& cdr(List s){return s->d;}

/*inline List cons(obj v, List list){
	Node *nn = node_alloc();
	nn->a = v;
	nn->d = list;
	return nn;
}/**/
