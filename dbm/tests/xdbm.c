#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "dbm/gen.h"

#define WIDTH     800
#define HEIGHT    800
#define MARGIN    10
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

void draw_dbm(xinfo_t* xinfo, const int* dbm, int dim, int x1, int x2)
{
    double max = getMax(DBM(x1, 0), DBM(x2, 0));
    double scalex = (WIDTH - 2 * MARGIN) / max;
    double scaley = (HEIGHT - 2 * MARGIN) / max;
    /*int i,j;*/
    char buf[32];
    XPoint pt[6];
    double inf;

    setcolor(xinfo, "black");
    draw_line(xinfo, MARGIN, MARGIN, MARGIN, HEIGHT - MARGIN);
    draw_line(xinfo, MARGIN, HEIGHT - MARGIN, WIDTH - MARGIN, HEIGHT - MARGIN);

    /*
    setcolor(xinfo, "green");
    for (i = 0; i < max; i+=10)
      for (j = 0; j < max; j+=10)
        draw_point(xinfo, MARGIN+i*scalex, MARGIN+j*scaley);
    */

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

    setcolorij(xinfo, DBM(0, x1));
    draw_sline(xinfo, scalex, scaley, -(DBM(0, x1) >> 1), 0, -(DBM(0, x1) >> 1), max);
    sprintf(buf, "%d", -(DBM(0, x1) >> 1));
    setcolor(xinfo, "magenta");
    draw_string(xinfo, pt[0].x - strlen(buf) * 6, HEIGHT - 1, buf);

    if (DBM(x1, 0) != dbm_LS_INFINITY) {
        setcolorij(xinfo, DBM(x1, 0));
        draw_sline(xinfo, scalex, scaley, DBM(x1, 0) >> 1, 0, DBM(x1, 0) >> 1, max);
        sprintf(buf, "%d", DBM(x1, 0) >> 1);
        setcolor(xinfo, "magenta");
        draw_string(xinfo, pt[3].x, HEIGHT - 1, buf);
    }

    setcolorij(xinfo, DBM(0, x2));
    draw_sline(xinfo, scalex, scaley, 0, -(DBM(0, x2) >> 1), max, -(DBM(0, x2) >> 1));
    sprintf(buf, "%d", -(DBM(0, x2) >> 1));
    setcolor(xinfo, "magenta");
    draw_string(xinfo, 1, pt[0].y + 10, buf);

    if (DBM(x2, 0) != dbm_LS_INFINITY) {
        setcolorij(xinfo, DBM(x2, 0));
        draw_sline(xinfo, scalex, scaley, 0, DBM(x2, 0) >> 1, max, DBM(x2, 0) >> 1);
        sprintf(buf, "%d", DBM(x2, 0) >> 1);
        setcolor(xinfo, "magenta");
        draw_string(xinfo, 1, pt[3].y, buf);
    }

    if (DBM(x1, x2) != dbm_LS_INFINITY) {
        setcolorij(xinfo, DBM(x1, x2));
        draw_sline(xinfo, scalex, scaley, 0, -(DBM(x1, x2) >> 1), max + (DBM(x1, x2) >> 1), max);
        sprintf(buf, "%d", -(DBM(x1, x2) >> 1));
        setcolor(xinfo, "magenta");
        draw_string(xinfo, 1, HEIGHT - MARGIN + (DBM(x1, x2) >> 1) * scaley + 10, buf);
    }

    if (DBM(x2, x1) != dbm_LS_INFINITY) {
        setcolorij(xinfo, DBM(x2, x1));
        draw_sline(xinfo, scalex, scaley, 0, DBM(x2, x1) >> 1, max - (DBM(x2, x1) >> 1), max);
        sprintf(buf, "%d", DBM(x2, x1) >> 1);
        setcolor(xinfo, "magenta");
        draw_string(xinfo, 1, HEIGHT - MARGIN - (DBM(x2, x1) >> 1) * scaley, buf);
    }

    XFillPolygon(xinfo->display, xinfo->window, xinfo->gr_context, pt, 6, Convex, CoordModeOrigin);

    setcolor(xinfo, "black");
    sprintf(buf, "%d", x2);
    draw_string(xinfo, MARGIN, MARGIN, buf);
    sprintf(buf, "%d", x1);
    draw_string(xinfo, WIDTH - 2 * MARGIN, HEIGHT - 1, buf);
}

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
    dbm_generate(values, 3, 1000);
    XClearWindow(xinfo->display, xinfo->window);
    draw_dbm(xinfo, values, 3, 1, 2);
    XFlush(xinfo->display);
}

void event_loop(xinfo_t* xinfo)
{
    XEvent event;
    for (;;) {
        XNextEvent(xinfo->display, &event);
        switch (event.type) {
        case Expose: draw_dbm(xinfo, values, 3, 1, 2); break;

        case KeyPress: handle_key((XKeyEvent*)&event, xinfo); break;
        }
    }
}

int main()
{
    xinfo_t xinfo;
    init_xinfo(&xinfo);
    event_loop(&xinfo);
    return 0;
}
