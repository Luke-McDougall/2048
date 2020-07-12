#include "colors.h"

Color from_pixel(u32 pixel)
{
    Color color;
    u8 red   = pixel >> 16;
    u8 green = pixel >> 8;
    u8 blue  = pixel;

    color.r = ((f32) red) / 255.0;
    color.g = ((f32) green) / 255.0;
    color.b = ((f32) blue) / 255.0;

    return color;
}

u32 pixel(Color col)
{
    u8 red   = (u8) (col.r * 255.0 + 0.5);
    u8 green = (u8) (col.g * 255.0 + 0.5);
    u8 blue  = (u8) (col.b * 255.0 + 0.5);

    return (red << 16) | (green << 8) | (blue << 0);
}

u32 blue(Color col)
{
    u8 blue  = (u8) (col.b * 255.0 + 0.5);
    return (u32) blue;
}

u32 green(Color col)
{
    u8 green = (u8) (col.g * 255.0 + 0.5);
    return (green << 8);
}

u32 red(Color col)
{
    u8 red   = (u8) (col.r * 255.0 + 0.5);
    return (red << 16);
}

Color change_saturation(Color color, f32 factor)
{
    Color new_color;
    new_color.r = color.r * factor;
    new_color.g = color.g * factor;
    new_color.b = color.b * factor;

    return new_color;
}
