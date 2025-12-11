/*
 * Nuklear - v1.40.8 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2017 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_XLIB_H_
#define NK_XLIB_H_

#include <X11/Xlib.h>

typedef struct XFont XFont;
#ifdef NK_XLIB_USE_XFT
NK_API struct nk_context*   nk_xlib_init(XFont*, Display*, int scrn, Window root, Visual *vis, Colormap cmap, unsigned w, unsigned h);
#else
NK_API struct nk_context*   nk_xlib_init(XFont*, Display*, int scrn, Window root, unsigned w, unsigned h);
#endif
NK_API int                  nk_xlib_handle_event(Display*, int scrn, Window, XEvent*);
NK_API void                 nk_xlib_render(Drawable screen, struct nk_color clear);
NK_API void                 nk_xlib_shutdown(void);
NK_API void                 nk_xlib_set_font(XFont*);
NK_API void                 nk_xlib_push_font(XFont*);
NK_API void                 nk_xlib_paste(nk_handle, struct nk_text_edit*);
NK_API void                 nk_xlib_copy(nk_handle, const char*, int len);

/* Image */
#ifdef NK_XLIB_INCLUDE_STB_IMAGE
NK_API struct nk_image nk_xsurf_load_image_from_file(char const *filename);
NK_API struct nk_image nk_xsurf_load_image_from_memory(const void *membuf, nk_uint membufSize);
#endif

/* Font */
NK_API XFont*               nk_xfont_create(Display *dpy, const char *name);
NK_API void                 nk_xfont_del(Display *dpy, XFont *font);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_XLIB_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>
#include <X11/Xatom.h>

#ifdef NK_XLIB_USE_XFT
#include <X11/Xft/Xft.h>
#endif

#include <sys/time.h>
#include <unistd.h>
#include <time.h>


#ifdef NK_XLIB_IMPLEMENT_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#endif

#ifdef NK_XLIB_INCLUDE_STB_IMAGE
#include "../../example/stb_image.h"
#endif


#ifndef NK_X11_DOUBLE_CLICK_LO
#define NK_X11_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_X11_DOUBLE_CLICK_HI
#define NK_X11_DOUBLE_CLICK_HI 0.20
#endif

typedef struct XSurface XSurface;
typedef struct XImageWithAlpha XImageWithAlpha;
struct XFont {
    int ascent;
    int descent;
    int height;
#ifdef NK_XLIB_USE_XFT
    XftFont * ft;
#else
    XFontSet set;
    XFontStruct *xfont;
#endif
    struct nk_user_font handle;
};
struct XSurface {
    GC gc;
    Display *dpy;
    int screen;
    Window root;
    Drawable drawable;
    unsigned int w, h;
#ifdef NK_XLIB_USE_XFT
    XftDraw * ftdraw;
#endif
};
struct XImageWithAlpha {
    XImage* ximage;
    GC clipMaskGC;
    Pixmap clipMask;
};
static struct  {
    char *clipboard_data;
    int clipboard_len;
    struct nk_text_edit* clipboard_target;

    Atom xa_clipboard;
    Atom xa_targets;
    Atom xa_text;
    Atom xa_utf8_string;

    struct nk_context ctx;
    struct XSurface *surf;
    Cursor cursor;
    Display *dpy;
    Window root;
#ifdef NK_XLIB_USE_XFT
    Visual *vis;
    Colormap cmap;
#endif
    double last_button_click;
    double time_of_last_frame;
} xlib;

NK_INTERN double
/**
 * Get current time in seconds since the Unix epoch with microsecond precision.
 * @returns Current time in seconds since the Unix epoch as a `double`; returns `0` on error.
 */
nk_get_time(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return ((double)tv.tv_sec + (double)tv.tv_usec/1000000);
}

/**
 * Convert a 3-byte RGB color to an X11-compatible pixel value.
 * @param c Pointer to three bytes representing red, green, and blue components in that order.
 * @returns An unsigned long pixel value with red in bits 16–23, green in bits 8–15, and blue in bits 0–7.
 */
NK_INTERN unsigned long
nk_color_from_byte(const nk_byte *c)
{
    unsigned long res = 0;
    res |= (unsigned long)c[0] << 16;
    res |= (unsigned long)c[1] << 8;
    res |= (unsigned long)c[2] << 0;
    return (res);
}

/**
 * Create an off-screen X rendering surface with the given dimensions for the specified screen.
 *
 * Allocates and initializes an XSurface structure, creates a graphics context and an off-screen
 * pixmap sized w-by-h (and an XftDraw when XFT is enabled). The returned surface holds the
 * display, screen and root references used for subsequent drawing and blitting.
 *
 * @param screen X11 screen number to use for visuals and default depth.
 * @param w      Width in pixels of the off-screen surface.
 * @param h      Height in pixels of the off-screen surface.
 *
 * @returns Pointer to a newly allocated XSurface populated with X resources (GC, pixmap, etc.).
 *          The caller is responsible for releasing these resources (e.g., via nk_xsurf_del).
 */
NK_INTERN XSurface*
nk_xsurf_create(int screen, unsigned int w, unsigned int h)
{
    XSurface *surface = (XSurface*)calloc(1, sizeof(XSurface));
    surface->w = w;
    surface->h = h;
    surface->dpy = xlib.dpy;
    surface->screen = screen;
    surface->root = xlib.root;
    surface->gc = XCreateGC(xlib.dpy, xlib.root, 0, NULL);
    XSetLineAttributes(xlib.dpy, surface->gc, 1, LineSolid, CapButt, JoinMiter);
    surface->drawable = XCreatePixmap(xlib.dpy, xlib.root, w, h,
        (unsigned int)DefaultDepth(xlib.dpy, screen));
#ifdef NK_XLIB_USE_XFT
    surface->ftdraw = XftDrawCreate(xlib.dpy, surface->drawable,
                                xlib.vis, xlib.cmap);
#endif
    return surface;
}

/**
 * Resize the XSurface's backing drawable to the specified pixel dimensions.
 *
 * Frees the existing pixmap (if any), updates the surface width and height,
 * and creates a new pixmap matching the new dimensions and the display depth.
 * When Xft is enabled, the XftDraw associated with the surface is updated
 * to target the new drawable.
 *
 * @param surf Pointer to the XSurface to resize.
 * @param w New width in pixels.
 * @param h New height in pixels.
 */
NK_INTERN void
nk_xsurf_resize(XSurface *surf, unsigned int w, unsigned int h)
{
    if(!surf) return;
    if (surf->w == w && surf->h == h) return;
    surf->w = w; surf->h = h;
    if(surf->drawable) XFreePixmap(surf->dpy, surf->drawable);
    surf->drawable = XCreatePixmap(surf->dpy, surf->root, w, h,
        (unsigned int)DefaultDepth(surf->dpy, surf->screen));
#ifdef NK_XLIB_USE_XFT
    XftDrawChange(surf->ftdraw, surf->drawable);
#endif
}

/**
 * Set the current drawing clip to the given rectangle (in surface pixels),
 * expanding the rectangle by 1 pixel on all sides.
 *
 * Subsequent drawing operations on the surface are constrained to this clip.
 * When Xft is enabled, the clip is applied to both the surface GC and the
 * XftDraw associated with the surface.
 *
 * @param surf Target surface whose clipping region will be set.
 * @param x    X coordinate of the clip rectangle origin (pixels).
 * @param y    Y coordinate of the clip rectangle origin (pixels).
 * @param w    Width of the clip rectangle (pixels).
 * @param h    Height of the clip rectangle (pixels).
 */
NK_INTERN void
nk_xsurf_scissor(XSurface *surf, float x, float y, float w, float h)
{
    XRectangle clip_rect;
    clip_rect.x = (short)(x-1);
    clip_rect.y = (short)(y-1);
    clip_rect.width = (unsigned short)(w+2);
    clip_rect.height = (unsigned short)(h+2);
    XSetClipRectangles(surf->dpy, surf->gc, 0, 0, &clip_rect, 1, Unsorted);

#ifdef NK_XLIB_USE_XFT
    XftDrawSetClipRectangles(surf->ftdraw, 0, 0, &clip_rect, 1);
#endif
}

/**
 * Draw a straight stroked line on the given surface.
 *
 * Draws a line between (x0, y0) and (x1, y1) onto surf using the specified
 * line_thickness and color.
 *
 * @param surf Target XSurface to draw into.
 * @param x0   X coordinate of the start point.
 * @param y0   Y coordinate of the start point.
 * @param x1   X coordinate of the end point.
 * @param y1   Y coordinate of the end point.
 * @param line_thickness Thickness of the stroked line in pixels.
 * @param col  Color to use for the line.
 */
NK_INTERN void
nk_xsurf_stroke_line(XSurface *surf, short x0, short y0, short x1,
    short y1, unsigned int line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, (int)x0, (int)y0, (int)x1, (int)y1);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Draw a stroked rectangle on the given XSurface, optionally with rounded corners.
 *
 * @param surf Target surface on which to draw.
 * @param x X coordinate of the rectangle's top-left corner.
 * @param y Y coordinate of the rectangle's top-left corner.
 * @param w Width of the rectangle in pixels.
 * @param h Height of the rectangle in pixels.
 * @param r Corner radius in pixels; if `r` is 0 a standard rectangle is drawn.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Stroke color.
 */
NK_INTERN void
nk_xsurf_stroke_rect(XSurface* surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    if (r == 0) {XDrawRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);return;}

    {short xc = x + r;
    short yc = y + r;
    short wc = (short)(w - 2 * r);
    short hc = (short)(h - 2 * r);

    XDrawLine(surf->dpy, surf->drawable, surf->gc, xc, y, xc+wc, y);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x+w, yc, x+w, yc+hc);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, xc, y+h, xc+wc, y+h);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x, yc, x, yc+hc);

    XDrawArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, y,
        (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, x, y,
        (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, x, yc + hc - r,
        (unsigned)r*2, (unsigned)2*r, 180 * 64, 90 * 64);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, yc + hc - r,
        (unsigned)r*2, (unsigned)2*r, -90 * 64, 90 * 64);}
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Fill a rectangle on the surface, optionally with rounded corners.
 *
 * Draws a filled rectangle at (x, y) with size (w, h) using color `col`.
 * If `r` is zero the rectangle is filled with square corners; otherwise the
 * rectangle is filled with corners rounded by radius `r` (in pixels).
 *
 * @param surf Target drawing surface.
 * @param x X coordinate of the rectangle's top-left corner.
 * @param y Y coordinate of the rectangle's top-left corner.
 * @param w Width of the rectangle in pixels.
 * @param h Height of the rectangle in pixels.
 * @param r Corner radius in pixels; when 0 corners are not rounded.
 * @param col Fill color.
 */
NK_INTERN void
nk_xsurf_fill_rect(XSurface* surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    if (r == 0) {XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h); return;}

    {short xc = x + r;
    short yc = y + r;
    short wc = (short)(w - 2 * r);
    short hc = (short)(h - 2 * r);

    XPoint pnts[12];
    pnts[0].x = x;
    pnts[0].y = yc;
    pnts[1].x = xc;
    pnts[1].y = yc;
    pnts[2].x = xc;
    pnts[2].y = y;

    pnts[3].x = xc + wc;
    pnts[3].y = y;
    pnts[4].x = xc + wc;
    pnts[4].y = yc;
    pnts[5].x = x + w;
    pnts[5].y = yc;

    pnts[6].x = x + w;
    pnts[6].y = yc + hc;
    pnts[7].x = xc + wc;
    pnts[7].y = yc + hc;
    pnts[8].x = xc + wc;
    pnts[8].y = y + h;

    pnts[9].x = xc;
    pnts[9].y = y + h;
    pnts[10].x = xc;
    pnts[10].y = yc + hc;
    pnts[11].x = x;
    pnts[11].y = yc + hc;

    XFillPolygon(surf->dpy, surf->drawable, surf->gc, pnts, 12, Convex, CoordModeOrigin);
    XFillArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, y,
        (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, x, y,
        (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, x, yc + hc - r,
        (unsigned)r*2, (unsigned)2*r, 180 * 64, 90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, yc + hc - r,
        (unsigned)r*2, (unsigned)2*r, -90 * 64, 90 * 64);}
}

/**
 * Fill a triangle on the given surface using a solid color.
 *
 * Draws and fills the triangle defined by the three vertex coordinates in
 * surface pixel space.
 *
 * @param surf Target XSurface to draw into.
 * @param x0   X coordinate of the first vertex (pixels).
 * @param y0   Y coordinate of the first vertex (pixels).
 * @param x1   X coordinate of the second vertex (pixels).
 * @param y1   Y coordinate of the second vertex (pixels).
 * @param x2   X coordinate of the third vertex (pixels).
 * @param y2   Y coordinate of the third vertex (pixels).
 * @param col  Fill color to use for the triangle.
 */
NK_INTERN void
nk_xsurf_fill_triangle(XSurface *surf, short x0, short y0, short x1,
    short y1, short x2, short y2, struct nk_color col)
{
    XPoint pnts[3];
    unsigned long c = nk_color_from_byte(&col.r);
    pnts[0].x = (short)x0;
    pnts[0].y = (short)y0;
    pnts[1].x = (short)x1;
    pnts[1].y = (short)y1;
    pnts[2].x = (short)x2;
    pnts[2].y = (short)y2;
    XSetForeground(surf->dpy, surf->gc, c);
    XFillPolygon(surf->dpy, surf->drawable, surf->gc, pnts, 3, Convex, CoordModeOrigin);
}

/**
 * Draws the outline of a triangle on the given XSurface using the specified vertex coordinates, stroke thickness, and color.
 * 
 * @param surf Surface to draw onto.
 * @param x0 X coordinate of the first vertex.
 * @param y0 Y coordinate of the first vertex.
 * @param x1 X coordinate of the second vertex.
 * @param y1 Y coordinate of the second vertex.
 * @param x2 X coordinate of the third vertex.
 * @param y2 Y coordinate of the third vertex.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Color used for the triangle outline.
 */
NK_INTERN void
nk_xsurf_stroke_triangle(XSurface *surf, short x0, short y0, short x1,
    short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x0, y0, x1, y1);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x1, y1, x2, y2);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x2, y2, x0, y0);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Fill a polygon on the given XSurface using the supplied points and color.
 * @param surf Target surface to render into.
 * @param pnts Array of integer 2D points that define the polygon vertices.
 * @param count Number of points in `pnts`; only the first 128 points will be used.
 * @param col Fill color to apply to the polygon.
 */
NK_INTERN void
nk_xsurf_fill_polygon(XSurface *surf,  const struct nk_vec2i *pnts, int count,
    struct nk_color col)
{
    int i = 0;
    #define MAX_POINTS 128
    XPoint xpnts[MAX_POINTS];
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    for (i = 0; i < count && i < MAX_POINTS; ++i) {
        xpnts[i].x = pnts[i].x;
        xpnts[i].y = pnts[i].y;
    }
    XFillPolygon(surf->dpy, surf->drawable, surf->gc, xpnts, count, Convex, CoordModeOrigin);
    #undef MAX_POINTS
}

/**
 * Draws a closed, stroked polygon onto the given X surface.
 *
 * Connects the sequence of integer 2D points in `pnts` (length `count`) with
 * straight line segments and closes the shape by connecting the last point to
 * the first. The stroke uses `line_thickness` and `col` as the color. The
 * surface's graphics context line attributes are restored to a thickness of 1
 * after drawing.
 *
 * @param surf Target XSurface to draw into.
 * @param pnts Array of points defining the polygon vertices (must contain at least 1 point).
 * @param count Number of points in `pnts`.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Stroke color.
 */
NK_INTERN void
nk_xsurf_stroke_polygon(XSurface *surf, const struct nk_vec2i *pnts, int count,
    unsigned short line_thickness, struct nk_color col)
{
    int i = 0;
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    for (i = 1; i < count; ++i)
        XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[i-1].x, pnts[i-1].y, pnts[i].x, pnts[i].y);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[count-1].x, pnts[count-1].y, pnts[0].x, pnts[0].y);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Draws a polyline by connecting successive points with the specified thickness and color.
 * 
 * @param surf Target XSurface to draw on.
 * @param pnts Array of points (length at least `count`) defining the polyline vertices.
 * @param count Number of points in `pnts`.
 * @param line_thickness Line thickness in pixels.
 * @param col Color used to draw the polyline.
 */
NK_INTERN void
nk_xsurf_stroke_polyline(XSurface *surf, const struct nk_vec2i *pnts,
    int count, unsigned short line_thickness, struct nk_color col)
{
    int i = 0;
    unsigned long c = nk_color_from_byte(&col.r);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XSetForeground(surf->dpy, surf->gc, c);
    for (i = 0; i < count-1; ++i)
        XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[i].x, pnts[i].y, pnts[i+1].x, pnts[i+1].y);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Fill an ellipse on the given surface at the specified position and size using the provided color.
 *
 * @param surf Target XSurface to draw into.
 * @param x X coordinate of the top-left corner of the ellipse's bounding rectangle (pixels).
 * @param y Y coordinate of the top-left corner of the ellipse's bounding rectangle (pixels).
 * @param w Width of the ellipse's bounding rectangle (pixels).
 * @param h Height of the ellipse's bounding rectangle (pixels).
 * @param col Color used to fill the ellipse.
 */
NK_INTERN void
nk_xsurf_fill_circle(XSurface *surf, short x, short y, unsigned short w,
    unsigned short h, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}

/**
 * Draws an outlined ellipse (circle when width equals height) on the given XSurface.
 *
 * @param surf Target surface to draw into.
 * @param x X coordinate of the top-left corner of the ellipse bounding box.
 * @param y Y coordinate of the top-left corner of the ellipse bounding box.
 * @param w Width of the ellipse bounding box in pixels.
 * @param h Height of the ellipse bounding box in pixels.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Stroke color.
 */
NK_INTERN void
nk_xsurf_stroke_circle(XSurface *surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Draws an outlined circular arc on the surface.
 * 
 * @param surf Surface to draw onto.
 * @param cx X coordinate of the arc center.
 * @param cy Y coordinate of the arc center.
 * @param radius Radius of the arc in pixels.
 * @param a_min Start angle in radians.
 * @param a_max Sweep angle in radians (angular extent to draw).
 * @param line_thickness Thickness of the arc stroke in pixels.
 * @param col Color used to draw the arc.
 */
NK_INTERN void
nk_xsurf_stroke_arc(XSurface *surf, short cx, short cy, unsigned short radius,
    float a_min, float a_max, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, (int)(cx - radius), (int)(cy - radius),
        (unsigned)(radius * 2), (unsigned)(radius * 2),
        (int)(a_min * 180 * 64 / NK_PI), (int)(a_max * 180 * 64 / NK_PI));
}

/**
 * Fill an arc (pie slice) on the surface.
 *
 * Draws and fills the arc defined by a circle centered at (cx, cy) with the given radius,
 * starting at angle `a_min` and sweeping by `a_max`, using `col` as the fill color.
 *
 * @param surf Target XSurface to draw on.
 * @param cx X coordinate of the arc center.
 * @param cy Y coordinate of the arc center.
 * @param radius Radius of the arc in pixels.
 * @param a_min Start angle in radians.
 * @param a_max Sweep (extent) angle in radians (the angle to draw from `a_min`).
 * @param col Fill color used for the arc.
 */
NK_INTERN void
nk_xsurf_fill_arc(XSurface *surf, short cx, short cy, unsigned short radius,
    float a_min, float a_max, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)(cx - radius), (int)(cy - radius),
        (unsigned)(radius * 2), (unsigned)(radius * 2),
        (int)(a_min * 180 * 64 / NK_PI), (int)(a_max * 180 * 64 / NK_PI));
}

/**
 * Draws a cubic Bezier curve onto the given XSurface by approximating it with line segments.
 *
 * The curve is defined by control points p1 (start), p2, p3 and p4 (end). The curve is rendered
 * as a sequence of straight segments; `num_segments` controls the tessellation density (values
 * less than 1 are treated as 1). The drawing uses the specified line thickness and color.
 *
 * @param surf Target surface to draw on.
 * @param p1 First control point (start point).
 * @param p2 Second control point.
 * @param p3 Third control point.
 * @param p4 Fourth control point (end point).
 * @param num_segments Number of straight segments to approximate the curve.
 * @param line_thickness Line thickness in pixels.
 * @param col Color used to stroke the curve.
 */
NK_INTERN void
nk_xsurf_stroke_curve(XSurface *surf, struct nk_vec2i p1,
    struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
    unsigned int num_segments, unsigned short line_thickness, struct nk_color col)
{
    unsigned int i_step;
    float t_step;
    struct nk_vec2i last = p1;

    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    num_segments = NK_MAX(num_segments, 1);
    t_step = 1.0f/(float)num_segments;
    for (i_step = 1; i_step <= num_segments; ++i_step) {
        float t = t_step * (float)i_step;
        float u = 1.0f - t;
        float w1 = u*u*u;
        float w2 = 3*u*u*t;
        float w3 = 3*u*t*t;
        float w4 = t * t *t;
        float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        nk_xsurf_stroke_line(surf, last.x, last.y, (short)x, (short)y, line_thickness,col);
        last.x = (short)x; last.y = (short)y;
    }
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

/**
 * Draws text onto the given XSurface at the specified position using the provided font and color.
 *
 * If `text` is NULL, `font` is NULL, or `len` is zero, the function does nothing.
 *
 * @param surf Target surface to draw onto.
 * @param x Horizontal position (pixels) for the left edge of the text.
 * @param y Vertical position (pixels) representing the top edge of the text; the function
 *          advances internally to the font baseline when rendering.
 * @param text Pointer to the character data to draw.
 * @param len Number of bytes from `text` to draw.
 * @param font Font to use for measuring and rendering the text.
 * @param cfg Color used to render the text. */
NK_INTERN void
nk_xsurf_draw_text(XSurface *surf, short x, short y, const char *text, int len,
    XFont *font, struct nk_color cfg)
{
    int tx, ty;
#ifdef NK_XLIB_USE_XFT
    XRenderColor xrc;
    XftColor color;
#else
    unsigned long fg = nk_color_from_byte(&cfg.r);
#endif

    if(!text || !font || !len) return;

    tx = (int)x;
    ty = (int)y + font->ascent;
#ifdef NK_XLIB_USE_XFT
    xrc.red = cfg.r * 257;
    xrc.green = cfg.g * 257;
    xrc.blue = cfg.b * 257;
    xrc.alpha = cfg.a * 257;
    XftColorAllocValue(surf->dpy, xlib.vis, xlib.cmap, &xrc, &color);
    XftDrawStringUtf8(surf->ftdraw, &color, font->ft, tx, ty, (FcChar8*)text, len);
    XftColorFree(surf->dpy, xlib.vis, xlib.cmap, &color);
#else
    XSetForeground(surf->dpy, surf->gc, fg);
    if(font->set)
        XmbDrawString(surf->dpy,surf->drawable,font->set,surf->gc,tx,ty,(const char*)text,(int)len);
    else XDrawString(surf->dpy, surf->drawable, surf->gc, tx, ty, (const char*)text, (int)len);
#endif
}


#ifdef NK_XLIB_INCLUDE_STB_IMAGE
NK_INTERN struct /**
 * Create an nk_image from raw image pixels loaded via stb_image and wrap them in an XImageWithAlpha for Xlib rendering.
 *
 * Converts input pixel data from RGBA/RGB order to the X11 expected byte order, creates an XImage that takes ownership of
 * the provided data buffer, and when an alpha channel is present also creates a 1-bit clip mask marking pixels with alpha
 * greater than 127 as opaque. The returned nk_image's width and height are set to the supplied values.
 *
 * @param data Pointer to raw pixel data (RGB or RGBA) as returned by stb_image; may be consumed by the returned image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param channels Number of channels per pixel (3 for RGB, 4 for RGBA).
 * @returns An nk_image that references an internally allocated XImageWithAlpha with its `w` and `h` set; returns `nk_image_id(0)` on allocation or input failure.
 *
 * @note The XImage takes ownership of the `data` buffer (it is used directly as the image data). Free the returned image and its resources using nk_xsurf_image_free to avoid leaks.
 */
nk_image
nk_stbi_image_to_xsurf(unsigned char *data, int width, int height, int channels) {
    XSurface *surf = xlib.surf;
    struct nk_image img;
    int bpl = channels;
    long i, isize = width*height*channels;
    XImageWithAlpha *aimage = (XImageWithAlpha*)calloc( 1, sizeof(XImageWithAlpha) );
    int depth = DefaultDepth(surf->dpy, surf->screen);
    if (data == NULL) return nk_image_id(0);
    if (aimage == NULL) return nk_image_id(0);

    switch (depth){
        case 24:
            bpl = 4;
        break;
        case 16:
        case 15:
            bpl = 2;
        break;
        default:
            bpl = 1;
        break;
    }

    /* rgba to bgra */
    if (channels >= 3){
        for (i=0; i < isize; i += channels) {
            unsigned char red  = data[i+2];
            unsigned char blue = data[i];
            data[i]   = red;
            data[i+2] = blue;
        }
    }

    if (channels == 4){
        const unsigned alpha_treshold = 127;
        aimage->clipMask = XCreatePixmap(surf->dpy, surf->drawable, width, height, 1);

        if( aimage->clipMask ){
            aimage->clipMaskGC = XCreateGC(surf->dpy, aimage->clipMask, 0, 0);
            XSetForeground(surf->dpy, aimage->clipMaskGC, BlackPixel(surf->dpy, surf->screen));
            XFillRectangle(surf->dpy, aimage->clipMask, aimage->clipMaskGC, 0, 0, width, height);

            XSetForeground(surf->dpy, aimage->clipMaskGC, WhitePixel(surf->dpy, surf->screen));
            for (i=0; i < isize; i += channels){
                unsigned char alpha = data[i+3];
                int div = i / channels;
                int x = div % width;
                int y = div / width;
                if( alpha > alpha_treshold )
                    XDrawPoint(surf->dpy, aimage->clipMask, aimage->clipMaskGC, x, y);
            }
        }
    }

    aimage->ximage = XCreateImage(surf->dpy,
           CopyFromParent, depth,
           ZPixmap, 0,
           (char*)data,
           width, height,
           bpl*8, bpl * width);
    img = nk_image_ptr( (void*)aimage);
    img.h = height;
    img.w = width;
    return img;
}

NK_API struct /**
 * Load an image from a memory buffer and convert it into an nk_image suitable for Xlib rendering.
 *
 * @param membuf Pointer to the image data in memory (encoded image bytes).
 * @param membufSize Size of the memory buffer in bytes.
 * @returns An nk_image representing the loaded image, or an empty/default nk_image if loading fails.
 */
nk_image
nk_xsurf_load_image_from_memory(const void *membuf, nk_uint membufSize)
{
    int x,y,n;
    unsigned char *data;
    data = stbi_load_from_memory(membuf, membufSize, &x, &y, &n, 0);
    return nk_stbi_image_to_xsurf(data, x, y, n);
}

NK_API struct /**
 * Load an image file and convert it into an nk_image suitable for rendering.
 * @param filename Path to the image file to load.
 * @returns An nk_image containing the loaded image data. If loading fails, the returned nk_image will be empty/invalid. Free resources with nk_xsurf_image_free when the image is no longer needed.
 */
nk_image
nk_xsurf_load_image_from_file(char const *filename)
{
    int x,y,n;
    unsigned char *data;
    data = stbi_load(filename, &x, &y, &n, 0);
    return nk_stbi_image_to_xsurf(data, x, y, n);
}
#endif /**
 * Draws an NK image onto the specified X surface at the given position and size, using the image's alpha clip mask when available.
 * @param surf Target XSurface to draw into.
 * @param x X coordinate (pixels) of the destination origin.
 * @param y Y coordinate (pixels) of the destination origin.
 * @param w Width (pixels) of the drawn image.
 * @param h Height (pixels) of the drawn image.
 * @param img Nuklear image whose handle should point to an XImageWithAlpha; if `img.handle.ptr` is NULL no drawing occurs.
 * @param col Tint color (currently ignored).
 */

NK_INTERN void
nk_xsurf_draw_image(XSurface *surf, short x, short y, unsigned short w, unsigned short h,
    struct nk_image img, struct nk_color col)
{
    XImageWithAlpha *aimage = img.handle.ptr;

    NK_UNUSED(col);

    if (aimage){
        if (aimage->clipMask){
            XSetClipMask(surf->dpy, surf->gc, aimage->clipMask);
            XSetClipOrigin(surf->dpy, surf->gc, x, y);
        }
        XPutImage(surf->dpy, surf->drawable, surf->gc, aimage->ximage, 0, 0, x, y, w, h);
        XSetClipMask(surf->dpy, surf->gc, None);
    }
}

/**
 * Free resources held by an nk_image that wrap an XImageWithAlpha.
 *
 * Releases the underlying XImage, clip pixmap and its GC, and frees the associated XImageWithAlpha structure.
 *
 * @param image Pointer to the nk_image whose native X surface resources should be freed; if `image->handle.ptr` is NULL the function does nothing.
 */
void
nk_xsurf_image_free(struct nk_image* image)
{
    XSurface *surf = xlib.surf;
    XImageWithAlpha *aimage = image->handle.ptr;
    if (!aimage) return;
    XDestroyImage(aimage->ximage);
    XFreePixmap(surf->dpy, aimage->clipMask);
    XFreeGC(surf->dpy, aimage->clipMaskGC);
    free(aimage);
}


/**
 * Clear the entire XSurface by filling its drawable area with a solid pixel color.
 * 
 * @param surf Target surface whose drawable area will be cleared.
 * @param color X11 pixel value used to fill the surface (foreground color for the operation).
 */
NK_INTERN void
nk_xsurf_clear(XSurface *surf, unsigned long color)
{
    XSetForeground(surf->dpy, surf->gc, color);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, 0, 0, surf->w, surf->h);
}

/**
 * Blit the surface's drawable to the specified target drawable.
 * @param target Destination X11 Drawable to receive the blitted pixels.
 * @param surf Source XSurface containing the pixmap and graphics context.
 * @param w Width of the region to copy, in pixels (from source origin 0).
 * @param h Height of the region to copy, in pixels (from source origin 0).
 */
NK_INTERN void
nk_xsurf_blit(Drawable target, XSurface *surf, unsigned int w, unsigned int h)
{
    XCopyArea(surf->dpy, surf->drawable, target, surf->gc, 0, 0, w, h, 0, 0);
}

/**
 * Destroy an XSurface and release all associated X11 resources.
 *
 * Frees the XftDraw (when Xft is enabled), the backing pixmap, the graphics
 * context, and the XSurface structure itself. After this call the provided
 * surface pointer must not be used.
 *
 * @param surf Pointer to the XSurface to destroy; must be a valid, non-NULL surface.
 */
NK_INTERN void
nk_xsurf_del(XSurface *surf)
{
#ifdef NK_XLIB_USE_XFT
    XftDrawDestroy(surf->ftdraw);
#endif
    XFreePixmap(surf->dpy, surf->drawable);
    XFreeGC(surf->dpy, surf->gc);
    free(surf);
}

/**
 * Create and initialize an XFont structure for the given font name.
 *
 * Initializes and returns a newly allocated XFont describing metrics (ascent,
 * descent, height) for the named font as available on the provided Display.
 *
 * @param dpy  X11 display connection used to load the font.
 * @param name Font name or pattern to load (platform/fontconfig/X11 format).
 * @returns Pointer to an allocated XFont populated with metric information on success.
 *          Returns `NULL` if the font could not be loaded and no fallback was available.
 *          (When built with Xft, an XFont pointer may be returned even if the Xft font
 *          handle is NULL to indicate the named font was not found.) 
 */
NK_API XFont*
nk_xfont_create(Display *dpy, const char *name)
{
#ifdef NK_XLIB_USE_XFT
    XFont *font = (XFont*)calloc(1, sizeof(XFont));
    font->ft = XftFontOpenName(dpy, XDefaultScreen(dpy), name);
    if (!font->ft) {
        fprintf(stderr, "missing font: %s\n", name);
        return font;
    }
    font->ascent = font->ft->ascent;
    font->descent = font->ft->descent;
    font->height = font->ft->height;
#else
    int n;
    char *def, **missing;
    XFont *font = (XFont*)calloc(1, sizeof(XFont));
    font->set = XCreateFontSet(dpy, name, &missing, &n, &def);
    if(missing) {
        while(n--)
            fprintf(stderr, "missing fontset: %s\n", missing[n]);
        XFreeStringList(missing);
    }
    if(font->set) {
        XFontStruct **xfonts;
        char **font_names;
        XExtentsOfFontSet(font->set);
        n = XFontsOfFontSet(font->set, &xfonts, &font_names);
        while(n--) {
            font->ascent = NK_MAX(font->ascent, (*xfonts)->ascent);
            font->descent = NK_MAX(font->descent,(*xfonts)->descent);
            xfonts++;
        }
    } else {
        if(!(font->xfont = XLoadQueryFont(dpy, name))
        && !(font->xfont = XLoadQueryFont(dpy, "fixed"))) {
            free(font);
            return 0;
        }
        font->ascent = font->xfont->ascent;
        font->descent = font->xfont->descent;
    }
    font->height = font->ascent + font->descent;
#endif
    return font;
}

/**
 * Compute the pixel width of a text string using the provided X font.
 *
 * Measures the horizontal advance of the first `len` bytes of `text` (UTF-8
 * on Xft, multibyte or byte text otherwise) using the XFont referenced by
 * `handle`.
 *
 * @param handle nk_handle whose `ptr` points to an XFont used for measurement.
 * @param height Font height hint (ignored by this implementation).
 * @param text Pointer to the text to measure.
 * @param len Number of bytes/chars from `text` to measure.
 * @return The measured width in pixels as a float; returns `0` if `handle` or
 * `text` is NULL.
 */
NK_INTERN float
nk_xfont_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    XFont *font = (XFont*)handle.ptr;

#ifdef NK_XLIB_USE_XFT
    XGlyphInfo g;

    NK_UNUSED(height);

    if(!font || !text)
        return 0;

    XftTextExtentsUtf8(xlib.dpy, font->ft, (FcChar8*)text, len, &g);
    return g.xOff;
#else
    XRectangle r;

    NK_UNUSED(height);

    if(!font || !text)
        return 0;

    if(font->set) {
        XmbTextExtents(font->set, (const char*)text, len, NULL, &r);
        return (float)r.width;
    } else{
        int w = XTextWidth(font->xfont, (const char*)text, len);
        return (float)w;
    }
#endif
}

/**
 * Release all resources associated with an XFont and free its memory.
 *
 * Frees the underlying X font resources (XftFont, XFontSet, or XFont depending
 * on build configuration) using the provided Display, then deallocates the
 * XFont structure. If `font` is NULL this function is a no-op.
 *
 * @param dpy   Display connection used to free font resources.
 * @param font  XFont instance to destroy; may be NULL.
 */
NK_API void
nk_xfont_del(Display *dpy, XFont *font)
{
    if(!font) return;
#ifdef NK_XLIB_USE_XFT
    XftFontClose(dpy, font->ft);
#else
    if(font->set)
        XFreeFontSet(dpy, font->set);
    else
        XFreeFont(dpy, font->xfont);
#endif
    free(font);
}

NK_API struct /**
 * Initialize and return a Nuklear context configured for Xlib rendering using the provided font and display parameters.
 *
 * @param xfont Pointer to an XFont describing font metrics and handle to use for rendering.
 * @param dpy   X11 Display connection to use.
 * @param screen X11 screen number for creating the off-screen surface.
 * @param root  Root window used for cursor creation and resource association.
 * @param vis   Visual to use for Xft rendering (required when NK_XLIB_USE_XFT is defined).
 * @param cmap  Colormap associated with `vis` (required when NK_XLIB_USE_XFT is defined).
 * @param w     Initial width of the off-screen rendering surface in pixels.
 * @param h     Initial height of the off-screen rendering surface in pixels.
 *
 * @return Pointer to an initialized `nk_context` on success, `NULL` on failure (for example, locale support or X resource creation failures).
 */
nk_context*
nk_xlib_init(XFont *xfont, Display *dpy, int screen, Window root,
#ifdef NK_XLIB_USE_XFT
    Visual *vis, Colormap cmap,
#endif
    unsigned int w, unsigned int h)
{
    struct nk_user_font *font = &xfont->handle;
    font->userdata = nk_handle_ptr(xfont);
    font->height = (float)xfont->height;
    font->width = nk_xfont_get_text_width;
    xlib.dpy = dpy;
    xlib.root = root;
#ifdef NK_XLIB_USE_XFT
    xlib.vis = vis;
    xlib.cmap = cmap;
#endif

    if (!setlocale(LC_ALL,"")) return 0;
    if (!XSupportsLocale()) return 0;
    if (!XSetLocaleModifiers("@im=none")) return 0;

    xlib.xa_clipboard = XInternAtom(dpy, "CLIPBOARD", False);
    xlib.xa_targets = XInternAtom(dpy, "TARGETS", False);
    xlib.xa_text = XInternAtom(dpy, "TEXT", False);
    xlib.xa_utf8_string = XInternAtom(dpy, "UTF8_STRING", False);

    /* create invisible cursor */
    {static XColor dummy; char data[1] = {0};
    Pixmap blank = XCreateBitmapFromData(dpy, root, data, 1, 1);
    if (blank == None) return 0;
    xlib.cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(dpy, blank);}

    xlib.surf = nk_xsurf_create(screen, w, h);
    nk_init_default(&xlib.ctx, font);
    xlib.time_of_last_frame = nk_get_time();
    return &xlib.ctx;
}

/**
 * Set the active font for the Nuklear Xlib context.
 *
 * Updates the context's font metrics and text-measurement callback so subsequent
 * rendering and layout use the provided font.
 *
 * @param xfont Pointer to the XFont to activate; expected to be a valid, initialized font.
 */
NK_API void
nk_xlib_set_font(XFont *xfont)
{
    struct nk_user_font *font = &xfont->handle;
    font->userdata = nk_handle_ptr(xfont);
    font->height = (float)xfont->height;
    font->width = nk_xfont_get_text_width;
    nk_style_set_font(&xlib.ctx, font);
}

/**
 * Pushes the given XFont onto Nuklear's font stack and makes it the active font for styling and text measurement.
 *
 * @param xfont Font to push; its metrics and measurement callback will be used for subsequent text layout and rendering until the font is popped.
 */
NK_API void
nk_xlib_push_font(XFont *xfont)
{
    struct nk_user_font *font = &xfont->handle;
    font->userdata = nk_handle_ptr(xfont);
    font->height = (float)xfont->height;
    font->width = nk_xfont_get_text_width;
    nk_style_push_font(&xlib.ctx, font);
}

/**
 * Request the X primary selection and deliver its contents into the provided text edit.
 *
 * Sets the internal clipboard target to `edit` and initiates an X selection conversion;
 * the selection data will be delivered to the edit asynchronously via X events.
 *
 * @param edit Destination text edit which will receive the pasted text. Must be a
 *        persistent edit (not a temporary/stack-allocated editor); passing a temporary
 *        editor is not supported and will trigger an assertion.
 */
NK_API void
nk_xlib_paste(nk_handle handle, struct nk_text_edit* edit)
{
    NK_UNUSED(handle);
    /* Paste in X is asynchronous, so can not use a temporary text edit */
    NK_ASSERT(edit != &xlib.ctx.text_edit && "Paste not supported for temporary editors");
    xlib.clipboard_target = edit;
    /* Request the contents of the primary buffer */
    XConvertSelection(xlib.dpy, XA_PRIMARY, XA_STRING, XA_PRIMARY, xlib.root, CurrentTime);
}

/**
 * Store a string in the internal clipboard and claim ownership of the X11 selections.
 *
 * Copies `len` bytes from `str` into the backend's internal clipboard buffer and sets
 * the application as owner of both the PRIMARY and CLIPBOARD X selections so the data
 * can be provided to other X clients on request.
 *
 * @param handle Ignored by this backend.
 * @param str Pointer to the bytes to copy into the clipboard.
 * @param len Number of bytes from `str` to copy.
 */
NK_API void
nk_xlib_copy(nk_handle handle, const char* str, int len)
{
    NK_UNUSED(handle);
    free(xlib.clipboard_data);
    xlib.clipboard_len = 0;
    xlib.clipboard_data = malloc((size_t)len);
    if (xlib.clipboard_data) {
        memcpy(xlib.clipboard_data, str, (size_t)len);
        xlib.clipboard_len = len;
        XSetSelectionOwner(xlib.dpy, XA_PRIMARY, xlib.root, CurrentTime);
        XSetSelectionOwner(xlib.dpy, xlib.xa_clipboard, xlib.root, CurrentTime);
    }
}

/**
 * Process a single X11 event and translate it into Nuklear input/clipboard/window actions.
 *
 * This handler updates Nuklear input state (keyboard, mouse, scrolling, double-clicks),
 * manages clipboard selection requests and responses, handles window resize/expose,
 * and performs optional pointer grab/ungrab behavior. It also feeds text glyphs
 * to Nuklear for printable key events.
 *
 * @param dpy   X11 display the event was received on.
 * @param screen Unused; kept for API compatibility.
 * @param win   Window that received the event (used for selection and resize queries).
 * @param evt   XEvent to be processed.
 * @returns `1` if the event was handled by the backend, `0` otherwise.
 */
NK_API int
nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt)
{
    struct nk_context *ctx = &xlib.ctx;
    static int insert_toggle = 0;

    NK_UNUSED(screen);

    /* optional grabbing behavior */
    if (ctx->input.mouse.grab) {
        XDefineCursor(xlib.dpy, xlib.root, xlib.cursor);
        ctx->input.mouse.grab = 0;
    } else if (ctx->input.mouse.ungrab) {
        XWarpPointer(xlib.dpy, None, xlib.root, 0, 0, 0, 0,
            (int)ctx->input.mouse.prev.x, (int)ctx->input.mouse.prev.y);
        XUndefineCursor(xlib.dpy, xlib.root);
        ctx->input.mouse.ungrab = 0;
    }

    if (evt->type == KeyPress || evt->type == KeyRelease)
    {
        /* Key handler */
        int ret, down = (evt->type == KeyPress);
        KeySym *code = XGetKeyboardMapping(xlib.surf->dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
        if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (*code == XK_Control_L || *code == XK_Control_R) nk_input_key(ctx, NK_KEY_CTRL, down);
        else if (*code == XK_Delete)    nk_input_key(ctx, NK_KEY_DEL, down);
        else if (*code == XK_Return || *code == XK_KP_Enter)    nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (*code == XK_Tab)       nk_input_key(ctx, NK_KEY_TAB, down);
        else if (*code == XK_Left)      nk_input_key(ctx, NK_KEY_LEFT, down);
        else if (*code == XK_Right)     nk_input_key(ctx, NK_KEY_RIGHT, down);
        else if (*code == XK_Up)        nk_input_key(ctx, NK_KEY_UP, down);
        else if (*code == XK_Down)      nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (*code == XK_BackSpace) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (*code == XK_Escape)    nk_input_key(ctx, NK_KEY_TEXT_RESET_MODE, down);
        else if (*code == XK_Page_Up)   nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        else if (*code == XK_Page_Down) nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if (*code == XK_Home) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (*code == XK_End) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else {
            if (*code == 'c' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_COPY, down);
            else if (*code == 'v' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_PASTE, down);
            else if (*code == 'x' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_CUT, down);
            else if (*code == 'z' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
            else if (*code == 'r' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
            else if (*code == XK_Left && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else if (*code == XK_Right && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else if (*code == 'b' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
            else if (*code == 'e' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
            else if (*code == 'a' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx,NK_KEY_TEXT_SELECT_ALL, down);
            else if (*code == XK_Insert) {
                if (down) insert_toggle = !insert_toggle;
                if (insert_toggle) {
                    nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
                } else {
                    nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
                }
            } else {
                if (down) {
                    char buf[32];
                    KeySym keysym = 0;
                    if (XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
                        nk_input_glyph(ctx, buf);
                }
            }
        }
        XFree(code);
        return 1;
    } else if (evt->type == ButtonPress || evt->type == ButtonRelease) {
        /* Button handler */
        int down = (evt->type == ButtonPress);
        const int x = evt->xbutton.x, y = evt->xbutton.y;
        if (evt->xbutton.button == Button1) {
            if (down) { /* Double-Click Button handler */
                float dt = nk_get_time() - xlib.last_button_click;
                if (dt > NK_X11_DOUBLE_CLICK_LO && dt < NK_X11_DOUBLE_CLICK_HI)
                    nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_true);
                xlib.last_button_click = nk_get_time();
            } else nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_false);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt->xbutton.button == Button2)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->xbutton.button == Button3)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt->xbutton.button == Button4)
            nk_input_scroll(ctx, nk_vec2(0, 1.0f));
        else if (evt->xbutton.button == Button5)
            nk_input_scroll(ctx, nk_vec2(0, -1.0f));
        else return 0;
        return 1;
    } else if (evt->type == MotionNotify) {
        /* Mouse motion handler */
        const int x = evt->xmotion.x, y = evt->xmotion.y;
        nk_input_motion(ctx, x, y);
        if (ctx->input.mouse.grabbed) {
            ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
            ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
            XWarpPointer(xlib.dpy, None, xlib.surf->root, 0, 0, 0, 0, (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
        }
        return 1;
    } else if (evt->type == Expose || evt->type == ConfigureNotify) {
        /* Window resize handler */
        unsigned int width, height;
        XWindowAttributes attr;
        XGetWindowAttributes(dpy, win, &attr);
        width = (unsigned int)attr.width;
        height = (unsigned int)attr.height;
        nk_xsurf_resize(xlib.surf, width, height);
        return 1;
    } else if (evt->type == KeymapNotify) {
        XRefreshKeyboardMapping(&evt->xmapping);
        return 1;
    } else if (evt->type == SelectionClear) {
        free(xlib.clipboard_data);
        xlib.clipboard_data = NULL;
        xlib.clipboard_len = 0;
        return 1;
    } else if (evt->type == SelectionRequest) {
        XEvent reply;
        reply.xselection.type = SelectionNotify;
        reply.xselection.requestor = evt->xselectionrequest.requestor;
        reply.xselection.selection = evt->xselectionrequest.selection;
        reply.xselection.target = evt->xselectionrequest.target;
        reply.xselection.property = None; /* Default refuse */
        reply.xselection.time = evt->xselectionrequest.time;

        if (reply.xselection.target == xlib.xa_targets) {
            Atom target_list[4];
            target_list[0] = xlib.xa_targets;
            target_list[1] = xlib.xa_text;
            target_list[2] = xlib.xa_utf8_string;
            target_list[3] = XA_STRING;

            reply.xselection.property = evt->xselectionrequest.property;
            XChangeProperty(evt->xselection.display,evt->xselectionrequest.requestor,
                reply.xselection.property, XA_ATOM, 32, PropModeReplace,
                (unsigned char*)&target_list, 4);
        } else if (xlib.clipboard_data && (reply.xselection.target == xlib.xa_text ||
            reply.xselection.target == xlib.xa_utf8_string || reply.xselection.target == XA_STRING)) {
            reply.xselection.property = evt->xselectionrequest.property;
            XChangeProperty(evt->xselection.display,evt->xselectionrequest.requestor,
                reply.xselection.property, reply.xselection.target, 8, PropModeReplace,
                (unsigned char*)xlib.clipboard_data, xlib.clipboard_len);
        }
        XSendEvent(evt->xselection.display, evt->xselectionrequest.requestor, True, 0, &reply);
        XFlush(evt->xselection.display);
        return 1;
    } else if (evt->type == SelectionNotify && xlib.clipboard_target) {
        if ((evt->xselection.target != XA_STRING) &&
            (evt->xselection.target != xlib.xa_utf8_string) &&
            (evt->xselection.target != xlib.xa_text))
            return 1;

        {Atom actual_type;
        int actual_format;
        unsigned long pos = 0, len, remain;
        unsigned char* data = 0;
        do {
            XGetWindowProperty(dpy, win, XA_PRIMARY, (int)pos, 1024, False,
                AnyPropertyType, &actual_type, &actual_format, &len, &remain, &data);
            if (len && data)
                nk_textedit_text(xlib.clipboard_target, (char*)data, (int)len);
            if (data != 0) XFree(data);
            pos += (len * (unsigned long)actual_format) / 32;
        } while (remain != 0);}
        return 1;
    }
    return 0;
}

/**
 * Shut down the NK Xlib integration and release all associated resources.
 *
 * Frees the off-screen rendering surface, frees the Nuklear context memory, destroys the created cursor, and clears the internal Xlib backend state.
 */
NK_API void
nk_xlib_shutdown(void)
{
    nk_xsurf_del(xlib.surf);
    nk_free(&xlib.ctx);
    XFreeCursor(xlib.dpy, xlib.cursor);
    memset(&xlib, 0, sizeof(xlib));
}

/**
 * Render the current Nuklear command buffer to the given X11 drawable.
 *
 * Renders all queued Nuklear draw commands to the internal off-screen surface,
 * clears the surface with the provided color, updates the internal frame timing,
 * and blits the final image to the specified X11 Drawable.
 *
 * @param screen Target X11 Drawable (window or pixmap) to blit the rendered output to.
 * @param clear  Background color used to clear the off-screen surface before drawing.
 */
NK_API void
nk_xlib_render(Drawable screen, struct nk_color clear)
{
    const struct nk_command *cmd;
    struct nk_context *ctx = &xlib.ctx;
    XSurface *surf = xlib.surf;

    double now = nk_get_time();
    xlib.ctx.delta_time_seconds = now - xlib.time_of_last_frame;
    xlib.time_of_last_frame = now;

    nk_xsurf_clear(xlib.surf, nk_color_from_byte(&clear.r));
    nk_foreach(cmd, &xlib.ctx)
    {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_xsurf_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_xsurf_stroke_line(surf, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_xsurf_stroke_rect(surf, r->x, r->y, NK_MAX(r->w -r->line_thickness, 0),
                NK_MAX(r->h - r->line_thickness, 0), (unsigned short)r->rounding,
                r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_xsurf_fill_rect(surf, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_xsurf_stroke_circle(surf, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_xsurf_fill_circle(surf, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_ARC: {
            const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
            nk_xsurf_stroke_arc(surf, a->cx, a->cy, a->r, a->a[0], a->a[1], a->line_thickness, a->color);
        } break;
        case NK_COMMAND_ARC_FILLED: {
            const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled *)cmd;
            nk_xsurf_fill_arc(surf, a->cx, a->cy, a->r, a->a[0], a->a[1], a->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_xsurf_stroke_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_xsurf_fill_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_xsurf_stroke_polygon(surf, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_xsurf_fill_polygon(surf, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_xsurf_stroke_polyline(surf, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_xsurf_draw_text(surf, t->x, t->y, (const char*)t->string, t->length,
                (XFont*)t->font->userdata.ptr, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_xsurf_stroke_curve(surf, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, 22, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_IMAGE: {
            const struct nk_command_image *i = (const struct nk_command_image *)cmd;
            nk_xsurf_draw_image(surf, i->x, i->y, i->w, i->h, i->img, i->col);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR:
        case NK_COMMAND_CUSTOM:
        default: break;
        }
    }
    nk_clear(ctx);
    nk_xsurf_blit(screen, surf, surf->w, surf->h);
}
#endif