#pragma once
#include "base.h"

struct ax_tree;

struct ax_geom {
    struct ax_dim root_dim;
};

static inline
void ax__init_geom(struct ax_geom* g)
{
    g->root_dim = AX_DIM(0.0, 0.0);
}

static inline
void ax__free_geom(struct ax_geom* g)
{
    (void) g;
}

void ax__layout(struct ax_tree* tr, struct ax_geom* g);
