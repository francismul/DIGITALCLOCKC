/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GDI_H_
#define NK_GDI_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>

typedef struct GdiFont GdiFont;
NK_API struct nk_context* nk_gdi_init(GdiFont *font, HDC window_dc, unsigned int width, unsigned int height);
NK_API int nk_gdi_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdi_render(struct nk_color clear);
NK_API void nk_gdi_shutdown(void);

/* font */
NK_API GdiFont* nk_gdifont_create(const char *name, int size);
NK_API GdiFont* nk_gdifont_create_with_style(const char *name, int size, int weight, BOOL italic);
NK_API void nk_gdifont_del(GdiFont *font);
NK_API void nk_gdi_set_font(GdiFont *font);

#endif

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GDI_IMPLEMENTATION
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

struct GdiFont {
    struct nk_user_font nk;
    int height;
    HFONT handle;
    HDC dc;
};

static struct {
    HBITMAP bitmap;
    HDC window_dc;
    HDC memory_dc;
    unsigned int width;
    unsigned int height;
    struct nk_context ctx;
} gdi;

/**
 * Create a Windows DIB-backed nk_image from an RGB24 frame buffer and store it in `image`.
 *
 * Converts a tightly packed 24-bit RGB buffer (row-major, 3 bytes per pixel) into a DIB
 * bitmap, sets image width/height and region, and stores the HBITMAP in image->handle.ptr.
 * If any input is invalid (null pointers or non-positive dimensions), the function does nothing.
 *
 * @param image Pointer to the nk_image to initialize; its fields (w, h, region, handle) are updated.
 * @param frame_buffer Pointer to a contiguous RGB24 pixel buffer (width * height * 3 bytes).
 * @param width Width of the image in pixels; must be greater than 0.
 * @param height Height of the image in pixels; must be greater than 0.
 */
static void
nk_create_image(struct nk_image * image, const char * frame_buffer, const int width, const int height)
{
    if (image && frame_buffer && (width > 0) && (height > 0))
    {
        const unsigned char * src = (const unsigned char *)frame_buffer;
        INT row = ((width * 3 + 3) & ~3);
        LPBYTE lpBuf, pb = NULL;
        BITMAPINFO bi = { 0 };
        HBITMAP hbm;
        int v, i;

        image->w = width;
        image->h = height;
        image->region[0] = 0;
        image->region[1] = 0;
        image->region[2] = width;
        image->region[3] = height;

        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = width;
        bi.bmiHeader.biHeight = height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = row * height;

        hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**)&lpBuf, NULL, 0);

        pb = lpBuf + row * height;
        for (v = 0; v < height; v++)
        {
            pb -= row;
            for (i = 0; i < row; i += 3)
            {
                pb[i + 0] = src[0];
                pb[i + 1] = src[1];
                pb[i + 2] = src[2];
                src += 3;
            }
        }
        SetDIBits(NULL, hbm, 0, height, lpBuf, &bi, DIB_RGB_COLORS);
        image->handle.ptr = hbm;
    }
}

/**
 * Frees the Windows bitmap associated with an nk_image and resets the image structure to zero.
 * @param image Pointer to the nk_image whose underlying HBITMAP will be deleted and which will be cleared; if NULL or the image has no handle, nothing is done.
 */
static void
nk_delete_image(struct nk_image * image)
{
    if (image && image->handle.id != 0)
    {
        HBITMAP hbm = (HBITMAP)image->handle.ptr;
        DeleteObject(hbm);
        memset(image, 0, sizeof(struct nk_image));
    }
}

/**
 * Draws an nk_image into the backend's offscreen bitmap at the given position and size.
 *
 * @param x X coordinate (left) in the offscreen bitmap where the image will be placed.
 * @param y Y coordinate (top) in the offscreen bitmap where the image will be placed.
 * @param w Width to draw the image (stretched or shrunk to this size).
 * @param h Height to draw the image (stretched or shrunk to this size).
 * @param img nk_image whose handle is expected to be a Windows HBITMAP; the bitmap pixels are blitted into the offscreen DC.
 * @param col Color tint to apply to the image (currently ignored by this backend).
 *
 * If the backend has no offscreen memory DC or the image has no valid bitmap handle, the call has no effect.
 */
static void
nk_gdi_draw_image(short x, short y, unsigned short w, unsigned short h,
    struct nk_image img, struct nk_color col)
{
    HBITMAP hbm = (HBITMAP)img.handle.ptr;
    HDC     hDCBits;
    BITMAP  bitmap;

    if (!gdi.memory_dc || !hbm)
        return;

    hDCBits = CreateCompatibleDC(gdi.memory_dc);
    GetObject(hbm, sizeof(BITMAP), (LPSTR)&bitmap);
    SelectObject(hDCBits, hbm);
    StretchBlt(gdi.memory_dc, x, y, w, h, hDCBits, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
    DeleteDC(hDCBits);
}

/**
 * Map an nk_color's RGB components into a Windows COLORREF value.
 * @param c nk_color whose RGB components will be used (alpha is ignored).
 * @returns COLORREF with red in the least-significant byte, green in the next byte, and blue in the next byte; the alpha component is discarded.
 */
static COLORREF
convert_color(struct nk_color c)
{
    return c.r | (c.g << 8) | (c.b << 16);
}

/**
 * Set the current GDI clipping region to the rectangle defined by (x, y, w, h).
 *
 * Resets any existing clip region and intersects the device context's clip with
 * the rectangle (x, y) — (x + w, y + h). The right and bottom edges use an
 * inclusive extent to cover pixel extents.
 *
 * @param x Left coordinate of the clipping rectangle in pixels.
 * @param y Top coordinate of the clipping rectangle in pixels.
 * @param w Width of the clipping rectangle in pixels.
 * @param h Height of the clipping rectangle in pixels.
 */
static void
nk_gdi_scissor(HDC dc, float x, float y, float w, float h)
{
    SelectClipRgn(dc, NULL);
    IntersectClipRect(dc, (int)x, (int)y, (int)(x + w + 1), (int)(y + h + 1));
}

/**
 * Draws a straight line between two points into the specified device context using the given color and thickness.
 *
 * @param dc Device context to draw into.
 * @param x0 X coordinate of the start point.
 * @param y0 Y coordinate of the start point.
 * @param x1 X coordinate of the end point.
 * @param y1 Y coordinate of the end point.
 * @param line_thickness Thickness of the line in pixels; a value of 1 uses the DC pen.
 * @param col Color to use for the line.
 */
static void
nk_gdi_stroke_line(HDC dc, short x0, short y0, short x1,
    short y1, unsigned int line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);

    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    MoveToEx(dc, x0, y0, NULL);
    LineTo(dc, x1, y1);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Draws an outlined rectangle onto the given device context, using rounded corners when a corner radius is provided.
 * @param dc Target device context to draw into.
 * @param x X coordinate of the rectangle's top-left corner.
 * @param y Y coordinate of the rectangle's top-left corner.
 * @param w Width of the rectangle in pixels.
 * @param h Height of the rectangle in pixels.
 * @param r Corner radius in pixels; use 0 for sharp corners.
 * @param line_thickness Outline thickness in pixels; a value of 1 uses the DC pen optimization.
 * @param col Outline color.
 */
static void
nk_gdi_stroke_rect(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HGDIOBJ br;
    HPEN pen = NULL;

    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    br = SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (r == 0) {
        Rectangle(dc, x, y, x + w, y + h);
    } else {
        RoundRect(dc, x, y, x + w, y + h, r, r);
    }
    SelectObject(dc, br);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Fill a rectangle on the given device context with a solid color.
 *
 * If `r` is zero the rectangle is filled with square corners; otherwise the
 * rectangle is filled with rounded corners using `r` as the corner radius.
 *
 * @param dc Handle to the device context to draw into.
 * @param x Left coordinate of the rectangle.
 * @param y Top coordinate of the rectangle.
 * @param w Width of the rectangle in pixels.
 * @param h Height of the rectangle in pixels.
 * @param r Corner radius in pixels; set to 0 for square corners.
 * @param col Color used to fill the rectangle.
 */
static void
nk_gdi_fill_rect(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, struct nk_color col)
{
    COLORREF color = convert_color(col);

    if (r == 0) {
        RECT rect;
        SetRect(&rect, x, y, x + w, y + h);
        SetBkColor(dc, color);
        ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
    } else {
        SetDCPenColor(dc, color);
        SetDCBrushColor(dc, color);
        RoundRect(dc, x, y, x + w, y + h, r, r);
    }
}
/**
 * Set RGBA components of a PTRIVERTEX from an nk_color.
 *
 * Maps each 8-bit channel in `col` into the vertex's 16-bit color fields and sets alpha to fully opaque.
 *
 * @param tri Pointer to the destination PTRIVERTEX whose color fields will be updated.
 * @param col Source nk_color providing red, green, blue components (0–255).
 */
static void
nk_gdi_set_vertexColor(PTRIVERTEX tri, struct nk_color col)
{
    tri->Red   = col.r << 8;
    tri->Green = col.g << 8;
    tri->Blue  = col.b << 8;
    tri->Alpha = 0xff << 8;
}

/**
 * Fill a rectangle with a four-corner color gradient and composite it using alpha blending.
 *
 * Draws a rectangle at (x,y) with width w and height h using the specified colors for
 * the left, top, right, and bottom corners and blends the result into the given device
 * context honoring per-vertex alpha.
 *
 * @param dc Device context used for gradient rendering.
 * @param x  Left coordinate of the rectangle.
 * @param y  Top coordinate of the rectangle.
 * @param w  Width of the rectangle.
 * @param h  Height of the rectangle.
 * @param left   Color applied to the left corner.
 * @param top    Color applied to the top corner.
 * @param right  Color applied to the right corner.
 * @param bottom Color applied to the bottom corner.
 */
static void
nk_gdi_rect_multi_color(HDC dc, short x, short y, unsigned short w,
    unsigned short h, struct nk_color left, struct nk_color top,
    struct nk_color right, struct nk_color bottom)
{
    BLENDFUNCTION alphaFunction;
    GRADIENT_RECT gRect;
    GRADIENT_TRIANGLE gTri[2];
    TRIVERTEX vt[4];
    alphaFunction.BlendOp = AC_SRC_OVER;
    alphaFunction.BlendFlags = 0;
    alphaFunction.SourceConstantAlpha = 0;
    alphaFunction.AlphaFormat = AC_SRC_ALPHA;

    /* TODO: This Case Needs Repair.*/
    /* Top Left Corner */
    vt[0].x     = x;
    vt[0].y     = y;
    nk_gdi_set_vertexColor(&vt[0], left);
    /* Top Right Corner */
    vt[1].x     = x+w;
    vt[1].y     = y;
    nk_gdi_set_vertexColor(&vt[1], top);
    /* Bottom Left Corner */
    vt[2].x     = x;
    vt[2].y     = y+h;
    nk_gdi_set_vertexColor(&vt[2], right);

    /* Bottom Right Corner */
    vt[3].x     = x+w;
    vt[3].y     = y+h;
    nk_gdi_set_vertexColor(&vt[3], bottom);

    gTri[0].Vertex1 = 0;
    gTri[0].Vertex2 = 1;
    gTri[0].Vertex3 = 2;
    gTri[1].Vertex1 = 2;
    gTri[1].Vertex2 = 1;
    gTri[1].Vertex3 = 3;
    GdiGradientFill(dc, vt, 4, gTri, 2 , GRADIENT_FILL_TRIANGLE);
    AlphaBlend(gdi.window_dc,  x, y, x+w, y+h,gdi.memory_dc, x, y, x+w, y+h,alphaFunction);

}

/**
 * Set the coordinates of a POINT structure.
 * @param p Pointer to the POINT to modify; must not be NULL.
 * @param x X coordinate to set.
 * @param y Y coordinate to set.
 * @returns `TRUE` if the POINT was updated, `FALSE` if `p` is NULL.
 */
static BOOL
SetPoint(POINT *p, LONG x, LONG y)
{
    if (!p)
        return FALSE;
    p->x = x;
    p->y = y;
    return TRUE;
}

/**
 * Fill a triangle on the specified device context using the given color.
 * 
 * @param dc Device context to draw into.
 * @param x0 X coordinate of the first vertex.
 * @param y0 Y coordinate of the first vertex.
 * @param x1 X coordinate of the second vertex.
 * @param y1 Y coordinate of the second vertex.
 * @param x2 X coordinate of the third vertex.
 * @param y2 Y coordinate of the third vertex.
 * @param col Color used to fill the triangle.
 */
static void
nk_gdi_fill_triangle(HDC dc, short x0, short y0, short x1,
    short y1, short x2, short y2, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT points[3];

    SetPoint(&points[0], x0, y0);
    SetPoint(&points[1], x1, y1);
    SetPoint(&points[2], x2, y2);

    SetDCPenColor(dc, color);
    SetDCBrushColor(dc, color);
    Polygon(dc, points, 3);
}

/**
 * Draws the outline of a triangle connecting three 2D points using the specified line thickness and color.
 *
 * @param dc Device context to draw into.
 * @param x0 X coordinate of the first vertex.
 * @param y0 Y coordinate of the first vertex.
 * @param x1 X coordinate of the second vertex.
 * @param y1 Y coordinate of the second vertex.
 * @param x2 X coordinate of the third vertex.
 * @param y2 Y coordinate of the third vertex.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Color used to draw the triangle edges.
 */
static void
nk_gdi_stroke_triangle(HDC dc, short x0, short y0, short x1,
    short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT points[4];
    HPEN pen = NULL;

    SetPoint(&points[0], x0, y0);
    SetPoint(&points[1], x1, y1);
    SetPoint(&points[2], x2, y2);
    SetPoint(&points[3], x0, y0);

    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    Polyline(dc, points, 4);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Fill a polygon on the given device context using the specified color.
 *
 * Draws and fills a polygon defined by `count` points from `pnts` into `dc` using `col`.
 * If `count` exceeds 64 points, only the first 64 points are used.
 *
 * @param dc Device context to draw into.
 * @param pnts Array of integer 2D points defining the polygon vertices.
 * @param count Number of points in `pnts`; values greater than 64 are truncated to 64.
 * @param col Fill color for the polygon.
 */
static void
nk_gdi_fill_polygon(HDC dc, const struct nk_vec2i *pnts, int count, struct nk_color col)
{
    int i = 0;
    #define MAX_POINTS 64
    POINT points[MAX_POINTS];
    COLORREF color = convert_color(col);
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, color);
    for (i = 0; i < count && i < MAX_POINTS; ++i) {
        points[i].x = pnts[i].x;
        points[i].y = pnts[i].y;
    }
    Polygon(dc, points, i);
    #undef MAX_POINTS
}

/**
 * Stroke a closed polygon onto the given device context.
 *
 * Draws the polygon defined by `count` points in `pnts` by connecting each
 * point in order and closing the shape back to the first point. Uses a
 * 1-pixel DC pen when `line_thickness` is 1; otherwise creates a solid pen of
 * the specified thickness and restores the DC pen afterwards.
 *
 * @param dc Device context to draw into.
 * @param pnts Array of integer 2D points defining the polygon vertices.
 * @param count Number of points in `pnts`. If zero, nothing is drawn.
 * @param line_thickness Thickness of the stroke in pixels.
 * @param col Stroke color as an nk_color.
 */
static void
nk_gdi_stroke_polygon(HDC dc, const struct nk_vec2i *pnts, int count,
    unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    if (count > 0) {
        int i;
        MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
        for (i = 1; i < count; ++i)
            LineTo(dc, pnts[i].x, pnts[i].y);
        LineTo(dc, pnts[0].x, pnts[0].y);
    }

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Draws a connected polyline onto the specified device context.
 *
 * Draws straight-line segments connecting the sequence of points in `pnts` in order. If `count` is less than or equal to zero, nothing is drawn.
 *
 * @param dc Device context to draw into.
 * @param pnts Pointer to an array of points (`struct nk_vec2i`) that define the polyline.
 * @param count Number of points in `pnts`.
 * @param line_thickness Stroke width in pixels.
 * @param col Color used for the stroke.
 */
static void
nk_gdi_stroke_polyline(HDC dc, const struct nk_vec2i *pnts,
    int count, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    if (count > 0) {
        int i;
        MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
        for (i = 1; i < count; ++i)
            LineTo(dc, pnts[i].x, pnts[i].y);
    }

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Draws an arc segment outline using the supplied device context.
 *
 * Renders an arc centered at (cx, cy) with radius r, spanning the angle range
 * from amin to amin + adelta (angles in radians). The arc is stroked with
 * the specified line_thickness and color; thick strokes use a geometric pen
 * with flat end caps for consistent appearance. The arc is drawn with
 * counterclockwise arc direction.
 *
 * @param dc Device context to draw into.
 * @param cx X coordinate of the arc center.
 * @param cy Y coordinate of the arc center.
 * @param r Radius of the arc in pixels.
 * @param amin Starting angle in radians.
 * @param adelta Sweep angle in radians (added to amin to form the end angle).
 * @param line_thickness Line thickness in pixels.
 * @param col Color used to stroke the arc.
 */
static void
nk_gdi_stroke_arc(HDC dc, short cx, short cy, unsigned short r, float amin, float adelta, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);

    /* setup pen */
    HPEN pen = NULL;
    if (line_thickness == 1)
        SetDCPenColor(dc, color);
    else
    {
        /* the flat endcap makes thick arcs look better */
        DWORD pen_style = PS_SOLID | PS_ENDCAP_FLAT | PS_GEOMETRIC;

        LOGBRUSH brush;
        brush.lbStyle = BS_SOLID;
        brush.lbColor = color;
        brush.lbHatch = 0;

        pen = ExtCreatePen(pen_style, line_thickness, &brush, 0, NULL);
        SelectObject(dc, pen);
    }

    /* calculate arc and draw */
    int start_x = cx + (int) ((float)r*nk_cos(amin+adelta)),
        start_y = cy + (int) ((float)r*nk_sin(amin+adelta)),
        end_x = cx + (int) ((float)r*nk_cos(amin)),
        end_y = cy + (int) ((float)r*nk_sin(amin));

    HGDIOBJ br = SelectObject(dc, GetStockObject(NULL_BRUSH));
    SetArcDirection(dc, AD_COUNTERCLOCKWISE);
    Pie(dc, cx-r, cy-r, cx+r, cy+r, start_x, start_y, end_x, end_y);
    SelectObject(dc, br);

    if (pen)
    {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Fill a circular sector (arc) centered at the given point with the specified color.
 *
 * @param dc Device context to draw into.
 * @param cx X coordinate of the arc center (pixels).
 * @param cy Y coordinate of the arc center (pixels).
 * @param r  Radius of the arc (pixels).
 * @param amin Starting angle of the arc, in radians.
 * @param adelta Sweep angle of the arc, in radians (positive or negative to control direction).
 * @param col Color used to fill the sector.
 */
static void
nk_gdi_fill_arc(HDC dc, short cx, short cy, unsigned short r, float amin, float adelta, struct nk_color col)
{
    COLORREF color = convert_color(col);
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, color);

    int start_x = cx + (int) ((float)r*nk_cos(amin+adelta)),
        start_y = cy + (int) ((float)r*nk_sin(amin+adelta)),
        end_x = cx + (int) ((float)r*nk_cos(amin)),
        end_y = cy + (int) ((float)r*nk_sin(amin));

    Pie(dc, cx-r, cy-r, cx+r, cy+r, start_x, start_y, end_x, end_y);
}

/**
 * Draws a filled ellipse inside the rectangle defined by (x, y, x + w, y + h) using the given color.
 *
 * @param dc   Device context to draw into.
 * @param x    X coordinate of the rectangle's top-left corner.
 * @param y    Y coordinate of the rectangle's top-left corner.
 * @param w    Width of the bounding rectangle.
 * @param h    Height of the bounding rectangle.
 * @param col  Color used to fill the ellipse.
 */
static void
nk_gdi_fill_circle(HDC dc, short x, short y, unsigned short w,
    unsigned short h, struct nk_color col)
{
    COLORREF color = convert_color(col);
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, color);
    Ellipse(dc, x, y, x + w, y + h);
}

/**
 * Draws an outlined ellipse at the given position and size using the specified color and stroke thickness.
 *
 * @param dc Device context to draw into; coordinates are in DC units.
 * @param x Left coordinate of the bounding rectangle.
 * @param y Top coordinate of the bounding rectangle.
 * @param w Width of the bounding rectangle.
 * @param h Height of the bounding rectangle.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Color used for the ellipse stroke.
 */
static void
nk_gdi_stroke_circle(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    HGDIOBJ br = SelectObject(dc, GetStockObject(NULL_BRUSH));
    SetDCBrushColor(dc, OPAQUE);
    Ellipse(dc, x, y, x + w, y + h);
    SelectObject(dc, br);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Draws a cubic Bezier curve defined by four control points using GDI.
 *
 * Draws a stroked cubic Bezier curve from p1 to p4 using p2 and p3 as control points,
 * rendered with the given line thickness and color on the provided device context.
 *
 * @param dc Device context to draw into.
 * @param p1 Starting point of the curve.
 * @param p2 First control point.
 * @param p3 Second control point.
 * @param p4 End point of the curve.
 * @param line_thickness Stroke thickness in pixels.
 * @param col Stroke color. 
 */
static void
nk_gdi_stroke_curve(HDC dc, struct nk_vec2i p1,
    struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
    unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT p[4];
    HPEN pen = NULL;

    SetPoint(&p[0], p1.x, p1.y);
    SetPoint(&p[1], p2.x, p2.y);
    SetPoint(&p[2], p3.x, p3.y);
    SetPoint(&p[3], p4.x, p4.y);

    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    SetDCBrushColor(dc, OPAQUE);
    PolyBezier(dc, p, 4);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

/**
 * Draws UTF-8 text into the given device context using the specified font and colors.
 *
 * Renders up to `len` bytes of UTF-8 `text` at position (x, y) within the rectangle of size
 * `w`×`h` on `dc`, using `font` for glyph metrics. The text background is filled with `cbg`
 * and the text color is `cfg`. If `text` is NULL, `font` is NULL, or `len` is zero, no action
 * is taken.
 *
 * @param dc Device context to draw into.
 * @param x X coordinate of the text origin.
 * @param y Y coordinate of the text baseline origin.
 * @param w Width of the drawing rectangle (unused by some platforms but provided for layout).
 * @param h Height of the drawing rectangle (unused by some platforms but provided for layout).
 * @param text UTF-8 encoded text buffer to render.
 * @param len Number of bytes from `text` to render.
 * @param font GdiFont to use for rendering (must be non-NULL).
 * @param cbg Background color for the text area.
 * @param cfg Foreground (text) color.
 */
static void
nk_gdi_draw_text(HDC dc, short x, short y, unsigned short w, unsigned short h,
    const char *text, int len, GdiFont *font, struct nk_color cbg, struct nk_color cfg)
{
    int wsize;
    WCHAR* wstr;

    if(!text || !font || !len) return;

    wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    wstr = (WCHAR*)_alloca(wsize * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);

    SetBkMode(dc, TRANSPARENT); /* Transparent Text Background */
    SetBkColor(dc, convert_color(cbg));
    SetTextColor(dc, convert_color(cfg));

    SelectObject(dc, font->handle);
    ExtTextOutW(dc, x, y, ETO_OPAQUE, NULL, wstr, wsize, NULL);
}

/**
 * Fill the provided device context with the given color.
 *
 * Sets the background color of the HDC and fills the full gdi width/height rectangle
 * using an opaque ExtTextOutW call.
 *
 * @param dc   Target device context to clear.
 * @param col  Fill color used to clear the context.
 */
static void
nk_gdi_clear(HDC dc, struct nk_color col)
{
    COLORREF color = convert_color(col);
    RECT rect;
    SetRect(&rect, 0, 0, gdi.width, gdi.height);
    SetBkColor(dc, color);

    ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

/**
 * Copy the internal offscreen backbuffer to the specified device context.
 * 
 * @param dc Destination HDC where the offscreen buffer is blitted.
 */
static void
nk_gdi_blit(HDC dc)
{
    BitBlt(dc, 0, 0, gdi.width, gdi.height, gdi.memory_dc, 0, 0, SRCCOPY);

}

/**
 * Create a GdiFont with the given face name, size, weight, and italic style and initialize its device context and metrics.
 *
 * @param name Face name of the font (UTF-8 null-terminated string) or NULL to use the default face.
 * @param size Height of the font in logical units (positive or negative depending on desired mapping mode).
 * @param weight Font weight (e.g., FW_NORMAL, FW_BOLD).
 * @param italic Nonzero to create an italic font, zero for normal.
 * @returns Pointer to an allocated GdiFont with its HFONT and compatible HDC initialized, or NULL on allocation or creation failure.
 */
GdiFont*
nk_gdifont_create_with_style(const char *name, int size, int weight, BOOL italic)
{
    TEXTMETRICW metric;
    GdiFont *font = (GdiFont*)calloc(1, sizeof(GdiFont));
    if (!font)
        return NULL;
    font->dc = CreateCompatibleDC(0);
    font->handle = CreateFontA(size, 0, 0, 0, weight, italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
    SelectObject(font->dc, font->handle);
    GetTextMetricsW(font->dc, &metric);
    font->height = metric.tmHeight;
    return font;
}

/**
 * Create a GdiFont using the specified font face and point size with normal weight and no italics.
 * @param name Font family name (UTF-8). If NULL or empty, the system default font may be used.
 * @param size Font size in points.
 * @return Pointer to a newly allocated GdiFont on success, or NULL on failure.
 */
GdiFont*
nk_gdifont_create(const char *name, int size)
{
    return nk_gdifont_create_with_style(name, size, FW_NORMAL, FALSE);
}

/**
 * Measure the pixel width of a UTF-8 string using the GDI font from the given handle.
 *
 * @param handle Font handle containing the GDI font to use for measurement.
 * @param height Requested font height (unused by this implementation).
 * @param text UTF-8 encoded string to measure.
 * @param len Number of bytes in `text`.
 * @returns Width in pixels of the measured text as a float, `0` if `handle` or `text` is NULL, or `-1.0f` if measurement failed.
 */
static float
nk_gdifont_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    GdiFont *font = (GdiFont*)handle.ptr;
    SIZE size;
    int wsize;
    WCHAR* wstr;
    if (!font || !text)
        return 0;

    wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    wstr = (WCHAR*)_alloca(wsize * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
    if (GetTextExtentPoint32W(font->dc, wstr, wsize, &size))
        return (float)size.cx;
    return -1.0f;
}

/**
 * Release all resources associated with a GdiFont and free its memory.
 *
 * Deletes the font's HFONT and HDC, then frees the GdiFont structure.
 * Does nothing if `font` is NULL.
 *
 * @param font Pointer to the GdiFont to delete; the pointer becomes invalid after this call.
 */
void
nk_gdifont_del(GdiFont *font)
{
    if(!font) return;
    DeleteObject(font->handle);
    DeleteDC(font->dc);
    free(font);
}

/**
 * Paste Unicode text from the system clipboard into a Nuklear text edit.
 *
 * If the clipboard contains CF_UNICODETEXT, the function converts it from UTF-16 to UTF-8
 * and inserts the resulting bytes into the provided nk_text_edit via nk_textedit_paste.
 *
 * @param usr Unused clipboard user data (ignored).
 * @param edit Target text edit to receive pasted text; must be a valid pointer.
 */
static void
nk_gdi_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    (void)usr;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
    {
        HGLOBAL mem = GetClipboardData(CF_UNICODETEXT);
        if (mem)
        {
            SIZE_T size = GlobalSize(mem) - 1;
            if (size)
            {
                LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
                if (wstr)
                {
                    int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), NULL, 0, NULL, NULL);
                    if (utf8size)
                    {
                        char* utf8 = (char*)malloc(utf8size);
                        if (utf8)
                        {
                            WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), utf8, utf8size, NULL, NULL);
                            nk_textedit_paste(edit, utf8, utf8size);
                            free(utf8);
                        }
                    }
                    GlobalUnlock(mem);
                }
            }
        }
        CloseClipboard();
    }
}

/**
 * Copy UTF-8 text into the system clipboard as CF_UNICODETEXT.
 *
 * Converts the provided UTF-8 buffer to a null-terminated UTF-16 (wide) string,
 * places it on the Windows clipboard as CF_UNICODETEXT, and closes the clipboard.
 *
 * @param usr Unused clipboard user handle (ignored).
 * @param text Pointer to the UTF-8 encoded text to copy.
 * @param len Number of bytes in `text`; may be -1 to indicate a null-terminated string.
 */
static void
nk_gdi_clipboard_copy(nk_handle usr, const char *text, int len)
{
    if (OpenClipboard(NULL))
    {
        int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
        if (wsize)
        {
            HGLOBAL mem = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
            if (mem)
            {
                wchar_t* wstr = (wchar_t*)GlobalLock(mem);
                if (wstr)
                {
                    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
                    wstr[wsize] = 0;
                    GlobalUnlock(mem);

                    SetClipboardData(CF_UNICODETEXT, mem);
                }
            }
        }
        CloseClipboard();
    }
}

NK_API struct /**
 * Initialize a Nuklear context and prepare an offscreen GDI bitmap and memory DC for rendering.
 *
 * @param gdifont Pointer to a previously created GdiFont to use as the Nuklear font and metrics.
 * @param window_dc Device context of the target window where the final image will be blitted.
 * @param width Initial width of the offscreen bitmap in pixels.
 * @param height Initial height of the offscreen bitmap in pixels.
 * @returns Pointer to the initialized nk_context on success; pointer refers to the internal static GDI context.
 */
nk_context*
nk_gdi_init(GdiFont *gdifont, HDC window_dc, unsigned int width, unsigned int height)
{
    struct nk_user_font *font = &gdifont->nk;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;

    gdi.bitmap = CreateCompatibleBitmap(window_dc, width, height);
    gdi.window_dc = window_dc;
    gdi.memory_dc = CreateCompatibleDC(window_dc);
    gdi.width = width;
    gdi.height = height;
    SelectObject(gdi.memory_dc, gdi.bitmap);

    nk_init_default(&gdi.ctx, font);
    gdi.ctx.clip.copy = nk_gdi_clipboard_copy;
    gdi.ctx.clip.paste = nk_gdi_clipboard_paste;
    return &gdi.ctx;
}

/**
 * Set the active Nuklear font to the provided GdiFont.
 *
 * Updates the Nuklear user-font userdata, height, and width callback, and applies it to the global GDI Nuklear context.
 *
 * @param gdifont GdiFont to activate as the current Nuklear font; must be non-NULL.
 */
NK_API void
nk_gdi_set_font(GdiFont *gdifont)
{
    struct nk_user_font *font = &gdifont->nk;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;
    nk_style_set_font(&gdi.ctx, font);
}

/**
 * Handle a Windows message and forward relevant input and window events to the Nuklear GDI backend.
 *
 * Processes resize, paint, keyboard, character, and mouse messages to update the backend's GDI state
 * and to feed input events into the embedded Nuklear context.
 *
 * @param wnd   Window handle that received the message.
 * @param msg   Windows message identifier (e.g., WM_SIZE, WM_PAINT, WM_KEYDOWN).
 * @param wparam Additional message-specific information (WPARAM).
 * @param lparam Additional message-specific information (LPARAM).
 * @returns `1` if the message was handled by the backend and should not be further processed, `0` otherwise.
 */
NK_API int
nk_gdi_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static int insert_toggle = 0;
    switch (msg)
    {
    case WM_SIZE:
    {
        unsigned width = LOWORD(lparam);
        unsigned height = HIWORD(lparam);
        if (width != gdi.width || height != gdi.height)
        {
            DeleteObject(gdi.bitmap);
            gdi.bitmap = CreateCompatibleBitmap(gdi.window_dc, width, height);
            gdi.width = width;
            gdi.height = height;
            SelectObject(gdi.memory_dc, gdi.bitmap);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(wnd, &paint);
        nk_gdi_blit(dc);
        EndPaint(wnd, &paint);
        return 1;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        int down = !((lparam >> 31) & 1);
        int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

        switch (wparam)
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            nk_input_key(&gdi.ctx, NK_KEY_SHIFT, down);
            return 1;

        case VK_DELETE:
            nk_input_key(&gdi.ctx, NK_KEY_DEL, down);
            return 1;

        case VK_RETURN:
        case VK_SEPARATOR:
            nk_input_key(&gdi.ctx, NK_KEY_ENTER, down);
            return 1;

        case VK_TAB:
            nk_input_key(&gdi.ctx, NK_KEY_TAB, down);
            return 1;

        case VK_LEFT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_LEFT, down);
            return 1;

        case VK_RIGHT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_RIGHT, down);
            return 1;

        case VK_BACK:
            nk_input_key(&gdi.ctx, NK_KEY_BACKSPACE, down);
            return 1;

        case VK_HOME:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_START, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_START, down);
            return 1;

        case VK_END:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_END, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_END, down);
            return 1;

        case VK_NEXT:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_DOWN, down);
            return 1;

        case VK_PRIOR:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_UP, down);
            return 1;

        case VK_ESCAPE:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_RESET_MODE, down);
            return 1;

        case VK_INSERT:
        /* Only switch on release to avoid repeat issues
         * kind of confusing since we have to negate it but we're already
         * hacking it since Nuklear treats them as two separate keys rather
         * than a single toggle state */
            if (!down) {
                insert_toggle = !insert_toggle;
                if (insert_toggle) {
                    nk_input_key(&gdi.ctx, NK_KEY_TEXT_INSERT_MODE, !down);
                    /* nk_input_key(&gdi.ctx, NK_KEY_TEXT_REPLACE_MODE, down); */
                } else {
                    nk_input_key(&gdi.ctx, NK_KEY_TEXT_REPLACE_MODE, !down);
                    /* nk_input_key(&gdi.ctx, NK_KEY_TEXT_INSERT_MODE, down); */
                }
            }
            return 1;

        case 'A':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_SELECT_ALL, down);
                return 1;
            }
            break;

        case 'B':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_LINE_START, down);
                return 1;
            }
            break;

        case 'E':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_LINE_END, down);
                return 1;
            }
            break;

        case 'C':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_COPY, down);
                return 1;
            }
            break;

        case 'V':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_PASTE, down);
                return 1;
            }
            break;

        case 'X':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_CUT, down);
                return 1;
            }
            break;

        case 'Z':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_UNDO, down);
                return 1;
            }
            break;

        case 'R':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_REDO, down);
                return 1;
            }
            break;
        }
        return 0;
    }

    case WM_CHAR:
        if (wparam >= 32)
        {
            nk_input_unicode(&gdi.ctx, (nk_rune)wparam);
            return 1;
        }
        break;

    case WM_LBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_LBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_RBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_RBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_MBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MOUSEWHEEL:
        nk_input_scroll(&gdi.ctx, nk_vec2(0,(float)(short)HIWORD(wparam) / WHEEL_DELTA));
        return 1;

    case WM_MOUSEMOVE:
        nk_input_motion(&gdi.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
        return 1;

    case WM_LBUTTONDBLCLK:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        return 1;
    }

    return 0;
}

/**
 * Shutdown the GDI backend and release its resources.
 *
 * Deletes the backend's offscreen bitmap and memory device context, and frees the associated Nuklear context and its allocations.
 */
NK_API void
nk_gdi_shutdown(void)
{
    DeleteObject(gdi.memory_dc);
    DeleteObject(gdi.bitmap);
    nk_free(&gdi.ctx);
}

/**
 * Render the current Nuklear command buffer to the window using Windows GDI.
 *
 * Clears the offscreen bitmap with the provided color, processes and draws each
 * recorded Nuklear command into the memory DC via the backend's GDI drawing
 * helpers, blits the resulting image to the window device context, and then
 * clears the Nuklear context.
 *
 * @param clear nk_color used to clear the offscreen buffer before drawing.
 */
NK_API void
nk_gdi_render(struct nk_color clear)
{
    const struct nk_command *cmd;

    HDC memory_dc = gdi.memory_dc;
    SelectObject(memory_dc, GetStockObject(DC_PEN));
    SelectObject(memory_dc, GetStockObject(DC_BRUSH));
    nk_gdi_clear(memory_dc, clear);

    nk_foreach(cmd, &gdi.ctx)
    {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_gdi_scissor(memory_dc, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_gdi_stroke_line(memory_dc, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_gdi_stroke_rect(memory_dc, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_gdi_fill_rect(memory_dc, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_gdi_stroke_circle(memory_dc, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_gdi_fill_circle(memory_dc, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_ARC: {
            const struct nk_command_arc *q = (const struct nk_command_arc *)cmd;
            nk_gdi_stroke_arc(memory_dc, q->cx, q->cy, q->r, q->a[0], q->a[1], q->line_thickness, q->color);
        } break;
        case NK_COMMAND_ARC_FILLED: {
            const struct nk_command_arc_filled *q = (const struct nk_command_arc_filled *)cmd;
            nk_gdi_fill_arc(memory_dc, q->cx, q->cy, q->r, q->a[0], q->a[1], q->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_gdi_stroke_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_gdi_fill_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_gdi_stroke_polygon(memory_dc, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_gdi_fill_polygon(memory_dc, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_gdi_stroke_polyline(memory_dc, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_gdi_draw_text(memory_dc, t->x, t->y, t->w, t->h,
                (const char*)t->string, t->length,
                (GdiFont*)t->font->userdata.ptr,
                t->background, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_gdi_stroke_curve(memory_dc, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR: {
            const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
            nk_gdi_rect_multi_color(memory_dc, r->x, r->y,r->w, r->h, r->left, r->top, r->right, r->bottom);
        } break;
        case NK_COMMAND_IMAGE: {
            const struct nk_command_image *i = (const struct nk_command_image *)cmd;
            nk_gdi_draw_image(i->x, i->y, i->w, i->h, i->img, i->col);
        } break;
        case NK_COMMAND_CUSTOM:
        default: break;
        }
    }
    nk_gdi_blit(gdi.window_dc);
    nk_clear(&gdi.ctx);
}

#endif
