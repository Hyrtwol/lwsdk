/************************************************************************
*                                                                       *
*       SampleRenderer.c:                                               *
*                                                                       *
*       Contains The Code For The SampleRenderer Plugin.                *
*                                                                       *
*       08.Sept 2008 Jamie Lisa Finch.                                  *
*                                                                       *
************************************************************************/

#include <lwserver.h>
#include <lwextrend.h>
#include <lwpanel.h>
#include <lwimage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char  PluginName[]        = { "SampleRenderer" };
      char  PluginUser[]        = { "Dark Renderer" };
const char  NotEnoughMemory[]   = { "Not enough memory." };
const char *PickRenderColor[]   = { "Random", "Colorbars", "Fade", 0 };
      char  CycleType[]         = { "Render Type" };

typedef struct st_render {
    GlobalFunc  *global;
    LWImageUtil *imageUtil;
    LWSceneInfo *sceneInfo;
    int          renderColor;
    } RENDER;

GlobalFunc *g_Global;


/************************************************************************
*                                                                       *
*       Instance Functions:                                             *
*                                                                       *
************************************************************************/

XCALL_ (static void) SampleDestroy( void *vRender ) {

    RENDER *render = (RENDER *) vRender;
    if( render ) {
        free( render );
        render = (RENDER *) NULL;
        }
    }


XCALL_ (static LWInstance) SampleCreate( void *Priv, LWItemID Id, LWError *Error ) {

    RENDER *render = (RENDER *) malloc( sizeof( RENDER ) );
    if( render ) {
        memset( render, 0, sizeof( RENDER ) );
        render->global      = g_Global;
        render->imageUtil   = (LWImageUtil *) render->global( LWIMAGEUTIL_GLOBAL, GFUSE_TRANSIENT );
        render->sceneInfo   = (LWSceneInfo *) render->global( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );
        render->renderColor = 0;
        }
    else
        *Error = NotEnoughMemory;
    return (LWInstance) render;
    }


XCALL_ (static LWError) SampleLoad( void *vRender, const LWLoadState *lState ) {

    RENDER *render = (RENDER *) vRender;

    LWLOAD_I4( lState, &render->renderColor, 1 );
    return NULL;
    }


XCALL_ (static LWError) SampleSave( void *vRender, const LWSaveState *sState ) {

    RENDER *render = (RENDER *) vRender;

    LWSAVE_I4( sState, &render->renderColor, 1 );
    return NULL;
    }


XCALL_ (static LWError) SampleCopy( void *vToRender, void *vFromRender ) {

    RENDER *toRender   = (RENDER *) vToRender;
    RENDER *fromRender = (RENDER *) vFromRender;

    toRender->global      = fromRender->global;
    toRender->imageUtil   = fromRender->imageUtil;
    toRender->sceneInfo   = fromRender->sceneInfo;
    toRender->renderColor = fromRender->renderColor;

    return NULL;
    }


XCALL_ (static const char *) SampleDescribe( void *vRender ) {

    RENDER *render = (RENDER *) vRender;

    return PickRenderColor[ render->renderColor ];
    }


/************************************************************************
*                                                                       *
*       SampleRender:                                                   *
*                                                                       *
*       This get called when the user hits F9, F10 or from screamer net.*
*                                                                       *
*       Render      = Address of instance data.                         *
*       first_frame = First frame to render in sequence.                *
*       last_frame  = Last  frame to render in sequence.                *
*       frame_step  = Number of frames to skip to next frame.           *
*       send_image  = Address of callback to send finished image        *
*                     back to LightWave.                                *
*                                                                       *
*       Returns:                                                        *
*           TRUE  == Execution without error.                           *
*           FALSE == Failed.                                            *
*                                                                       *
************************************************************************/

static unsigned char darkblack[ 4 ] = { 0x07, 0x07, 0x07, 0xFF };
static unsigned char black[     4 ] = { 0x10, 0x10, 0x10, 0xFF };
static unsigned char lightblack[4 ] = { 0x18, 0x18, 0x18, 0xFF };
static unsigned char darkgrey[  4 ] = { 0x00, 0x1D, 0x42, 0xFF };
static unsigned char grey[      4 ] = { 0x84, 0x84, 0x84, 0xFF };
static unsigned char red[       4 ] = { 0x84, 0x10, 0x10, 0xFF };
static unsigned char green[     4 ] = { 0x10, 0x84, 0x10, 0xFF };
static unsigned char blue[      4 ] = { 0x10, 0x10, 0x84, 0xFF };
static unsigned char yellow[    4 ] = { 0x84, 0x84, 0x10, 0xFF };
static unsigned char cyan[      4 ] = { 0x10, 0x84, 0x84, 0xFF };
static unsigned char magenta[   4 ] = { 0x84, 0x10, 0x84, 0xFF };
static unsigned char white[     4 ] = { 0xEB, 0xEB, 0xEB, 0xFF };
static unsigned char purple[    4 ] = { 0x2C, 0x00, 0x5C, 0xFF };

static unsigned char *toprow[]    = { grey, yellow, cyan,    green, magenta, red,   blue  };
static unsigned char *middlerow[] = { blue, black,  magenta, black, cyan,    black, grey  };
static unsigned char *bottomrow[] = {
    darkgrey,  darkgrey, darkgrey,
    white,     white,    white,
    purple,    purple,   purple,
    black,     black,    black,
    darkblack, black,    lightblack,
    black,     black,    black };

static unsigned char **rowtable[] = { toprow, toprow, toprow, toprow, toprow, toprow, toprow, toprow, toprow, middlerow, bottomrow, bottomrow, bottomrow };
static int           counttable[] = {      7,      7,      7,      7,      7,      7,       7,     7,      7,         7,        18,        18,        18 };

static unsigned char *randomtable[] = { black, red, green, blue, cyan, magenta, yellow, white };


int SampleRender( void *vRender, int first_frame, int last_frame, int frame_step, EXTRENDERERIMAGE *send_image, int render_mode ) {

    RENDER *render = (RENDER *) vRender;
    LWPixmapID image;
    int x, y, frame;
    unsigned int random;
    unsigned char *randomcolor;
    unsigned char *barcolor;
    unsigned char  color[ 4 ];
    double   colormix;

    if( ( render_mode == lwrm_Frame          ) ||
        ( render_mode == lwrm_Scene          ) ||
        ( render_mode == lwrm_SelectedObject ) ) {

        for( frame = first_frame; frame <= last_frame; frame += frame_step ) {
            image =  render->imageUtil->create( render->sceneInfo->frameWidth, render->sceneInfo->frameHeight, LWIMTYP_RGBAFP );
            if( !image )
                return 0;

            colormix = (double) ( frame - first_frame ) / (double) ( last_frame - first_frame + 1 );
            switch( render->renderColor ) {

                /* Random. */

                case 0:
                    random = 7919 * frame;
                    for( y = 0; y < render->sceneInfo->frameHeight; y++ ) {
                        for( x = 0; x < render->sceneInfo->frameWidth; x++ ) {
                            random = ( random * 4903 ) + 3361;
                            render->imageUtil->setPixelTyped( image, x, y, LWIMTYP_RGBA32, randomtable[ random >> 29 ] );
                            }
                        }
                break;

                /* Color Bars. */

                case 1:
                    for( y = 0; y < render->sceneInfo->frameHeight; y++ ) {
                        int yindex = ( y * ( sizeof( rowtable ) / sizeof( rowtable[ 0 ] ) ) ) / render->sceneInfo->frameHeight;
                        unsigned char **ytable = rowtable[ yindex ];
                        for( x = 0; x < render->sceneInfo->frameWidth; x++ ) {
                            int xindex = x * counttable[ yindex ] / render->sceneInfo->frameWidth;
                            render->imageUtil->setPixelTyped( image, x, y, LWIMTYP_RGBA32, ytable[ xindex ] );
                            }
                        }
                    break;

                /* Color Bars and Random. */

                case 2:
                    random = 7919 * frame;
                    for( y = 0; y < render->sceneInfo->frameHeight; y++ ) {
                        int yindex = ( y * ( sizeof( rowtable ) / sizeof( rowtable[ 0 ] ) ) ) / render->sceneInfo->frameHeight;
                        unsigned char **ytable = rowtable[ yindex ];
                        for( x = 0; x < render->sceneInfo->frameWidth; x++ ) {
                            int xindex = x * counttable[ yindex ] / render->sceneInfo->frameWidth;
                            random      = ( random * 4903 ) + 3361;
                            randomcolor = randomtable[ random >> 29 ];
                            barcolor    = ytable[ xindex ];
                            color[ 0 ]  = (unsigned char) ( barcolor[ 0 ] * ( 1 - colormix ) + randomcolor[ 0 ] * colormix );
                            color[ 1 ]  = (unsigned char) ( barcolor[ 1 ] * ( 1 - colormix ) + randomcolor[ 1 ] * colormix );
                            color[ 2 ]  = (unsigned char) ( barcolor[ 2 ] * ( 1 - colormix ) + randomcolor[ 2 ] * colormix );
                            color[ 3 ]  = 0xFF;
                            render->imageUtil->setPixelTyped( image, x, y, LWIMTYP_RGBA32, color );
                            }
                        }
                    break;
                }

            /* Send to LightWave the finished image. */

            if( !send_image( frame, 0, image ) )
                return 0;
            }
        }
    return 1;
    }


/************************************************************************
*                                                                       *
*       SampleOptions:                                                  *
*                                                                       *
************************************************************************/

static LWValue ival = { LWT_INTEGER };

int SampleOptions( void *vRender ) {

    RENDER *render = (RENDER *) vRender;
    LWPanelFuncs    *Panel;
    LWPanelID        PanelID;
    int              Return;
    int              x, y;
    LWPanControlDesc desc;      /* Required By Macros In lwpanel.h */
    LWControl       *renderColorControl;

    Return = 0;
    Panel = (LWPanelFuncs *) g_Global( PANEL_SERVICES_NAME, GFUSE_TRANSIENT );
    if( Panel != (LWPanelFuncs *) NULL ) {
        PanelID = Panel->create( PluginUser, Panel );
        if( PanelID != (LWPanelID) NULL ) {

            renderColorControl = WPOPUP_CTL( Panel, PanelID, CycleType, PickRenderColor, 106 );
            y = CON_Y( renderColorControl );
            x = CON_X( renderColorControl );
            MOVE_CON(  renderColorControl, x + 30, y );
            SET_INT(   renderColorControl, render->renderColor );

            if( Panel->open( PanelID, PANF_FRAME | PANF_BLOCKING | PANF_CANCEL ) == 1 ) {
                GET_INT( renderColorControl, render->renderColor );
                Return = 1;
                }
            Panel->destroy( PanelID );
            PanelID = (LWPanelID)   NULL;
            }
        }
    return Return;
    }


/************************************************************************
*                                                                       *
*       SampleRendererActivate:                                         *
*                                                                       *
************************************************************************/

XCALL_ (static int) SampleRendererActivate( int Version, GlobalFunc *Global, void *vLocal, void *ServerData ) {

    LWExtRendererHandler *Local;

    XCALL_INIT;

    Local = (LWExtRendererHandler *) vLocal;

    if( ( Version != LWEXTRENDERER_VERSION ) )
        return AFUNC_BADVERSION;

    g_Global = Global;

    Local->inst->create   = SampleCreate;
    Local->inst->destroy  = SampleDestroy;
    Local->inst->load     = SampleLoad;
    Local->inst->save     = SampleSave;
    Local->inst->copy     = SampleCopy;
    Local->inst->descln   = SampleDescribe;

    Local->options        = SampleOptions;
    Local->render         = SampleRender;

    return AFUNC_OK;
    }


/************************************************************************
*                                                                       *
*       ServerDesc:                                                     *
*                                                                       *
*       The Name "PluginName" Is The Unique Name That Identifies        *
*       The Plugin.                                                     *
*                                                                       *
*       08.Sept 2008 Jamie Lisa Finch.                                  *
*                                                                       *
************************************************************************/

static ServerTagInfo SampleRenderer_Tags[] = {
    { PluginUser, SRVTAG_USERNAME | LANGID_USENGLISH },
    { NULL } };

#ifdef __cplusplus
extern "C" {
#endif

ServerRecord ServerDesc[] = {
    { LWEXTRENDERER_HCLASS, PluginName, SampleRendererActivate, SampleRenderer_Tags },
    { NULL }
    };

#ifdef __cplusplus
}
#endif
