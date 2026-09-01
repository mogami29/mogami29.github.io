/* part of cipher language, by Tsuguo Mogami  */
obj eval(obj exp);
obj Map(obj vi);
//obj udef_op(obj op, obj lt, obj rt);
//obj eval_function(obj lt, obj rt);
obj applyV(double (*func)(double), obj v, obj name);
obj prod(obj v, obj (*func)(obj, obj));
obj applySC(obj (*func)(obj, obj), obj s1, obj v2);
obj applyCS(obj (*func)(obj, obj), obj v1, obj v2);
obj applyCC( obj (*func)(obj, obj), obj v1, obj v2);

class eval_error {};
