#pragma once
#include "base.h"
#include "core/region.h"

struct ax_tree;

struct ax_geom {
    struct ax_dim root_dim;
    struct region layout_rgn;
    struct region temp_rgn;
};

void ax__init_geom(struct ax_geom* g);
void ax__free_geom(struct ax_geom* g);

void ax__layout(struct ax_tree* tr, struct ax_geom* g);
