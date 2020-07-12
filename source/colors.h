#include "types.h"

#ifndef COLORS
#define COLORS

typedef struct
{
    f32 r;
    f32 g;
    f32 b;
} Color;

Color from_pixel(u32);
u32 pixel(Color);
u32 blue(Color);
u32 green(Color);
u32 red(Color);
Color change_saturation(Color, f32);

#endif
