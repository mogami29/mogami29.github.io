/*	LIB 2004 Tsuguo Mogami  */
#include "ciph.h"
#include "value.h"
#include "list.h"
#include <math.h>
#include "app.h"
#include "eval.h"
#include "vector.h"
#include <string.h>
#include "lib.h"


inline obj req_one(obj v){
	return v;
}

static int req_int(obj v){
	if(type(v) != INT) error("argument must be an int.");
	return uint(v);
}

static obj createDSeries(double d){
	//throw(eval_error());
	const int step=100;
	int i;
	obj v = dblArray(step);
	for(i=0; i<step; i++) set(&(udar(v)), i, d*i/step);
	return v;
}

static obj Ser(obj v){
	if(type(v) == tDouble) return createDSeries(udbl(v));
	if(type(v) == INT) return createDSeries(uint(v));
	error("ser: scalar nomi.");
	return nil;
}

static obj Rand(obj arg){
	if(type(arg)==INT) {
		int step = uint(arg);
		obj rr = aArray(step);
		for(int i=0; i<step; i++) uar(rr).v[i] = Double(((double)rand())/(1<<15));
		return rr;
	} 
	if(type(arg)==LIST){
		assert(! ul(arg));
		return Double(((double)rand())/(1<<15));
	} 
	error("rand: inappropriate argument.");
	return nil;
}

static obj ISer(obj arg){
	if(type(arg)!=INT) error("i-series, not a int");
	int step = uint(arg);
	obj rr = aArray(step);
	for(int i=0; i<step; i++) uar(rr).v[i]=Int(i);
	return rr;
}

static obj Range(obj vi){
	assert(type(vi)==LIST);
	if(type(em0(vi))!=INT) error("range: int required");
	if(type(em1(vi))!=INT) error("range: int required");
	int b = uint(em0(vi));
	int e = uint(em1(vi));
	obj v = aArray(abs(e-b));
	for(int i=0; i < abs(e-b); i++) uar(v).v[i]=Int(b+i);
	return v;
}

static obj Range3(obj vi){
	assert(type(vi)==LIST);
	if(type(em0(vi))!=INT) error("range: int required");
	if(type(em1(vi))!=INT) error("range: int required");
	int b = uint(em0(vi));
	int e = uint(em1(vi));
	obj v = aArray(abs(e-b)+1);
	for(int i=0; i < abs(e-b)+1; i++) uar(v).v[i]=Int(b+i);
	return v;
}

static obj matrix(obj vi){
	assert(type(vi)==LIST);
	obj s1,s2;
	
	s1= em0(vi);
	if(type(s1) != INT) error("matrix: argument must be int.");
	s2= em1(vi);
	if(type(s2) != INT) error("matrix: argument must be int.");
	
	int m = uint(s1);
	int n = uint(s2);
	obj mx = aArray(n);
	for(int i=0; i<m; i++) {
		obj v = dblArray(n);
		for(int j=0; j<n; j++) udar(v).v[j] = 0;
		if(m==n) udar(v).v[i] = 1;
		uar(mx).v[i] = v;
	}
	return mx;	
}

//　あたらしく型を定義した場合いじらなければならないところ。
// lib.cのなかに、なんらかの生成オペレータとprintを加える。
// value.c のなかでfree_vに破壊子を加える。できればcopyも書く。

static obj SpVec(obj vi){
	if(type(vi) != INT) error("spvec: int required");
	int n = uint(vi);
//	return tag(tSpVec, new spvec(n));
	obj rr = alloc();
	rr->type = tSpVec;
	uspv(rr) = new spvec(n);
	return rr;
}

static obj Sparse(obj v){
	// [ v, v, ...] をspvecに変換
	assert(type(v)==tDblArray);
//	return tag(tSpVec, new spvec(v));
	obj rr = alloc();
	rr->type = tSpVec;
	uspv(rr) = new spvec(v);
	return rr;
}

static obj Hash(obj v){
	// [ (k,v), (k,v), ...] をhashに変換
	obj rr=alloc();
	rr->type=tHash;
	uhash(rr) = new hash(v);
	return rr;
}

static short fileRefNum;

static obj fseek(obj vi){
	obj num= req_one(vi);
	if(type(num) != INT) error("fseek: argument must be an int.");

	SetFPos(fileRefNum, fsFromStart, uint(num));//fsFromMark
	return nil;
}

//GetFPos(fileRefNum, long * filePos);
//GetEOF(fileRefNum,  long *logEOF);

static obj readUShort(obj num){
	if(type(num) != INT) error("read: argument must be an int.");
	long bytes = uint(num)*sizeof(unsigned short);
	unsigned short* buf = (unsigned short *)malloc(bytes);
	FSRead(fileRefNum, &bytes, buf);
	obj r = vector(uint(num));
	for(int i=0; i < uint(num); i++) udar(r).v[i] = (double) buf[i];
	free(buf);
	return r;
}

static obj readChar(obj num){
	if(type(num) != INT) error("read: argument must be an int.");
	long bytes = uint(num)*sizeof(char);
	unsigned char* buf = (unsigned char *)malloc(bytes);
	FSRead(fileRefNum, &bytes, buf);
	obj r = intArray(uint(num));
	for(int i=0; i < uint(num); i++) uiar(r).v[i] = buf[i];	
	free(buf);
	return r;
}

static obj readDouble(obj num){
	if(type(num) != INT) error("read: argument must be an int.");
 	long bytes = uint(num)*sizeof(double);	
	double* buf = (double *)malloc(bytes);
	FSRead(fileRefNum, &bytes, buf);
	obj r = vector(uint(num));
	for(int i=0; i < uint(num); i++) udar(r).v[i] = buf[i];	
	free(buf);
	return r;
}

char* merge(char* s1, char* s2){
	char* s = (char *)malloc(strlen(s1)+strlen(s2)+1);
	strcpy(s, s1);
	strcpy(s+strlen(s1), s2);
	return s;
}

#define defaultDIR ":cipher:"

obj open(obj vi){
	obj fileName= req_one(vi);
	OSErr fs_er;
	
	if(type(fileName) != STRING) error("open: file name must be a string.");
	char* fn = merge(defaultDIR, ustr(fileName));
	CtoPstr(fn);
	fs_er = HOpenDF(0, fsRtDirID, (StringPtr)fn, fsRdPerm, &fileRefNum);
	if(fs_er) error("Cannot open the file.");
	free(fn);
	return nil;
}

obj close(obj v){
	FSClose(fileRefNum);
	return nil;
}

void write(char* str, long bytes, obj fileName){
	short refNum;
	
	if(type(fileName) != STRING) error("file name must be a string.");
	char* fn = merge(defaultDIR, ustr(fileName));
	CtoPstr(fn);
	OSErr fs_err = HCreate(0, fsRtDirID, (StringPtr)fn, '????', 'TEXT');  //create a file in root directory of default volume.	
	fs_err = HOpenDF(0, fsRtDirID, (StringPtr)fn, fsWrPerm, &refNum);
	free(fn);
	if(fs_err) error("Cannot open the file.");
	
	fs_err = FSWrite(refNum, &bytes, str);
	FSClose(refNum);
}

static obj Write(obj v){
	if(type(v)!=LIST) error("");
	obj fn = em0(v);
	obj str= em1(v);

	assert(str->type==STRING);	
	write(ustr(str), strlen(ustr(str))+1, fn);
	return nil;
}

char* read(long* bytes, obj fileName){	//returns bytes
	short refNum;
	
	if(type(fileName) != STRING) error("file name must be a string.");
	char* fn = merge(defaultDIR, ustr(fileName));
	CtoPstr(fn);
	OSErr fs_err = HOpenDF(0, fsRtDirID, (StringPtr)fn, fsRdPerm, &refNum);
	free(fn);
	if(fs_err) error("Cannot open the file.");
	
	fs_err = GetEOF(refNum, bytes);
	char* str = (char*)malloc(*bytes+1);
	fs_err = FSRead(refNum, bytes, str);
	FSClose(refNum);
	*(str+*bytes) = NUL;
	return str;
}

static obj read_as_string(obj vi){
	long bytes;
	char* prog = read(&bytes, vi);
	for(char* st=prog; st < prog+bytes; st++) if(*st==NUL) error("a null char in the file.");
	return val(prog);
}

obj read_lines(obj vi){
	long bytes;
	char* prog = read(&bytes, vi);
	
	list rl = phi();
	char* st = prog;
	while(st < prog+bytes){
		char* end = st;
		for(; *end!=CR; end++){
			if(*end==NUL) error("a null char in the file.");
		}
		rl = cons(cString(st, end), rl);
		st = end+1;
	}
	free(prog);
	return List2v(reverse(rl));
}

static obj Load(obj vi){
	long bytes;
	char *prog = read(&bytes, vi);
	// execute	
	char* execPtr = prog;
	while(1){
		obj rr = parseString(&execPtr);
		if(rr) rr = eval(rr);
		if(rr) {
			print(rr);
			scroll();
			release(rr);
		}
		if(*execPtr==CR) execPtr++;
		if (execPtr >= prog+bytes) break;
	}
	free(prog);
	return nil;
}

double max(DblArray* a){
	double m = a->v[0];
	assert(a->size >0);
	for(int i=1; i<a->size; i++) if(m < a->v[i]) m = a->v[i];
	return m;
}
double min(DblArray* a){
	double m = a->v[0];
	assert(a->size >0);
	for(int i=1; i<a->size; i++) if(m > a->v[i]) m=a->v[i];
	return m;
}

static obj max(obj v){
	obj m;
	switch(type(v)){
	case tDblArray:
		return Double(max(&udar(v)));
	case tArray:
		int n = size(v);
		if(n<1) error("max: at least one element.");
		m = uar(v).v[0];
		for(int i=1; i<n; i++){
			if(vrInt(cclt(m, uar(v).v[i]))) m = uar(v).v[i];
		}
		return retain(m);
	case LIST:
		list l=ul(v);
		if(!l) error("max: at least one element.");
		m = fpp(l);
		for(; l; l=rest(l)){
			if(vrInt(cclt(m, first(l)))) m = first(l);
		}
		return retain(m);
	}
	error("not defined yet.");
	return nil;
}

static obj min(obj v){
	obj m;
	switch(type(v)){
	case tDblArray:
		return Double(min(&udar(v)));
	case tArray:
		int n = size(v);
		if(n<1) error("min: at least one element.");
		m = uar(v).v[0];
		for(int i=1; i<n; i++){
			if(vrInt(ccgt(m, uar(v).v[i]))) m = uar(v).v[i];
		}
		return retain(m);
	case LIST:
		list l=ul(v);
		if(!l) error("min: at least one element.");
		m = fpp(l);
		for(; l; l=rest(l)){
			if(vrInt(ccgt(m, first(l)))) m = first(l);
		}
		return retain(m);
	}
	error("not defined yet.");
	return nil;
}

void showline(obj y){
	Point pt;
	GetPen(&pt);
	int baseLine = pt.v;
	MoveTo(10, baseLine-udar(y).v[0]);
	for(int i=1; i< udar(y).size; i++) LineTo(10+i*3, baseLine-udar(y).v[i]);
}

static obj plot(obj v){
	double u,d;
	obj ar;

	switch(v->type){
	case tDblArray:
		DblArray* a = &udar(v);
		assert(a->size > 0);
		u = max(a);
		d = min(a);
		int n = a->size;
		ar = dblArray(n);
	//	ar = new dblarr(n);
		for(int i=0; i < n; i++) udar(ar).v[i] = 200*(a->v[i] - d)/(u-d);
		break;
	case tArray:
		u = v2Double(max(v));
		d = v2Double(min(v));
		n = size(v);
		ar = dblArray(n);
		for(int i=0; i < n; i++) udar(ar).v[i] = 200*(v2Double(uar(v).v[i]) - d)/(u-d);
		break;
	default:
		error("plot: argument must be an array or a dbl array.");
	}
	scrollBy(200);
	ar->type = tLine;
	addObjToText(ar);
	showline(ar);
	scroll();
	return nil;
}

static obj dot0(obj x, obj y){
	RGBColor color;
	color.red=color.green=color.blue=0;
	Point pt;
	GetPen(&pt);
	int baseLine = pt.v;
	SetCPixel(LEFTMARGIN+100+v2Double(x), baseLine-100-v2Double(y), &color);
	return nil;
}

static obj dots(obj v){
	scrollBy(200);
/*	applyCC(dot0, em0(v), em1(v));		// release ?
	scroll();
	return nil;
*/
	obj x = em0(v);
	obj y = em1(v);
	assert(x->type==tDblArray && y->type==tDblArray);
	int n = size(x);
	for(int i=0; i<n; i++){
		obj lt = Double(udar(x).v[i]);//遅い
		obj rt = Double(udar(y).v[i]);
		obj rx = dot0(lt,rt);
		release(lt);
		release(rt);
		release(rx);
	}
	scroll();
	return nil;
}

static obj Floor(obj vi){
	obj arg=req_one(vi);
	assert(type(arg)==tDouble);
	return Int(floor(udbl(arg)));
}

static obj Abs(obj vi){
	return applyV(fabs, vi, nil);
}
static obj Sin(obj vi){
	return applyV(sin, vi, nil);
}
static obj Cos(obj vi){
	return applyV(cos, vi, nil);
}
static obj Tan(obj vi){
	obj arg= req_one(vi);
	return applyV(tan, arg, nil);
}
static obj exp(obj vi){
	static obj symexp = Symbol("exp");
	return applyV(exp, vi, symexp);
}
static obj Log(obj vi){
	obj arg= req_one(vi);
	return applyV(log, arg, nil);
}
static obj Sqrt(obj vi){
	return applyV(sqrt, vi, nil);
}
// ----string

static int strlen_multibyte(char* st){
	int len = 0;
	for(;*st; st = next(st)) len++;
	return len;
}

static char* subs(char* src, char* pat, char* sub){	//最短一致
	string rs = nullstr();
	for(;*src;){
		char*s=src;
		char*p=pat;
		for(; *s; p=next(p), s=next(s)){
			if(*p==NUL) goto yes0;
			if(*p=='?') goto match1;
			if(*p=='*') break;
			if(readchar(s) != readchar(p)) goto no;
		}
		{	// wild card *
			char*s1=s, *s2,*s3;
			p++;
			assert(*p);
			for(;*s; s=next(s)){
				char *ss=s;
				for(char*pp=p; *pp; pp++,ss++) if(*ss != *pp) goto no1;
				// match
				s2 = s;
				s3 = ss;
				goto match;
			no1:	;
			}
match:		char*sx = sub;
			for(; *sx && !(*sx=='*'); sx=next(sx)) appendS(&rs, readchar(sx));
			if(! (*sx)) goto end_m;
			for(char*s=s1; s<s2; s++) appendS(&rs, (unsigned char)*s);
			sx++;
			for(; *sx; sx++) appendS(&rs, (unsigned char)*sx);
end_m:		src = s3;
			continue;
		}
no:		appendS(&rs, readchar(src));
		src = next(src);
		continue;

yes0:		for(char*sx = sub; *sx; sx=next(sx)) {
			appendS(&rs, readchar(sx));
		}
		src = s;
		continue;

match1:	{// add here 
			char*sx = sub;
			for(; *sx && !(*sx=='?'); sx=next(sx)) appendS(&rs, readchar(sx));
			appendS(&rs, readchar(s));
			s=next(s);
			sx++;
			for(; *sx; sx++) appendS(&rs, (unsigned char)*sx);
			src = s;
			continue;
		}
	}
	return rs.s;
}

static obj subs(obj v){
	obj src = em0(v);
	obj pat = em1(v);
	obj sub = em2(v);
	if(src->type!=STRING) error("subs: arguments must be strings.");
	if(pat->type!=STRING) error("subs: arguments must be strings.");
	if(sub->type!=STRING) error("subs: arguments must be strings.");
	char* st = subs(ustr(src), ustr(pat), ustr(sub));
	return val(st);
}

static int charhex(int c){
	assert('0' < '1');
	if( '0'<=c && c<='9') return c - '0' ;
	if( 'a'<=c && c<='f') return c - 'a' +10 ;
	if( 'A'<=c && c<='F') return c - 'A' +10 ;
//RRR	assert(0);
	return 0;
}

static char* binarize_percent(char* src){
	string rs = nullstr();
	for(;*src;){
		if(*src == '%'){
			src = next(src);
			int h1 = charhex(readchar(src));
			src = next(src);
			int h2 = charhex(readchar(src));
			appendS(&rs, h1*16+h2);
		} else {
			appendS(&rs, readchar(src));
		}
		src = next(src);
	}
	return rs.s;
}

static obj binarize_percent(obj src){
//	obj src = em0(v);
	if(src->type!=STRING) error("binarize_percent: arguments must be a string.");
	char* st = binarize_percent(ustr(src));
	return val(st);
}

//-- end string ---

static obj toInt(obj v){
	if(type(v)==tDouble) return Int(udbl(v));
	if(type(v)==STRING){		// obsolete
		char* st = ustr(v);	
		int len = strlen_multibyte(st);
		obj r = intArray(len);
		for(int i=0; i<len; i++){
			uiar(r).v[i] = readchar(st);
			st = next(st);
		}
		return r;
	}
	if(isCon(type(v))) return map_obj(toInt, v);
	error("int: not defined");
	return nil;
}

static obj toChar(int c){
	obj r = alloc();
	r->type = tChar;
	uint(r) = c;
	return r;
}

static obj toArr(obj v){	//list to array
	if(type(v)==STRING){
		char* st = ustr(v);	
		int len = strlen_multibyte(st);
		obj r = aArray(len);
		for(int i=0; i<len; i++){
			uar(r).v[i] = toChar(readchar(st));
			st = next(st);
		}
		return r;
	}
	if(type(v)==tHash){
		return uhash(v)->array();
	}
	if(isCon(type(v))) return map_obj(retain, v);
	error("int: not defined");
	return nil;
}

static char* putchar(char* st, int c){
	assert(c);
	if(c >= 256){
		*(unsigned short*)st = c;
		return st+2;
	}
	*st = c;
	return st+1;
}

static obj toStr(obj v){
	if(type(v)==tIntArr){
		int len = 0;
		for(int i=0; i < uiar(v).size; i++) 
			if(uiar(v).v[i] >=256) len+=2; else len+=1;
		obj r = aString(len);
		char*s = ustr(r);
		for(int i=0; i < uiar(v).size; i++)
			s = putchar(s, uiar(v).v[i]);
		return r;
	}
	assert(0);
	return nil;
}

#include <fp.h>

double clipnan(double d){
	if(isnormal(d)) return d;
	return 0;
}

static obj clipnan(obj v){
	return applyV(clipnan, v, nil);

}
static obj isnan1(obj v){
	if(v->type==tDouble) return Int(isnan(udbl(v)));
	error("not defined");
//	applyV(isnan, v);
	return nil;
}

static obj Pow(obj vi){
	return power(em0(vi), em1(vi));
}
static obj Mod(obj vi){
	return Int(uint(em0(vi)) % uint(em1(vi)));
}
static obj image(obj vi){
	obj v = map_obj(toDblArr, vi);
	v->type=IMAGE;
	return v;
}
static obj CImg(obj vi){
	assert(type(vi)==LIST);
	obj v = aArray(3);
	uar(v).v[0] = map_obj(toDblArr, em0(vi));
	uar(v).v[1] = map_obj(toDblArr, em1(vi));
	uar(v).v[2] = map_obj(toDblArr, em2(vi));
	v->type = tCImg;
	return v;
}

static obj lineTo(obj vi){
	static int px=0,py=0;
	int x,y;
	
	x=uint(em0(vi));	
	y=uint(em1(vi));

	MoveTo(px,windowHeight-py);
	LineTo(x, windowHeight-y);
	px=x;
	py=y;
	return nil;
}

//------------intrinsic functions ---

static obj isid(obj vi){
	return Int(type(vi) ==tSymbol);
}
static obj isint(obj vi){
	return Int(type(vi) ==INT);
}
static obj islist(obj vi){
	return Int(type(vi) ==LIST);
}
static obj Type(obj vi){
	obj v=req_one(vi);
	switch(type(v)){
	case ARITH: return Symbol("add");
	case MULT:  return Symbol("mult");
	case tMinus:return Symbol("minus");
	case INT:	return Symbol("int");
	case tDouble:return Symbol("dbl");
	case tOp:	return retain(car(v));
	}
	return Int(type(v));
}
static obj Crack(obj vi){
	obj v=req_one(vi);
	if(type(v) < tLast) error("innate cannot be cracked");
	assert(type(v) > tLast);
	return retain(uref(v));
}
static obj ToDblArr(obj v){
	return toDblArr(v);
}
static obj Cons(obj vi){
	assert(type(vi)==LIST);
	obj v = em0(vi);
	obj l = em1(vi);
	if(type(l) !=LIST) error("cons: second arg must be a list");
	return List2v(cons(retain(v), retain(ul(l))));
}
static list merge0(list l){
	if(!l) return l;
	obj l1=first(l);
	if(type(l1) !=LIST) error("merge: arg must be a list");
	return merge(copy(ul(l1)), merge0(rest(l)));
	
	l = reverse(l);
	list rl=nil;
	for(; l; l=rest(l)) {
		obj l1=first(l);
		if(type(l1) !=LIST) error("merge: arg must be a list");
		rl = merge(copy(ul(l1)), rl);
	}
	reverse(l);
	return rl;
}

static obj Merge(obj vi){
	if(type(vi)==LIST) return List2v(merge0(ul(vi)));
	assert(type(vi)==tArray);
	list rl = phi();
	int len = size(vi);
	if(len==0) return nil;
	for(int i=len-1; i >= 0; i--){
		obj lt=ind(vi, i);
		assert(type(lt)==LIST);
		rl = merge(copy(ul(lt)), rl);
		release(lt);
	}
	return List2v(rl);
}

static void do_assign(obj y[], obj x){
	int i=0;
	switch(type(x)){
	case tArray:
		for(i=0; i < uar(x).size; i++) y[i] = retain(uar(x).v[i]);
		break;	
	case LIST:
		for(list l=ul(x); l; l=rest(l),i++) y[i] = retain(first(l));
	case tDblArray:
	case tIntArr:
		for(i=0; i < uiar(x).size; i++) y[i] = ind(x,i);
		break;
	default:
		assert(0);	
	}
}

static obj Flatten(obj v){
	if(isVec(type(v))){
		int len=size(v);
		int n=0;
		for(int i=0; i<len; i++){
			obj lt=ind(v,i);
			n += size(lt);
			release(lt);
		}
		obj rr = aArray(n);
		n=0;
		for(int i=0; i<len; i++){
			obj lt=ind(v,i);
			do_assign(uar(rr).v + n, lt);
			n += size(lt);
			release(lt);
		}
		return rr;
	}
	if(type(v)==LIST){
		assert(0);
		list l=phi();
		for(list l1=ul(v); l1; l1=rest(l1)){
//			l = cons(func(first(l1), v), l);
		}
		return List2v(reverse(l));
	}
	assert(0);
	return nil;

}
/*
sort(a, n){
	b = a
	u=1
	p=0
	while(u<n){
		while(p<n){
			i=0
			j=0
			k=p
			while(i<u &  j<u &  p+u+j<n){
				if(a[i]<=a[j]) {
					b[k] = a[p+i]
					k=k+1
					i=i+1
				} else {
					b[k] = a[p+u+j]
					k=k+1
					j=j+1
				}
			}
			while(i<u) b[k++] = a[p+i++]
			while(j<u & p+u+j<n) b[k++] = a[p+u+j++]
			p=p+u*2
		}
		u=u*2
		{a,b}={b,a}
	}
	return a
}
*/
static obj req_list(obj v){
	if(type(v)!=LIST) error("list required");
	return v;
}

static obj Reverse(obj l1){
	if(type(l1) !=LIST) 
		error("reverse: arg must be a list");
	list r=reverse(copy(ul(l1)));
	return List2v(r);
}
static obj Rest(obj v){
	if(type(v)!=LIST && type(v)!=MULT &&type(v)!=ARITH)
		error("rest: can't operate on this type.");
	if(! ul(v)) error("rest: empty.");
//	return render(type(v), copyList(rest(v->u.list)));
	list r = retain(rest(ul(v)));
	return render(type(v), r);
}
static obj isEmpty(obj v){
	if(type(v)!=LIST && type(v)!=MULT &&type(v)!=ARITH)
		error("isempty: not defined for this type.");
	return Int(! ul(v));
}

static obj Length1(obj v){
	return Int(size(v));
}

static obj Not(obj vi){
	obj v=req_one(vi);
	if(type(v) !=INT) error("not: arg must be a int");
	return Int(! uint(v));
}

static obj Sum(obj v){
	switch(v->type){
	case LIST:
		if(! ul(v)) return Int(0);	// type is questionable
		break;
	case tArray:
		if(size(v)==0) return Int(0);
		break;
	}
	return prod(v, add);
}

static obj Prod(obj v){
	return prod(v, mult);
}

static obj inassoc(obj vi){
	assert(type(vi)==LIST);
	obj assoc = em0(vi);
	obj key =  em1(vi);

	if(assoc->type==tAssoc){
		obj val = search_assoc(assoc, key);
		if(!val) return Int(0);
		return Int(1);
	} if(assoc->type==tHash){
		obj*val = uhash(assoc)->reference(key);
		if(!val) return Int(0);
		return Int(1);
	}
	error("inassoc: must be an associative");
	return nil;
}

static obj assoc(obj vi){
	return Assoc();	
}

static obj search(obj vi){
	assert(type(vi)==LIST);
	obj as = em0(vi);
	obj id = em1(vi);

	if(as->type!=LIST) error("search: must be a list");
	int n = find(ul(as), id);
	return Int(n);
}

static obj find(obj vi){
	if(!isvec(vi)) error("not a vector class");
	int n = size(vi);
	obj r = aArray(n);

	int p=0;
	for(int i=0; i<n; i++) 
		if(uint(uar(vi).v[i])) uar(r).v[p++] = Int(i);
	return nil;
}

static obj print2(obj v){
	print(v);
	scroll();
	return nil;
}

static obj Op(obj v){
	req_list(v);
	return op(retain(em0(v)),retain(em1(v)));
}
static obj lt(obj v){
	if(v->type!=tOp) error("lt: an operation required.");
	return retain(ult(v));
}
static obj rt(obj v){
	if(v->type!=tOp) error("rt: an operation required.");
	return retain(urt(v));
}
static obj Add(obj vi){
	obj arg = req_list(vi);
	assert(type(arg)==LIST);
	obj rr = alloc();
	rr->type = ARITH;
	ul(rr) = copy(ul(arg));
	return rr;
}

static obj Arg(obj vi){
	switch(type(vi)){
	case ARITH:
	case MULT:
		return List2v(copy(ul(vi)));
	case tOp:
		return retain(cdr(vi));
	default:
		assert(0);
	}
	return nil;
}

static obj transpose(obj v){
	if(! isCon(type(v))) error("transpose: not an array ");
	int size1= size(v);
	int size2 = size(ind(v,0));
	for(int i1=0; i1 < size1; i1++){
		obj a = ind(v,0);
		for(int i2=0; i2<size2; i2++){
		
		// not yet done
		}
	}
	return nil;
}

static obj readLine(obj v){
	return editline(String2v(""));
}
/*static obj editLine(obj v){
	return editline(v);
}*/

struct funcbind infnbind[] = {	//internal function bind
	{"readUShort",readUShort},
	{"readDouble",readDouble},
	{"readChar",readChar},
	{"write",	Write},
	{"reads",	read_as_string},
	{"readl",	read_lines},
	{"open",	open	},
	{"close",	close	},
	{"plot",	plot	},
	{"dots",	dots	},
	{"load",	Load	},
	{"lineto",	lineTo	},
	{"fseek",	fseek	},
	{"print",	print2},
	{"readline",readLine},
	{"edit",	edit},

	{"spvec",	SpVec	},
	{"sparse",	Sparse},
	{"hash",	Hash	},
	{"inv",	inv	},	// in value.c
	{"image",	image},
	{"cimg",	CImg	},
	{"imgc",	CImg	},

	{"max",	max	},
	{"min",	min	},
	{"floor",	Floor	},
	{"abs",	Abs	},
	{"sin",	Sin	},
	{"cos",	Cos	},
	{"tan",	Tan	},
	{"exp",	exp	},
	{"log",	Log	},
	{"sqr",	Sqrt	},
	{"int",	toInt	},
	{"clipnan",	clipnan},
	{"isnan",	isnan1},
	{"pow",	Pow	},
	{"mod",	Mod	},
	{"isid",	isid	},
	{"isint",	isint	},
	{"islist",	islist	},
	{"type",	Type	},
	{"crack",	Crack	},
	{"not",	Not	},

	{"darr",	ToDblArr},
	{"arr",	toArr},
	{"subs",	subs	},
	{"binarizePercent",	binarize_percent},
	{"string",	toStr	},
	{"rand",	Rand	},
	{"ser",	Ser	},
	{"#"	,	ISer	},
	{".."	,	Range	},
	{"..."	,	Range3},
	{"append",	Cons	},
	{"cons",	Cons	},
	{"merge",	Merge},
	{"flatten",	Flatten},
	{"reverse",	Reverse},
	{"rest",	Rest	},
	{"isempty",	isEmpty},
	{"empty",	isEmpty},
	{"length",	Length1},
	{"len",	Length1},
	{"inassoc",	inassoc},
	{"assoc",	assoc	},
	{"search",	search},
	{"sum",	Sum	},
	{"prod",	Prod	},
	{"map",	Map	},

	{"op"	,	Op	},
	{"lt"	,	lt	},
	{"rt"	,	rt	},
	{"add",	Add	},
	{"arg",	Arg	},
	{"",nil}
};

obj (*searchFunc (obj id, struct funcbind fnbind[]))(obj){
	obj (*func)(obj v);
	func=nil;
	for(int i=0; *(fnbind[i].fname) != NULL; i++){
		if(strcmp(ustr(id), fnbind[i].fname)==0){
			func = fnbind[i].fn;
			break;
		}
	}
	return func;
}
