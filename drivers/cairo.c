/* June 2, 2007
 *
 * Graphics drivers that are based on the Cairo / Pango Libraries.
 *
 * Copyright (C) 2008 Hazen Babcock
 *
 * This file is part of PLplot.
 *
 * PLplot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Library Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * PLplot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with PLplot; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/*---------------------------------------------------------------------
 * Header files
 * ---------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cairo.h>
#include <pango/pangocairo.h>

/* PLplot header files (must occur before driver-dependent includes) */

#include "plDevs.h"
#include "plplotP.h"
#include "drivers.h"

/* Driver-dependent includes */
#if defined ( PLD_xcairo )
#include <cairo-xlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#endif
#if defined ( PLD_pdfcairo )
#include <cairo-pdf.h>
#endif
#if defined ( PLD_pscairo )
#include <cairo-ps.h>
#endif
#if defined ( PLD_svgcairo )
#include <cairo-svg.h>
#endif


/*---------------------------------------------------------------------
 * Constants & global (to this file) variables
 * ---------------------------------------------------------------------*/

#define DPI                  72
#define PLCAIRO_DEFAULT_X    720
#define PLCAIRO_DEFAULT_Y    540

#define MAX_STRING_LEN       500
#define MAX_MARKUP_LEN       MAX_STRING_LEN * 10

static int    text_clipping;
static int    text_anti_aliasing;
static int    graphics_anti_aliasing;
static int    external_drawable;
static int    rasterize_image;
static int    set_background;

static DrvOpt cairo_options[] = { { "text_clipping",          DRV_INT, &text_clipping,          "Use text clipping (text_clipping=0|1)"                                                                                                                                                                                          },
                                  { "text_anti_aliasing",     DRV_INT, &text_anti_aliasing,     "Set desired text anti-aliasing (text_anti_aliasing=0|1|2|3). The numbers are in the same order as the cairo_antialias_t enumeration documented at http://cairographics.org/manual/cairo-cairo-t.html#cairo-antialias-t)"        },
                                  { "graphics_anti_aliasing", DRV_INT, &graphics_anti_aliasing, "Set desired graphics anti-aliasing (graphics_anti_aliasing=0|1|2|3). The numbers are in the same order as the cairo_antialias_t enumeration documented at http://cairographics.org/manual/cairo-cairo-t.html#cairo-antialias-t" },
                                  { "external_drawable",      DRV_INT, &external_drawable,      "Plot to external X drawable"                                                                                                                                                                                                    },
                                  { "rasterize_image",        DRV_INT, &rasterize_image,        "Raster or vector image rendering (rasterize_image=0|1)"                                                                                                                                                                         },
                                  { "set_background",         DRV_INT, &set_background,         "Set the background for the extcairo device (set_background=0|1). If 1 then the plot background will set by PLplot"                                                                                                              },
                                  { NULL,                     DRV_INT, NULL,                    NULL                                                                                                                                                                                                                             } };

typedef struct
{
    cairo_surface_t *cairoSurface;
    cairo_t         *cairoContext;
    cairo_surface_t *cairoSurface_raster;
    cairo_t         *cairoContext_raster;
    short           text_clipping;
    short           text_anti_aliasing;
    short           graphics_anti_aliasing;
    short           rasterize_image;
    short           set_background;
    double          downscale;
    char            *pangoMarkupString;
    short           upDown;
    float           fontSize;
#if defined ( PLD_xcairo )
    cairo_surface_t *cairoSurface_X;
    cairo_t         *cairoContext_X;
    short           exit_event_loop;
    Display         *XDisplay;
    Window          XWindow;
    unsigned int    xdrawable_mode;
#endif
#if defined ( PLD_memcairo )
    unsigned char   *memory;
    unsigned char   *cairo_format_memory;
    char            bigendian;
#endif
} PLCairo;

PLDLLIMPEXP_DRIVER const char* plD_DEVICE_INFO_cairo =
#if defined ( PLD_xcairo )
    "xcairo:Cairo X Windows Driver:1:cairo:59:xcairo\n"
#endif
#if defined ( PLD_pdfcairo )
    "pdfcairo:Cairo PDF Driver:0:cairo:60:pdfcairo\n"
#endif
#if defined ( PLD_pscairo )
    "pscairo:Cairo PS Driver:0:cairo:61:pscairo\n"
#endif
#if defined ( PLD_svgcairo )
    "svgcairo:Cairo SVG Driver:0:cairo:62:svgcairo\n"
#endif
#if defined ( PLD_pngcairo )
    "pngcairo:Cairo PNG Driver:0:cairo:63:pngcairo\n"
#endif
#if defined ( PLD_memcairo )
    "memcairo:Cairo Memory Driver:0:cairo:64:memcairo\n"
#endif
#if defined ( PLD_extcairo )
    "extcairo:Cairo External Context Driver:0:cairo:65:extcairo\n"
#endif
;

/*
 * Structure for passing external drawables to xcairo devices via
 * the PLESC_DEVINIT escape function.
 */
#if defined ( PLD_xcairo )
typedef struct
{
    Display  *display;
    Drawable drawable;
} PLXcairoDrawableInfo;
#endif

/*---------------------------------------------------------------------
 * Font style and weight lookup tables (copied
 * from the psttf driver).
 * ---------------------------------------------------------------------*/

#define NPANGOLOOKUP    5

const char *defaultFamilyLookup[NPANGOLOOKUP] = {
    "sans",
    "serif",
    "monospace",
    "sans,serif",
    "sans,serif"
};

const char *envFamilyLookup[NPANGOLOOKUP] = {
    "PLPLOT_FREETYPE_SANS_FAMILY",
    "PLPLOT_FREETYPE_SERIF_FAMILY",
    "PLPLOT_FREETYPE_MONO_FAMILY",
    "PLPLOT_FREETYPE_SCRIPT_FAMILY",
    "PLPLOT_FREETYPE_SYMBOL_FAMILY"
};

#define FAMILY_LOOKUP_LEN    1024
char familyLookup[NPANGOLOOKUP][FAMILY_LOOKUP_LEN];

#define TAG_LEN              200

const char *weightLookup[2] = {
    "normal",
    "bold"
};

const char *styleLookup[3] = {
    "normal",
    "italic",
    "oblique"
};


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is common to all the Cairo Drivers
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

/*---------------------------------------------------------------------
 * function declarations
 * ---------------------------------------------------------------------*/

/* General */

PLCairo *stream_and_font_setup( PLStream *, int );
cairo_status_t write_to_stream( void *, unsigned char *, unsigned int );
void set_clip( PLStream *pls );

/* String processing */

static void proc_str( PLStream *, EscText * );
static void text_begin_cairo( PLStream *pls, EscText *args );
static void text_char_cairo( PLStream *pls, EscText *args );
static void text_esc_cairo( PLStream *pls, EscText *args );
static void text_end_cairo( PLStream *pls, EscText *args );
static char *ucs4_to_pango_markup_format( PLUNICODE *, int, float );
static void open_span_tag( char *, PLUNICODE, float, int );
static void close_span_tag( char *, int );
static char *rise_span_tag( int, int );

/* Graphics */

static void set_current_context( PLStream * );
static void poly_line( PLStream *, short *, short *, PLINT );
static void filled_polygon( PLStream *pls, short *xa, short *ya, PLINT npts );
static void arc( PLStream *, arc_struct * );
static void rotate_cairo_surface( PLStream *, float, float, float, float, float, float );
/* Rasterization of plotted material */
static void start_raster( PLStream* );
static void end_raster( PLStream* );

/* PLplot interface functions */

/* general */
void plD_bop_cairo( PLStream * );
void plD_eop_cairo( PLStream * );
void plD_state_cairo( PLStream *, PLINT );
void plD_esc_cairo( PLStream *, PLINT, void * );
void plD_tidy_cairo( PLStream * );
void plD_line_cairo( PLStream *, short, short, short, short );
void plD_polyline_cairo( PLStream *, short *, short *, PLINT );

/*----------------------------------------------------------------------
 * start_raster()
 *
 * Set up off-screen rasterized rendering
 * ----------------------------------------------------------------------*/

void start_raster( PLStream *pls )
{
    PLCairo         *aStream;
    cairo_surface_t *tmp_sfc;
    cairo_t         *tmp_context;

    aStream = (PLCairo *) pls->dev;

    /* Do not use the external surface if the user says not to */
    if ( !aStream->rasterize_image )
        return;

    /* Create an image surface and context for the offscreen rendering */
    aStream->cairoSurface_raster =
        cairo_image_surface_create( CAIRO_FORMAT_ARGB32, pls->xlength, pls->ylength );
    aStream->cairoContext_raster = cairo_create( aStream->cairoSurface_raster );

    /* Disable antialiasing for the raster surface.  The output seems to look
     * better that way. */
    cairo_set_antialias( aStream->cairoContext_raster, CAIRO_ANTIALIAS_NONE );

    /* Swap the raster and main plot surfaces and contexts */
    tmp_sfc               = aStream->cairoSurface;
    tmp_context           = aStream->cairoContext;
    aStream->cairoSurface = aStream->cairoSurface_raster;
    aStream->cairoContext = aStream->cairoContext_raster;
    /* Save the main plot surface and context for when we are finished */
    aStream->cairoSurface_raster = tmp_sfc;
    aStream->cairoContext_raster = tmp_context;
}

/*----------------------------------------------------------------------
 * end_raster()
 *
 * Finish off-screen rasterized rendering and copy the result on to the
 * main plot surface.
 * ----------------------------------------------------------------------*/

void end_raster( PLStream *pls )
{
    PLCairo         *aStream;
    cairo_surface_t *tmp_sfc;
    cairo_t         *tmp_context;

    aStream = (PLCairo *) pls->dev;

    /* Do not use the external surface if the user says not to */
    if ( !aStream->rasterize_image )
        return;

    /* Some Cairo devices support delayed device setup (eg: xcairo with
     * external drawable and extcairo with an external context). */
    if ( aStream->cairoContext == NULL )
        plexit( "Can not plot to a Cairo device with no context" );

    /* Restore the main plot surface and context for future plotting */
    tmp_sfc                      = aStream->cairoSurface;
    tmp_context                  = aStream->cairoContext;
    aStream->cairoSurface        = aStream->cairoSurface_raster;
    aStream->cairoContext        = aStream->cairoContext_raster;
    aStream->cairoSurface_raster = tmp_sfc;
    aStream->cairoContext_raster = tmp_context;

    /* Blit the raster surface on to the main plot */
    cairo_set_source_surface( aStream->cairoContext, aStream->cairoSurface_raster, 0.0, 0.0 );
    cairo_paint( aStream->cairoContext );

    /* Free the now extraneous surface and context */
    cairo_destroy( aStream->cairoContext_raster );
    cairo_surface_destroy( aStream->cairoSurface_raster );
}

/*----------------------------------------------------------------------
 * plD_bop_cairo()
 *
 * Set up for the next page.
 * ----------------------------------------------------------------------*/

void plD_bop_cairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Some Cairo devices support delayed device setup (eg: xcairo with
     * external drawable and extcairo with an external context). */
    if ( aStream->cairoContext == NULL )
        return;

    /* Fill in the window with the background color. */
    cairo_rectangle( aStream->cairoContext, 0.0, 0.0, pls->xlength, pls->ylength );
    cairo_set_source_rgba( aStream->cairoContext,
        (double) pls->cmap0[0].r / 255.0,
        (double) pls->cmap0[0].g / 255.0,
        (double) pls->cmap0[0].b / 255.0,
        (double) pls->cmap0[0].a );
    cairo_fill( aStream->cairoContext );
}

/*---------------------------------------------------------------------
 * plD_line_cairo()
 *
 * Draw a line in the current color from (x1,y1) to (x2,y2).
 * ----------------------------------------------------------------------*/

/*---------------------------------------------------------------------
 * (get|set)_line_properties
 *
 * (Get|Set) the current Cairo line drawing properties.
 * ---------------------------------------------------------------------*/
void get_line_properties( PLCairo *aStream, cairo_line_join_t *join, cairo_line_cap_t *cap )
{
    *join = cairo_get_line_join( aStream->cairoContext );
    *cap  = cairo_get_line_cap( aStream->cairoContext );
}

void set_line_properties( PLCairo *aStream, cairo_line_join_t join, cairo_line_cap_t cap )
{
    cairo_set_line_join( aStream->cairoContext, join );
    cairo_set_line_cap( aStream->cairoContext, cap );
}

void plD_line_cairo( PLStream *pls, short x1a, short y1a, short x2a, short y2a )
{
    PLCairo           *aStream;
    cairo_line_join_t old_join;
    cairo_line_cap_t  old_cap;

    aStream = (PLCairo *) pls->dev;

    set_current_context( pls );

    get_line_properties( aStream, &old_join, &old_cap );
    set_line_properties( aStream, old_join, CAIRO_LINE_CAP_ROUND );

    cairo_move_to( aStream->cairoContext, aStream->downscale * (double) x1a, aStream->downscale * (double) y1a );
    cairo_line_to( aStream->cairoContext, aStream->downscale * (double) x2a, aStream->downscale * (double) y2a );
    cairo_stroke( aStream->cairoContext );

    set_line_properties( aStream, old_join, old_cap );
}

/*---------------------------------------------------------------------
 * plD_polyline_cairo()
 *
 * Draw a polyline in the current color.
 * ---------------------------------------------------------------------*/

void plD_polyline_cairo( PLStream *pls, short *xa, short *ya, PLINT npts )
{
    PLCairo           *aStream;
    cairo_line_join_t old_join;
    cairo_line_cap_t  old_cap;

    aStream = (PLCairo *) pls->dev;

    get_line_properties( aStream, &old_join, &old_cap );
    set_line_properties( aStream, CAIRO_LINE_JOIN_BEVEL, CAIRO_LINE_CAP_BUTT );

    poly_line( pls, xa, ya, npts );
    cairo_stroke( aStream->cairoContext );

    set_line_properties( aStream, old_join, old_cap );
}

/*---------------------------------------------------------------------
 * plD_eop_cairo()
 *
 * Generic end of page.
 * ---------------------------------------------------------------------*/

void plD_eop_cairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;
    cairo_show_page( aStream->cairoContext );
}

/*---------------------------------------------------------------------
 * plD_tidy_cairo()
 *
 * General: Close graphics file or otherwise clean up.
 * ---------------------------------------------------------------------*/

void plD_tidy_cairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Free the cairo context and surface. */
    cairo_destroy( aStream->cairoContext );
    cairo_surface_destroy( aStream->cairoSurface );

    if ( pls->OutFile )
        fclose( pls->OutFile );
}

/*---------------------------------------------------------------------
 * plD_state_cairo()
 *
 * Handle change in PLStream state (color, pen width, fill attribute, etc).
 *
 * Nothing is done here because these attributes are acquired from
 * PLStream for each element that is drawn.
 * ---------------------------------------------------------------------*/

void plD_state_cairo( PLStream *pls, PLINT op )
{
}

/*---------------------------------------------------------------------
 * plD_esc_cairo()
 *
 * Generic escape function.
 * ---------------------------------------------------------------------*/

void plD_esc_cairo( PLStream *pls, PLINT op, void *ptr )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    switch ( op )
    {
    case PLESC_FILL:     /* filled polygon */
        filled_polygon( pls, pls->dev_x, pls->dev_y, pls->dev_npts );
        break;
    case PLESC_HAS_TEXT:
        if ( !pls->alt_unicode )
        {
            proc_str( pls, (EscText *) ptr );
        }
        break;
    case PLESC_BEGIN_TEXT: /* get ready to get a handle a string of text */
        text_begin_cairo( pls, (EscText *) ptr );
        break;
    case PLESC_TEXT_CHAR: /* handle a character of text to display */
        text_char_cairo( pls, (EscText *) ptr );
        break;
    case PLESC_CONTROL_CHAR: /* handle a control character (super/subscript of fontchange) */
        text_esc_cairo( pls, (EscText *) ptr );
        break;
    case PLESC_END_TEXT: /* finish a string of text */
        text_end_cairo( pls, (EscText *) ptr );
        break;
    case PLESC_START_RASTERIZE: /* Start offscreen/rasterized rendering */
        start_raster( pls );
        break;
    case PLESC_END_RASTERIZE: /* End offscreen/rasterized rendering */
        end_raster( pls );
        break;
    case PLESC_ARC: /* Draw an arc, either filled or outline */
        arc( pls, (arc_struct *) ptr );
        break;
    }
}

/*---------------------------------------------------------------------
 * text_begin_cairo()
 *
 * Begin text.
 * ---------------------------------------------------------------------*/

void text_begin_cairo( PLStream *pls, EscText *args )
{
    int     i;
    PLCairo *aStream;

    aStream                    = (PLCairo *) pls->dev;
    aStream->upDown            = 0;
    aStream->pangoMarkupString = (char *) malloc( sizeof ( char ) * MAX_MARKUP_LEN );
    aStream->fontSize          = pls->chrht * DPI / 25.4;
    for ( i = 0; i < MAX_MARKUP_LEN; i++ )
    {
        aStream->pangoMarkupString[i] = 0;
    }
    open_span_tag( aStream->pangoMarkupString, args->n_fci, aStream->fontSize, 0 );
}

/*---------------------------------------------------------------------
 * text_char_cairo()
 *
 * Add text.
 * ---------------------------------------------------------------------*/

void text_char_cairo( PLStream *pls, EscText *args )
{
    char    utf8[5];
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;
    /* make sure we are not too close to the end of the string */
    if ( strlen( aStream->pangoMarkupString ) < ( MAX_MARKUP_LEN - 50 ))
    {
        switch ( args->n_char )
        {
        case 38:
            strncat( aStream->pangoMarkupString, "&#38;", MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
            break;
        case 60:
            strncat( aStream->pangoMarkupString, "&#60;", MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
            break;
        case 62:
            strncat( aStream->pangoMarkupString, "&#62;", MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
            break;
        default:
            ucs4_to_utf8( args->n_char, utf8 );
            strncat( aStream->pangoMarkupString, utf8, MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
            break;
        }
    }
}

/*---------------------------------------------------------------------
 * text_esc_cairo()
 *
 * A font change, superscript, subscript, etc...
 * ---------------------------------------------------------------------*/

void text_esc_cairo( PLStream *pls, EscText *args )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;
    switch ( args->n_ctrl_char )
    {
    case PLTEXT_FONTCHANGE:
        close_span_tag( aStream->pangoMarkupString, aStream->upDown );
        open_span_tag( aStream->pangoMarkupString, args->n_fci, aStream->fontSize, aStream->upDown );
        break;
    case PLTEXT_SUPERSCRIPT:
        if ( aStream->upDown < 0 )
        {
            strncat( aStream->pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
        }
        else
        {
            strncat( aStream->pangoMarkupString, rise_span_tag( aStream->upDown, 1 ), MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
        }
        aStream->upDown++;
        break;
    case PLTEXT_SUBSCRIPT:
        if ( aStream->upDown > 0 )
        {
            strncat( aStream->pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
        }
        else
        {
            strncat( aStream->pangoMarkupString, rise_span_tag( aStream->upDown, -1 ), MAX_MARKUP_LEN - 1 - strlen( aStream->pangoMarkupString ));
        }
        aStream->upDown--;
        break;
    }
}

/*---------------------------------------------------------------------
 * text_end_cairo()
 *
 * Draw the text and clean up.
 * ---------------------------------------------------------------------*/

void text_end_cairo( PLStream *pls, EscText *args )
{
    int   textXExtent, textYExtent;
    PLFLT rotation, shear, stride, cos_rot, sin_rot, cos_shear, sin_shear;
    cairo_matrix_t       *cairoTransformMatrix;
    cairo_font_options_t *cairoFontOptions;
    PangoContext         *context;
    PangoLayout          *layout;
    PLCairo              *aStream;

    aStream = (PLCairo *) pls->dev;

    set_current_context( pls );

    /* Close the last span tag. */
    close_span_tag( aStream->pangoMarkupString, aStream->upDown );

    /* Create the Pango text layout so we can figure out how big it is */
    layout = pango_cairo_create_layout( aStream->cairoContext );
    pango_layout_set_markup( layout, aStream->pangoMarkupString, -1 );
    pango_layout_get_pixel_size( layout, &textXExtent, &textYExtent );

    /* Set font aliasing */
    context          = pango_layout_get_context( layout );
    cairoFontOptions = cairo_font_options_create();
    cairo_font_options_set_antialias( cairoFontOptions, aStream->text_anti_aliasing );
    pango_cairo_context_set_font_options( context, cairoFontOptions );
    pango_layout_context_changed( layout );
    cairo_font_options_destroy( cairoFontOptions );

    /* Save current transform matrix & clipping region */
    cairo_save( aStream->cairoContext );

    /* Set up the clipping region if we are doing text clipping */
    if ( aStream->text_clipping )
    {
        set_clip( pls );
    }

    /* Move to the string reference point */
    cairo_move_to( aStream->cairoContext, aStream->downscale * (double) args->x, aStream->downscale * (double) args->y );

    /* Invert the coordinate system so that the text is drawn right side up */
    cairoTransformMatrix = (cairo_matrix_t *) malloc( sizeof ( cairo_matrix_t ));
    cairo_matrix_init( cairoTransformMatrix, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0 );
    cairo_transform( aStream->cairoContext, cairoTransformMatrix );

    /* Extract rotation angle and shear from the PLplot tranformation matrix.
     * Compute sines and cosines of the angles as an optimization. */
    plRotationShear( args->xform, &rotation, &shear, &stride );
    rotation -= pls->diorot * PI / 2.0;
    cos_rot   = cos( rotation );
    sin_rot   = sin( rotation );
    cos_shear = cos( shear );
    sin_shear = sin( shear );

    /* Apply the transform matrix */
    cairo_matrix_init( cairoTransformMatrix,
        cos_rot * stride,
        -sin_rot * stride,
        cos_rot * sin_shear + sin_rot * cos_shear,
        -sin_rot * sin_shear + cos_rot * cos_shear,
        0, 0 );
    cairo_transform( aStream->cairoContext, cairoTransformMatrix );
    free( cairoTransformMatrix );

    /* Move to the text starting point */
    cairo_rel_move_to( aStream->cairoContext,
        (double) ( -1.0 * args->just * (double) textXExtent ),
        (double) ( -0.5 * textYExtent ));

    /* Render the text */
    pango_cairo_show_layout( aStream->cairoContext, layout );

    /* Restore the transform matrix to its state prior to the text transform. */
    cairo_restore( aStream->cairoContext );

    /* Free the layout object and the markup string. */
    g_object_unref( layout );
    free( aStream->pangoMarkupString );
}

/*---------------------------------------------------------------------
 * proc_str()
 *
 * Processes strings for display.
 * ---------------------------------------------------------------------*/

void proc_str( PLStream *pls, EscText *args )
{
    float fontSize;
    int   textXExtent, textYExtent;
    char                 *textWithPangoMarkup;
    PLFLT rotation, shear, stride, cos_rot, sin_rot, cos_shear, sin_shear;
    cairo_matrix_t       *cairoTransformMatrix;
    cairo_font_options_t *cairoFontOptions;
    PangoContext         *context;
    PangoLayout          *layout;
    PLCairo              *aStream;

    aStream = (PLCairo *) pls->dev;

    set_current_context( pls );

    /* Check that we got unicode, warning message and return if not */
    if ( args->unicode_array_len == 0 )
    {
        printf( "Non unicode string passed to a cairo driver, ignoring\n" );
        return;
    }

    /* Check that unicode string isn't longer then the max we allow */
    if ( args->unicode_array_len >= MAX_STRING_LEN )
    {
        printf( "Sorry, the cairo drivers only handles strings of length < %d\n", MAX_STRING_LEN );
        return;
    }

    /* Calculate the font size (in pixels) */
    fontSize = pls->chrht * DPI / 25.4;

    /* Convert the escape characters into the appropriate Pango markup */
    textWithPangoMarkup = ucs4_to_pango_markup_format( args->unicode_array, args->unicode_array_len, fontSize );

    /* Create the Pango text layout so we can figure out how big it is */
    layout = pango_cairo_create_layout( aStream->cairoContext );
    pango_layout_set_markup( layout, textWithPangoMarkup, -1 );
    pango_layout_get_pixel_size( layout, &textXExtent, &textYExtent );

    /* Set font aliasing */
    context          = pango_layout_get_context( layout );
    cairoFontOptions = cairo_font_options_create();
    cairo_font_options_set_antialias( cairoFontOptions, aStream->text_anti_aliasing );
    pango_cairo_context_set_font_options( context, cairoFontOptions );
    pango_layout_context_changed( layout );
    cairo_font_options_destroy( cairoFontOptions );

    /* Save current transform matrix & clipping region */
    cairo_save( aStream->cairoContext );

    /* Set up the clipping region if we are doing text clipping */
    if ( aStream->text_clipping )
    {
        set_clip( pls );
    }

    /* Move to the string reference point */
    cairo_move_to( aStream->cairoContext, aStream->downscale * (double) args->x, aStream->downscale * (double) args->y );

    /* Invert the coordinate system so that the text is drawn right side up */
    cairoTransformMatrix = (cairo_matrix_t *) malloc( sizeof ( cairo_matrix_t ));
    cairo_matrix_init( cairoTransformMatrix, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0 );
    cairo_transform( aStream->cairoContext, cairoTransformMatrix );

    /* Extract rotation angle and shear from the PLplot tranformation matrix.
     * Compute sines and cosines of the angles as an optimization. */
    plRotationShear( args->xform, &rotation, &shear, &stride );
    rotation -= pls->diorot * PI / 2.0;
    cos_rot   = cos( rotation );
    sin_rot   = sin( rotation );
    cos_shear = cos( shear );
    sin_shear = sin( shear );

    /* Apply the transform matrix */
    cairo_matrix_init( cairoTransformMatrix,
        cos_rot * stride,
        -sin_rot * stride,
        cos_rot * sin_shear + sin_rot * cos_shear,
        -sin_rot * sin_shear + cos_rot * cos_shear,
        0, 0 );
    cairo_transform( aStream->cairoContext, cairoTransformMatrix );
    free( cairoTransformMatrix );

    /* Move to the text starting point */
    cairo_rel_move_to( aStream->cairoContext,
        (double) ( -1.0 * args->just * (double) textXExtent ),
        (double) ( -0.5 * textYExtent ));

    /* Render the text */
    pango_cairo_show_layout( aStream->cairoContext, layout );

    /* Restore the transform matrix to its state prior to the text transform. */
    cairo_restore( aStream->cairoContext );

    /* Free the layout object and the markup string. */
    g_object_unref( layout );
    free( textWithPangoMarkup );
}

/*---------------------------------------------------------------------
 * ucs4_to_pango_markup_format()
 *
 * Converts the plplot string (in ucs4) to a utf8 string that includes
 * pango markup.
 *
 * http://developer.gnome.org/doc/API/2.0/pango/PangoMarkupFormat.html
 * ---------------------------------------------------------------------*/

char *ucs4_to_pango_markup_format( PLUNICODE *ucs4, int ucs4Len, float fontSize )
{
    char      plplotEsc;
    int       i;
    int       upDown = 0;
    PLUNICODE fci;
    char      utf8[5];
    char      *pangoMarkupString;

    /* Will this be big enough? We might have lots of markup. */
    pangoMarkupString = (char *) malloc( sizeof ( char ) * MAX_MARKUP_LEN );
    for ( i = 0; i < MAX_MARKUP_LEN; i++ )
    {
        pangoMarkupString[i] = 0;
    }

    /* Get PLplot escape character */
    plgesc( &plplotEsc );

    /* Get the curent font and open the first span tag */
    plgfci( &fci );
    open_span_tag( pangoMarkupString, fci, fontSize, 0 );

    /* Parse the string to generate the tags */
    i = 0;
    while ( i < ucs4Len )
    {
        /* Try to avoid going off the end of the string */
        if ( strlen( pangoMarkupString ) > ( MAX_MARKUP_LEN - 50 ))
        {
            continue;
        }
        if ( ucs4[i] < PL_FCI_MARK )                /* not a font change */
        {
            if ( ucs4[i] != (PLUNICODE) plplotEsc ) /* a character to display */
            {                                       /* we have to handle "<", ">" and "&" separately since they throw off the XML */
                switch ( ucs4[i] )
                {
                case 38:
                    strncat( pangoMarkupString, "&#38;", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    break;
                case 60:
                    strncat( pangoMarkupString, "&#60;", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    break;
                case 62:
                    strncat( pangoMarkupString, "&#62;", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    break;
                default:
                    ucs4_to_utf8( ucs4[i], utf8 );
                    strncat( pangoMarkupString, utf8, MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    break;
                }
                i++;
                continue;
            }
            i++;
            if ( ucs4[i] == (PLUNICODE) plplotEsc ) /* a escape character to display */
            {
                ucs4_to_utf8( ucs4[i], utf8 );
                strncat( pangoMarkupString, utf8, MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                i++;
                continue;
            }
            else
            {
                if ( ucs4[i] == (PLUNICODE) 'u' ) /* Superscript */
                {
                    if ( upDown < 0 )
                    {
                        strncat( pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    }
                    else
                    {
                        strncat( pangoMarkupString, rise_span_tag( upDown, 1 ), MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    }
                    upDown++;
                }
                if ( ucs4[i] == (PLUNICODE) 'd' ) /* Subscript */
                {
                    if ( upDown > 0 )
                    {
                        strncat( pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    }
                    else
                    {
                        strncat( pangoMarkupString, rise_span_tag( upDown, -1 ), MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
                    }
                    upDown--;
                }
                i++;
            }
        }
        else /* a font change */
        {
            close_span_tag( pangoMarkupString, upDown );
            open_span_tag( pangoMarkupString, ucs4[i], fontSize, upDown );
            i++;
        }
    }

    /* Close the last span tag. */
    close_span_tag( pangoMarkupString, upDown );

    /* printf("%s\n", pangoMarkupString); */

    return pangoMarkupString;
}

/*---------------------------------------------------------------------
 * open_span_tag
 *
 * 1. Opens a span tag with the appropriate font description given the
 *   current fci.
 * 2. Add the appropriate number of <sub> or <sup> tags to bring us
 *   back to our current sub/super-script level.
 * ---------------------------------------------------------------------*/

void open_span_tag( char *pangoMarkupString, PLUNICODE fci, float fontSize, int upDown )
{
    unsigned char fontFamily, fontStyle, fontWeight;
    char          openTag[TAG_LEN];

    /* Generate the font info for the open tag & concatenate this
     * onto the markup string. */
    plP_fci2hex( fci, &fontFamily, PL_FCI_FAMILY );
    plP_fci2hex( fci, &fontStyle, PL_FCI_STYLE );
    plP_fci2hex( fci, &fontWeight, PL_FCI_WEIGHT );
    snprintf( openTag, TAG_LEN, "<span font_desc=\"%s %.2f\" ", familyLookup[fontFamily], fontSize );
    strncat( pangoMarkupString, openTag, MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));

    snprintf( openTag, TAG_LEN, "style=\"%s\" ", styleLookup[fontStyle] );
    strncat( pangoMarkupString, openTag, MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));

    snprintf( openTag, TAG_LEN, "weight=\"%s\">", weightLookup[fontWeight] );
    strncat( pangoMarkupString, openTag, MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));

    /* Move to the right sub/super-script level */
    if ( upDown > 0 )
    {
        while ( upDown > 0 )
        {
            upDown--;
            strncat( pangoMarkupString, rise_span_tag( upDown, 1 ), MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
        }
    }
    if ( upDown < 0 )
    {
        while ( upDown < 0 )
        {
            upDown++;
            strncat( pangoMarkupString, rise_span_tag( upDown, -1 ), MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
        }
    }
}

/*---------------------------------------------------------------------
 * close_span_tag
 *
 * Close a span tag & brings us down to zero sub/super-script level.
 * ---------------------------------------------------------------------*/

void close_span_tag( char *pangoMarkupString, int upDown )
{
    if ( upDown > 0 )
    {
        while ( upDown > 0 )
        {
            strncat( pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
            upDown--;
        }
    }
    if ( upDown < 0 )
    {
        while ( upDown < 0 )
        {
            strncat( pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
            upDown++;
        }
    }

    strncat( pangoMarkupString, "</span>", MAX_MARKUP_LEN - 1 - strlen( pangoMarkupString ));
}

/*---------------------------------------------------------------------
 * rise_span_tag
 *
 * Create a rise span tag w/ appropriate font size & baseline offset
 * ---------------------------------------------------------------------*/

#define STEPSIZE    7500.0

char *rise_span_tag( int level, int direction )
{
    int         i;
    float       multiplier = 1.0;
    float       rise       = STEPSIZE;
    static char tag[100];

    for ( i = 0; i < abs( level ); i++ )
    {
        multiplier = multiplier * 0.9;
        rise      += STEPSIZE * multiplier;
    }
    if ( direction > 0 )
    {
        sprintf( tag, "<span rise=\"%d\" size=\"smaller\">", (int) rise );
    }
    else
    {
        sprintf( tag, "<span rise=\"%d\" size=\"smaller\">", (int) -rise );
    }

    return ( tag );
}

/*---------------------------------------------------------------------
 * write_to_stream()
 *
 * Writes data to a open file stream. This function is passed to the
 * Cairo file IO devices.
 * ---------------------------------------------------------------------*/

cairo_status_t write_to_stream( void *filePointer, unsigned char *data, unsigned int length )
{
    int bytes_written;

    bytes_written = fwrite( data, 1, length, (FILE*) filePointer );
    if ( bytes_written == length )
    {
        return CAIRO_STATUS_SUCCESS;
    }
    else
    {
        return CAIRO_STATUS_WRITE_ERROR;
    }
}

/*---------------------------------------------------------------------
 * stream_and_font_setup()
 *
 * Initializes the PLStream structure for the cairo devices.
 * Initializes the font lookup table.
 * Checks for cairo specific user options.
 * Returns a new PLCairo structure.
 * ---------------------------------------------------------------------*/

PLCairo *stream_and_font_setup( PLStream *pls, int interactive )
{
    int     i;
    char    *a;
    PLCairo *aStream;
    PLFLT   downscale;
    downscale = 0.0;

    /* Stream setup */
    pls->termin      = interactive; /* Interactive device */
    pls->dev_flush   = 1;           /* Handles flushes */
    pls->color       = 1;           /* Supports color */
    pls->dev_text    = 1;           /* Handles text */
    pls->dev_unicode = 1;           /* Wants unicode text */
    pls->alt_unicode = 1;           /* Wants to handle unicode character by character */
    pls->page        = 0;
    pls->dev_fill0   = 1;           /* Supports hardware solid fills */
    pls->dev_arc     = 1;           /* Supports driver-level arcs */
    pls->plbuf_write = interactive; /* Activate plot buffer */


    if ( pls->xlength <= 0 || pls->ylength <= 0 )
    {
        pls->xlength = PLCAIRO_DEFAULT_X;
        pls->ylength = PLCAIRO_DEFAULT_Y;
    }
    /* Calculate ratio of (smaller) external coordinates used for cairo
     * devices to (larger) internal PLplot coordinates. */
    if ( pls->xlength > pls->ylength )
        downscale = (double) pls->xlength / (double) ( PIXELS_X - 1 );
    else
        downscale = (double) pls->ylength / (double) PIXELS_Y;
    plP_setphy((PLINT) 0, (PLINT) ( pls->xlength / downscale ), (PLINT) 0, (PLINT) ( pls->ylength / downscale ));
    plP_setpxl( DPI / 25.4 / downscale, DPI / 25.4 / downscale );

    /* Initialize font table with either enviroment variables or defaults.
     * This was copied from the psttf driver. */
    for ( i = 0; i < NPANGOLOOKUP; i++ )
    {
        if (( a = getenv( envFamilyLookup[i] )) != NULL )
        {
            strncpy( familyLookup[i], a, FAMILY_LOOKUP_LEN - 1 );
            familyLookup[i][FAMILY_LOOKUP_LEN - 1] = '\0';
        }
        else
        {
            strncpy( familyLookup[i], defaultFamilyLookup[i], FAMILY_LOOKUP_LEN - 1 );
            familyLookup[i][FAMILY_LOOKUP_LEN - 1] = '\0';
        }
    }

    /* Allocate a cairo stream structure */
    aStream = malloc( sizeof ( PLCairo ));
#if defined ( PLD_xcairo )
    aStream->XDisplay = NULL;
    aStream->XWindow  = -1;
#endif
    aStream->cairoSurface = NULL;
    aStream->cairoContext = NULL;
    aStream->downscale    = downscale;

    /* Set text clipping on by default since it makes little difference in
     * speed for a modern cairo stack.*/
    aStream->text_clipping = 1;
    text_clipping          = 1;
    text_anti_aliasing     = 0; /* use 'default' text aliasing by default */
    graphics_anti_aliasing = 0; /* use 'default' graphics aliasing by default */
    rasterize_image        = 1; /* Enable rasterization by default */
    set_background         = 0; /* Default for extcairo is that PLplot not change the background */

    /* Check for cairo specific options */
    plParseDrvOpts( cairo_options );

    /* Turn off text clipping if the user desires this */
    if ( !text_clipping )
    {
        aStream->text_clipping = 0;
    }

    /* Record users desired text and graphics aliasing and rasterization */
    aStream->text_anti_aliasing     = text_anti_aliasing;
    aStream->graphics_anti_aliasing = graphics_anti_aliasing;
    aStream->rasterize_image        = rasterize_image;
    aStream->set_background         = set_background;

    return aStream;
}

/*---------------------------------------------------------------------
 * set_current_context()
 *
 * Updates the cairo graphics context with the current values in
 * PLStream.
 * ---------------------------------------------------------------------*/

void set_current_context( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;
    cairo_set_source_rgba( aStream->cairoContext,
        (double) pls->curcolor.r / 255.0,
        (double) pls->curcolor.g / 255.0,
        (double) pls->curcolor.b / 255.0,
        (double) pls->curcolor.a );
    /* In Cairo, zero width lines are not hairlines, they are completely invisible. */
    if ( pls->width < 1 )
    {
        cairo_set_line_width( aStream->cairoContext, 1.0 );
    }
    else
    {
        cairo_set_line_width( aStream->cairoContext, (double) pls->width );
    }
}

/*---------------------------------------------------------------------
 * poly_line()
 *
 * Draws a multi-segmented line. It is then up to the calling function
 * to decide whether to just draw the line, or fill in the area
 * enclosed by the line.
 * ---------------------------------------------------------------------*/

void poly_line( PLStream *pls, short *xa, short *ya, PLINT npts )
{
    int     i;
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    set_current_context( pls );

    cairo_move_to( aStream->cairoContext, aStream->downscale * (double) xa[0], aStream->downscale * (double) ya[0] );
    for ( i = 1; i < npts; i++ )
    {
        cairo_line_to( aStream->cairoContext, aStream->downscale * (double) xa[i], aStream->downscale * (double) ya[i] );
    }
}

/*---------------------------------------------------------------------
 * filled_polygon()
 *
 * Draws a filled polygon.
 * ---------------------------------------------------------------------*/

void filled_polygon( PLStream *pls, short *xa, short *ya, PLINT npts )
{
    int i;
    PLCairo           *aStream;
    cairo_line_join_t old_line_join;
    cairo_line_cap_t  old_line_cap;
    cairo_antialias_t aa_state;

    aStream = (PLCairo *) pls->dev;

    /* Save the previous line drawing style */
    get_line_properties( aStream, &old_line_join, &old_line_cap );

    /* These line properties make for a nicer looking polygon mesh */
    set_line_properties( aStream, CAIRO_LINE_JOIN_BEVEL, CAIRO_LINE_CAP_BUTT );

    /* Draw the polygons */
    cairo_move_to( aStream->cairoContext, aStream->downscale * (double) xa[0], aStream->downscale * (double) ya[0] );
    for ( i = 1; i < npts; i++ )
        cairo_line_to( aStream->cairoContext, aStream->downscale * (double) xa[i], aStream->downscale * (double) ya[i] );
    cairo_set_source_rgba( aStream->cairoContext,
        (double) pls->curcolor.r / 255.0,
        (double) pls->curcolor.g / 255.0,
        (double) pls->curcolor.b / 255.0,
        (double) pls->curcolor.a );
    aa_state = cairo_get_antialias( aStream->cairoContext );
    /* Add an extra outline stroke to the polygon unless the plotting color is
     * not opaque or antialiasing is disabled. */
    if (( pls->curcolor.a > 0.99 ) && ( aa_state != CAIRO_ANTIALIAS_NONE ))
    {
        cairo_fill_preserve( aStream->cairoContext );
        cairo_set_line_width( aStream->cairoContext, 1.0 );
        cairo_stroke( aStream->cairoContext );
    }
    else
        cairo_fill( aStream->cairoContext );

    /* Restore the previous line drawing style */
    set_line_properties( aStream, old_line_join, old_line_cap );
}

/*---------------------------------------------------------------------
 * set_clip()
 *
 * Set the clipping region to the plot window.
 * NOTE: cairo_save() and cairo_restore() should probably be called before
 * and after this, respectively.
 * ---------------------------------------------------------------------*/

void set_clip( PLStream *pls )
{
    PLINT   rcx[4], rcy[4];
    PLCairo *aStream;
    aStream = (PLCairo *) pls->dev;

    /* Use PLplot core routine to get the corners of the clipping rectangle */
    difilt_clip( rcx, rcy );

    /* Layout the bounds of the clipping region */
    /* Should we convert PLINT to short and use the polyline routine? */
    cairo_move_to( aStream->cairoContext,
        aStream->downscale * (double) rcx[0],
        aStream->downscale * (double) rcy[0] );
    cairo_line_to( aStream->cairoContext,
        aStream->downscale * (double) rcx[1],
        aStream->downscale * (double) rcy[1] );
    cairo_line_to( aStream->cairoContext,
        aStream->downscale * (double) rcx[2],
        aStream->downscale * (double) rcy[2] );
    cairo_line_to( aStream->cairoContext,
        aStream->downscale * (double) rcx[3],
        aStream->downscale * (double) rcy[3] );
    cairo_line_to( aStream->cairoContext,
        aStream->downscale * (double) rcx[0],
        aStream->downscale * (double) rcy[0] );

    /* Set the clipping region */
    cairo_clip( aStream->cairoContext );

    /* Apparently, in some older Cairo versions, cairo_clip does not consume
     * the current path. */
    cairo_new_path( aStream->cairoContext );
}

/*---------------------------------------------------------------------
 * arc()
 *
 * Draws an arc, possibly filled.
 * ---------------------------------------------------------------------*/

void arc( PLStream *pls, arc_struct *arc_info )
{
    PLCairo *aStream;
    double  x, y, a, b;
    double  angle1, angle2;

    set_current_context( pls );

    aStream = (PLCairo *) pls->dev;

    /* Scale to the proper Cairo coordinates */
    x = aStream->downscale * arc_info->x;
    y = aStream->downscale * arc_info->y;
    a = aStream->downscale * arc_info->a;
    b = aStream->downscale * arc_info->b;

    /* Degrees to radians */
    angle1 = arc_info->angle1 * M_PI / 180.0;
    angle2 = arc_info->angle2 * M_PI / 180.0;

    cairo_save( aStream->cairoContext );

    /* Clip the output to the plotting window */
    set_clip( pls );

    /* Make sure the arc is properly shaped and oriented */
    cairo_save( aStream->cairoContext );
    cairo_translate( aStream->cairoContext, x, y );
    cairo_scale( aStream->cairoContext, a, b );
    cairo_arc( aStream->cairoContext, 0.0, 0.0, 1.0, angle1, angle2 );
    if ( arc_info->fill )
        cairo_line_to( aStream->cairoContext, 0.0, 0.0 );
    cairo_restore( aStream->cairoContext );

    cairo_set_source_rgba( aStream->cairoContext,
        (double) pls->curcolor.r / 255.0,
        (double) pls->curcolor.g / 255.0,
        (double) pls->curcolor.b / 255.0,
        (double) pls->curcolor.a );
    if ( arc_info->fill )
    {
        cairo_fill( aStream->cairoContext );
    }
    else
    {
        cairo_stroke( aStream->cairoContext );
    }
    cairo_restore( aStream->cairoContext );
}

/*---------------------------------------------------------------------
 * rotate_cairo_surface()
 *
 * Rotates the cairo surface to the appropriate orientation.
 * ---------------------------------------------------------------------*/

void rotate_cairo_surface( PLStream *pls, float x11, float x12, float x21, float x22, float x0, float y0 )
{
    cairo_matrix_t *matrix;
    PLCairo        *aStream;

    aStream = (PLCairo *) pls->dev;

    matrix = (cairo_matrix_t *) malloc( sizeof ( cairo_matrix_t ));
    cairo_matrix_init( matrix, x11, x12, x21, x22, x0, y0 );
    cairo_transform( aStream->cairoContext, matrix );
    free( matrix );
}

/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is common to all familying Cairo Drivers
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/
#if defined ( PLD_pngcairo ) || defined ( PLD_svgcairo )

void plD_bop_famcairo( PLStream * );
/*----------------------------------------------------------------------
 * plD_bop_famcairo()
 *
 * Familying Devices: Set up for the next page.
 * ----------------------------------------------------------------------*/

void plD_bop_famcairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Plot familying stuff. Not really understood, just copying gd.c */
    plGetFam( pls );
    pls->famadv = 1;
    pls->page++;

    /* Fill in the window with the background color. */
    cairo_rectangle( aStream->cairoContext, 0.0, 0.0, pls->xlength, pls->ylength );
    cairo_set_source_rgba( aStream->cairoContext,
        (double) pls->cmap0[0].r / 255.0,
        (double) pls->cmap0[0].g / 255.0,
        (double) pls->cmap0[0].b / 255.0,
        (double) pls->cmap0[0].a );
    cairo_fill( aStream->cairoContext );
}

#endif
/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the xcairo driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_xcairo )

static int    XScreen;
static Window rootWindow;

void plD_dispatch_init_xcairo( PLDispatchTable *pdt );
void plD_init_xcairo( PLStream * );
void plD_bop_xcairo( PLStream * );
void plD_eop_xcairo( PLStream * );
void plD_tidy_xcairo( PLStream * );
void plD_esc_xcairo( PLStream *, PLINT, void * );
static void xcairo_get_cursor( PLStream *, PLGraphicsIn * );

/*---------------------------------------------------------------------
 * plD_dispatch_init_xcairo()
 *
 * xcairo dispatch table initialization.
 * ----------------------------------------------------------------------*/

void plD_dispatch_init_xcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo X Windows Driver";
    pdt->pl_DevName = "xcairo";
#endif
    pdt->pl_type     = plDevType_Interactive;
    pdt->pl_seq      = 59;
    pdt->pl_init     = (plD_init_fp) plD_init_xcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_xcairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_xcairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_xcairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_xcairo;
}

/*---------------------------------------------------------------------
 * xcairo_init_cairo()
 *
 * Configures Cairo to use whichever X Drawable is set up in the given
 * stream.  This is called by plD_init_xcairo() in the event we are
 * drawing into a plplot-managed window, and plD_esc_xcairo() if
 * we are using an external X Drawable.
 *
 * A return value of 0 indicates success.  Currently this function only
 * returns 0.
 * ----------------------------------------------------------------------*/

static signed int xcairo_init_cairo( PLStream *pls )
{
    PLCairo *aStream;
    Visual  *defaultVisual;

    aStream = (PLCairo *) pls->dev;

    /* Create an cairo surface & context that are associated with the X window. */
    defaultVisual = DefaultVisual( aStream->XDisplay, 0 );
    /* Dimension units are pixels from cairo documentation. */
    /* This is the X window Cairo surface. */
    aStream->cairoSurface_X = cairo_xlib_surface_create( aStream->XDisplay, aStream->XWindow, defaultVisual, pls->xlength, pls->ylength );
    aStream->cairoContext_X = cairo_create( aStream->cairoSurface_X );
    /* This is the Cairo surface PLplot will actually plot to. */
    aStream->cairoSurface = cairo_image_surface_create( CAIRO_FORMAT_RGB24, pls->xlength, pls->ylength );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Invert the surface so that the graphs are drawn right side up. */
    rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

    /* Set graphics aliasing */
    cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );

    return 0;
}

/*---------------------------------------------------------------------
 * plD_init_xcairo()
 *
 * Initialize Cairo X Windows device.
 * ----------------------------------------------------------------------*/

void plD_init_xcairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table. */
    aStream = stream_and_font_setup( pls, 1 );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;

    /* Create a X Window if required. */
    if ( external_drawable != 0 )
    {
        aStream->xdrawable_mode = 1;
    }
    else
    {
        /* X Windows setup */
        aStream->XDisplay = NULL;
        aStream->XDisplay = XOpenDisplay( NULL );
        if ( aStream->XDisplay == NULL )
        {
            printf( "Failed to open X Windows display\n" );
            /* some sort of error here */
        }
        XScreen    = DefaultScreen( aStream->XDisplay );
        rootWindow = RootWindow( aStream->XDisplay, XScreen );

        aStream->XWindow = XCreateSimpleWindow( aStream->XDisplay, rootWindow, 0, 0, pls->xlength, pls->ylength,
            1, BlackPixel( aStream->XDisplay, XScreen ), BlackPixel( aStream->XDisplay, XScreen ));
        XStoreName( aStream->XDisplay, aStream->XWindow, pls->plwindow);
        XSelectInput( aStream->XDisplay, aStream->XWindow, NoEventMask );
        XMapWindow( aStream->XDisplay, aStream->XWindow );
        aStream->xdrawable_mode = 0;

        xcairo_init_cairo( pls );
    }

    aStream->exit_event_loop = 0;
}

/*---------------------------------------------------------------------
 * blit_to_x()
 *
 * Blit the offscreen image to the X window.
 * ---------------------------------------------------------------------*/

void blit_to_x( PLCairo *aStream )
{
    cairo_set_source_surface( aStream->cairoContext_X, aStream->cairoSurface, 0.0, 0.0 );
    cairo_paint( aStream->cairoContext_X );
}

/*----------------------------------------------------------------------
 * plD_bop_xcairo()
 *
 * X Windows specific start of page.
 * ----------------------------------------------------------------------*/

void plD_bop_xcairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    plD_bop_cairo( pls );

    if ( aStream->xdrawable_mode )
        return;

    XFlush( aStream->XDisplay );
}

/*---------------------------------------------------------------------
 * plD_eop_xcairo()
 *
 * X Windows specific end of page.
 * ---------------------------------------------------------------------*/

void plD_eop_xcairo( PLStream *pls )
{
    int            number_chars;
    long           event_mask;
    char           event_string[10];
    KeySym         keysym;
    XComposeStatus cs;
    XEvent         event;
    PLCairo        *aStream;
    char           helpmsg[] = " - Press Enter or right-click to continue";
    char           *plotTitle;

    aStream = (PLCairo *) pls->dev;

    if ( aStream->xdrawable_mode )
        return;

    /* Blit the offscreen image to the X window. */
    blit_to_x( aStream );

    XFlush( aStream->XDisplay );

    /* Only pause if nopause is unset. */
    if ( pls->nopause )
        aStream->exit_event_loop = 1;

    /* Loop, handling selected events, till the user elects to close the plot. */
    event_mask = ButtonPressMask | KeyPressMask | ExposureMask;
    XSelectInput( aStream->XDisplay, aStream->XWindow, event_mask );
    while ( !aStream->exit_event_loop )
    {
        XWindowEvent( aStream->XDisplay, aStream->XWindow, event_mask, &event );
        switch ( event.type )
        {
        case KeyPress:
            number_chars = XLookupString((XKeyEvent *) &event, event_string, 10, &keysym, &cs );
            event_string[number_chars] = '\0';
            if ( keysym == XK_Return )
            {
                aStream->exit_event_loop = 1;
            }
            break;
        case ButtonPress:
            if (((XButtonEvent *) &event )->button == Button3 )
                aStream->exit_event_loop = 1;
            break;
        case Expose:
            /* Blit the image again after an expose event, but only for the last
             * available event.  Otherwise multiple redraws occur needlessly. */
            if (((XExposeEvent *) &event )->count == 0 )
            {
                blit_to_x( aStream );
                XFlush( aStream->XDisplay );
            }
            break;
        }
    }
    aStream->exit_event_loop = 0;
}

/*---------------------------------------------------------------------
 * plD_tidy_xcairo()
 *
 * X Windows: close graphics file or otherwise clean up.
 * ---------------------------------------------------------------------*/

void plD_tidy_xcairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    plD_tidy_cairo( pls );

    /* Also free up the Cairo X surface and context */
    cairo_destroy( aStream->cairoContext_X );
    cairo_surface_destroy( aStream->cairoSurface_X );

    if ( aStream->xdrawable_mode )
        return;

    /* Close the window and the display. */
    XFlush( aStream->XDisplay );

    XDestroyWindow( aStream->XDisplay, aStream->XWindow );

    XCloseDisplay( aStream->XDisplay );
}

/*---------------------------------------------------------------------
 * plD_esc_xcairo()
 *
 * Escape function, specialized for the xcairo driver
 * ---------------------------------------------------------------------*/

void plD_esc_xcairo( PLStream *pls, PLINT op, void *ptr )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    switch ( op )
    {
    case PLESC_FLUSH:    /* forced update of the window */
        blit_to_x( aStream );
        XFlush( aStream->XDisplay );
        break;
    case PLESC_GETC:     /* get cursor position */
        XFlush( aStream->XDisplay );
        xcairo_get_cursor( pls, (PLGraphicsIn*) ptr );
        break;
    case PLESC_DEVINIT: { /* Set external drawable */
        Window               rootwin;
        PLXcairoDrawableInfo *xinfo = (PLXcairoDrawableInfo *) ptr;
        signed int           x, y;
        unsigned int         w, h, b, d;
        if ( xinfo == NULL )
        {
            printf( "xcairo: PLESC_DEVINIT ignored, no drawable info provided\n" );
            return;
        }
        if ( aStream->xdrawable_mode == 0 )
        {
            printf( "xcairo: PLESC_DEVINIT called with drawable but stream not in xdrawable mode\n" );
            return;
        }
        aStream->XDisplay = xinfo->display;
        aStream->XWindow  = xinfo->drawable;

        /* Ensure plplot knows the real dimensions of the drawable */
        XGetGeometry( aStream->XDisplay, aStream->XWindow, &rootwin,
            &x, &y, &w, &h, &b, &d );
        pls->xlength = w;
        pls->ylength = h;
        plP_setphy((PLINT) 0, (PLINT) ( pls->xlength / aStream->downscale ), (PLINT) 0,
            (PLINT) ( pls->ylength / aStream->downscale ));

        /* Associate cairo with the supplied drawable */
        xcairo_init_cairo( pls );

        /* Recalculate dimensions and the like now that the drawable is known */
        plbop();

        break;
    }
    default:
        plD_esc_cairo( pls, op, ptr );
        break;
    }
}

/*---------------------------------------------------------------------
 * xcairo_get_cursor()
 *
 * X Windows: returns the location of the next mouse click or key press.
 * ---------------------------------------------------------------------*/

void xcairo_get_cursor( PLStream *pls, PLGraphicsIn *gin )
{
    int            number_chars;
    KeySym         keysym;
    XComposeStatus cs;
    XEvent         event;
    XButtonEvent   *xButtonEvent;
    Cursor         xHairCursor;
    PLCairo        *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Initialize PLplot mouse event structure */
    plGinInit( gin );

    /* Create cross hair cursor & switch to using it */
    xHairCursor = XCreateFontCursor( aStream->XDisplay, XC_crosshair );
    XDefineCursor( aStream->XDisplay, aStream->XWindow, xHairCursor );

    /* Get the next mouse button release or key press event */
    XSelectInput( aStream->XDisplay, aStream->XWindow, ButtonReleaseMask | KeyPressMask );
    XMaskEvent( aStream->XDisplay, ButtonReleaseMask | KeyPressMask, &event );
    XSelectInput( aStream->XDisplay, aStream->XWindow, NoEventMask );

    /* Get key pressed (if any) */
    if ( event.type == KeyPress )
    {
        number_chars = XLookupString((XKeyEvent *) &event, gin->string, 10, &keysym, &cs );
        gin->string[number_chars] = '\0';
        switch ( keysym )
        {
        case XK_BackSpace:
        case XK_Tab:
        case XK_Linefeed:
        case XK_Return:
        case XK_Escape:
        case XK_Delete:
            gin->keysym = 0xFF & keysym;
            break;
        default:
            gin->keysym = keysym;
        }
    }
    else
    {
        gin->string[0] = '\0';
        gin->keysym    = 0x20;
    }

    /* Update PLplot's mouse event structure */
    xButtonEvent = (XButtonEvent *) &event;
    gin->state   = xButtonEvent->state;
    gin->button  = xButtonEvent->button;
    gin->pX      = event.xbutton.x;
    gin->pY      = pls->ylength - event.xbutton.y;
    gin->dX      = (PLFLT) event.xbutton.x / ((PLFLT) ( pls->xlength ));
    gin->dY      = (PLFLT) ( pls->ylength - event.xbutton.y ) / ((PLFLT) ( pls->ylength ));

    /* Switch back to normal cursor */
    XUndefineCursor( aStream->XDisplay, aStream->XWindow );
    XFlush( aStream->XDisplay );
}

#endif


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo PDF driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_pdfcairo )

void plD_dispatch_init_pdfcairo( PLDispatchTable *pdt );
void plD_init_pdfcairo( PLStream * );

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* pdfcairo */
void plD_dispatch_init_pdfcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo PDF Driver";
    pdt->pl_DevName = "pdfcairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 60;
    pdt->pl_init     = (plD_init_fp) plD_init_pdfcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_cairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_cairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_cairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_cairo;
}

/*---------------------------------------------------------------------
 * plD_init_pdfcairo()
 *
 * Initialize Cairo PDF device
 * ----------------------------------------------------------------------*/

void plD_init_pdfcairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table */
    aStream = stream_and_font_setup( pls, 0 );

    /* Prompt for a file name if not already set. */
    plOpenFile( pls );

    /* Create an cairo surface & context for PDF file. */
    /* Dimension units are pts = 1/72 inches from cairo documentation. */
    aStream->cairoSurface = cairo_pdf_surface_create_for_stream((cairo_write_func_t) write_to_stream, pls->OutFile, (double) pls->xlength, (double) pls->ylength );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;

    /* Invert the surface so that the graphs are drawn right side up. */
    rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

    /* Set graphics aliasing */
    cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );
}

#endif


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo PS driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_pscairo )

void plD_dispatch_init_pscairo( PLDispatchTable *pdt );
void plD_init_pscairo( PLStream * );

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* pscairo */
void plD_dispatch_init_pscairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo PS Driver";
    pdt->pl_DevName = "pscairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 61;
    pdt->pl_init     = (plD_init_fp) plD_init_pscairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_cairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_cairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_cairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_cairo;
}

/*---------------------------------------------------------------------
 * plD_init_pscairo()
 *
 * Initialize Cairo PS device
 * ----------------------------------------------------------------------*/

void plD_init_pscairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table */
    aStream = stream_and_font_setup( pls, 0 );

    /* Prompt for a file name if not already set. */
    plOpenFile( pls );

    /* Create an cairo surface & context for PS file. */
    /* Dimension units are pts = 1/72 inches from cairo documentation. */
    aStream->cairoSurface = cairo_ps_surface_create_for_stream((cairo_write_func_t) write_to_stream, pls->OutFile, (double) pls->ylength, (double) pls->xlength );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;

    /* Handle portrait or landscape */
    if ( pls->portrait )
    {
        plsdiori( 1 );
        pls->freeaspect = 1;
    }
    rotate_cairo_surface( pls, 0.0, -1.0, -1.0, 0.0, pls->ylength, pls->xlength );
}


#endif


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo SVG driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_svgcairo )

void plD_dispatch_init_svgcairo( PLDispatchTable *pdt );
void plD_init_svgcairo( PLStream * );

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* svgcairo */
void plD_dispatch_init_svgcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo SVG Driver";
    pdt->pl_DevName = "svgcairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 62;
    pdt->pl_init     = (plD_init_fp) plD_init_svgcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_cairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_famcairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_cairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_cairo;
}

/*---------------------------------------------------------------------
 * plD_init_svgcairo()
 *
 * Initialize Cairo SVG device
 * ----------------------------------------------------------------------*/

void plD_init_svgcairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table and allocate a cairo
     * stream structure.
     *
     * NOTE: The check below is necessary since, in family mode, this function
     *  will be called multiple times. While you might think that it is
     *  sufficient to update what *should* be the only pointer to the contents
     *  of pls->dev, i.e. the pointer pls->dev itself, it appears that
     *  something else somewhere else is also pointing to pls->dev. If you
     *  change what pls->dev points to then you will get a "bus error", from
     *  which I infer the existence of said bad stale pointer.
     */
    if ( pls->dev == NULL )
    {
        aStream = stream_and_font_setup( pls, 0 );
    }
    else
    {
        stream_and_font_setup( pls, 0 );
        aStream = pls->dev;
    }

    /* Initialize family file info */
    plFamInit( pls );

    /* Prompt for a file name if not already set. */
    plOpenFile( pls );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;

    /* Create an cairo surface & context for SVG file. */
    /* Dimension units are pts = 1/72 inches from cairo documentation. */
    aStream->cairoSurface = cairo_svg_surface_create_for_stream((cairo_write_func_t) write_to_stream, pls->OutFile, (double) pls->xlength, (double) pls->ylength );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Invert the surface so that the graphs are drawn right side up. */
    rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

    /* Set graphics aliasing */
    cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );
}

#endif


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo PNG driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_pngcairo )

void plD_dispatch_init_pngcairo( PLDispatchTable *pdt );
void plD_init_pngcairo( PLStream * );
void plD_eop_pngcairo( PLStream * );

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* pngcairo */
void plD_dispatch_init_pngcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo PNG Driver";
    pdt->pl_DevName = "pngcairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 63;
    pdt->pl_init     = (plD_init_fp) plD_init_pngcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_pngcairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_famcairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_cairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_cairo;
}

/*---------------------------------------------------------------------
 * plD_init_pngcairo()
 *
 * Initialize Cairo PNG device
 * ----------------------------------------------------------------------*/

void plD_init_pngcairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table and allocate a cairo
     * stream structure.
     *
     * NOTE: The check below is necessary since, in family mode, this function
     *  will be called multiple times. While you might think that it is
     *  sufficient to update what *should* be the only pointer to the contents
     *  of pls->dev, i.e. the pointer pls->dev itself, it appears that
     *  something else somewhere else is also pointing to pls->dev. If you
     *  change what pls->dev points to then you will get a "bus error", from
     *  which I infer the existence of said bad stale pointer.
     */
    if ( pls->dev == NULL )
    {
        aStream = stream_and_font_setup( pls, 0 );
    }
    else
    {
        stream_and_font_setup( pls, 0 );
        aStream = pls->dev;
    }

    /* Initialize family file info */
    plFamInit( pls );

    /* Prompt for a file name if not already set. */
    plOpenFile( pls );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;

    /* Create a new cairo surface & context for PNG file. */
    /* Dimension units are pixels from cairo documentation. */
    aStream->cairoSurface = cairo_image_surface_create( CAIRO_FORMAT_RGB24, (double) pls->xlength, (double) pls->ylength );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Invert the surface so that the graphs are drawn right side up. */
    rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

    /* Set graphics aliasing */
    cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );
}

/*---------------------------------------------------------------------
 * plD_eop_pngcairo()
 *
 * PNG: End of page.
 * ---------------------------------------------------------------------*/

void plD_eop_pngcairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;
    cairo_surface_write_to_png_stream( aStream->cairoSurface, (cairo_write_func_t) write_to_stream, pls->OutFile );
}

#endif


/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo memory driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_memcairo )

void plD_dispatch_init_memcairo( PLDispatchTable *pdt );
void plD_init_memcairo( PLStream * );
void plD_eop_memcairo( PLStream * );
void plD_bop_memcairo( PLStream * );

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* memcairo */
void plD_dispatch_init_memcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo memory driver";
    pdt->pl_DevName = "memcairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 64;
    pdt->pl_init     = (plD_init_fp) plD_init_memcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_memcairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_memcairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_cairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_cairo;
}

/*----------------------------------------------------------------------
 * plD_bop_memcairo()
 *
 * Set up for the next page.
 * ----------------------------------------------------------------------*/

void plD_bop_memcairo( PLStream *pls )
{
    /* nothing to do here (we want to preserve the memory as it is) */
}

/*---------------------------------------------------------------------
 * plD_init_memcairo()
 *
 * Initialize Cairo memory device
 * ----------------------------------------------------------------------*/

void plD_init_memcairo( PLStream *pls )
{
    PLCairo       *aStream;
    int           stride, i;
    unsigned char *cairo_mem;
    unsigned char *input_mem;

    /* used for checking byte order */
    union
    {
        int  testWord;
        char testByte[sizeof ( int )];
    } endianTest;
    endianTest.testWord = 1;

    /* Set the plot size to the memory buffer size, on the off chance
     * that they are different. */
    pls->xlength = pls->phyxma;
    pls->ylength = pls->phyyma;

    /* Setup the PLStream and the font lookup table */
    aStream = stream_and_font_setup( pls, 0 );

    /* Test byte order */
    if ( endianTest.testByte[0] == 1 ) aStream->bigendian = 0;
    else aStream->bigendian = 1;

    /* Check that user supplied us with some memory to draw in */
    if ( pls->dev == NULL )
    {
        plexit( "Must call plsmem first to set user plotting area!" );
    }

    /* Save a pointer to the memory. */
    aStream->memory = pls->dev;

    /* Create a cairo surface & context.  Copy data in from the input memory area */

    /* Malloc memory the way cairo likes it.  Aligned on the stride computed by cairo_format_stride_for_width
     * and in the RGB24 format (from http://cairographics.org/manual/cairo-Image-Surfaces.html):
     * Each pixel is a 32-bit quantity, with the upper 8 bits unused.
     * Red, Green, and Blue are stored in the remaining 24 bits in that order */
    stride = pls->xlength * 4;
    /* stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, pls->xlength);  This function missing from version 1.4 :-( */
    aStream->cairo_format_memory = (unsigned char *) calloc( stride * pls->ylength, 1 );

    /* Copy the input data into the Cairo data format */
    cairo_mem = aStream->cairo_format_memory;
    input_mem = aStream->memory;

    /* 32 bit word order
     * cairo mem:  Big endian:  0=A, 1=R, 2=G, 3=B
     *             Little endian:  3=A, 2=R, 1=G, 0=B */
    if ( aStream->bigendian )
    {
        for ( i = 0; i < ( pls->xlength * pls->ylength ); i++ )
        {
            cairo_mem[1] = input_mem[0]; /* R */
            cairo_mem[2] = input_mem[1]; /* G */
            cairo_mem[3] = input_mem[2]; /* B */
            input_mem   += 3;
            cairo_mem   += 4;
        }
    }
    else
    {
        for ( i = 0; i < ( pls->xlength * pls->ylength ); i++ )
        {
            cairo_mem[2] = input_mem[0]; /* R */
            cairo_mem[1] = input_mem[1]; /* G */
            cairo_mem[0] = input_mem[2]; /* B */
            input_mem   += 3;
            cairo_mem   += 4;
        }
    }

    /* Create a Cairo drawing surface from the input data */
    aStream->cairoSurface =
        /* Dimension units are width, height of buffer image from cairo
         * documentation. */
        cairo_image_surface_create_for_data( aStream->cairo_format_memory, CAIRO_FORMAT_RGB24, pls->xlength, pls->ylength, stride );
    aStream->cairoContext = cairo_create( aStream->cairoSurface );

    /* Save the pointer to the structure in the PLplot stream.
     * Note that this wipes out the direct pointer to the memory buffer. */
    pls->dev = aStream;

    /* Invert the surface so that the graphs are drawn right side up. */
    rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

    /* Set graphics aliasing */
    cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );
}

/*---------------------------------------------------------------------
 * plD_eop_memcairo()
 *
 * Memory device specific end of page. This copies the contents
 * of the cairo surface into the user supplied memory buffer.
 * ---------------------------------------------------------------------*/

void plD_eop_memcairo( PLStream *pls )
{
    int           i;
    unsigned char *memory;
    unsigned char *cairo_surface_data;
    PLCairo       *aStream;

    aStream            = (PLCairo *) pls->dev;
    memory             = aStream->memory;
    cairo_surface_data = cairo_image_surface_get_data( aStream->cairoSurface );
    /* 32 bit word order
     * cairo mem:  Big endian:  0=A, 1=R, 2=G, 3=B
     *             Little endian:  3=A, 2=R, 1=G, 0=B */
    if ( aStream->bigendian )
    {
        for ( i = 0; i < ( pls->xlength * pls->ylength ); i++ )
        {
            memory[0]           = cairo_surface_data[1]; /* R */
            memory[1]           = cairo_surface_data[2]; /* G */
            memory[2]           = cairo_surface_data[3]; /* B */
            memory             += 3;
            cairo_surface_data += 4;
        }
    }
    else
    {
        for ( i = 0; i < ( pls->xlength * pls->ylength ); i++ )
        {
            memory[0]           = cairo_surface_data[2]; /* R */
            memory[1]           = cairo_surface_data[1]; /* G */
            memory[2]           = cairo_surface_data[0]; /* B */
            memory             += 3;
            cairo_surface_data += 4;
        }
    }

    /* Free up the temporary memory malloc'ed in plD_init_memcairo */
    free( aStream->cairo_format_memory );
}

#endif

/*---------------------------------------------------------------------
 * ---------------------------------------------------------------------
 *
 * That which is specific to the cairo external context driver.
 *
 * ---------------------------------------------------------------------
 * ---------------------------------------------------------------------*/

#if defined ( PLD_extcairo )

void extcairo_setbackground( PLStream * );
void plD_dispatch_init_extcairo( PLDispatchTable *pdt );
void plD_init_extcairo( PLStream * );
void plD_bop_extcairo( PLStream * );
void plD_eop_extcairo( PLStream * );
void plD_esc_extcairo( PLStream *, PLINT, void * );
void plD_tidy_extcairo( PLStream * );

/*---------------------------------------------------------------------
 * extcairo_setbackground()
 *
 * Set the background color for the extcairo device
 * ----------------------------------------------------------------------*/

void extcairo_setbackground( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Fill the context with the background color if the user so desires. */
    if ( aStream->cairoContext != NULL )
    {
        cairo_rectangle( aStream->cairoContext, 0.0, 0.0, pls->xlength, pls->ylength );
        cairo_set_source_rgba( aStream->cairoContext,
            (double) pls->cmap0[0].r / 255.0,
            (double) pls->cmap0[0].g / 255.0,
            (double) pls->cmap0[0].b / 255.0,
            (double) pls->cmap0[0].a );
        cairo_fill( aStream->cairoContext );
    }
}

/*---------------------------------------------------------------------
 * dispatch_init_init()
 *
 * Initialize device dispatch table
 * ----------------------------------------------------------------------*/

/* extcairo */
void plD_dispatch_init_extcairo( PLDispatchTable *pdt )
{
#ifndef ENABLE_DYNDRIVERS
    pdt->pl_MenuStr = "Cairo external context driver";
    pdt->pl_DevName = "extcairo";
#endif
    pdt->pl_type     = plDevType_FileOriented;
    pdt->pl_seq      = 65;
    pdt->pl_init     = (plD_init_fp) plD_init_extcairo;
    pdt->pl_line     = (plD_line_fp) plD_line_cairo;
    pdt->pl_polyline = (plD_polyline_fp) plD_polyline_cairo;
    pdt->pl_bop      = (plD_bop_fp) plD_bop_extcairo;
    pdt->pl_eop      = (plD_eop_fp) plD_eop_extcairo;
    pdt->pl_tidy     = (plD_tidy_fp) plD_tidy_extcairo;
    pdt->pl_state    = (plD_state_fp) plD_state_cairo;
    pdt->pl_esc      = (plD_esc_fp) plD_esc_extcairo;
}

/*---------------------------------------------------------------------
 * plD_init_extcairo()
 *
 * Initialize Cairo external context driver.
 * ----------------------------------------------------------------------*/

void plD_init_extcairo( PLStream *pls )
{
    PLCairo *aStream;

    /* Setup the PLStream and the font lookup table */
    aStream = stream_and_font_setup( pls, 0 );

    /* Save the pointer to the structure in the PLplot stream */
    pls->dev = aStream;
}

/*----------------------------------------------------------------------
 * plD_bop_extcairo()
 *
 * Set up for the next page.
 * ----------------------------------------------------------------------*/

void plD_bop_extcairo( PLStream *pls )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    /* Set background if desired */
    if ( aStream->set_background )
    {
        extcairo_setbackground( pls );
    }
}

/*----------------------------------------------------------------------
 * plD_eop_extcairo()
 *
 * End of page.
 * ----------------------------------------------------------------------*/

void plD_eop_extcairo( PLStream *pls )
{
    /* nothing to do here, we leave it to the calling program to display
     * (or not) the update cairo context. */
}

/*---------------------------------------------------------------------
 * plD_esc_extcairo()
 *
 * The generic escape function, extended so that user can pass in
 * an external Cairo context to use for rendering.
 * ---------------------------------------------------------------------*/

void plD_esc_extcairo( PLStream *pls, PLINT op, void *ptr )
{
    PLCairo *aStream;

    aStream = (PLCairo *) pls->dev;

    switch ( op )
    {
    case PLESC_DEVINIT: /* Set external context */
        aStream->cairoContext = (cairo_t *) ptr;
        /* Set graphics aliasing */
        cairo_set_antialias( aStream->cairoContext, aStream->graphics_anti_aliasing );

        /* Invert the surface so that the graphs are drawn right side up. */
        rotate_cairo_surface( pls, 1.0, 0.0, 0.0, -1.0, 0.0, pls->ylength );

        /* Should adjust plot size to fit in the given cairo context?
         * Cairo does not provide a way to query the dimensions of a context? */

        /* Set background if desired */
        if ( aStream->set_background )
        {
            extcairo_setbackground( pls );
        }
        break;
    default: /* Fall back on default Cairo actions */
        plD_esc_cairo( pls, op, ptr );
        break;
    }
}

/*---------------------------------------------------------------------
 * plD_tidy_extcairo()
 *
 * This is nop, it is up to the calling program to clean up the Cairo
 * context, etc...
 * ---------------------------------------------------------------------*/

void plD_tidy_extcairo( PLStream *pls )
{
}

#endif
