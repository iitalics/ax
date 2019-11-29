#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "text.h"
#include "utils.h"


void ax__text_iter_init(struct ax_text_iter* ti, const char* text)
{
    ti->text = text;
    ti->word = NULL;
    ti->line = malloc(strlen(text) + 1);
    ASSERT(ti->line != NULL, "malloc ax_text_iter.line");
    ti->line_len = 0;
    ti->line_need_reset = false;
}

void ax__text_iter_free(struct ax_text_iter* ti)
{
    free(ti->line);
    free(ti->word);
}


static inline const char* beg_of_word(const char* s, bool* is_eol)
{
    char c;
    while (c = s[0], isspace(c)) {
        s++;
        if (c == '\n') {
            *is_eol = true;
            return s;
        }
    }
    *is_eol = false;
    return s;
}

static inline const char* end_of_word(const char* s)
{
    while (s[0] && !isspace(s[0])) { s++; }
    return s;
}

enum ax_text_elem ax__text_iter_next(struct ax_text_iter* ti)
{
    if (ti->line_need_reset) {
        ti->line[ti->line_len = 0] = '\0';
        ti->line_need_reset = false;
    }

    bool is_eol;
    const char* bow = beg_of_word(ti->text, &is_eol);
    const char* eow = end_of_word(bow);
    if (is_eol) {
        ti->text = bow;
        ti->line_need_reset = true;
        return AX_TEXT_EOL;
    } else if (bow >= eow) {
        return AX_TEXT_END;
    } else {
        size_t w_len = eow - bow;
        free(ti->word);
        ti->word = malloc(w_len + 1);
        ASSERT(ti->word != NULL, "malloc ax_text_iter.word");
        memcpy(ti->word, bow, w_len);
        ti->word[w_len] = '\0';

        if (ti->line_len > 0) {
            ti->line[ti->line_len++] = ' ';
        }
        strcpy(&ti->line[ti->line_len], ti->word);
        ti->line_len += w_len;

        ti->text = eow;
        return AX_TEXT_WORD;
    }
}
