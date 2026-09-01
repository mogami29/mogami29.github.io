/* value.h a part of cipher language, by Tsuguo Mogami  */
#include <stdlib.h>

typedef struct {
	int size;
	double* v;
} DblArray;

typedef struct {
	int size;
	int* v;
} IntArr;

typedef struct {
	int size;
	char* v;
} ByteArr;

typedef struct {
	int size;
	obj *v;
} Array;

typedef struct {
	short size1;
	short size2;
	double* v;
} DblArr2;

typedef struct {
	obj rator;
	obj rand;
} opExp;

typedef enum {
	INT = 1,
	tDouble,
	tChar,	//inte
	STRING,	//string
	tNull,
// vector type
	tDblArray,	//dbl_arr
	tIntArr,	//int_arr
	LIST,		//list
	tArray,	//array
	tHash,
// vector derivative type
	tDblAr2,	//dbl_ar2
	tLAVec,	//dbl_arr
	IMAGE,	//array of dbl_arr
	tCImg,	//3-array of array of dbl_arr
	tSpVec,	//spvec
// expression type
	tSymbol,	//string
	tOp,		//op
	POW,		//list-2
	MULT,	//list
	DIVIDE,	//list
	ARITH,	//list
	CONDITION,	//list
	tAssign,	//op
	tDefine,	//list-3
	tSyntaxDef,	//list-3
	tIf,		//list-3
	tWhile,	//op
	tReturn,	//ref
	tBreak,
	tSeq,		//not used
	tAnd,		//list-2
	tInd,		//op
	tAssoc,	//list of list-2
	tSyntaxLam,//list-2, similar to lambda
	tMeth,	//list-2
	tMinus,	//obj
	tExec,	//list
	tType,	// op
	tArrow,	// list
	tClosure,	// list-2,3
	tCurry,	// list
	tScope,	// op
// naibu
	tInternalFn,	//func
	tSpecial,	//func
	TOKEN,	//int
	tRef,		//ref, 
	tCont,	//ref
	tSigRet,	//ref
// naibude (shell) tsukau
	FRACTION,	//list-2
	SuperScript,//list
	SubScript,	// list
	tShow,	// list
	tHide,		// list
	tIns,		// list
	tDel,		// intv
	tMove,	// list
	tLine,		// dbl_arr
	tLast		// 52
} ValueType;

//struct value {
class value {
public:
	int	refcount;
	ValueType	type;
//	virtual ~value() {}
	void*operator new(size_t size);
	void operator delete(void* val, size_t size);
};

class int_: public value {
public:
	int		intv;
//	int_(int i){type=INT; intv=i;}
	int_(int i):intv(i){type=INT;}
};
#define uint(v) (((int_*)v)->intv)

class dbl_: public value {
public:
	double	dblv;
//	double double(v){return dblv;}
};

class capsl: public value {
public:
	obj		ref;
};
#define uref(v) (((capsl*)v)->ref)

#define uda2(v) (((dblar2*)v)->dbl_ar2)
#define udar(v) (((dblarr*)v)->dbl_arr)
#define uiar(v) (((intarr*)v)->int_arr)
#define udbl(v) (((dbl_*)v)->dblv)

class dblar2: public value {
public:
	DblArr2	dbl_ar2;
};
class intarr: public value {
public:
	IntArr	int_arr;
};
class dblarr: public value {
public:
	DblArray	dbl_arr;

	int size();
	dblarr(int n) ;
/*	inline dblarr(int n) {
		type = tDblArray; 
		dbl_arr.size = n;
		dbl_arr.v = (double *)my_malloc(sizeof(double)*n);
	}/**/
};
class str_: public value {
public:
	char*		string;
};
#define ustr(v) (((str_*)v)->string)

class hash_: public value {
public:
	class hash*	hash;
};
#define uhash(v) (((hash_*)v)->hash)

class spvec_: public value {
public:
	class spvec*	spv;
};
#define uspv(v) (((spvec_*)v)->spv)

class infn: public value {
public:
	obj	(*fn)(obj);
};
#define ufn(v) (((infn*)v)->fn)

class list_: public value {
public:
	list		list;
	int size();
};
#define ul(ex)	(((list_*)ex)->list)
//inline List& ul(obj ex){return ex->u.list;}

class opr: public value {
public:
	opExp		ope;
};

class arr: public value {
public:
	union {
		Array		array;
	} u;
	int size();
};
#define uar(ex)	(((arr*)ex)->u.array)

inline ValueType type(ref v) {return v->type;};
//void fill_pool();
/*inline obj alloc(){
	if(!pool) fill_pool();
	obj r = pool;
	pool = pool->u.ope.rand;
	r->refcount=1;
	return r;
}/**/
obj alloc();
//obj Null();
obj Assoc();
obj Token(int i);
double v2Double(obj v);	//cast to double
char* copyString(const char* str);
str_* val(char* str);
inline obj String2v(const char* s){return val(copyString(s));}
obj cString(const char* st, const char* en);
obj aString(int n);	//alloc string
obj Symbol(const char* s);
obj Int(int i);
obj Double(double d);
obj dblArray(int n);
obj intArray(int n);
arr* aArray(int n);
arr* cArray(obj v[], int n);
obj vector(int n);
list_* render(ValueType type, list l);
inline list_* create(ValueType t, list l){return render(t, l);}
obj encap(ValueType t, obj v);
list_* List2v(list l);
bool isToken(obj v, int tk);
obj copy(obj v);
int vrInt(obj v);		// value-release to Int
void print_vector(DblArray v);
void print_array(DblArray ar);
void print_list(list lis);
void print_image(obj v);
void print(obj v);

obj inv(obj v);	// in matrix.c

void 	set(DblArray* a, int i, double d);
obj	add(obj lt, obj r);
obj	uMinus(obj left);
obj	divide(obj lt, obj rt);
obj	power(obj lt, obj rt);
obj	mult(obj lt, obj rt);
obj	and1(obj lt, obj rt);	//releasing
bool 	equal(obj left, obj right);
obj 	ccgt(obj lt, obj rt);
obj 	cclt(obj lt, obj rt);
obj 	ccge(obj lt, obj rt);
obj 	ccle(obj lt, obj rt);

//---------------
#define ult(ex)	(((opr*)ex)->ope.rator)
#define urt(ex)	(((opr*)ex)->ope.rand)

inline obj& car(obj s){return ((opr*)s)->ope.rator;}
inline obj& cdr(obj s){return ((opr*)s)->ope.rand;}

inline obj operate(ValueType op, obj lt, obj rt){
	opr* rr = (opr*)alloc();
	rr->type=op;
	rr->ope.rator=lt;
	rr->ope.rand=rt;
	return (obj)rr;
}
inline obj op(obj lt, obj rt){
	opr* r = (opr*) alloc();
	r->type = tOp;
	r->ope.rator = lt;
	r->ope.rand  = rt;
	return (obj)r;
}

#define  em0(ex) first(ul(ex))
#define  em1(ex) second(ul(ex))
#define  em2(ex) third(ul(ex))

/*inline obj capstr(char* str){	// taking the string
	str_* r = (str_*)alloc();
	r->type = STRING;
	r->string = str;	
	return (obj)r;
}*/
