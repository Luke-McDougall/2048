#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "draw.h"
#include "types.h"


/* --------------------------------- 2048 logic ---------------------------------  */

// Number of cells in the grid. I might change this to be dynamic at some point
#define CELL_NUM 16

// Width/height of the grid i.e. LENGTH * LENGTH == CELL_NUM
#define LENGTH 4

static u32 colors[11] = {
      /* R             G            B */
    (105 << 16) | (105 << 8) | (105 << 0),         // Grey
    (255 << 16) | (255 << 8) | (0   << 0),         // Yellow
    (200 << 16) | (100 << 8) | (0   << 0),         // Orange
    (255 << 16) | (0   << 8) | (0   << 0),         // Red
    (130 << 16) | (18  << 8) | (75  << 0),         // Magenta
    (255 << 16) | (100 << 8) | (255 << 0),         // Purple
    (0   << 16) | (0   << 8) | (255 << 0),         // Blue
    (0   << 16) | (255 << 8) | (255 << 0),         // Cyan
    (55  << 16) | (206 << 8) | (68  << 0),         // Green
    (20  << 16) | (14  << 8) | (15  << 0),         // Brown
    (0   << 16) | (0   << 8) | (0   << 0),         // Black
};

/* File pointer used for writing debug info to a log file */
FILE *debug;

typedef struct 
{
    u8 data[CELL_NUM];
    u8 empty_count;
} Matrix;

typedef enum
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
} Dir;

#define NUM_FRAMES 10
typedef struct
{
    /* Minimum required info */
    Dir dir;
    f32 distance;
    /* Index of the cells array that the animation applies to */
    u32 index;
    /* Index of the destination of the cell after the animation has played. This is used to update animations */
    /* because calculating the next board state is done in multiple steps requiring some animations to be updated */
    u32 destination;
} AnimationData;

typedef struct
{
    AnimationData queue[CELL_NUM];
    size_t count;
} AnimationQueue;

void fill_cell(f32 x, f32 y, u32 length, u32 color, XImage *window_buffer);
void push_animation(Dir dir, u32 index, u32 destination);
void update_destination(u32 current, u32 new);
void render(XImage *window_buffer);

// Simple random number generator.
// TODO: Initialize x with time()
u32 rand_int()
{
    static u32 x = 723498734;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return x;
}

b32 matrix_equals(Matrix *a, Matrix *b)
{
    for(int i = 0; i < CELL_NUM; ++i)
    {
        if(a->data[i] != b->data[i]) return false;
    }
    return true;
}

void shift_line(Matrix *matrix, int start, int step, Dir dir, int iter)
{
    int idx = start;
    for(int i = 0; i < LENGTH; ++i)
    {
        u8 val = matrix->data[start];
        if(val)
        {
            matrix->data[start] = 0;
            matrix->data[idx] = val;

            if(iter == 1)
            {
                push_animation(dir, start, idx);
            }
            else
            {
                update_destination(start, idx);
            }

            idx += step;
        }
        start += step;
    }
}

void combine_line(Matrix *matrix, u8 start, i8 step)
{
    for(int i = 0; i < LENGTH - 1; ++i)
    {
        u8 val = matrix->data[start];
        if(val && val == matrix->data[start + step])
        {
            matrix->data[start] += 1;
            matrix->data[start + step] = 0;
            update_destination(start + step, start);
            matrix->empty_count += 1;
        }
        start += step;
    }
}

void matrix_update(Matrix *m)
{
    size_t one, two;
    u8 empty_idx, empty_count;

    empty_count = m->empty_count;
    empty_idx = 0;
    
    one = rand_int() % empty_count;
    two = rand_int() % empty_count;

    while(empty_count > 1 && one == (two = rand_int() % empty_count));

    for(int i = 0; i < CELL_NUM; ++i)
    {
        if(m->data[i] == 0)
        {
            if(empty_idx == one || empty_idx == two)
            {
                m->data[i] = 1;
                m->empty_count -= 1;
            }
            empty_idx++;
        }
    }
}

b32 game_over(Matrix *matrix)
{
    if(matrix->empty_count > 0) return false;

    // Check rows for matching adjacent cells
    for(int i = 0; i < LENGTH - 1; ++i)
    {
        for(int j = 0; j < LENGTH; ++j)
        {
            int idx = i + j * LENGTH;
            if(matrix->data[idx] == matrix->data[idx + 1]) return false;
        }
    }

    // Check columns for matching adjacent cells
    for(int i = 0; i < LENGTH; ++i)
    {
        for(int j = 0; j < LENGTH - 1; ++j)
        {
            int idx = i + j * LENGTH;
            if(matrix->data[idx] == matrix->data[idx + LENGTH]) return false;
        }
    }

    return true;
}

void shift(Matrix *matrix, Dir dir)
{
    u8 start;
    i8 step;
    Matrix old;

    memcpy((void *) &old, (void *) matrix, sizeof(Matrix));

    switch(dir)
    {
    case LEFT:
        start = 0;
        step = LENGTH;
        for(int i = 0; i < LENGTH; ++i)
        {
            shift_line(matrix, start, 1, LEFT, 1);
            combine_line(matrix, start, 1);
            shift_line(matrix, start, 1, LEFT, 2);
            start += step;
        }
        break;

    case UP:
        start = 0;
        step = 1;
        for(int i = 0; i < LENGTH; ++i)
        {
            shift_line(matrix, start, LENGTH, UP, 1);
            combine_line(matrix, start, LENGTH);
            shift_line(matrix, start, LENGTH, UP, 2);
            start += step;
        }
        break;

    case RIGHT:
        start = LENGTH - 1;
        step = LENGTH;
        for(int i = 0; i < LENGTH; ++i)
        {
            shift_line(matrix, start, -1, RIGHT, 1);
            combine_line(matrix, start, -1);
            shift_line(matrix, start, -1, RIGHT, 2);
            start += step;
        }
        break;

    case DOWN:
        start = (LENGTH - 1) * LENGTH;
        step = 1;
        for(int i = 0; i < LENGTH; ++i)
        {
            shift_line(matrix, start, -LENGTH, DOWN, 1);
            combine_line(matrix, start, -LENGTH);
            shift_line(matrix, start, -LENGTH, DOWN, 2);
            start += step;
        }
        break;
    }
    // Shift is called even if nothing would happen to the grid. Thus
    // matrix_update is only called if the matrix has been changed.
    if(!matrix_equals(matrix, &old)) matrix_update(matrix);
}

void matrix_print(Matrix *m)
{
    for(int i = 0; i < CELL_NUM; ++i)
    {
        if(i % LENGTH == 0) printf("\n");
        printf("%d ", m->data[i]);
    }
    printf("\n");
}

/* ----------------------- End of 2048 logic ----------------------- */





/* ------------------------- Rendering ------------------------- */

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

// TODO: I'm assuming 1440p change this to be based on screen width/height
#define WINDOW_X 880 
#define WINDOW_Y 320 

typedef struct
{
    b32 active;
    f32 x;
    f32 y;
    u32 color;
} Cell;


static Cell cells[CELL_NUM];
static AnimationQueue animations;

void push_animation(Dir dir, u32 index, u32 destination)
{
    int start_x = index % LENGTH;
    int start_y = index / LENGTH;
    int idx_x = destination % LENGTH;
    int idx_y = destination / LENGTH;

    f32 distance;
    if(dir == LEFT || dir == RIGHT) distance = (idx_x - start_x) * 200;
    else distance = (idx_y - start_y) * 200;

    if(distance < 0) distance = -distance;

    AnimationData new = {
        .dir = dir,
        .distance = distance,
        .index = index,
        .destination = destination,
    };

    animations.queue[animations.count++] = new;
}

/* This function is predicated on the fact that there should never be more than one animation with the same destination */
/* when it is called. */
void update_destination(u32 current, u32 new)
{
    for(int i = 0; i < animations.count; ++i)
    {
        if(animations.queue[i].destination == current)
        {
            animations.queue[i].destination = new;
            int start_x = animations.queue[i].index % LENGTH;
            int start_y = animations.queue[i].index / LENGTH;
            int idx_x = animations.queue[i].destination % LENGTH;
            int idx_y = animations.queue[i].destination / LENGTH;

            f32 distance;
            if(animations.queue[i].dir == LEFT || animations.queue[i].dir == RIGHT) distance = (idx_x - start_x) * 200;
            else distance = (idx_y - start_y) * 200;

            if(distance < 0) distance = -distance;

            animations.queue[i].distance = distance;
            return;
        }
    }
}

void play_animations(Display *display, GC gc, Window window, XImage *window_buffer)
{
    // TODO: Ungrab keyboard is here for debugging because I had to turn off my computer when the program froze and still had
    // control of the keyboard. Should remove this at some point.
    XUngrabKeyboard(display, CurrentTime);

    f64 render_start, render_time;
    struct timespec req = {0};

    for(int frame_counter = 0; frame_counter < NUM_FRAMES; ++frame_counter)
    {
        render_start = clock();
        for(int i = 0; i < animations.count; ++i)
        {
            AnimationData animation = animations.queue[i];
            if(animation.distance == 0) continue;
            f32 step = animation.distance / (f32) NUM_FRAMES;
            switch(animation.dir)
            {
                case LEFT:
                {
                    cells[animation.index].x -= step;
                } break;

                case RIGHT:
                {
                    cells[animation.index].x += step;
                } break;

                case UP:
                {
                    cells[animation.index].y -= step;
                } break;

                case DOWN:
                {
                    cells[animation.index].y += step;
                } break;
            }
        }
        render(window_buffer);
        XPutImage(display, window, gc, window_buffer, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        /* NOTE: This whole things feels convoluted like I'm casting a bunch for no reason. Maybe it's fine but keep thinking about this */
        render_time = ((f64) clock() - render_start) / (f64) CLOCKS_PER_SEC;
        req.tv_nsec = 16666666l - (i64) (render_time * 1000000000 + 0.5); /* 60 fps, might change this to be variable at some point */
        nanosleep(&req, NULL);
    }
    animations.count = 0;
}

void push_cell(f32 x, f32 y, u32 color, u32 index)
{
    cells[index].active = true;
    cells[index].x = x;
    cells[index].y = y;
    cells[index].color = color;
}

void draw_grid(XImage *window_buffer, u32 color)
{
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            rect(i * 200, j * 200, 199, 199, color, window_buffer);
        }
    }
}

void fill_cell(f32 x, f32 y, u32 length, u32 color, XImage *window_buffer)
{
    u32 int_x = (u32) (x + 0.5);
    u32 int_y = (u32) (y + 0.5);
    /* fprintf(debug, "fill_cell called:\nx: %d, y: %d, length: %d\n", int_x, int_y, length); */

    u32 *data = (u32 *) window_buffer->data;
    for(int i = int_x; i < int_x + length; ++i)
    {
        for(int j = int_y; j < int_y + length; ++j)
        {
            data[i + window_buffer->width * j] = color;
        }
    }
}

void render(XImage *window_buffer)
{
    memset((void *) window_buffer->data, ~0, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(u32));
    draw_grid(window_buffer, 0);

    for(int i = 0; i < CELL_NUM; ++i)
    {
        if(cells[i].active)
        {
            fill_cell(cells[i].x, cells[i].y, 197, cells[i].color, window_buffer);
        }
    }
}

int main()
{
    /* debug = fopen("debug.log", "w"); */
    /* Setup window */
    Display *display = XOpenDisplay(NULL);
    u32 screen = DefaultScreen(display);
    Window window;
    GC gc;
    XImage *window_buffer;
    XSetWindowAttributes window_attributes = {
        .override_redirect = 1,
        .background_pixel = WhitePixel(display, screen),
        .border_pixel = (255 << 8) | (255 << 0), // Cyan
        .event_mask = StructureNotifyMask | KeyPressMask | EnterWindowMask | LeaveWindowMask,
    };

    window = XCreateWindow(display, RootWindow(display, screen),
                           WINDOW_X, WINDOW_Y,
                           WINDOW_WIDTH, WINDOW_HEIGHT,
                           2, DefaultDepth(display, screen),
                           InputOutput, DefaultVisual(display, screen),
                           CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask,
                           &window_attributes);

    gc = XCreateGC(display, window, 0, NULL);

    window_buffer = XCreateImage(display, DefaultVisual(display, screen),
                                 24, ZPixmap, 0,
                                 (char *) calloc(WINDOW_WIDTH * WINDOW_HEIGHT, sizeof(u32)),
                                 WINDOW_WIDTH, WINDOW_HEIGHT, 32, WINDOW_WIDTH * sizeof(u32));
    memset((void *) window_buffer->data, ~0, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(u32));
    XMapWindow(display, window);

    /* Setup matrix */
    Matrix game_state = {0};
    game_state.empty_count = CELL_NUM;
    matrix_update(&game_state);

    XEvent event;
    for(;;)
    {
        XNextEvent(display, &event);
        switch(event.type)
        {
            case MapNotify:
            {
                for(int i = 0; i < CELL_NUM; ++i)
                {
                    u32 color_index;
                    if((color_index = game_state.data[i]) > 0)
                    {
                        int x = i % LENGTH;
                        int y = i / LENGTH;
                        push_cell(x * 200 + 1, y * 200 + 1, colors[color_index - 1], i);
                    }
                }
                render(window_buffer);
                XPutImage(display, window, gc, window_buffer, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            } break;

            case EnterNotify:
            {
                XGrabKeyboard(display, window, 1, GrabModeAsync, GrabModeAsync, CurrentTime);
            } break;

            case LeaveNotify:
            {
                XUngrabKeyboard(display, CurrentTime);
            } break;
            
            case KeyPress:
            {
                if(game_over(&game_state)) return 0;
                // I have no idea what the 0 does. It's an index?? for something??
                KeySym symbol = XLookupKeysym(&event.xkey, 0);
                switch(symbol)
                {
                    case XK_Return: case XK_Escape:
                    {
                        XDestroyImage(window_buffer);
                        XCloseDisplay(display);
                        return 0;
                    } break;

                    case XK_h: shift(&game_state, LEFT);  break;
                    case XK_j: shift(&game_state, DOWN);  break;
                    case XK_k: shift(&game_state, UP);    break;
                    case XK_l: shift(&game_state, RIGHT); break;
                }
                play_animations(display, gc, window, window_buffer);

                // Reset rendering state after playing animations
                for(int i = 0; i < CELL_NUM; ++i)
                {
                    cells[i].active = false;
                }

                // TODO: This is here for debugging, remove at some point.
                XGrabKeyboard(display, window, 1, GrabModeAsync, GrabModeAsync, CurrentTime);

                for(int i = 0; i < CELL_NUM; ++i)
                {
                    u32 color_index;
                    if((color_index = game_state.data[i]) > 0)
                    {
                        int x = i % LENGTH;
                        int y = i / LENGTH;
                        push_cell(x * 200 + 1, y * 200 + 1, colors[color_index - 1], i);
                    }
                }
                render(window_buffer);
                XPutImage(display, window, gc, window_buffer, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            } break;
        }
    }
}
