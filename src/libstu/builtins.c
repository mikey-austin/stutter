/*
 * Copyright (c) 2016, 2017 Mikey Austin <mikey@jackiemclean.net>
 * Copyright (c) 2017 Raphael Sousa Santos <contact@raphaelss.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "env.h"
#include "gc.h"
#include "native_func.h"
#include "stu_private.h"
#include "sv.h"
#include "svlist.h"
#include "types.h"

typedef enum {
  INTEGER,
  RATIONAL,
  REAL
} value_type;

#define DEF(name, func, nargs, rest) Sv_native_func_register((stu), (name), (func), (nargs), (rest))
#define DEFAULT SV_NATIVE_FUNC_DEFAULT
#define REST SV_NATIVE_FUNC_REST
#define PURE SV_NATIVE_FUNC_PURE
extern void
Builtin_init(Stu *stu)
{
    DEF("+", Builtin_add, 1, REST | PURE);
    DEF("-", Builtin_sub, 1, REST | PURE);
    DEF("*", Builtin_mul, 1, REST | PURE);
    DEF("/", Builtin_div, 1, REST | PURE);
    DEF("cons", Builtin_cons, 2, PURE);
    DEF("list", Builtin_list, 1, REST | PURE);
    DEF("macroexpand-1", Builtin_macroexpand_1, 1, DEFAULT);
    DEF("macroexpand", Builtin_macroexpand, 1, DEFAULT);
    DEF("progn", Builtin_progn, 1, REST);
    DEF("eval", Builtin_eval, 1, DEFAULT);
    DEF("car", Builtin_car, 1, PURE);
    DEF("cdr", Builtin_cdr, 1, PURE);
    DEF("reverse", Builtin_reverse, 1, PURE);
    DEF("read", Builtin_read, 1, DEFAULT);
    DEF("print", Builtin_print, 1, DEFAULT);
    DEF("=", Builtin_eq, 1, REST | PURE);
    DEF(">", Builtin_gt, 1, REST | PURE);
    DEF("<", Builtin_lt, 1, REST | PURE);
    DEF(">=", Builtin_gte, 1, REST | PURE);
    DEF("<=", Builtin_lte, 1, REST | PURE);
    DEF("vector", Builtin_vector, 1, REST | PURE);
    DEF("tuple-constructor", Builtin_tuple_constructor, 2, PURE);
    DEF("size", Builtin_size, 1, PURE);
    DEF("at", Builtin_at, 2, PURE);
    DEF("type-of", Builtin_type_of, 1, PURE);
}

#define INIT_ACC(init) value_type acc_type = INTEGER, cur_type = INTEGER; \
                       Sv_rational racc, r; \
                       double facc = (init), f; \
                       long acc = (init), i; \
                       racc.n = 0; racc.d = 0;

#define SET_ACC(op) switch (cur->type) { \
                    case SV_INT: \
                        cur_type = INTEGER; \
                        i = cur->val.i; \
                        break; \
                    case SV_RATIONAL: \
                        if (acc_type == REAL) { \
                            cur_type = REAL; \
                            f = (float) cur->val.rational.n / cur->val.rational.d; \
                        } else { \
                            if (cur_type != RATIONAL) { \
                                racc.n = acc; \
                                racc.d = acc; \
                            } \
                            acc_type = RATIONAL; \
                            cur_type = RATIONAL; \
                            r.n = cur->val.rational.n; \
                            r.d = cur->val.rational.d; \
                        } \
                        break; \
                    case SV_FLOAT: \
                        if (acc_type == RATIONAL) { \
                            facc = (float) racc.n / racc.d; \
                            acc_type = REAL; \
                        } \
                        if (acc_type != REAL) \
                            facc = acc; \
                        cur_type = REAL; \
                        acc_type = REAL; \
                        f = cur->val.f; \
                        break; \
                    default: \
                        return Sv_new_err(stu, op " can operate on numbers only"); \
                    }

#define RET_ACC return acc_type == RATIONAL \
                    ? Sv_new_rational(stu, racc.n, racc.d) \
                    : acc_type == REAL \
                        ? Sv_new_float(stu, facc) \
                        : Sv_new_int(stu, acc); \

extern Sv
*Builtin_add(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    Sv *cur = NULL;
    INIT_ACC(0);

    while (!IS_NIL(x) && (cur = CAR(x))) {
        SET_ACC("+");
        if (acc_type == RATIONAL) {
            if (cur_type != RATIONAL) {
              r.n = i;
              r.d = 1;
            }

            if (racc.n == 0) {
                racc.n = r.n;
                racc.d = r.d;
            } else if (r.d == racc.d) {
                racc.n += r.n;
                racc.d = r.d;
            } else {
                racc.n *= r.d;
                r.n *= racc.d;
                r.d *= racc.d;
                racc.d = r.d;
                racc.n += r.n;
            }
        } else if (acc_type == REAL)
            facc += (cur_type == REAL ? f : i);
        else
            acc += i;
        x = CDR(x);
    }

    RET_ACC;
}

extern Sv
*Builtin_sub(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    Sv *cur = NULL;
    INIT_ACC(0);

    if (!IS_NIL(x) && (cur = CAR(x))) {
        SET_ACC("-");
        if (acc_type == RATIONAL) {
            racc.n = r.n;
            racc.d = r.d;
        } else if (acc_type == REAL)
            facc = (cur_type == REAL ? f : i);
        else
            acc = i;
        x = CDR(x);

        if (!IS_NIL(x)) {
            while (!IS_NIL(x) && (cur = CAR(x))) {
                SET_ACC("-");
                if (acc_type == RATIONAL) {
                    /*
                     * If the current value is not a float, it's for sure an
                     * integer since we reset the rational flag otherwise.
                     */
                    if (cur_type != RATIONAL) {
                      r.n = i;
                      r.d = 1;
                    }

                    if (r.d == racc.d) {
                        racc.n -= r.n;
                        racc.d = r.d;
                    } else {
                        racc.n *= r.d;
                        r.n *= racc.d;
                        r.d *= racc.d;
                        racc.d = r.d;
                        racc.n -= r.n;
                    }
                } else if (acc_type == REAL)
                    facc -= (cur_type == REAL ? f : i);
                else
                    acc -= i;
                x = CDR(x);
            }
        } else {
          if (acc_type == RATIONAL) {
                racc.n = -racc.n;
          } else if (acc_type == REAL)
                facc = -facc;
            else
                acc = -acc;
        }
    }

    RET_ACC;
}

extern Sv
*Builtin_mul(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    Sv *cur = NULL;
    INIT_ACC(1);
    racc.n = 1;
    racc.d = 1;

    while (!IS_NIL(x) && (cur = CAR(x))) {
        SET_ACC("*");
        if (acc_type == RATIONAL) {
            if (acc != 1) {
              racc.n = acc;
              racc.d = 1;
            }

            /*
             * If the current value is not a float, it's for sure an
             * integer since we reset the rational flag otherwise.
             */
            if (cur_type != RATIONAL) {
                racc.n *= i;
            } else {
                racc.n *= r.n;
                racc.d *= r.d;
            }
        } else if (acc_type == REAL)
            facc *= (cur_type == REAL ? f : i);
        else
            acc *= i;
        x = CDR(x);
    }

    RET_ACC;
}

extern Sv
*Builtin_div(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    Sv *cur = NULL;
    INIT_ACC(0);

    if (!IS_NIL(x) && (cur = CAR(x))) {
        SET_ACC("/");
        if (acc_type == RATIONAL) {
            racc.n = r.n;
            racc.d = r.d;
        } else if (acc_type == REAL)
            facc = (cur_type == REAL ? f : i);
        else
            acc = i;
        x = CDR(x);

        if (!IS_NIL(x)) {
            while (!IS_NIL(x) && (cur = CAR(x))) {
                SET_ACC("/");
                if (cur_type == REAL && f == 0)
                    return Sv_new_err(stu, "'/' cannot divide by zero!");
                if (cur_type != RATIONAL && i == 0)
                    return Sv_new_err(stu, "'/' cannot divide by zero!");

                if (acc_type == RATIONAL) {
                  if (cur_type == RATIONAL) {
                    racc.n *= r.d;
                    racc.d *= r.n;
                  } else {
                    racc.d *= i;
                  }
                } else if (acc_type == REAL)
                    facc /= (cur_type == REAL ? f : i);
                else
                    acc /= i;
                x = CDR(x);
            }
        } else {
            if (acc_type == REAL)
                facc = 1 / (cur_type == REAL ? f : i);
            else {
                acc_type = RATIONAL;
                racc.n = 1;
                racc.d = i;
            }
        }
    } else {
        return Sv_new_err(stu, "'/' requires one or more arguments");
    }

    RET_ACC;
}

extern Sv
*Builtin_cons(Stu *stu, Env *env, Sv **args)
{
    return Sv_cons(stu, args[0], args[1]);
}

extern Sv
*Builtin_list(Stu *stu, Env *env, Sv **x)
{
    return Sv_list(stu, *x);
}

extern Sv
*Builtin_macroexpand_1(Stu *stu, Env *env, Sv **x)
{
    return Sv_expand_1(stu, *x);
}

extern Sv
*Builtin_macroexpand(Stu *stu, Env *env, Sv **x)
{
    return Sv_expand(stu, *x);
}

extern Sv
*Builtin_progn(Stu *stu, Env *env, Sv **x)
{
    return Sv_eval_list(stu, env, *x);
}

extern Sv
*Builtin_eval(Stu *stu, Env *env, Sv **x)
{
    return Sv_eval(stu, env, *x);
}

extern Sv
*Builtin_car(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    if (x->type != SV_CONS)
        return Sv_new_err(stu, "'car' needs a single list argument");
    return CAR(x);
}

extern Sv
*Builtin_cdr(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    if (x->type != SV_CONS)
        return Sv_new_err(stu, "'cdr' needs a single list argument");
    return CDR(x);
}

extern Sv
*Builtin_reverse(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    if (x->type != SV_CONS)
        return Sv_new_err(stu, "'reverse' needs a single list argument");
    return Sv_reverse(stu, x);
}

extern Sv
*Builtin_read(Stu *stu, Env *env, Sv **x)
{
    Sv *code = *x, *result = NULL;
    Svlist *forms = NULL;

    if (code->type != SV_STR || code->val.buf == NULL)
        return Sv_new_err(stu, "read expects a string argument");

    forms = Stu_parse_buf(stu, code->val.buf);
    if (forms->count != 1) {
        result = Sv_new_err(stu, "read argument must contain exactly one form");
    } else {
        result = forms->head->sv;
    }

    Svlist_destroy(&forms);

    return result;
}

extern Sv
*Builtin_print(Stu *stu, Env *env, Sv **x)
{
    Sv_dump(stu, *x, stdout);
    return *x;
}

/*
 * Generate comparison functions using a macro.
 */

#define OP_EQ  1
#define OP_GT  2
#define OP_LT  3
#define OP_GTE 4
#define OP_LTE 5

#define CMP_SV(op) \
    Sv *sv = *args; \
    Sv *x = CAR(sv), *y = CADR(sv), *rest = CDR(sv); \
    short result = 1; \
    if (!x && !y) return Sv_new_bool(stu, 1);    \
    if (x && y) { \
        while (!IS_NIL(rest) && (y = CAR(rest))) { \
            if (x->type == y->type) { \
                switch (x->type) { \
                    case SV_NIL: \
                        result = 1; \
                        break; \
                    case SV_INT: \
                    case SV_BOOL: \
                    case SV_SYM: \
                        result = result && compare_numbers(op, x->val.i, y->val.i); \
                        break; \
                    case SV_RATIONAL: \
                        result = result && compare_rationals(op, x->val.rational, y->val.rational); \
                        break; \
                    case SV_STR: \
                    case SV_ERR: \
                        result = result && compare_strings(op, x->val.buf, y->val.buf); \
                        break; \
                    default: \
                        return Sv_new_err(stu, "'eq' does not support these types"); \
                } \
            } else if (x->type == SV_INT && x->type == SV_RATIONAL) { \
                result = result && compare_numbers(op, x->val.i * y->val.rational.d, y->val.rational.n); \
            } else if (y->type == SV_INT && x->type == SV_RATIONAL) { \
                result = result && compare_numbers(op, x->val.rational.n, y->val.i * x->val.rational.d); \
            } else { \
                return Sv_new_bool(stu, 0);      \
            } \
            \
            if (!result) return Sv_new_bool(stu, 0); \
            \
            x = y; rest = CDR(rest); \
        } \
        \
        if (IS_NIL(rest)) { \
            return Sv_new_bool(stu, result);     \
        } \
    } \
    return Sv_new_bool(stu, 0);

static int
compare_numbers(int op, long x, long y)
{
    switch (op) {
    case OP_EQ:
        return x == y ? 1 : 0;

    case OP_GT:
        return x > y ? 1 : 0;

    case OP_LT:
        return x < y ? 1 : 0;

    case OP_GTE:
        return x >= y ? 1 : 0;

    case OP_LTE:
        return x <= y ? 1 : 0;

    default:
        return 0;
    }
}

static int
compare_rationals(int op, Sv_rational x, Sv_rational y)
{
    return compare_numbers(op, x.n * y.d, x.d * y.n);
}

static int
compare_strings(int op, const char *x, const char *y)
{
    switch (op) {
    case OP_EQ:
        return !strcmp(x, y) ? 1 : 0;

    case OP_GT:
        return strcmp(x, y) > 0 ? 1 : 0;

    case OP_LT:
        return strcmp(x, y) < 0 ? 1 : 0;

    case OP_GTE:
        return strcmp(x, y) >= 0 ? 1 : 0;

    case OP_LTE:
        return strcmp(x, y) <= 0 ? 1 : 0;

    default:
        return 0;
    }
}

extern Sv
*Builtin_eq(Stu *stu, Env *env, Sv **args)
{
    CMP_SV(OP_EQ);
}

extern Sv
*Builtin_gt(Stu *stu, Env *env, Sv **args)
{
    CMP_SV(OP_GT);
}

extern Sv
*Builtin_lt(Stu *stu, Env *env, Sv **args)
{
    CMP_SV(OP_LT);
}

extern Sv
*Builtin_gte(Stu *stu, Env *env, Sv **args)
{
    CMP_SV(OP_GTE);
}

extern Sv
*Builtin_lte(Stu *stu, Env *env, Sv **args)
{
    CMP_SV(OP_LTE);
}

extern Sv
*Builtin_vector(Stu *stu, Env *env, Sv **args)
{
    return Sv_new_vector(stu, args[0]);
}

extern Sv
*Builtin_tuple_constructor(Stu *stu, Env *env, Sv **args)
{
    Sv *name = args[0];
    Sv *arity = args[1];

    if (name->type != SV_SYM)
        return Sv_new_err(stu, "tuple-constructor first argument not a symbol");

    if (arity->type != SV_INT)
        return Sv_new_err(stu, "tuple-constructor second argument not an integer");

    long num = arity->val.i;
    if (num < 0)
        return Sv_new_err(stu, "tuple-costructor second argument lower than zero");

    Type tt = Type_new(stu, name, arity->val.i);

    return Sv_new_tuple_constructor(stu, tt);
}

extern Sv
*Builtin_size(Stu *stu, Env *env, Sv **args)
{
    Sv *tuple = args[0];
    if (tuple->type != SV_TUPLE)
        return Sv_new_err(stu, "size argument not a tuple");
    return Sv_new_int(stu, Type_arity(stu, tuple->val.tuple->type));
}

extern Sv
*Builtin_at(Stu *stu, Env *env, Sv **args)
{
    Sv *index = args[0];
    Sv *tuple_sv = args[1];
    if (index->type != SV_INT)
        return Sv_new_err(stu, "at first argument not an integer");
    if (tuple_sv->type != SV_TUPLE)
        return Sv_new_err(stu, "at second argument not a tuple");
    Sv_tuple *tuple = tuple_sv->val.tuple;
    long i = index->val.i;
    if (i < 0 || i >= Type_arity(stu, tuple->type))
        return Sv_new_err(stu, "at index out of bounds");
    return tuple->values[i];
}

extern Sv
*Builtin_type_of(Stu *stu, Env *env, Sv **args)
{
    Sv *x = *args;
    switch (x->type) {
    case SV_NIL:
        return x;

    case SV_SYM:
        return Sv_new_sym(stu, "symbol");

    case SV_INT:
        return Sv_new_sym(stu, "integer");

    case SV_FLOAT:
        return Sv_new_sym(stu, "float");

    case SV_RATIONAL:
        return Sv_new_sym(stu, "rational");

    case SV_BOOL:
        return Sv_new_sym(stu, "boolean");

    case SV_STR:
        return Sv_new_sym(stu, "string");

    case SV_CONS:
        return Sv_new_sym(stu, "cons");

    case SV_NATIVE_FUNC:
    case SV_NATIVE_CLOS:
    case SV_LAMBDA:
    case SV_TUPLE_CONSTRUCTOR:
        return Sv_new_sym(stu, "function");

    case SV_TUPLE:
        return Type_value(stu, x->val.tuple->type);

    case SV_SPECIAL:
    case SV_ERR:
    default:
        return Sv_new_err(stu, "unknown type found in type-of");
    }
}
