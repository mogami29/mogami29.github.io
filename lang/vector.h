/* this is a part of cipher language, by Tsuguo Mogami  */
int find(list l, obj v);
int size(obj v);
int isVec(ValueType t);
int isCon(ValueType t);
inline int isvec(obj v){return isVec(v->type);};
inline int iscon(obj v){return isCon(v->type);};
obj toDblArr(obj v);
obj map_obj( obj (*func)(obj), obj v);
obj map_arr( obj (*func)(obj), obj v);
void do_obj(void (*func)(obj), obj v);	// not tested yet
ValueType vec_type(obj v);

obj search_pair(list l, obj key);
obj search_pair(obj vars, obj key);
obj *left_search_assoc(obj,obj);
obj search_assoc(obj,obj);
obj* add_assoc(obj* vars, obj key, obj val);

obj 	ind(obj left, int index);
obj	doInd(obj lt, list indices);
void	doLInd(obj* lt, list indices, obj rt);

//typedef struct {
class spvec {
	int room;
	int len;
	int size;
	int* ix;
	double* v;
public:
	spvec(int n);
	spvec(obj v);
	~spvec();
	void print();
	double operator[](int i); //右辺でのみ使うように
	double& reference(int i);

//	spvec& operator=(const spvec& rhs);
	spvec(spvec& source);
	friend spvec* add(spvec* lhs, spvec* rhs);
	friend spvec* mul(spvec* lhs, spvec* rhs);
	friend spvec* mul(double lhs, spvec* rhs);
private:
	spvec(int n, int room);
	void sort();
};
spvec* add(spvec* lhs, spvec* rhs);


#define HASH_SIZE 256

class hash {
	list table[HASH_SIZE];
public:
	hash();
	hash(obj v);
	~hash();
	hash& operator=(const hash& rhs);
	hash(hash& source);
	obj operator[](obj key);
	obj* reference(obj key);
	void add(obj key, obj val);
	void print();
	int size();
	obj array();
};

