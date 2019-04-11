/*
 * This file is part of DGD, https://github.com/dworkin/dgd
 * Copyright (C) 1993-2010 Dworkin B.V.
 * Copyright (C) 2010-2019 DGD Authors (see the commit log for details)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

# define INCLUDE_FILE_IO
# include "kfun.h"

/*
 * prototypes
 */
# define FUNCDEF(name, func, proto, v) extern int func(Frame*, int, struct kfunc*); extern char proto[];
# include "builtin.cpp"
# include "std.cpp"
# include "file.cpp"
# include "math.cpp"
# include "extra.cpp"
# undef FUNCDEF

/*
 * kernel function table
 */
static kfunc kforig[] = {
# define FUNCDEF(name, func, proto, v) { name, proto, func, (extfunc) NULL, v },
# include "builtin.cpp"
# include "std.cpp"
# include "file.cpp"
# include "math.cpp"
# include "extra.cpp"
# undef FUNCDEF
};

kfunc kftab[KFTAB_SIZE];	/* kfun tab */
kfunc kfenc[KFCRYPT_SIZE];	/* encryption */
kfunc kfdec[KFCRYPT_SIZE];	/* decryption */
kfunc kfhsh[KFCRYPT_SIZE];	/* hashing */
kfindex kfind[KFTAB_SIZE];	/* n -> index */
static kfindex kfx[KFTAB_SIZE];	/* index -> n */
int nkfun, ne, nd, nh;		/* # kfuns */

extern void kf_enc(Frame *, int, Value *);
extern void kf_enc_key(Frame *, int, Value *);
extern void kf_dec(Frame *, int, Value *);
extern void kf_dec_key(Frame *, int, Value *);
extern void kf_xcrypt(Frame *, int, Value *);
extern void kf_md5(Frame *, int, Value *);
extern void kf_sha1(Frame *, int, Value *);

extern Value *ext_value_temp(Dataspace*);
extern void ext_kfuns(char*, int, int);

/*
 * NAME:	kfun->clear()
 * DESCRIPTION:	clear previously added kfuns from the table
 */
void kf_clear()
{
    static char proto[] = { T_VOID, 0 };
    static extkfunc builtin[] = {
	{ "encrypt DES", proto, kf_enc },
	{ "encrypt DES key", proto, kf_enc_key },
	{ "decrypt DES", proto, kf_dec },
	{ "decrypt DES key", proto, kf_dec_key },
	{ "hash MD5", proto, kf_md5 },
	{ "hash SHA1", proto, kf_sha1 },
	{ "hash crypt", proto, kf_xcrypt }
    };

    nkfun = sizeof(kforig) / sizeof(kfunc);
    ne = nd = nh = 0;
    kf_ext_kfun(builtin, 7);
}

/*
 * NAME:	kfun->callgate()
 * DESCRIPTION:	extra kfun call gate
 */
static int kf_callgate(Frame *f, int nargs, kfunc *kf)
{
    Value val;

    val = Value::nil;
    (kf->ext)(f, nargs, &val);
    val.ref();
    i_pop(f, nargs);
    *--f->sp = val;
    if (kf->lval) {
	f->kflv = TRUE;
	PUSH_ARRVAL(f, ext_value_temp(f->data)->array);
    }

    return 0;
}

/*
 * NAME:	prototype()
 * DESCRIPTION:	construct proper prototype for new kfun
 */
static char *prototype(char *proto, bool *lval)
{
    char *p, *q;
    int nargs, vargs;
    int tclass, type;
    bool varargs;

    tclass = C_STATIC;
    type = *proto++;
    p = proto;
    nargs = vargs = 0;
    varargs = FALSE;

    /* pass 1: check prototype */
    if (*p != T_VOID) {
	while (*p != '\0') {
	    if (*p == T_VARARGS) {
		/* varargs or ellipsis */
		if (p[1] == '\0') {
		    tclass |= C_ELLIPSIS;
		    if (!varargs) {
			--nargs;
			vargs++;
		    }
		    break;
		}
		varargs = TRUE;
	    } else {
		if (*p != T_MIXED) {
		    if (*p == T_LVALUE) {
			/* lvalue arguments: turn off typechecking */
			tclass &= ~C_TYPECHECKED;
			*lval = TRUE;
		    } else {
			/* non-mixed arguments: typecheck this function */
			tclass |= C_TYPECHECKED;
		    }
		}
		if (varargs) {
		    vargs++;
		} else {
		    nargs++;
		}
	    }
	    p++;
	}
    }

    /* allocate new prototype */
    p = proto;
    q = proto = ALLOC(char, 6 + nargs + vargs);
    *q++ = tclass;
    *q++ = nargs;
    *q++ = vargs;
    *q++ = 0;
    *q++ = 6 + nargs + vargs;
    *q++ = type;

    /* pass 2: fill in new prototype */
    if (*p != T_VOID) {
	while (*p != '\0') {
	    if (*p != T_VARARGS) {
		*q++ = *p;
	    }
	    p++;
	}
    }

    return proto;
}

/*
 * NAME:	kfun->ext_kfun()
 * DESCRIPTION:	add new kfuns
 */
void kf_ext_kfun(const extkfunc *kfadd, int n)
{
    kfunc *kf;

    for (; n != 0; kfadd++, --n) {
	if (strncmp(kfadd->name, "encrypt ", 8) == 0) {
	    kf = &kfenc[ne++];
	    kf->name = kfadd->name + 8;
	} else if (strncmp(kfadd->name, "decrypt ", 8) == 0) {
	    kf = &kfdec[nd++];
	    kf->name = kfadd->name + 8;
	} else if (strncmp(kfadd->name, "hash ", 5) == 0) {
	    kf = &kfhsh[nh++];
	    kf->name = kfadd->name + 5;
	} else {
	    kf = &kftab[nkfun++];
	    kf->name = kfadd->name;
	}
	kf->lval = FALSE;
	kf->proto = prototype(kfadd->proto, &kf->lval);
	kf->func = &kf_callgate;
	kf->ext = kfadd->func;
	kf->version = 0;
    }
}

char pt_unused[] = { C_STATIC, 0, 0, 0, 6, T_MIXED };

/*
 * NAME:	kfun->unused()
 * DESCRIPTION:	unused kfun
 */
int kf_unused(Frame *f, int nargs, kfunc *kf)
{
    UNREFERENCED_PARAMETER(f);
    UNREFERENCED_PARAMETER(nargs);
    UNREFERENCED_PARAMETER(kf);

    return 1;
}

/*
 * NAME:	kfun->cmp()
 * DESCRIPTION:	compare two kftable entries
 */
static int kf_cmp(cvoid *cv1, cvoid *cv2)
{
    return strcmp(((kfunc *) cv1)->name, ((kfunc *) cv2)->name);
}

/*
 * NAME:	kfun->init()
 * DESCRIPTION:	initialize the kfun table
 */
void kf_init()
{
    int i, n;
    kfindex *k1, *k2;

    memcpy(kftab, kforig, sizeof(kforig));
    for (i = 0, k1 = kfind, k2 = kfx; i < KF_BUILTINS; i++) {
	*k1++ = i;
	*k2++ = i;
    }
    qsort((void *) (kftab + KF_BUILTINS), nkfun - KF_BUILTINS,
	  sizeof(kfunc), kf_cmp);
    qsort(kfenc, ne, sizeof(kfunc), kf_cmp);
    qsort(kfdec, nd, sizeof(kfunc), kf_cmp);
    qsort(kfhsh, nh, sizeof(kfunc), kf_cmp);
    for (n = 0; kftab[i].name[1] == '.'; n++) {
	*k2++ = '\0';
	i++;
    }
    for (k1 = kfind + 128; i < nkfun; i++) {
	*k1++ = i;
	*k2++ = i + 128 - KF_BUILTINS - n;
    }
}

/*
 * NAME:	kfun->jit()
 * DESCRIPTION:	prepare JIT compiler
 */
void kf_jit()
{
    int size, i, n, nkf;
    char *protos, *proto;

    for (size = 0, i = 0; i < KF_BUILTINS; i++) {
	size += PROTO_SIZE(kftab[i].proto);
    }
    nkf = nkfun - KF_BUILTINS;
    for (; i < nkfun; i++) {
	if (kfx[i] != 0) {
	    size += PROTO_SIZE(kftab[i].proto);
	} else {
	    --nkf;
	}
    }

    protos = ALLOCA(char, size);
    for (i = 0; i < KF_BUILTINS; i++) {
	proto = kftab[i].proto;
	memcpy(protos, proto, PROTO_SIZE(proto));
	protos += PROTO_SIZE(proto);
    }
    for (; i < nkfun; i++) {
	n = kfind[i + 128 - KF_BUILTINS];
	if (kfx[n] != 0) {
	    proto = kftab[n].proto;
	    memcpy(protos, proto, PROTO_SIZE(proto));
	    protos += PROTO_SIZE(proto);
	}
    }

    protos -= size;
    ext_kfuns(protos, size, nkf);
    AFREE(protos);
}

/*
 * NAME:	kfun->index()
 * DESCRIPTION:	search for kfun in the kfun table, return raw index or -1
 */
static int kf_index(kfunc *kf, unsigned int l, unsigned int h, const char *name)
{
    unsigned int m;
    int c;

    do {
	c = strcmp(name, kf[m = (l + h) >> 1].name);
	if (c == 0) {
	    return m;	/* found */
	} else if (c < 0) {
	    h = m;	/* search in lower half */
	} else {
	    l = m + 1;	/* search in upper half */
	}
    } while (l < h);
    /*
     * not found
     */
    return -1;
}

/*
 * NAME:	kfun->func()
 * DESCRIPTION:	search for kfun in the kfun table, return index or -1
 */
int kf_func(const char *name)
{
    int n;

    n = kf_index(kftab, KF_BUILTINS, nkfun, name);
    if (n >= 0) {
	n = kfx[n];
    }
    return n;
}

/*
 * NAME:	kfun->encrypt()
 * DESCRIPTION:	encrypt a string
 */
int kf_encrypt(Frame *f, int nargs, kfunc *func)
{
    Value val;
    int n;

    UNREFERENCED_PARAMETER(func);

    n = kf_index(kfenc, 0, ne, f->sp[nargs - 1].string->text);
    if (n < 0) {
	error("Unknown cipher");
    }
    val = Value::nil;
    (kfenc[n].ext)(f, nargs - 1, &val);
    val.ref();
    i_pop(f, nargs);
    *--f->sp = val;
    return 0;
}

/*
 * NAME:	kfun->decrypt()
 * DESCRIPTION:	decrypt a string
 */
int kf_decrypt(Frame *f, int nargs, kfunc *func)
{
    Value val;
    int n;

    UNREFERENCED_PARAMETER(func);

    n = kf_index(kfdec, 0, nd, f->sp[nargs - 1].string->text);
    if (n < 0) {
	error("Unknown cipher");
    }
    val = Value::nil;
    (kfdec[n].ext)(f, nargs - 1, &val);
    val.ref();
    i_pop(f, nargs);
    *--f->sp = val;
    return 0;
}

/*
 * NAME:	kfun->hash_string()
 * DESCRIPTION:	hash a string
 */
int kf_hash_string(Frame *f, int nargs, kfunc *func)
{
    Value val;
    int n;

    UNREFERENCED_PARAMETER(func);

    n = kf_index(kfhsh, 0, nh, f->sp[nargs - 1].string->text);
    if (n < 0) {
	error("Unknown hash algorithm");
    }
    val = Value::nil;
    (kfhsh[n].ext)(f, nargs - 1, &val);
    val.ref();
    i_pop(f, nargs);
    *--f->sp = val;
    return 0;
}

/*
 * NAME:	kfun->reclaim()
 * DESCRIPTION:	reclaim kfun space
 */
void kf_reclaim()
{
    int i, n, last;

    /* skip already-removed kfuns */
    for (last = nkfun; kfind[--last + 128 - KF_BUILTINS] == '\0'; ) ;

    /* remove duplicates at the end */
    for (i = last; i >= KF_BUILTINS; --i) {
	n = kfind[i + 128 - KF_BUILTINS];
	if (kfx[n] == i + 128 - KF_BUILTINS) {
	    if (i != last) {
		message("*** Reclaimed %d kernel function%s\012", last - i,
			((last - i > 1) ? "s" : ""));
	    }
	    break;
	}
	kfind[i + 128 - KF_BUILTINS] = '\0';
    }

    /* copy last to 0.removed_kfuns */
    for (i = KF_BUILTINS; i < nkfun && kftab[i].name[1] == '.'; i++) {
	if (kfx[i] != '\0') {
	    message("*** Preparing to reclaim unused kfun %s\012",
		    kftab[i].name);
	    n = kfind[last-- + 128 - KF_BUILTINS];
	    kfx[n] = kfx[i];
	    kfind[kfx[n]] = n;
	    kfx[i] = '\0';
	}
    }
}


struct dump_header {
    short nbuiltin;	/* # builtin kfuns */
    short nkfun;	/* # other kfuns */
    short kfnamelen;	/* length of all kfun names */
};

static char dh_layout[] = "sss";

/*
 * NAME:	kfun->dump()
 * DESCRIPTION:	dump the kfun table
 */
bool kf_dump(int fd)
{
    int i, n;
    unsigned int len, buflen;
    kfunc *kf;
    dump_header dh;
    char *buffer;
    bool flag;

    /* prepare header */
    dh.nbuiltin = KF_BUILTINS;
    dh.nkfun = nkfun - KF_BUILTINS;
    dh.kfnamelen = 0;
    for (i = KF_BUILTINS; i < nkfun; i++) {
	n = kfind[i + 128 - KF_BUILTINS];
	if (kfx[n] != '\0') {
	    dh.kfnamelen += strlen(kftab[n].name) + 1;
	    if (kftab[n].name[1] != '.') {
		dh.kfnamelen += 2;
	    }
	} else {
	    --dh.nkfun;
	}
    }

    /* write header */
    if (!Swap::write(fd, &dh, sizeof(dump_header))) {
	return FALSE;
    }

    /* write kfun names */
    buffer = ALLOCA(char, dh.kfnamelen);
    buflen = 0;
    for (i = KF_BUILTINS; i < nkfun; i++) {
	n = kfind[i + 128 - KF_BUILTINS];
	if (kfx[n] != '\0') {
	    kf = &kftab[n];
	    if (kf->name[1] != '.') {
		buffer[buflen++] = '0' + kf->version;
		buffer[buflen++] = '.';
	    }
	    len = strlen(kf->name) + 1;
	    memcpy(buffer + buflen, kf->name, len);
	    buflen += len;
	}
    }
    flag = Swap::write(fd, buffer, buflen);
    AFREE(buffer);

    return flag;
}

/*
 * NAME:	kfun->restore()
 * DESCRIPTION:	restore the kfun table
 */
void kf_restore(int fd)
{
    int i, n, buflen;
    dump_header dh;
    char *buffer;

    /* read header */
    conf_dread(fd, (char *) &dh, dh_layout, (Uint) 1);

    /* fix kfuns */
    buffer = ALLOCA(char, dh.kfnamelen);
    if (P_read(fd, buffer, (unsigned int) dh.kfnamelen) < 0) {
	fatal("cannot restore kfun names");
    }
    memset(kfx + KF_BUILTINS, '\0', (nkfun - KF_BUILTINS) * sizeof(kfindex));
    buflen = 0;
    for (i = 0; i < dh.nkfun; i++) {
	if (buffer[buflen + 1] == '.') {
	    n = kf_index(kftab, KF_BUILTINS, nkfun, buffer + buflen + 2);
	    if (n < 0 || kftab[n].version != buffer[buflen] - '0') {
		n = kf_index(kftab, KF_BUILTINS, nkfun, buffer + buflen);
		if (n < 0) {
		    error("Restored unknown kfun: %s", buffer + buflen);
		}
	    }
	} else {
	    n = kf_index(kftab, KF_BUILTINS, nkfun, buffer + buflen);
	    if (n < 0) {
		error("Restored unknown kfun: %s", buffer + buflen);
	    }
	}
	kfx[n] = i + 128;
	kfind[i + 128] = n;
	buflen += strlen(buffer + buflen) + 1;
    }
    AFREE(buffer);

    if (dh.nkfun < nkfun - KF_BUILTINS) {
	/*
	 * There are more kfuns in the current driver than in the driver
	 * which created the snapshot: deal with those new kfuns.
	 */
	n = dh.nkfun + 128;
	for (i = KF_BUILTINS; i < nkfun; i++) {
	    if (kfx[i] == '\0' && kftab[i].name[1] != '.') {
		/* new kfun */
		kfind[n] = i;
		kfx[i] = n++;
	    }
	}
    }
}
