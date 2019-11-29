#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "text.h"
#include "utils.h"


void ax__text_iter_init(struct ax_text_iter* ti, const char* text)
{
    ti->text = text;
    ti->word = NULL;
}

void ax__text_iter_free(struct ax_text_iter* ti)
{
    free(ti->word);
}


static inline const char* beg_of_word(const char* s)
{
    while (isspace(s[0])) { s++; }
    return s;
}

static inline const char* end_of_word(const char* s)
{
    while (s[0] && !isspace(s[0])) { s++; }
    return s;
}

enum ax_text_elem ax__text_iter_next(struct ax_text_iter* ti)
{
    const char* bow = beg_of_word(ti->text);
    const char* eow = end_of_word(bow);
    if (bow >= eow) {
        return AX_TEXT_END;
    } else {
        free(ti->word);
        size_t len = eow - bow;
        ti->word = malloc(len + 1);
        ASSERT(ti->word != NULL, "malloc ax_text_iter.word");
        memcpy(ti->word, bow, len);
        ti->word[len] = '\0';
        ti->text = eow;
        return AX_TEXT_WORD;
    }
}
