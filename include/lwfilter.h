/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWFILTER.H -- LightWave Image and Pixel Filters
 *
 * This header contains the basic declarations need to define the
 * simplest LightWave plug-in server.
 */
#ifndef LWSDK_FILTER_H
#define LWSDK_FILTER_H


#include <lwmonitor.h>
#include <lwrender.h>


#define LWIMAGEFILTER_HCLASS    "ImageFilterHandler"
#define LWIMAGEFILTER_ICLASS    "ImageFilterInterface"
#define LWIMAGEFILTER_GCLASS    "ImageFilterGizmo"
#define LWIMAGEFILTER_VERSION   4


#define LWPIXELFILTER_HCLASS    "PixelFilterHandler"
#define LWPIXELFILTER_ICLASS    "PixelFilterInterface"
#define LWPIXELFILTER_GCLASS    "PixelFilterGizmo"
#define LWPIXELFILTER_VERSION   6


//
// Buffer types, used with getLine(), getVal(), setVal()
//
enum
{
    LWBUF_SPECIAL,
    LWBUF_LUMINOUS,
    LWBUF_DIFFUSE,
    LWBUF_SPECULAR,
    LWBUF_MIRROR,
    LWBUF_TRANS,
    LWBUF_RAW_RED,
    LWBUF_RAW_GREEN,
    LWBUF_RAW_BLUE,
    LWBUF_SHADING,
    LWBUF_SHADOW,
    LWBUF_GEOMETRY,
    LWBUF_DEPTH,
    LWBUF_DIFFSHADE,
    LWBUF_SPECSHADE,
    LWBUF_MOTION_X,
    LWBUF_MOTION_Y,
    LWBUF_REFL_RED,
    LWBUF_REFL_GREEN,
    LWBUF_REFL_BLUE,
    LWBUF_DIFF_RED,
    LWBUF_DIFF_GREEN,
    LWBUF_DIFF_BLUE,
    LWBUF_SPEC_RED,
    LWBUF_SPEC_GREEN,
    LWBUF_SPEC_BLUE,
    LWBUF_BACKDROP_RED,
    LWBUF_BACKDROP_GREEN,
    LWBUF_BACKDROP_BLUE,
    LWBUF_PREEFFECT_RED,
    LWBUF_PREEFFECT_GREEN,
    LWBUF_PREEFFECT_BLUE,
    LWBUF_RED,
    LWBUF_GREEN,
    LWBUF_BLUE,
    LWBUF_ALPHA,
    LWBUF_REFR_RED,
    LWBUF_REFR_GREEN,
    LWBUF_REFR_BLUE,
    LWBUF_REFR_ALPHA,

    LWBUF_MAX_CHANNELS
};

//
// Buffer flags, used with the flags() callback
//
#define LWBUFF_SPECIAL    (1 << LWBUF_SPECIAL)
#define LWBUFF_LUMINOUS   (1 << LWBUF_LUMINOUS)
#define LWBUFF_DIFFUSE    (1 << LWBUF_DIFFUSE)
#define LWBUFF_SPECULAR   (1 << LWBUF_SPECULAR)
#define LWBUFF_MIRROR     (1 << LWBUF_MIRROR)
#define LWBUFF_TRANS      (1 << LWBUF_TRANS)
#define LWBUFF_RAW_RED    (1 << LWBUF_RAW_RED)
#define LWBUFF_RAW_GREEN  (1 << LWBUF_RAW_GREEN)
#define LWBUFF_RAW_BLUE   (1 << LWBUF_RAW_BLUE)
#define LWBUFF_SHADING    (1 << LWBUF_SHADING)
#define LWBUFF_SHADOW     (1 << LWBUF_SHADOW)
#define LWBUFF_GEOMETRY   (1 << LWBUF_GEOMETRY)
#define LWBUFF_DEPTH      (1 << LWBUF_DEPTH)
#define LWBUFF_DIFFSHADE  (1 << LWBUF_DIFFSHADE)
#define LWBUFF_SPECSHADE  (1 << LWBUF_SPECSHADE)
#define LWBUFF_MOTION_X   (1 << LWBUF_MOTION_X)
#define LWBUFF_MOTION_Y   (1 << LWBUF_MOTION_Y)
#define LWBUFF_REFL_RED   (1 << LWBUF_REFL_RED)
#define LWBUFF_REFL_GREEN (1 << LWBUF_REFL_GREEN)
#define LWBUFF_REFL_BLUE  (1 << LWBUF_REFL_BLUE)
#define LWBUFF_DIFF_RED   (1 << LWBUF_DIFF_RED)
#define LWBUFF_DIFF_GREEN (1 << LWBUF_DIFF_GREEN)
#define LWBUFF_DIFF_BLUE  (1 << LWBUF_DIFF_BLUE)
#define LWBUFF_SPEC_RED   (1 << LWBUF_SPEC_RED)
#define LWBUFF_SPEC_GREEN (1 << LWBUF_SPEC_GREEN)
#define LWBUFF_SPEC_BLUE  (1 << LWBUF_SPEC_BLUE)
#define LWBUFF_BACKDROP_RED    (1 << LWBUF_BACKDROP_RED)    // Getting the red channel gets green and blue as well
#define LWBUFF_BACKDROP_GREEN  (1 << LWBUF_BACKDROP_RED)
#define LWBUFF_BACKDROP_BLUE   (1 << LWBUF_BACKDROP_RED)
#define LWBUFF_PREEFFECT_RED   (1 << LWBUF_PREEFFECT_RED)   // Note that PREEFFECT is not available in pixel filters
#define LWBUFF_PREEFFECT_GREEN (1 << LWBUF_PREEFFECT_GREEN)
#define LWBUFF_PREEFFECT_BLUE  (1 << LWBUF_PREEFFECT_BLUE)
#define LWBUFF_RED             (0)                          // Basic RGBA is always available
#define LWBUFF_GREEN           (0)
#define LWBUFF_BLUE            (0)
#define LWBUFF_ALPHA           (0)
#define LWBUFF_REFR_RED        (LWBUFF_REFL_RED | LWBUFF_DIFF_RED | LWBUFF_SPEC_RED)
#define LWBUFF_REFR_GREEN      (LWBUFF_REFL_GREEN | LWBUFF_DIFF_GREEN | LWBUFF_SPEC_GREEN)
#define LWBUFF_REFR_BLUE       (LWBUFF_REFL_BLUE | LWBUFF_DIFF_BLUE | LWBUFF_SPEC_BLUE)
#define LWBUFF_REFR_ALPHA      (LWBUFF_TRANS)

/* Note: These channels make use of the same bits as LWBUF_BACKDROP_BLUE...LWBUF_PREEFFECT_BLUE in filter plugins. */
/* To set LWPFF_MULTITHREADED, make sure the LWBUF_BACKDROP_GREEN is not set. LWBUF_BACKDROP_RED can be used to
/* request all three backdrop color chanels. The PREEFFECT channels cannot be accessed by filter plugins. */
#define LWPFF_MULTITHREADED     (1<<28)
#define LWPFF_EVERYPIXEL        (1<<29)
#define LWPFF_BEFOREVOLUME      (1<<30)
#define LWPFF_RAYTRACE          (1<<31)

typedef struct st_LWFilterAccess {
        int               width, height;
        LWFrame           frame;
        LWTime            start, end;
        float *         (*getLine)  (int type, int y);
        void            (*setRGB)   (int x, int y, const LWFVector rgb);
        void            (*setAlpha) (int x, int y, float alpha);
        LWMonitor        *monitor;
} LWFilterAccess;


typedef struct st_LWImageFilterHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        void            (*process) (LWInstance, const LWFilterAccess *);
        unsigned int    (*flags) (LWInstance);
} LWImageFilterHandler;



typedef struct st_LWPixelAccess {
        double            sx, sy;
        void            (*getVal)  (int type, int num, float *);
        void            (*setRGBA) (const float[4]);
        void            (*setVal)  (int type, int num, float *);
        LWIlluminateFunc *illuminate;
        LWRayTraceFunc   *rayTrace;
        LWRayCastFunc    *rayCast;
        LWRayShadeFunc   *rayShade;
        LWRayTraceModeFunc     *rayTraceMode;
        LWIlluminateSampleFunc *illuminateSample;
        RandomFloatData        randomData;
        LWRandomFloatFunc      *randomFloat;
        LWIlluminateNormalFunc *illuminateNormal;
        LWIlluminateSampleNormalFunc *illuminateSampleNormal;
        LWRayTraceDataFunc      *rayTraceData;
        LWRayTraceShadeFunc     *rayTraceShade;
        LWRenderData            render;
} LWPixelAccess;


typedef struct st_LWPixelFilterHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        void            (*evaluate) (LWInstance, const LWPixelAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWPixelFilterHandler;


typedef unsigned int LWFilterContext;


#define LWFCF_PREPROCESS  (1<<0) /* Filter applied in image editor or as pre process */


#endif
