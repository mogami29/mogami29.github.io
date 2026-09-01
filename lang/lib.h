/* part of cipher language, by Tsuguo Mogami  */
char* read(long* bytes, obj fileName);	//returns bytes
void write(char* str, long bytes, obj fileName);
void showline(obj y);

struct funcbind {
	char* fname;
	obj (*fn)(obj v);
};
obj (*searchFunc (obj id, struct funcbind fnbind[]))(obj);
extern struct funcbind infnbind[];
