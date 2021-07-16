// Cool hack to display federations of dimension 3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "dbm/dbm.h"
#include "dbm/print.h"

#define BORDER 1
#define SLEEP  1

// weak
#define W(B) ((B << 1) | 1)
// strict
#define S(B) (B << 1)

// What to display

/*
src:
<=0    <=-250  <-25    <=-535
<1707   <=0     <=721   <1172
<986    <592    <=0     <451
<2120   <1388   <2095   <=0
dst:
<=0     <-1081  <-69    <-151
<1589   <=0     <1520   <=1371
<=369   <-712   <=0     <=106
<=587   <=-644  <=218   <=0
src&dst:
<=0     <-1081  <-69    <=-535
<1589   <=0     <=721   <1172
<=369   <-712   <=0     <=106
<=587   <=-644  <=218   <=0
close(src&dst):
<=-1    <-1081  <-360   <=-535
<=1090  <=0     <=721   <=555
<=369   <-712   <=0     <=-166
<=446   <=-644  <=77    <=-89
*/

raw_t dbm41[16] = {W(0),   S(-1081), S(-69), S(-151), S(1589), W(0),    S(1520), W(1371),
                   W(369), S(-712),  W(0),   W(106),  W(587),  W(-644), W(218),  W(0)};
raw_t dbm42[16] = {W(0),   W(-250), S(-25), W(-535), S(1707), W(0),    W(721),  S(1172),
                   S(986), S(592),  W(0),   S(451),  S(2120), S(1388), S(2095), W(0)};

// Subtraction example

raw_t f1d1[9] = {W(0), S(-106), S(-199), S(107), W(0), S(-92), S(241), S(134), W(0)};
raw_t* start = f1d1;
raw_t f1d2[9] = {W(0), S(-19), S(-11), S(277), W(0), S(266), S(247), S(228), W(0)};
raw_t* f1[2] = {f1d1, f1d2};

raw_t f2d1[9] = {W(0), S(-74), W(-181), W(113), W(0), W(-88), W(201), S(127), W(0)};
raw_t* f2[1] = {f2d1};

raw_t f1f2d1[9] = {W(0), S(-106), S(-199), S(107), W(0), S(-92), W(201), S(95), W(0)};
raw_t f1f2d2[9] = {W(0), S(-74), W(-181), W(113), W(0), W(-88), W(201), S(127), W(0)};
raw_t* f1f2[2] = {f1f2d1, f1f2d2};

raw_t f1mf2d1[9] = {W(0), S(-19), S(-11), W(74), W(0), S(63), S(247), S(228), W(0)};
raw_t f1mf2d2[9] = {W(0), S(-74), S(-11), S(277), W(0), S(266), S(181), S(107), W(0)};
raw_t f1mf2d3[9] = {W(0), S(-93), W(-181), S(277), W(0), S(96), S(247), S(88), W(0)};
raw_t f1mf2d4[9] = {W(0), S(-74), S(-201), S(159), W(0), W(-88), S(247), S(173), W(0)};
raw_t f1mf2d5[9] = {
    W(0), S(-106), S(-201), S(107), W(0), S(-94), S(241), S(134), W(0),
};
raw_t* f1mf2[5] = {f1mf2d1, f1mf2d2, f1mf2d3, f1mf2d4, f1mf2d5};

raw_t f1mf4d1[9] = {W(0), S(-111), S(-199), S(277), W(0), S(78), S(247), S(88), W(0)};
raw_t f1mf4d2[9] = {W(0), W(-107), S(-201), S(159), W(0), W(-88), S(247), S(140), W(0)};
raw_t f1mf4d3[9] = {W(0), S(-106), S(-11), S(277), W(0), S(266), W(199), S(88), W(0)};
raw_t f1mf4d4[9] = {W(0), S(-19), S(-11), W(74), W(0), S(63), S(247), S(228), W(0)};
raw_t f1mf4d5[9] = {W(0), S(-74), S(-11), W(106), W(0), S(95), S(181), S(107), W(0)};
raw_t f1mf4d6[9] = {W(0), S(-93), W(-181), W(106), W(0), W(-75), S(194), S(88), W(0)};
raw_t f1mf4d7[9] = {W(0), S(-74), S(-201), W(106), W(0), S(-95), S(247), S(173), W(0)};
raw_t* f1mf4[7] = {f1mf4d1, f1mf4d2, f1mf4d3, f1mf4d4, f1mf4d5, f1mf4d6, f1mf4d7};

// f1, f2, f1&f2 (f1f2), f1-f2 (f1mf2), f1-(f1&f2) (f1mf4)

raw_t hull[16];

#define NB(F) (sizeof(F) / sizeof(raw_t*))

// X-Stuff

#define WIDTH     800
#define HEIGHT    800
#define MARGIN    12
#define DBM(I, J) dbm[(I)*dim + (J)]

typedef struct
{
    Display* display;
    int screen;
    Visual* visual;
    int depth;
    XSetWindowAttributes attributes;
    Window window;
    XFontStruct* fontinfo;
    XGCValues gr_values;
    GC gr_context;
    XColor color;
} xinfo_t;

void init_xinfo(xinfo_t* xinfo)
{
    XColor dummy;
    xinfo->display = XOpenDisplay(NULL);
    xinfo->screen = DefaultScreen(xinfo->display);
    xinfo->visual = DefaultVisual(xinfo->display, xinfo->screen);
    xinfo->depth = DefaultDepth(xinfo->display, xinfo->screen);
    xinfo->attributes.background_pixel = XWhitePixel(xinfo->display, xinfo->screen);
    xinfo->window = XCreateWindow(xinfo->display, XRootWindow(xinfo->display, xinfo->screen), 200,
                                  200, WIDTH, HEIGHT, 5, xinfo->depth, InputOutput, xinfo->visual,
                                  CWBackPixel, &xinfo->attributes);
    XSelectInput(xinfo->display, xinfo->window, ExposureMask | KeyPressMask);
    xinfo->fontinfo = XLoadQueryFont(xinfo->display, "6x10");
    XAllocNamedColor(xinfo->display, DefaultColormap(xinfo->display, xinfo->screen), "black",
                     &xinfo->color, &dummy);
    xinfo->gr_values.font = xinfo->fontinfo->fid;
    xinfo->gr_values.foreground = xinfo->color.pixel;
    xinfo->gr_context =
        XCreateGC(xinfo->display, xinfo->window, GCFont | GCForeground, &xinfo->gr_values);
    XFlush(xinfo->display);
    XMapWindow(xinfo->display, xinfo->window);
    XFlush(xinfo->display);
}

void setcolor(xinfo_t* xinfo, const char* col)
{
    XColor dummy;
    XFreeGC(xinfo->display, xinfo->gr_context);
    XAllocNamedColor(xinfo->display, DefaultColormap(xinfo->display, xinfo->screen), col,
                     &xinfo->color, &dummy);
    xinfo->gr_values.foreground = xinfo->color.pixel;
    xinfo->gr_context =
        XCreateGC(xinfo->display, xinfo->window, GCFont | GCForeground, &xinfo->gr_values);
}

void draw_point(xinfo_t* xinfo, int x, int y)
{
    XDrawPoint(xinfo->display, xinfo->window, xinfo->gr_context, x, y);
}

void draw_line(xinfo_t* xinfo, int x1, int y1, int x2, int y2)
{
    XDrawLine(xinfo->display, xinfo->window, xinfo->gr_context, x1, y1, x2, y2);
}

void draw_string(xinfo_t* xinfo, int x, int y, const char* str)
{
    XDrawString(xinfo->display, xinfo->window, xinfo->gr_context, x, y, str, strlen(str));
}

int values[] = {1, 1, 1, dbm_LS_INFINITY, 1, dbm_LS_INFINITY, dbm_LS_INFINITY, dbm_LS_INFINITY, 1};

void setcolorij(xinfo_t* xinfo, int cij) { setcolor(xinfo, (cij & 1) ? "blue" : "red"); }

void draw_sline(xinfo_t* xinfo, double rx, double ry, int x1, int y1, int x2, int y2)
{
    draw_line(xinfo, MARGIN + x1 * rx, HEIGHT - MARGIN - ry * y1, MARGIN + x2 * rx,
              HEIGHT - MARGIN - ry * y2);
}

double getMax(int c1, int c2)
{
    if (c1 < dbm_LS_INFINITY && c1 > c2)
        return c1 >> 1;
    if (c2 < dbm_LS_INFINITY)
        return c2 >> 1;
    return 1000;
}

void getScale(const int* dbm, int dim, int x1, int x2, double* sx, double* sy)
{
    double max = getMax(DBM(x1, 0), DBM(x2, 0));
    *sx = (WIDTH - 2 * MARGIN) / max;
    *sy = (HEIGHT - 2 * MARGIN) / max;
}

void dbm_draw(xinfo_t* xinfo, const int* dbm, int dim, int x1, int x2, const char* dbmCol,
              int border, double scalex, double scaley)
{
    XPoint pt[6];
    char buf[32];
    double inf;
    double max = getMax(DBM(x1, 0), DBM(x2, 0));

    pt[0].x = MARGIN - (DBM(0, x1) >> 1) * scalex;
    pt[0].y = HEIGHT - MARGIN + (DBM(0, x2) >> 1) * scaley;

    pt[1].x = DBM(x1, x2) < dbm_LS_INFINITY
                  ? MARGIN + scalex * ((DBM(x1, x2) >> 1) - (DBM(0, x2) >> 1))
                  : WIDTH;
    pt[1].y = pt[0].y;

    inf = DBM(x1, 0) < dbm_LS_INFINITY ? (DBM(x1, 0) >> 1) : max;
    pt[2].x = DBM(x1, 0) < dbm_LS_INFINITY ? MARGIN + (DBM(x1, 0) >> 1) * scalex : WIDTH;
    pt[2].y = DBM(x1, x2) < dbm_LS_INFINITY ? HEIGHT - MARGIN - (inf - (DBM(x1, x2) >> 1)) * scaley
                                            : HEIGHT - MARGIN;

    pt[3].x = pt[2].x;
    pt[3].y = DBM(x2, 0) < dbm_LS_INFINITY ? HEIGHT - MARGIN - (DBM(x2, 0) >> 1) * scaley : 0;

    inf = DBM(x2, 0) < dbm_LS_INFINITY ? (DBM(x2, 0) >> 1) : max;
    pt[4].x = DBM(x2, x1) < dbm_LS_INFINITY ? MARGIN + (inf - (DBM(x2, x1) >> 1)) * scalex : WIDTH;
    pt[4].y = pt[3].y;

    pt[5].x = pt[0].x;
    pt[5].y = DBM(x2, x1) < dbm_LS_INFINITY
                  ? HEIGHT - MARGIN - ((DBM(x2, x1) >> 1) - (DBM(0, x1) >> 1)) * scaley
                  : 0;

    setcolor(xinfo, dbmCol);
    XFillPolygon(xinfo->display, xinfo->window, xinfo->gr_context, pt, 6, Convex, CoordModeOrigin);

    if (border) {
        setcolorij(xinfo, DBM(0, x1));
        draw_sline(xinfo, scalex, scaley, -(DBM(0, x1) >> 1), 0, -(DBM(0, x1) >> 1), max);
        sprintf(buf, "%d", -(DBM(0, x1) >> 1));
        setcolor(xinfo, dbmCol);
        draw_string(xinfo, pt[0].x - strlen(buf) * 6, HEIGHT - 1, buf);

        if (DBM(x1, 0) != dbm_LS_INFINITY) {
            setcolorij(xinfo, DBM(x1, 0));
            draw_sline(xinfo, scalex, scaley, DBM(x1, 0) >> 1, 0, DBM(x1, 0) >> 1, max);
            sprintf(buf, "%d", DBM(x1, 0) >> 1);
            setcolor(xinfo, dbmCol);
            draw_string(xinfo, pt[3].x, HEIGHT - 1, buf);
        }

        setcolorij(xinfo, DBM(0, x2));
        draw_sline(xinfo, scalex, scaley, 0, -(DBM(0, x2) >> 1), max, -(DBM(0, x2) >> 1));
        sprintf(buf, "%d", -(DBM(0, x2) >> 1));
        setcolor(xinfo, dbmCol);
        draw_string(xinfo, 1, pt[0].y + 10, buf);

        if (DBM(x2, 0) != dbm_LS_INFINITY) {
            setcolorij(xinfo, DBM(x2, 0));
            draw_sline(xinfo, scalex, scaley, 0, DBM(x2, 0) >> 1, max, DBM(x2, 0) >> 1);
            sprintf(buf, "%d", DBM(x2, 0) >> 1);
            setcolor(xinfo, dbmCol);
            draw_string(xinfo, 1, pt[3].y, buf);
        }

        if (DBM(x1, x2) != dbm_LS_INFINITY) {
            setcolorij(xinfo, DBM(x1, x2));
            draw_sline(xinfo, scalex, scaley, 0, -(DBM(x1, x2) >> 1), max + (DBM(x1, x2) >> 1),
                       max);
            sprintf(buf, "%d", -(DBM(x1, x2) >> 1));
            setcolor(xinfo, dbmCol);
            draw_string(xinfo, 1, HEIGHT - MARGIN + (DBM(x1, x2) >> 1) * scaley + 10, buf);
        }

        if (DBM(x2, x1) != dbm_LS_INFINITY) {
            setcolorij(xinfo, DBM(x2, x1));
            draw_sline(xinfo, scalex, scaley, 0, DBM(x2, x1) >> 1, max - (DBM(x2, x1) >> 1), max);
            sprintf(buf, "%d", DBM(x2, x1) >> 1);
            setcolor(xinfo, dbmCol);
            draw_string(xinfo, 1, HEIGHT - MARGIN - (DBM(x2, x1) >> 1) * scaley, buf);
        }
    }

    /*
    setcolor(xinfo, "black");
    sprintf(buf, "%d", x2);
    draw_string(xinfo, MARGIN-10, MARGIN, buf);
    sprintf(buf, "%d", x1);
    draw_string(xinfo, WIDTH-2*MARGIN, HEIGHT-1, buf);
    */
}

void fed_draw(xinfo_t* xinfo, int** dbm, int n, int dim, int x1, int x2, const char* dbmCol,
              int border, double scalex, double scaley)
{
    int i;
    printf("%d and %d of DBM %dx%d:\n", x1, x2, dim, dim);
    for (i = 0; i < n; ++i) {
        sleep(SLEEP);
        dbm_draw(xinfo, dbm[i], dim, x1, x2, dbmCol, border, scalex, scaley);
        dbm_print(stdout, dbm[i], dim);
        if (i + 1 < n)
            printf("|\n");
    }
}

void init_draw(xinfo_t* xinfo, const int* dbm, int dim, int x1, int x2, double scalex,
               double scaley)
{
    setcolor(xinfo, "black");
    draw_line(xinfo, MARGIN, MARGIN, MARGIN, HEIGHT - MARGIN);
    draw_line(xinfo, MARGIN, HEIGHT - MARGIN, WIDTH - MARGIN, HEIGHT - MARGIN);
    // dbm_draw(xinfo, dbm, dim, x1, x1, "lightBlue", 0, scalex, scaley);
}

#define UNION(F)                          \
    for (i = 0, n = NB(F); i < n; ++i) {  \
        dbm_convexUnion(hull, F[i], dim); \
    }

void init(xinfo_t* xinfo, int dim, int x1, int x2, double* scalex, double* scaley)
{
    int i, n;
    // convex hull
    dbm_copy(hull, start, dim);
    UNION(f1);
    UNION(f2);
    UNION(f1f2);
    UNION(f1mf2);
    UNION(f1mf4);
    getScale(hull, dim, x1, x2, scalex, scaley);
}

void draw1(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    printf("f1\n");
    fed_draw(xinfo, f1, NB(f1), dim, x1, x2, "lightGreen", BORDER, sx, sy);
}
void draw2(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    printf("f2\n");
    fed_draw(xinfo, f2, NB(f2), dim, x1, x2, "lightBlue", BORDER, sx, sy);
}
void draw3(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    draw1(xinfo, dim, x1, x2, sx, sy);
    draw2(xinfo, dim, x1, x2, sx, sy);
}
void draw4(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    draw3(xinfo, dim, x1, x2, sx, sy);
    printf("f1&f2\n");
    fed_draw(xinfo, f1f2, NB(f1f2), dim, x1, x2, "darkGreen", BORDER, sx, sy);
}
void draw5(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    draw3(xinfo, dim, x1, x2, sx, sy);
    printf("f1-f2\n");
    fed_draw(xinfo, f1mf2, NB(f1mf2), dim, x1, x2, "magenta", BORDER, sx, sy);
}
void draw6(xinfo_t* xinfo, int dim, int x1, int x2, double sx, double sy)
{
    printf("f1-(f1&f2)\n");
    draw3(xinfo, dim, x1, x2, sx, sy);
    fed_draw(xinfo, f1mf4, NB(f1mf4), dim, x1, x2, "magenta", BORDER, sx, sy);
}

int draw = 3;
double scaleX, scaleY;

void drawX2(xinfo_t* xinfo, int dim, int x1, int x2)
{
    dbm_copy(hull, dbm41, dim);
    dbm_convexUnion(hull, dbm42, dim);
    getScale(hull, dim, x1, x2, &scaleX, &scaleY);
    init_draw(xinfo, hull, dim, x1, x2, scaleX, scaleY);
    dbm_draw(xinfo, dbm41, dim, x1, x2, "lightBlue", BORDER, scaleX, scaleY);
    dbm_draw(xinfo, dbm42, dim, x1, x2, "lightGreen", BORDER, scaleX, scaleY);
}

void drawX(xinfo_t* xinfo, int dim, int x1, int x2)
{
    init_draw(xinfo, hull, dim, x1, x2, scaleX, scaleY);
    switch (draw) {
    case 1: draw1(xinfo, dim, x1, x2, scaleX, scaleY); break;
    case 2: draw2(xinfo, dim, x1, x2, scaleX, scaleY); break;
    case 3: draw3(xinfo, dim, x1, x2, scaleX, scaleY); break;
    case 4: draw4(xinfo, dim, x1, x2, scaleX, scaleY); break;
    case 5: draw5(xinfo, dim, x1, x2, scaleX, scaleY); break;
    case 6: draw6(xinfo, dim, x1, x2, scaleX, scaleY); break;
    }
}

int xi[3] = {1, 2, 1};
int yi[3] = {2, 3, 3};
int drawi = 0;

void handle_key(XKeyEvent* event, xinfo_t* xinfo)
{
    char buffer[80];
    KeySym keysym;

    XLookupString(event, buffer, sizeof(buffer), &keysym, NULL);
    if (keysym == XK_Escape) {
        XFreeGC(xinfo->display, xinfo->gr_context);
        XUnmapWindow(xinfo->display, xinfo->window);
        XDestroyWindow(xinfo->display, xinfo->window);
        XCloseDisplay(xinfo->display);
        exit(0);
    }
    XClearWindow(xinfo->display, xinfo->window);
    // if (++draw > 6) draw = 3;
    if (++drawi > 2)
        drawi = 0;
    // drawX(xinfo, 3, 1, 2);
    drawX2(xinfo, 4, xi[drawi], yi[drawi]);
    XFlush(xinfo->display);
}

void event_loop(xinfo_t* xinfo)
{
    XEvent event;
    for (;;) {
        XNextEvent(xinfo->display, &event);
        switch (event.type) {
        case Expose:
            // drawX(xinfo, 3, 1, 2);
            drawX2(xinfo, 4, xi[drawi], yi[drawi]);
            break;

        case KeyPress: handle_key((XKeyEvent*)&event, xinfo); break;
        }
    }
}

int main()
{
    xinfo_t xinfo;
    init_xinfo(&xinfo);
    // init(&xinfo, 3, 1, 2, &scaleX, &scaleY);
    event_loop(&xinfo);
    return 0;
}
