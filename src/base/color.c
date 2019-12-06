#include "../base.h"

bool ax_color_to_rgb(ax_color color, uint8_t* out_rgb)
{
    out_rgb[0] = (color & 0xff0000) >> 16;
    out_rgb[1] = (color & 0x00ff00) >> 8;
    out_rgb[2] = (color & 0x0000ff) >> 0;
    return !AX_COLOR_IS_NULL(color);
}

ax_color ax_color_from_rgb(uint8_t* rgb)
{
    if (rgb == NULL) {
        return AX_NULL_COLOR;
    } else {
        return (uint32_t) rgb[0] * 0x010000 +
            (uint32_t) rgb[1] * 0x000100 +
            (uint32_t) rgb[2];
    }
}
