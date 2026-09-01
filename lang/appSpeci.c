/*	AppSpeci 2002 Tsuguo Mogami  */
#include "ciph.h"
#include "value.h"
#include "list.h"
#include "app.h"
#include <string.h>
#include "lib.h"

void	drawLines(list*line, bool draw);
static int getWidth(obj string);
static void drawFormula(obj line, bool draw);
static int findPreviousLine();
static list CStringToLine(obj str);
static void serialize(string*rs, list l, list end);
static void win_normalize();
//-----
inline int_* create(ValueType t, int i){
	int_* r = (int_*)alloc();
	r->type = t;
	uint(r) = i;	
	return r;
}
class frac: public list_ {
public:
	frac();
	~frac() {}
};
typedef list text;


int viewPosition = 0;
static list lines;
//---------current line with insertion point object---------------------------

static text line;
static int startOfThisLine;	//絶対座標, multiple lineのときの始め。

static Point cursorPosition;
static Point curBase;
int baseLine = 50;// ほかに使っているのはplot() 
			// baseLineはカーソル位置の拡張とおもう。
			// line編集中はcursorPositionと同時に変更を行う。

static list 	insList=nil;		// insertion point のスタック
					// insと合わせてinsertion pointを表す
static list	beginSelList;
struct insp {
	int pos;
	list *curstr;	// current string
	list* lpos;

	inline void moveInto(list *l){
		curstr = l;
		lpos = l;
		pos = 0;	}
	inline void moveRightmost(list *l){
		curstr = l;
		pos = length(*l);	
		lpos = rest(l, pos);}
	insp(list* l, int i):curstr(l), pos(i) {lpos=rest(l,i);}
	insp():curstr(nil), pos(0){lpos=nil;}
	void setpos(int p){
		lpos= rest(curstr, p);
		pos = p; }
	list* list_point(){		// functionalize !
		return lpos;
		return rest(curstr, pos);
	}
} beginOfSel, ins;		//insは現在のinsertion point

static Point cursorBeforeVertMove;

static Point selectionCursorPosition;
static bool 	nowSelected = false;

// didBufのデータ形式の定義
// 	list of insert_string("hoge"), move_to(list ins), delete(int n)
// undoBufのデータ形式の定義
//		delete(int n), move_to(list ins), insert_string("hoge")

//いまのところundobufをつかい1-levelのundoだけ
static list didBuf = nil;
static list undoBuf = nil;
static obj undobuf = nil;
//------------draw系----------

void drawFraction(list_* f, bool draw){
	Point pt;
	GetPen(&pt);
	assert(f->type==FRACTION);
	int numerWidth = getWidth(em0(f));
	int denomWidth = getWidth(em1(f));
	int width = 2 + larger(numerWidth, denomWidth) +2;
	MoveTo(pt.h, pt.v-FONTSIZE/3);
	if(draw) Line(width,0);
	MoveTo(pt.h+width/2-numerWidth/2, pt.v-FONTSIZE*2/3);
	drawFormula(em0(f), draw);
	MoveTo(pt.h+width/2-denomWidth/2, pt.v+FONTSIZE);
	drawFormula(em1(f), draw);
	MoveTo(pt.h+width+2, pt.v);
}
void drawSuperScript(obj v, bool draw){
	Move(0,-FONTSIZE*2/3);
	TextSize(FONTSIZE*3/4);
	assert(type(v)==SuperScript);
	drawFormula(v, draw);
	TextSize(FONTSIZE);
	Move(0,FONTSIZE*2/3);
}
void drawSubScript(obj v, bool draw){
	Move(0,+FONTSIZE/3);
	TextSize(FONTSIZE*3/4);
	assert(type(v)==SubScript);
	drawFormula(v, draw);
	TextSize(FONTSIZE);
	Move(0,-FONTSIZE/3);
}
// CRは行末に付属すると考える。
Point curbase;	// curBaseはcursorのbaseline, curbaseは現在描画中のbaseline
static bool crossed;

int drawOne(list& l, int& pos, bool draw){
	Point pt;
	obj v = first(l);
	switch(type(v)){
	case INT:
		char buf[8];
		// read
		*buf=0;
		int c = uint(v);
		buf[++*buf] = c;
		if(c&0x80){
			if(! rest(l)) break;//2 byte文字が1byteずつ挿入されるから
			if(second(l)->type != INT) break;
			buf[++*buf] = uint(second(l));
		}
		int width = StringWidth((StringPtr)buf);
		if(c=='\t') width = StringWidth("\p    ");
		//draw:
		if(c=='\t') {
			DrawString("\p    ");
			break;
		}
		GetPen(&pt);
		if(!draw || pt.v < -FONTSIZE) Move(width, 0);
			else	DrawString((StringPtr)buf);
		break;
	case FRACTION:
		drawFraction((list_*)v, draw);
		break;
	case SuperScript:
		drawSuperScript(v, draw);
		break;
	case SubScript:
		drawSubScript(v, draw);
		break;
	case tShow:
		DrawString("\p▽");
		drawLines(&ul(v), draw);
		DrawString("\p▽");
		break;
	case tHide:
		DrawString("\p△");
		break;
	}
	return 0;
}


inline int getInsertionCloseTo0(list& l, int &pos, int h, int&curr_mark){
	Point pt;
	for(; ;){
		GetPen(&pt);
		if(pt.h <= h) curr_mark = pos;
		if(! l) goto endline;
		if(&line==ins.curstr && l==*ins.lpos) crossed = true;

		obj v=first(l);
		if(type(v)==INT && uint(v)==CR) {pos++, l=rest(l);goto newline;};	//newlineifneccesary
		drawOne(l, pos, false);
		if(type(v)==INT && uint(v)&0x80 && rest(l) && second(l)->type==INT){
			pos++; l=rest(l);
		}
		pos++, l=rest(l);

		GetPen(&pt);
		if(pt.h > LEFTMARGIN+colWidth) goto newline; //wrap
 	}
newline:
	return 1;
endline:
	return 0;
}

void findInsertionCloseTo(int h, int &next, int &prev){
	int pos = findPreviousLine();
	int curr_mark=0; 
	if(pos<0) pos=0;
	list l=rest(line, pos);
	next=-1;
	prev=-1;
	crossed = false;
	bool metend = false;
	MoveTo(LEFTMARGIN,0);
	for(; l;){
		if(getInsertionCloseTo0(l, pos, h, curr_mark)==1) goto newline;
			else goto endline;
newline:	Point pt;
		GetPen(&pt);
		MoveTo(LEFTMARGIN, pt.v+LINEHEIGHT);
		if(! crossed) prev = curr_mark;
endline:	if(metend) {next = curr_mark; return;}
		if(crossed) metend = true;
	}
}

insp click;
Point clickpnt;
Point curclick;

int drawFormula0(list* line, list& l, int& pos, bool draw){
	Point pt;
	int linew=0;
	char buf[256];
	for(; ; ){	// chars
		if(line==ins.curstr && l == *ins.lpos){
			GetPen(&cursorPosition);
			curBase = curbase;
			crossed=true;
		}
		if(! l) goto endline;

		obj v= first(l);
		if(type(v)==INT && uint(v)==CR) {pos++, l=rest(l);goto newline;};	//newlineifneccesary
		drawOne(l, pos, draw);
		if(type(v)==INT && uint(v)&0x80 && rest(l) && second(l)->type==INT){
			pos++; l=rest(l);
		}
		pos++, l=rest(l);

		GetPen(&pt);
		if(pt.h > 50+colWidth) goto newline;//wrap
		if(pt.v < clickpnt.v + FONTSIZE/2 && pt.h < clickpnt.h){
			click = insp(line, pos);
			curclick = pt;
		}
	}
endline:
	return 0;
newline:
	return 1;
}

void drawFormula(obj line, bool draw){
	list l=ul(line);
	int pos=0;
	drawFormula0(&ul(line), l, pos, draw);
}
void drawLines(list*line, bool draw){
	list l = *line;
	int pos = 0;
	Point pt;
	GetPen(&pt);

	int vv=pt.v;
	for(int col=0; col < nCols; col++){	// columns
		for(; ; ){				// lines by CR
			GetPen(&curbase);	// get baseline of the line
			if(drawFormula0(line, l, pos, draw)){
				vv += LINEHEIGHT;
				MoveTo(LEFTMARGIN+(colWidth+colSep)*col, vv);	
			}
			if(line==ins.curstr && l == *ins.lpos){
				GetPen(&cursorPosition);
				curBase = curbase;
				crossed=true;
			}
			if(! l) return;
			if(vv > windowHeight) break;
		}
		vv += -windowHeight + FONTSIZE;
		MoveTo(LEFTMARGIN+(colWidth+colSep)*(col+1), vv);
	}
}

int getWidth(obj str){
	Point pt, np;
	GetPen(&pt);
	drawFormula(str, false);	// wrapされるとまずい
	GetPen(&np);
	return np.h-pt.h;
}

void drawObj(obj line){		//set cursorPositino at the same time
	char str[256];
	if(type(line) ==STRING){
		strcpy(str, ustr(line));
		CtoPstr(str);
		DrawString((StringPtr)str);
		return;
	} else if(type(line)==IMAGE || type(line)==tCImg){
		print_image(line);
		return;
	} else if(type(line)==tLine){
		showline(line);
		return;
	}
	assert(line->type==LIST);
	drawFormula(line, true);
}

//------accessors of the current line -----------------

inline void set_insp(int pos){		// <-> move insertion
	release(insList);
	insList = nil;
	ins = insp(&line, pos);
}
int findPreviousLetter(){		// いずれlistを返すように
	int p=0;
	int i=0;
	for(list l=*(ins.curstr); l && i<ins.pos; l=rest(l), i++) {
		p=i;
		if(type(first(l))==INT && (uint(first(l))&0x80)) {
			l=rest(l); i++;
		}
	}
	return p;
}
int findPreviousLine(){//returns -1 in fail
	int pp=-1,p=0, curr_pos;
	if(insList) curr_pos = uint(*last(insList));
	else curr_pos = ins.pos;
	int i=0;
	for(list l=line; l && i<curr_pos; l=rest(l), i++) 
		if(first(l)->type==INT && uint(first(l))==CR) {
			pp=p;
			p=i+1;
		}
	return pp;
}
list deleteALetter0(){
	int p = findPreviousLetter();
	list* lpp = rest(ins.curstr, p);
	list l = *lpp;
	*lpp = rest(l, ins.pos-p);
	*rest(&l, ins.pos-p) = nil;
	ins.setpos(p);
	return l;
}
static void putinUndobuf(list l){
	if(!undobuf) undobuf = (obj)create(tIns, phi());
	else if(undobuf->type!=tIns){
		release(undobuf);
		undobuf = (obj)create(tIns, phi());
	}
	ul(undobuf) = merge(l, ul(undobuf));
}
static void deleteALetter(){
	list l = deleteALetter0();
	putinUndobuf(l);
}

static int peekPreviousLetter(){	// not good for 2-bytes
	if(ins.pos==0) return NUL;
	int p = findPreviousLetter();
	obj vp = first(rest(*ins.curstr, p));
	if(vp->type!=INT) return NUL;
	return uint(vp);
}
inline void insert0(obj v){
	list* inspp = ins.list_point();
	*inspp = cons(v, *inspp);
	ins.setpos(ins.pos+1);
}
void insert(obj v){
	insert0(v);
	if(!undobuf) undobuf = create(tDel, 1);
	else if(undobuf && undobuf->type==tDel){
		(uint(undobuf))++;
	} else {
		release(undobuf);
		undobuf = create(tDel, 1);
	}
}
void newLine(){
	line = phi();
	ins.moveInto(&line);
	insList = nil;

	startOfThisLine = baseLine+viewPosition;
	cursorPosition.h = LEFTMARGIN;
	cursorPosition.v = baseLine;
	MoveTo(LEFTMARGIN, baseLine);

	cursorBeforeVertMove = cursorPosition;

//	beginOfSel.pos = 0;
	beginOfSel = insp();
	beginSelList = nil;
	selectionCursorPosition = cursorPosition;
	nowSelected = false;
	
	release(didBuf);
	release(undoBuf);
	didBuf = nil;
	undoBuf = nil;
}
static void pushInsertion(){
	insList = cons(Int(ins.pos), insList);
}
static int popInsertion(){
	return vrInt(pop(&insList));
}
void moveIntoDenom(list_* fr){
	pushInsertion();
//	ins.pos = 2;
//	pushInsertion();
	insList = cons(Int(2), insList);
	ins.moveInto(&ul(em1(fr)));
}
void insertFraction(list num, list denom){
	list_* v = render(FRACTION, list2(List2v(num), List2v(denom)));
	insert(v);
	moveIntoDenom(v);
}
list* curr_str(list l){
	if(! l) return &line;
	obj v = first(rest(*curr_str(rest(l)), uint(first(l))-1));
	assert(v->type==FRACTION || v->type==SubScript || v->type==SuperScript || v->type==LIST);
	return &ul(v);
}
list ins_list(list*scan, list*cstr){	// finding insList from curstr
	if(scan == cstr) return nil;
	list l = *scan;
	for(int i=0; l; l=rest(l), i++) {
		obj v=first(l);
		if(v->type==INT) continue;
		if( &ul(v)==cstr) return list1(Int(i+1));
		else if (list ll = ins_list(&ul(v), cstr)) return merge(ll, list1(Int(i+1)));
	}
	return nil;
}
/*list* upper_str(list*scan){
	if(scan == ins.curstr) assert(0);
	List l = *scan;
	for(; l; l=rest(l)) {
		obj v=first(l);
		if(v->type==INT) continue;
		if( &ul(v)==ins.curstr) return scan;
		if(list*ll = upper_str(&ul(v))) return ll;
	}
	return nil;
}*/

//inline obj snthOf(list l, int i){return first(rest(l, i));};

static list isInFracRecur(){
	list l = insList;
	if(! l) return nil;
	for(;; l=rest(l)){
		if(! rest(l)) return nil;
		obj v = first(rest(*curr_str(rest(rest(l))), uint(second(l))-1));
		if(v->type==FRACTION) return l;
	}
}
static bool isInFrac(){
	if(! insList) return false;
	obj v = first(rest(*curr_str(rest((insList))), uint(first(insList))-1));
	if(type(v)!=FRACTION) return false;
	return true;
}
void moveToUpperLevel(){
	int pos = popInsertion();
	if(isInFrac()) pos = popInsertion();
	ins = insp(curr_str(insList), pos);
}
int getNLine(list l){//line数-1,CRの数を数える
	int i=0;
	for(; l; l=rest(l)) if(first(l)->type==INT && uint(first(l))==CR) i++;
	return i;
}
void insertSuperScriptAndMoveInto(){
	obj vp = render(SuperScript, nil);
	insert(vp);
	pushInsertion();		//insertion pointはみぎで待っていてもらうことにする。
	ins.moveInto(&ul(vp));
}

void insertSubScriptAndMoveInto(){
	obj vp = render(SubScript, phi());
	insert(vp);
	pushInsertion();
	ins.moveInto(&ul(vp));
}
// -------------- controllers ------------
void moveLeft(){
	if(insList && ins.pos==0) {
		moveToUpperLevel();
		ins.setpos(ins.pos-1);
		return;
	}
	if(ins.pos==0) return;
	obj c = first(rest(*(ins.curstr), ins.pos-1));
	if(c->type==SuperScript || c->type==SubScript || c->type==tShow){
		pushInsertion();
		ins.moveRightmost(&ul(c));
		return;
	} if(c->type==FRACTION){
		moveIntoDenom((list_*)c);
		ins.setpos(length(*ins.curstr));
		return;
	}
	ins.setpos(findPreviousLetter());
}
void moveRight(){
	if(insList && ins.pos==length(*ins.curstr) ) {
		moveToUpperLevel();
		return;
	}
	if(ins.pos >= length(*ins.curstr)) return;
	obj c = first(*ins.list_point());
	if(c->type==SuperScript || c->type==SubScript || c->type==tShow){
		ins.setpos(ins.pos+1);
		pushInsertion();
		ins.moveInto(&ul(c));
		return;
	} if(c->type==FRACTION){
		ins.setpos(ins.pos+1);
		moveIntoDenom((list_*)c);
		return;
	}
	if(type(c)==INT && (uint(c)&0x80)) ins.setpos(ins.pos+1);
	if(type(c)==INT && uint(c)==CR) scrollBy(+LINEHEIGHT);
	ins.setpos(ins.pos+1);
}
list cutSelected(){
	if(beginSelList != insList){
		SysBeep(1);
		return nil;
	}
	int b = smaller(beginOfSel.pos, ins.pos);
	int e = larger(beginOfSel.pos, ins.pos);
	if(b==e) return nil;

	ins.setpos(b);
/*	list*lp = rest(ins.curstr, b);
	for(int i=0; i<e-b; i++) release(pop(lp));
	return;	*/
	list*bp = rest(ins.curstr, b);
	list*ep = rest(ins.curstr, e);
	list l= *bp;
	*bp = *ep;
	*ep = nil;
	return l;
}
void moveUp(){
	if(list l = isInFracRecur()) {
		 if(uint(first(l))==2){	// in donominator?
			l = rest(l);
			retain(l);
			release(insList);
			l = cons(Int(1), l);	// moveToNumerator
			insList = l;
			ins.moveRightmost(curr_str(l));
			return;
		}
	}
	int nx,pv;
	findInsertionCloseTo(cursorBeforeVertMove.h, nx, pv);
	if(pv == -1) return;
	set_insp(pv);
	baseLine += -LINEHEIGHT;
}
void moveDown(){
	if(list l = isInFracRecur()) {
		 if(uint(first(l))==1){	// in numerator?
			l = rest(l);
			retain(l);
			release(insList);
			l = cons(Int(2), l);	// moveToDenominator
			insList = l;
			ins.moveRightmost(curr_str(l));
			return;
		}
	}
	int nx,pv;
	findInsertionCloseTo(cursorBeforeVertMove.h, nx, pv);
	if(nx==-1) return;
	set_insp(nx);
	baseLine += +LINEHEIGHT;
}

//--------------------
static unsigned long caretLastChanged;
static int caretState;	//==0 if hidden, ==1 if shown

// Caretは qd.ptを見ているのがまずい

void ShowCaret(){
	MoveTo(cursorPosition.h, cursorPosition.v);
	Line(0,-FONTSIZE);
	Move(0, FONTSIZE);
	caretLastChanged = TickCount();
	caretState = 1;
}

void HideCaret(){
	MoveTo(cursorPosition.h, cursorPosition.v);
	PenPat(&qd.white);
	Line(0,-FONTSIZE);
	Move(0, FONTSIZE);
	PenPat(&qd.black);
	caretLastChanged = TickCount();
	caretState = 0;
}

void updateCaret(){
	if(TickCount()-caretLastChanged >= GetCaretTime() ){
		if(caretState == 0) ShowCaret();
		else HideCaret();
	}
}

#define upboundby(b,x) ((x)<(b)?(x):(b))
extern WindowPtr	currWindow;

void win_normalize(){
	if(!baseLine > windowHeight-FONTSIZE && !baseLine < FONTSIZE) return;
	RgnHandle updateRgn = NewRgn();
	Rect rect = currWindow->portRect;
	while(baseLine > windowHeight-FONTSIZE) {
		int move= upboundby(FONTSIZE, baseLine -(windowHeight-FONTSIZE));
		baseLine -= move;
		viewPosition += move;
		ScrollRect(&rect, 0, -move, updateRgn);
	}
	while(baseLine < FONTSIZE) {
		int move= -upboundby(FONTSIZE, -baseLine +FONTSIZE);
		baseLine -= move;
		viewPosition += move;
		ScrollRect(&rect, 0, -move, updateRgn);
	}
	DisposeRgn(updateRgn);
	MoveTo(LEFTMARGIN,baseLine);
}

void scrollBy(int pixels){
	baseLine += pixels;
	win_normalize();
}

void scroll(){
	scrollBy(FONTSIZE);
}

//-----------------
extern Interpreter	interpreter;

static list csparse(char* str, int len);

void initLines(){
	lines = phi();
//	ins.curstr = &line;
	ins = insp(&line, 0);
}

void addObjToText(obj line){	//taking line
	list aLine = list2(line, Int(baseLine+viewPosition));
	append(&lines, List2v(aLine));
}

static void addLineToText(obj line){	//taking line
	list aLine = list2(line, Int(startOfThisLine));
	append(&lines, List2v(aLine));
}

void addStringToText(char* string){
	Point pt;
	GetPen(&pt);
	list aLine = list3(String2v(string), Int(viewPosition+baseLine), Int(pt.h));
	append(&lines, List2v(aLine));
}

#include <stdarg.h>
#include <stdio.h>

void myPrintf(char *fmt,...){
	va_list	ap;
	char str[256];
	Point pt;
	GetPen(&pt);
	if(pt.h > LEFTMARGIN+colWidth) return;
//	if(pt.h > LEFTMARGIN+colWidth) scroll();
	va_start(ap,fmt);
	if (fmt) {
		vsprintf(str,fmt,ap);
	}
	va_end(ap);
	if(strlen(str)>255) assert_func("app.c", __LINE__);
	addStringToText(str);
	for(char* s=str; *s; s++) if(*s=='\n') *s=' ';
	CtoPstr(str);
	DrawString((StringPtr)str);
}

void print_str(char*s){
	char str[256];
	Point pt;
	GetPen(&pt);
	if(pt.h > LEFTMARGIN+colWidth) return;
//	addStringToText(str);
//	for(char* s=str; *s; s++) if(*s=='\n') *s=' ';
	int p=0;
	for(; s[p] && p<250; p++) str[p] = s[p];
	s[p] = NUL;
	CtoPstr(str);
	DrawString((StringPtr)str);
}

int imbalanced(list line){
	int paren=0,brace=0;
	for(list l=line; l; l=rest(l)){
		switch(first(l)->type){
		case INT:
			char c=uint(first(l));
			if(c=='(') paren++;
			if(c==')') paren--;
			if(c=='{') brace++;
			if(c=='}') brace--;
			break;
		case FRACTION:
		case SuperScript:
		case SubScript:
			break;
		}
	}
	return abs(paren)+abs(brace);
}
void updateAround(bool erase){
	Rect r;
	r.left = LEFTMARGIN;
	r.right= LEFTMARGIN+colWidth+50;
	r.top 	= baseLine-FONTSIZE*2;
	r.bottom = baseLine+FONTSIZE;
//	r.top 	= cursorPosition.v-FONTSIZE;
//	r.bottom = cursorPosition.v;
	if(erase){
		r.top 	= baseLine-FONTSIZE*2;
		r.bottom = windowHeight;
	}	
	if(erase) EraseRect(&r);
	InvalRect(&r);
	DoUpdate(currWindow);
}
void HandleTyping0(char c){
	HideCaret();
	if(c==CR){
		if(! insList){
			insert(Int(CR));
			baseLine+=LINEHEIGHT;
		} else moveToUpperLevel();
		goto sho;
	} else if(c==BS){
		if(nowSelected) putinUndobuf(cutSelected());
		else deleteALetter();
		goto sho;
	} else if(c==arrowLeft){
		moveLeft();
		goto sho;
	} else if(c==arrowRight){
		moveRight();
		goto sho;
	} else if(c==arrowUp){
		moveUp();
		win_normalize();
		goto sho;
	} else if(c==arrowDown){
		moveDown();
		win_normalize();
		goto sho;
	}

	static int halfchar = 0;
	if(c=='~' && ! halfchar){
		insertSuperScriptAndMoveInto();
	} else if(c=='_' && ! halfchar){
		insertSubScriptAndMoveInto();
	} else if(c=='/' && peekPreviousLetter() =='/') {
		deleteALetter();	// delete '/'
		list l = deleteALetter0();
		insertFraction(l, nil);
	} else {
		if(nowSelected) putinUndobuf(cutSelected());
//		assert(c | 0xff == -1);
		insert(Int(c & 0xff));
		if(c & 0x80) halfchar = c;
		else halfchar = 0;
	}
	
sho:	if(c==arrowLeft||c==arrowRight||c==arrowUp||c==arrowDown){
		if(type(undobuf) !=tMove){
			undoBuf = cons(undobuf, undoBuf);
			undobuf = create(tMove, cons(Int(ins.pos), retain(insList)));
		}
	}
	updateAround(!(c==arrowLeft||c==arrowRight||c==arrowUp||c==arrowDown));
	baseLine = curBase.v;
	ShowCaret();
	
	if(!(c==arrowUp||c==arrowDown)) cursorBeforeVertMove = cursorPosition;		// keep position for short line
	beginOfSel = ins;	//for text selection
	release(beginSelList);
	beginSelList = retain(insList);
	selectionCursorPosition = cursorPosition;
	nowSelected = false;
}
void handleCR(){
	addLineToText(List2v(line));
	baseLine = startOfThisLine-viewPosition+FONTSIZE*(2+getNLine(line));//dame
	scrollBy(0);	//改行します。
	interpret(interpreter, line);
	scrollBy(FONTSIZE*2);
	newLine();
}
void HandleTyping(char c){
	if(c==CR && !insList && !imbalanced(line)){
		HideCaret();
		handleCR();
		ShowCaret();
		return;
	} else HandleTyping0(c);
}
void HandleShifted(char c){
	if(!nowSelected && beginSelList != insList) return;
	HideCaret();
	if(c==arrowLeft){
		moveLeft();
	} else if(c==arrowRight){
		moveRight();
	} else if(c==arrowUp){
		moveUp();
		win_normalize();
	} else if(c==arrowDown){
		moveDown();
		win_normalize();
	}
	updateAround(true);
	MoveTo(cursorPosition.h, cursorPosition.v);

	UInt8 curMode = LMGetHiliteMode();
	LMSetHiliteMode(curMode & 0x7f);
	Rect r;
	r.left =	smaller(selectionCursorPosition.h, cursorPosition.h);
	r.right=	larger(selectionCursorPosition.h, cursorPosition.h);
	r.top =	smaller(selectionCursorPosition.v, cursorPosition.v)-FONTSIZE;
	r.bottom=	larger(selectionCursorPosition.v, cursorPosition.v);
	InvertRect(&r);
	LMSetHiliteMode(curMode | 0x80);
//	ShowCaret();
	nowSelected = true;
}
void getClickPosition(Point pt){
	clickpnt = pt;
	MoveTo(LEFTMARGIN, startOfThisLine-viewPosition);
	click = insp(nil, 0);
	drawLines(&line, false);
	if(!click.curstr) return;
	ins = click;
	release(insList);
	insList = ins_list(&line, click.curstr);
	cursorPosition = curclick;
}
void HandleContentClick(Point pt){
	getClickPosition(pt);
}
void DoUpdate(WindowPtr targetWindow) {
	SetPortWindowPort(targetWindow);
	BeginUpdate(targetWindow);
//	EraseRect(&targetWindow->portRect);
	Redraw();
//	DrawGrowIcon(targetWindow);	//サイズボックスを描く
//	DrawControls(targetWindow);	//コントロール、つまりスクロールバーを描く
	EndUpdate(targetWindow);
}
void DoUndo(){
	obj doit=retain(undobuf);
	if(type(doit)==tDel){
		int n = uint(doit);
		for(int i=0; i<n; i++) deleteALetter();
	} else if(type(doit)==tIns){
		for(list l=ul(doit); l; l=rest(l)) insert(retain(first(l)));
	} else if(type(doit)==tMove) {
		release(insList);
		insList = rest(ul(doit));
		ins = insp(curr_str(insList), uint(em0(doit)));	// fuan
	}
	release(doit);
	updateAround(true);
}
void DoCopy(){
	if(beginSelList != insList) {
		SysBeep(1);
		return;
	}
	int b = smaller(beginOfSel.pos, ins.pos);
	int e = larger(beginOfSel.pos, ins.pos);
	if(b==e) return;

	string rs = nullstr();
	serialize(&rs, rest(*ins.curstr,b), rest(*ins.curstr,e));
	ZeroScrap();
	PutScrap(strlen(rs.s), 'TEXT', rs.s);
	freestr(&rs);
}

void DoHide(){
	insert(create(tShow, cutSelected()));
	updateAround(true);
	// next, make switch possible, then make it Hide not Show
}

void DoCut(){
	DoCopy();
	putinUndobuf(cutSelected());
	updateAround(true);
	MoveTo(cursorPosition.h, cursorPosition.v);
}

void DoPaste(){
	Handle	dataBlock;
	long		offset, dataSize;

	if((dataSize = GetScrap(0, 'TEXT', &offset)) > 0) {	//TEXTの存在をチェック
		dataBlock = NewHandle(dataSize);
		dataSize = GetScrap(dataBlock, 'TEXT', &offset);

		list tt = csparse((char*)*dataBlock, dataSize);
		for(list l=tt; l; l=rest(l)) insert(first(l));
		updateAround(true);
		MoveTo(cursorPosition.h, cursorPosition.v);
		DisposeHandle(dataBlock);	//これでいいのかな？
	}
	// not yet ?
}

void DoOpen(){
	static obj fn = String2v("j");
	long bytes;
	obj rr = val(read(&bytes, (fn)));
	newLine();
	line = CStringToLine(rr);
	MoveTo(LEFTMARGIN, startOfThisLine-viewPosition);	
	drawLines(&line, true);
	MoveTo(cursorPosition.h, cursorPosition.v);
}

void DoSave(){
	static obj fn = String2v("j");
	obj st = listToCString(line);
	write(ustr(st), strlen(ustr(st))+1, fn);
	release(st);
}

void DoLatex(){
	assert(0);
}

static int CRmode = 0;

obj edit(obj fn){	// open edit save
	long bytes;
	obj rr = val(read(&bytes, (fn)));
	newLine();
	line = CStringToLine(rr);
	MoveTo(LEFTMARGIN, startOfThisLine-viewPosition);	
	drawLines(&line, true);
	MoveTo(cursorPosition.h, cursorPosition.v);

	while(! getKey(shiftCR)) ;
	addLineToText(List2v(line));

	obj st = listToCString(line);
	write(ustr(st), strlen(ustr(st))+1, fn);
	release(st);
	return nil;
}

obj editline(obj v){
	newLine();
	line = CStringToLine(v);
	MoveTo(LEFTMARGIN, startOfThisLine-viewPosition);	
	drawLines(&line, true);
	MoveTo(cursorPosition.h, cursorPosition.v);
	while(! getKey(onlyCR)) ;
	addLineToText(List2v(line));
	baseLine = startOfThisLine-viewPosition;
	scrollBy(FONTSIZE*2+getNLine(line)*FONTSIZE);	//改行します。
	obj lin = listToCString(line);
	newLine();
	return lin;
}

static void append(string*rs, char*s){
	for(; *s; s++) appendS(rs, *s);
}

static void serialize(string*rs, list l, list end){
	for(; l != end; l=rest(l)){
		obj v = first(l);
		switch(type(v)){
		case INT:
			appendS(rs, uint(v));
			break;
		case SuperScript:
			appendS(rs, '^');
			appendS(rs, '{');
			serialize(rs, ul(v), nil);
			appendS(rs, '}');
			break;
		case SubScript:
			appendS(rs, '_');
			appendS(rs, '{');
			serialize(rs, ul(v), nil);
			appendS(rs, '}');
			break;
		case FRACTION:
			append(rs, "//{");
			serialize(rs, ul(first(ul(v))), nil);
			append(rs, "}{");
			serialize(rs, ul(second(ul(v))), nil);
			append(rs, "}");
			break;
		default:
			assert(0);
		}
	}
}

obj listToCString(list l){
	string rs = nullstr();
	serialize(&rs, l, nil);
	return val(rs.s);
}

static char* clp;	//
static char* clpe;	// end
list csparse0();

list putchar(int c, list l){
	if((c&0xff00)==0) {
		l = cons(Int(c), l);
	} else {
		l = cons(Int(c >> 8), l);
		l = cons(Int(c & 0xff), l);
	}
	return l;
}
list csparen(){
	if(*clp != '{') {
		list l=putchar(readchar(clp), nil);
		clp = next(clp);
		return l;
	}
	clp++;
	list l = csparse0();
	if(*clp != '}') ;//assert(0);
	else clp++;
	return l;
}
bool getchar(char**pp, int c){
	if(readchar(*pp)!=c) return false;
	*pp = next(*pp);
	return true;
}
list csparse0(){
	list l = nil;
	int bracelevel = 0;
	for(; *clp && clp!=clpe; ){
//		if(readchar(clp) == '}') break;
		if(getchar(&clp, '^')){
			assert(*clp);
			if(readchar(clp) != '{') continue;
			else l = cons(render(SuperScript, csparen()), l);
		} else if(getchar(&clp, '_')){
			assert(*clp);
			if(readchar(clp) != '{') continue;
			else l = cons(render(SubScript, csparen()), l);
		} else if(get_pat((unsigned char**)&clp, "//")){
			if(readchar(clp) != '{') continue;
			list nu = csparen();
			list de = csparen();
			l = cons(render(FRACTION, list2(List2v(nu), List2v(de))) ,l);
		} else {
			int c = readchar(clp);
			if( c =='{' ) bracelevel++;
			if( c =='}' ) {bracelevel--; if(bracelevel < 0) break;}
			l = putchar(c, l);
			clp = next(clp);
		}
	}
	return reverse(l);
}
list csparse(char* str, int len){
	clp = str;
	clpe = clp+len;
	return csparse0();
}
list CStringToLine(obj str){
	assert(str->type==STRING);
	return csparse(ustr(str), strlen(ustr(str)));
}
void Redraw(){
	for(list l=lines; l; l=rest(l)){
		assert(type(first(l))==LIST);
		list aLine = ul(first(l));
		int position = uint(second(aLine));
	//	obj hpos = snthOf(aLine,2);
	//	if(hpos!=nil) h = uint(hpos); else h=LEFTMARGIN; 
		int h;
		if(rest(rest(aLine))) h = uint(third(aLine)); else h = LEFTMARGIN;;
		MoveTo(h, position-viewPosition);
		drawObj(first(aLine));
	}
	MoveTo(LEFTMARGIN, startOfThisLine-viewPosition);
	drawLines(&line, true);
}



//the lowest 2 bits
// 00: others
// 01: direct integer
// 10: not used
// 11: a character
#define dVal	3	// mask
#define idInt	1
#define idChar	3
//inline obj dInt(int i){return (obj)((i<<2)+1);}
//inline int rInt(obj v){return (int)v>>2;}
