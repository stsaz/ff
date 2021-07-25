/** ffbase: conf2.h tester
2020, Simon Zolin
*/

#include <FF/string.h>
#include <FF/data/conf2.h>
#include <FF/data/conf2-scheme.h>
#include <FF/data/conf2-writer.h>
#include <FF/data/conf-copy.h>
#include <FFOS/file.h>
#include <test/test.h>
#include "all.h"

void test_conf2_r()
{
	ffvec data = {};
	xieq(0, fffile_readwhole(TESTDATADIR "/test.conf", &data, -1));
	ffstr d = FFSTR_INITSTR(&data);

	ffconf c;
	ffconf_init(&c);

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "key#0");

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "key0");

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "key1");
	xieq(FFCONF_RVAL, ffconf_parse(&c, &d));
	xseq(&c.val, "my str");

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "key2\n");
	xieq(FFCONF_RVAL, ffconf_parse(&c, &d));
	xseq(&c.val, "my str ing");

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "obj");
	xieq(FFCONF_ROBJ_OPEN, ffconf_parse(&c, &d));
	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "key3");
	xieq(FFCONF_RVAL, ffconf_parse(&c, &d));
	xseq(&c.val, "-1234");
	xieq(FFCONF_ROBJ_CLOSE, ffconf_parse(&c, &d));

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "obj 1");
	xieq(FFCONF_RVAL, ffconf_parse(&c, &d));
	xseq(&c.val, "value1");
	xieq(FFCONF_ROBJ_OPEN, ffconf_parse(&c, &d));
	xieq(FFCONF_ROBJ_CLOSE, ffconf_parse(&c, &d));

	xieq(FFCONF_RKEY, ffconf_parse(&c, &d));
	xseq(&c.val, "list");
	xieq(FFCONF_RVAL, ffconf_parse(&c, &d));
	xseq(&c.val, "11");
	xieq(FFCONF_RVAL_NEXT, ffconf_parse(&c, &d));
	xseq(&c.val, "22");
	xieq(FFCONF_RVAL_NEXT, ffconf_parse(&c, &d));
	xseq(&c.val, "33");

	xieq(0, ffconf_fin(&c));

	ffvec_free(&data);
}

typedef struct cstruct cstruct;
struct cstruct {
	ffstr s;
	char *sz;
	ffint64 n;
	ffint64 size;
	ffbyte b;
	ffstr list[2];
	int ilist;

	cstruct *obj;
};

static const ffconf_arg cargs[];

static int cs_obj(ffconf_scheme *cs, cstruct *o)
{
	o->obj = ffmem_new(cstruct);
	ffconf_scheme_addctx(cs, cargs, o->obj);
	return 0;
}

static int cs_list(ffconf_scheme *cs, cstruct *o, ffstr *val)
{
	o->list[o->ilist++] = *val;
	return 0;
}

#define OFF(m)  FF_OFF(cstruct, m)
static const ffconf_arg cargs[] = {
	{ "str",	FFCONF_TSTR,	OFF(s) },
	{ "sz",	FFCONF_TSTRZ,	OFF(sz) },
	{ "int",	FFCONF_TINT64,	OFF(n) },
	{ "size",	FFCONF_TSIZE64,	OFF(size) },
	{ "bool",	FFCONF_TBOOL8,	OFF(b) },
	{ "list",	FFCONF_TSTR | FFCONF_FLIST,	(ffsize)cs_list },

	{ "obj",	FFCONF_TOBJ,	(ffsize)cs_obj },
	{},
};
#undef OFF

void test_conf2_scheme()
{
	ffstr err = {};
	ffvec d = {};
	x(0 == fffile_readwhole(TESTDATADIR "/object.conf", &d, -1));
	ffstr s = FFSTR_INITSTR(&d);

	cstruct o = {};
	int r = ffconf_parse_object(cargs, &o, &s, 0, &err);
	if (r != 0)
		fprintf(stderr, "%d %.*s\n", r, (int)err.len, err.ptr);
	ffstr_free(&err);
	xieq(0, r);
	xseq(&o.s, "string");
	x(ffsz_eq(o.sz, "stringz"));
	x(o.n == 1234);
	x(o.size == 1234*1024);
	x(o.b == 1);
	xseq(&o.list[0], "val1");
	xseq(&o.list[1], "val2");
	ffstr_free(&o.s);
	ffmem_free(o.sz);

	xseq(&o.obj->s, "objstring");
	x(o.obj->n == 1234);
	x(o.obj->b == 1);
	x(o.obj->obj != NULL);
	ffstr_free(&o.obj->s);
	ffmem_free(o.obj->obj);
	ffmem_free(o.obj);

	ffvec_free(&d);
}

#define RES_STR \
"k 1234567890 val\n\
\"k 2\" \"v 2\"\n\
kctx {\n\
k3 v3\n\
}\
\n"

void test_conf2_write()
{
	ffconfw cw;
	ffconfw_init(&cw, 0);
	// cw.flags |= FFCONF_PRETTY;

	ffconfw_addkeyz(&cw, "k");
	ffconfw_addint(&cw, 1234567890);
	ffconfw_addstrz(&cw, "val");

	ffconfw_addpairz(&cw, "k 2", "v 2");
	ffconfw_addkeyz(&cw, "kctx");
	ffconfw_addobj(&cw, 1);
	{
		ffconfw_addpairz(&cw, "k3", "v3");
	}
	ffconfw_addobj(&cw, 0);
	ffconfw_fin(&cw);

	ffstr s;
	ffconfw_output(&cw, &s);
	if (!ffstr_eqz(&s, RES_STR)) {
		fffile_write(ffstdout, s.ptr, s.len);
		x(ffstr_eqz(&s, RES_STR));
	}
	ffconfw_close(&cw);
}


struct def_s {
	ffconf_ctxcopy ctx;
	ffvec data;
	uint done;
	uint active;
};
static struct def_s defer;

static int deferred_key(ffconf_scheme *cs, void *obj)
{
	ffconf_ctxcopy_init(&defer.ctx);
	static const ffconf_arg dummy = {};
	ffconf_scheme_addctx(cs, &dummy, NULL);
	defer.active = 1;
	return 0;
}
static int deferred_key2(ffconf_scheme *cs, void *obj, const ffstr *val)
{
	x(ffstr_eqz(val, "done"));
	defer.done = 1;
	return 0;
}
static const ffconf_arg deferred_args[] = {
	{ "key",	FFCONF_TOBJ, (ffsize)deferred_key },
	{ "k",	FFCONF_TSTR, (ffsize)deferred_key2 },
	{}
};

#define DEFERRED_DATA "\
key1 val1\n\
key1 val1 val2\n\
key2 arg2 {\n\
key3 val3\n\
}\n"

static void test_conf2_deferred()
{
	ffvec a = {};
	x(0 == fffile_readwhole(TESTDATADIR "/deferred.conf", &a, -1));

	int r;

	ffconf conf;
	ffconf_init(&conf);

	ffconf_scheme cs;
	ffconf_scheme_init(&cs, &conf);
	ffconf_scheme_addctx(&cs, deferred_args, NULL);

	ffstr data = *(ffstr*)&a;
	while (data.len != 0) {

		r = ffconf_parse(&conf, &data);
		if (r < 0) {
			x(0);
			return;
		}

		if (defer.active) {
			int r2 = ffconf_ctx_copy(&defer.ctx, conf.val, r);
			x(r2 >= 0);
			if (r2 == 0)
				continue;
			if (r2 > 0) {
				ffstr d = ffconf_ctxcopy_acquire(&defer.ctx);
				ffvec_set(&defer.data, d.ptr, d.len);
				defer.data.cap = d.len;
				defer.active = 0;
			}
		}

		r = ffconf_scheme_process(&cs, r);
		if (r < 0) {
			x(0);
			return;
		}
	}

	r = ffconf_fin(&conf);
	if (r < 0)
		x(0);
	ffvec_free(&a);

	x(defer.done);

	x(ffstr_eqz((ffstr*)&defer.data, DEFERRED_DATA));
	ffvec_free(&defer.data);
	ffconf_ctxcopy_destroy(&defer.ctx);
	ffconf_scheme_destroy(&cs);
}


void test_conf2()
{
	test_conf2_r();
	test_conf2_scheme();
	test_conf2_write();
	test_conf2_deferred();
}
