#include "draw.h"

#define LEFT    (1 << 0)
#define RIGHT   (1 << 1)
#define TOP     (1 << 2)
#define BOTTOM  (1 << 3)
void rect(i32 x, i32 y, u32 width, u32 height, u32 color_pixel, XImage *img)
{
    // Safety precautions
    // A bit being set in sides indicates that the side should be drawn as it is inside the visible area
    u8 sides = LEFT | RIGHT | TOP | BOTTOM;
    if(x >= img->width || y >= img->height) return;
    if(x <= -((i32) width) || y <= -((i32) height)) return;
    if(x < 0)
    {
        width += x;
        x = 0;
        sides &= ~LEFT;
    }
    if(y < 0)
    {
        height += y;
        y = 0;
        sides &= ~TOP;
    }
    if(x + width >= img->width) 
    {
        width = img->width - x;
        sides &= ~RIGHT;
    }
    if(y + height >= img->height)
    {
        height = img->height - y;
        sides &= ~BOTTOM;
    }

    u32 *data = (u32 *) img->data;


    u32 idx1 = y * img->width + x;
    u32 idx2 = idx1 + img->width * (height - 1);
    u32 idx3 = y * img->width + x;
    u32 idx4 = idx3 + (width - 1);

    // All sides visible
    if(sides == (LEFT | RIGHT | TOP | BOTTOM))
    {
        // Note: There need to be two separate loops because I want this to work for
        // all rectangles if it just drew squares it could be one loop.
        
        // Draw horizontal lines
        for(u32 i = 0; i < width; ++i)
        {
            data[idx1++] = color_pixel;
            data[idx2++] = color_pixel;
        }

        // Draw vertical lines
        for(u32 j = y; j < y + height; ++j)
        {
            data[idx3] = color_pixel;
            data[idx4] = color_pixel;
            idx3 += img->width;
            idx4 += img->width;
        }
    }
    else 
    {
        // Draw left side
        if((sides & LEFT))
        {
            for(u32 j = y; j < y + height; ++j)
            {
                data[idx3] = color_pixel;
                idx3 += img->width;
            }
        }

        // Draw right side
        if((sides & RIGHT))
        {
            for(u32 j = y; j < y + height; ++j)
            {
                data[idx4] = color_pixel;
                idx4 += img->width;
            }
        }

        // Draw top side
        if((sides & TOP))
        {
            for(u32 i = 0; i < width; ++i)
            {
                data[idx1++] = color_pixel;
            }
        }

        // Draw bottom side
        if((sides & BOTTOM))
        {
            for(u32 i = 0; i < width; ++i)
            {
                data[idx2++] = color_pixel;
            }
        }
    }
}

void fill_circle(f32 x, f32 y, f32 radius, XImage *buffer, u32 color)
{
    i32 centre_x = (i32) (x + 0.5);
    i32 centre_y = (i32) (y + 0.5);
    i32 int_radius = (i32) (radius + 0.5);
    i32 square_radius = (i32) (radius * radius + 0.5);
    u32 *data = (u32 *) buffer->data;

    for(i32 i = -int_radius; i <= 0; ++i)
    {
        for(i32 j = -int_radius; j <= 0; ++j)
        {
            if(j * j + i * i <= square_radius)
            {
                i32 x1, x2, y1, y2;
                x1 = centre_x + i;
                x2 = centre_x - i;
                y1 = centre_y + j;
                y2 = centre_y - j;

                if(x1 >= 0 && x1 < buffer->width)
                {
                    if(y1 >= 0 && y1 < buffer->height)
                    {
                        data[x1 + y1 * buffer->width] = color;
                    }

                    if(y2 >= 0 && y2 < buffer->height)
                    {
                        data[x1 + y2 * buffer->width] = color;
                    }
                }

                if(x2 >= 0 && x2 < buffer->width)
                {
                    if(y1 >= 0 && y1 < buffer->height)
                    {
                        data[x2 + y1 * buffer->width] = color;
                    }

                    if(y2 >= 0 && y2 < buffer->height)
                    {
                        data[x2 + y2 * buffer->width] = color;
                    }
                }
            }
        }
    }
}
