typedef struct {
	int dummy;
} PyObject_t;

typedef struct {
	int dummy;
} V8Object_t;

typedef struct {
	int dummy;
} V8Array_t;

typedef struct {
	int dummy;
} V8Script_t;

typedef const char * string_t;

provider wrapper {
	probe js__object__getattr(V8Object_t *obj, string_t name);
	probe js__object__setattr(V8Object_t *obj, string_t name, PyObject_t *o);
	probe js__object__delattr(V8Object_t *obj, string_t name);

	probe js__array__getitem(V8Array_t *obj, PyObject_t *key);
	probe js__array__setitem(V8Array_t *obj, PyObject_t *key, PyObject_t *value);
	probe js__array__delitem(V8Array_t *obj, PyObject_t *key);
};

provider engine {
	probe script__compile(V8Script_t *s, string_t source, string_t name, int line, int column) : (V8Script_t *s);
	probe script__run(V8Script_t *s);
};
