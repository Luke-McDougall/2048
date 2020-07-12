#include <X11/Xlib.h>
#include "types.h"

void rect(i32 x, i32 y, u32 width, u32 height, u32 color_pixel, XImage *img);
void fill_circle(f32 x, f32 y, f32 radius, XImage *buffer, u32 color);
