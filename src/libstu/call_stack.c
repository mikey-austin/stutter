/*
 * Copyright (c) 2018 Mikey Austin <mikey@jackiemclean.net>
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

#include "env.h"
#include "stu_private.h"
#include "call_stack.h"
#include "symtab.h"
#include "sv.h"

extern Sv
*Call_stack_copy(Stu *stu) {
    return IS_NIL(stu->call_stack)
        ? NIL
        : Sv_copy(stu, stu->call_stack);
}

extern Sv
*Call_stack_pop(Stu *stu) {
    if (IS_NIL(stu->call_stack)) {
        return NIL;
    }

    Sv *head = CAR(stu->call_stack);
    stu->call_stack = CDR(stu->call_stack);

    return head;
}

extern void
Call_stack_push(Stu *stu, Sv *x) {
    Sv *head = NULL;

    switch (x->type) {
    case SV_SYM:
        head = Sv_new_str(stu, Symtab_get_name(stu, x->val.i));
        break;

    case SV_STR:
        head = Sv_copy(stu, x);
        break;

    case SV_NATIVE_FUNC:
        head = Sv_new_str(stu, "<NATIVE FUNCTION>");
        break;

    case SV_NATIVE_CLOS:
        head = Sv_new_str(stu, "<NATIVE CLOSURE>");
        break;

    case SV_LAMBDA:
        head = Sv_new_str(stu, "<LAMBDA>");
        break;

    default:
        return;
    }

    stu->call_stack = Sv_cons(stu, head, stu->call_stack);
}