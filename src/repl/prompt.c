/*
 * Copyright (c) 2017 Mikey Austin <mikey@jackiemclean.net>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

#include "prompt.h"

static EditLine *__editor = NULL;
static History *__history = NULL;
static HistEvent __hist_event;

static char
*stu_prompt(EditLine *el)
{
    // TODO: change prompt depending on the context (eg continuation, ...)
    return "stu> ";
}

extern void
Prompt_init(const char *progname)
{
    if ((__editor = el_init(progname, stdin, stdout, stderr)) == NULL)
        errx(1, "could not init editline");
    el_set(__editor, EL_PROMPT, &stu_prompt);
    el_set(__editor, EL_EDITOR, "emacs");

    if ((__history = history_init()) == NULL)
        errx(1, "could not init editline history");
    history(__history, &__hist_event, H_SETSIZE, 800);
    el_set(__editor, EL_HIST, history, __history);
}

extern int
Prompt_readline(char **line)
{
    int count = -1;
    char const *input = NULL;

    if ((input = el_gets(__editor, &count)) != NULL) {
        *line = strdup(input);
    } else {
        *line = NULL;
    }

    return count;
}

extern void
Prompt_save(const char *to_save)
{
    history(__history, &__hist_event, H_ENTER, to_save);
}

extern void
Prompt_finish(void)
{
    history_end(__history);
    el_end(__editor);
}
