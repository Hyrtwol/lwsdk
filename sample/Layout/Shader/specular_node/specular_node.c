/***************************************************************
 *
 *   specular_node.c
 *
 *   A shading model node example.
 *
 *   Antti Järvelä   16 May 2006
 *
 ***************************************************************/

#include <lwserver.h>
#include <lwnodes.h>
#include <lwsurf.h>
#include <lwhost.h>
#include <lwxpanel.h>
#include <lwmath.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/***************************************************************
 *
 *  Function globals.
 *
 ***************************************************************/
static  LWNodeInputFuncs    *inputfuncs;
static  LWNodeOutputFuncs   *outputfuncs;
static  LWNodeFuncs         *nodefuncs;
static  LWXPanelFuncs       *xpanf;
static  LWInstUpdate        *lwupdate;
static  LWItemInfo          *iteminfo;
static  LWLightInfo         *lightinfo;
static  LWObjectInfo        *objinfo;

/***************************************************************
 *
 *  XPanel control id's.
 *
 ***************************************************************/
enum { Spec_color = 0x8001, Spec_specular, Spec_gloss };

/***************************************************************
 *
 *  Block id's for saving and loading parameter values.
 *
 ***************************************************************/
#define	ID_SPEC_COLOR			LWID_( 'S','C','O','L' )
#define	ID_SPEC_SPECULAR		LWID_( 'S','S','P','C' )
#define	ID_SPEC_GLOSS			LWID_( 'S','G','L','S' )
static LWBlockIdent Spec_Block[] = {
	ID_SPEC_COLOR,				"Color",
	ID_SPEC_SPECULAR,			"Specularity",
	ID_SPEC_GLOSS,				"Glossiness",
	0
};

/***************************************************************
 *
 *  Instance structure definition.
 *
 ***************************************************************/
typedef struct Specular_t {
	
    LWXPanelID		panel;              // The id of the xpanel user interface
	NodeID			node;               // The id of the node this instance is applied to

    LWDVector       v_color;
    double          v_spec, v_gloss;    // Panel values for the parameters of the shader

    NodeInputID     color, spec, gloss; // Input id's for the incoming connections
    NodeOutputID    output;             // Output id for the output color

} Specular, *Specular_p;

/***************************************************************
 *
 *  Normalize function
 *
 *  Normalizes the vector, ie. makes it's length equal to 1.
 *  Returns the length of the vector before normalization.
 *
 ***************************************************************/
static double Normalize( LWDVector vec )
{
    const double d = VLEN( vec );   // Length of the vector
    if( d > 0.0 )                   // The function will not normalize a degenerate vector
    {
        const double x = 1.0 / d;
        VSCL( vec, x );             // Scale the vector by the inverse of it's length
    }
    return d;
}

/***************************************************************
 *
 *  Input event function
 *
 *  This function is called whenever the user connects to
 *  this input, disconnects a connection from the input,
 *  or destroys a node connected to the input this callback
 *  was assigned to.
 *
 *  The connection type can be used when you want to know which
 *  type the connection to this input is.
 *  Useful when you do not want to allow the connection to occur
 *  if the types do not match. In this case you can call
 *  inputfuncs->disconnect, and the connection will not occur.
 *
 ***************************************************************/
static int InputEvent( Specular_p inst, NodeInputID input, LWNodalEvent event, ConnectionType type )
{
    // Update the user interface
	if( inst->panel )
		xpanf->viewRefresh( inst->panel );

    // Update the instance data when the node connected to the input is destroyed
	if( lwupdate && event == NIE_INPUTNODEDESTROY ) 
		lwupdate( LWNODE_HCLASS, inst );

	return 1;
}

/***************************************************************
 *
 *  Create
 *
 *  Allocate instance data, create inputs and outputs for
 *  the node, and setup default parameters.
 *
 ***************************************************************/
XCALL_( static LWInstance )
Create( void *priv, NodeID node, LWError *err )
{
    Specular_p inst;

    // Allocate memory for the instance.
    inst = calloc( 1, sizeof( Specular ) );
    if( !inst )
    {
        // Return an error, if the allocation failed.
        *err = "Couldn't allocate memory for instance.";
        return NULL;
    }

    // Create inputs, and assign InputEvent as the input event callback.
    inst->color = inputfuncs->create( node, NOT_COLOR, "Color", InputEvent );
    VSET( inst->v_color, 1.0 );

    inst->spec = inputfuncs->create( node, NOT_SCALAR, "Specularity", InputEvent );
    inst->v_spec = 1.0;

    inst->gloss = inputfuncs->create( node, NOT_SCALAR, "Glossiness", InputEvent );
    inst->v_gloss = 0.4;

    // Create a single output for the node.
    inst->output = outputfuncs->create( node, NOT_COLOR, "Color" );

    // Store the node id.
    inst->node = node;

    return inst;
}

/***************************************************************
 *
 *  Destroy
 *
 *  Destroy our instance data.
 *  There is no need to destroy all the inputs and outputs,
 *  because that will be taken care of automatically by the
 *  Node Editor.
 *
 ***************************************************************/
XCALL_( static void )
Destroy( Specular_p inst )
{
    if( inst->panel )
        xpanf->destroy( inst->panel );

    free( inst );
}

/***************************************************************
 *
 *  Copy
 *
 *  Copy the parameter values to a new copy of our instance.
 *
 ***************************************************************/
XCALL_( static LWError )
Copy( Specular_p to, Specular_p from )
{
    if( !to || !from )
        return NULL;

    VCPY( to->v_color, from->v_color );
    to->v_spec = from->v_spec;
    to->v_gloss = from->v_gloss;

    return NULL;
}

/***************************************************************
 *
 *  Load
 *
 *  Load our parameter values.
 *
 ***************************************************************/
XCALL_( static LWError )
Load( Specular_p inst, const LWLoadState *load )
{
    LWID    id;

    // Find the data blocks
    while( id = LWLOAD_FIND( load, Spec_Block ) )
    {
        switch( id )
        {
        case ID_SPEC_COLOR:
            {
                LWFVector   fvec;
                LWLOAD_FP( load, fvec, 3 );
                VCPY( inst->v_color, fvec );
            }
            break;
        
        case ID_SPEC_SPECULAR:
            {
                float   fval;
                LWLOAD_FP( load, &fval, 1 );
                inst->v_spec = fval;
            }
            break;
		
        case ID_SPEC_GLOSS:
            {
                float   fval;
                LWLOAD_FP( load, &fval, 1 );
                inst->v_gloss = fval;
            }
            break;
        }
        LWLOAD_END( load );
    }
    return NULL;
}

/***************************************************************
 *
 *  Save
 *
 *  Save our parameter values.
 *
 ***************************************************************/
XCALL_( static LWError )
Save( Specular_p inst, const LWSaveState *save )
{
	LWSAVE_BEGIN( save, &Spec_Block[ 0 ], 1 );
    {
        LWFVector   fvec;

        fvec[0] = (float)inst->v_color[0];
        fvec[1] = (float)inst->v_color[1];
        fvec[2] = (float)inst->v_color[2];

        LWSAVE_FP( save, fvec, 3 );
    }
	LWSAVE_END( save );

	LWSAVE_BEGIN( save, &Spec_Block[ 1 ], 1 );
    {
        float   fval = (float)inst->v_spec;
        LWSAVE_FP( save, &fval, 1 );
    }
	LWSAVE_END( save );

	LWSAVE_BEGIN( save, &Spec_Block[ 2 ], 1 );
    {
        float   fval = (float)inst->v_gloss;
        LWSAVE_FP( save, &fval, 1 );
    }
	LWSAVE_END( save );

    return NULL;
}

/***************************************************************
 *
 *  Init
 *
 *  Initialize render data here. Mode determines if init is
 *  called for a previewer or the renderer. In which case you
 *  might want to initialize data differently.
 *  This shading model does not need to initialize data.
 *
 ***************************************************************/
XCALL_( static LWError )
Init( Specular_p inst, int mode )
{
    return NULL;
}

/***************************************************************
 *
 *  Cleanup
 *
 *  Cleanup render data here.
 *  If there was data allocated in init, this is where you free
 *  the data. This function is called when the renderer or
 *  the previewer has finished.
 *
 ***************************************************************/
XCALL_( static void )
Cleanup( Specular_p inst )
{
}

/***************************************************************
 *
 *  NewTime
 *
 *  NewTime is called when a new pass starts.
 *  This is where you setup your time dependant data.
 *
 ***************************************************************/
XCALL_( static LWError )
NewTime( Specular_p inst, LWFrame f, LWTime t )
{
    return NULL;
}

/***************************************************************
 *
 *  A structure for the light sampler function.
 *  The light sampler function is useful to sample arealights
 *  correctly. The light sampler callback is called for each
 *  sample of the light. Only 1 sample is taken for
 *  non arealights.
 *
 ***************************************************************/
typedef struct LightSample_t {
    LWDVector   N, V, Color;
    double      x;
} LightSample, *LightSample_p;

/***************************************************************
 *
 *  The light sampler function.
 *
 ***************************************************************/
static int LightSamplerFunc( void *data, LWItemID id, const LWDVector dir, const LWDVector color )
{
    LightSample_p   sample = (LightSample_p)data;
    LWDVector       H;
    double          val;
	
    VADD3( H, dir, sample->V );
    Normalize( H );

    val = pow( MAX( -VDOT( H, sample->N ), 0.0 ), sample->x );
    if( val > 0.0 )
    {
        VADDS( sample->Color, color, val );
        return 1;
    }
    return 0;
}

/***************************************************************
 *
 *  Evaluate
 *
 *  The main evaluation function.
 *
 ***************************************************************/
XCALL_( static void )
Evaluate( Specular_p inst, LWNodalAccess *na, NodeOutputID out, NodeValue Value )
{
    LWDVector   color, final_color;
    double      specular = inst->v_spec;
    double      gloss = inst->v_gloss;
    LWItemID    light = (void*)1;

    // If the light sampler is not available, then we cannot render this shader.
	if( !na->illuminateSample )
		return;

    VCLR( final_color );
    VCPY( color, inst->v_color );

    // If the incoming ray is a shadow ray, there is no point of rendering this shader.
	if( ( na->flags & LWRT_SHADOW ) )
	{
		outputfuncs->setValue( Value, final_color );
		return;
	}

    // Evaluate the "Specularity" input from the node.
    inputfuncs->evaluate( inst->spec, na, &specular );
    if( specular > 0.0 ) // If the specularity value is larger than 0, we can proceed with the shading.
    {
        LWDVector   V;
        double      x;
        LightSample sampleData;

        // Evaluate the "Glossiness" input.
        inputfuncs->evaluate( inst->gloss, na, &gloss );
        x = pow( 2.0, gloss * 10.0 + 2.0 );
        
        // Evaluate the "Color" input.
        inputfuncs->evaluate( inst->color, na, color );

        // Calculate the eye vector.
        VSUB3( V, na->wPos, na->raySource );
        Normalize( V );

        if( iteminfo ) // In Modeler there is no iteminfo.
            light = iteminfo->first( LWI_LIGHT, LWITEM_NULL );
		
        // Store the normal we want to use in the light sampler data.
        VCPY( sampleData.N, na->wNorm );

        // Store the eye vector in the light sampler data.
        VCPY( sampleData.V, V );

        // Store the glossiness exponent in the light sampler data.
        sampleData.x = x;

        // Start sampling the lights.
        while( light )
        {
            unsigned int    flags = lightinfo->flags( light );
            int             excluded = 0; // By default, the exclusion is off.

            if( objinfo && na->objID )
                excluded = objinfo->excluded( na->objID, light );

            // Check if the object being rendered is excluded from this light,
            // or if the light should not render a specular.
            if( !excluded && !( flags & LWLFL_NO_SPECULAR ) )
            {
                LWDVector   dir;
                double      intensity;

                VCLR( sampleData.Color );

                // Start sampling the illumination for this light.
                intensity = na->illuminateSample( light, na->wPos, dir, LightSamplerFunc, &sampleData );
                if( intensity > 0.0 )
                    VADD( final_color, sampleData.Color );
            }
			
            // Get the next light in the scene...
            if( iteminfo )
                light = iteminfo->next( light );
            else break; // Otherwise break the loop.
        }

        // Scale the shading by the specularity amount.
        VSCL( final_color, specular );

        // Tint the shading with the color parameter of the node.
        final_color[0] *= color[0];
        final_color[1] *= color[1];
        final_color[2] *= color[2];
    }
    // Set the output color value.
    outputfuncs->setValue( Value, final_color );
}

/***************************************************************
 *
 *  The user interface get callback.
 *
 ***************************************************************/
static void *Spec_Get( Specular_p inst, unsigned int vid )
{
    /*  Return NULL to disable the control,
        if there is something connected to the corresponding input. */

    switch( vid )
    {
    case Spec_color:
        if( inputfuncs->check( inst->color ) )
            return NULL;
        return inst->v_color;

    case Spec_specular:
        if( inputfuncs->check( inst->spec ) )
            return NULL;
        return &inst->v_spec;

    case Spec_gloss:
        if( inputfuncs->check( inst->gloss ) )
            return NULL;
        return &inst->v_gloss;

    default:
        return NULL;
    }
}

/***************************************************************
 *
 *  The user interface set callback.
 *
 ***************************************************************/
static LWXPRefreshCode Spec_Set( Specular_p inst, unsigned int vid, void *value )
{
    double  *d = (double*)value;

    switch( vid )
    {
    case Spec_color:
        inst->v_color[0] = d[0];
        inst->v_color[1] = d[1];
        inst->v_color[2] = d[2];
        break;

    case Spec_specular:
        inst->v_spec = *d;
        break;

    case Spec_gloss:
        inst->v_gloss = *d;
        break;

    default:
        return LWXPRC_NONE;
    }
    return LWXPRC_DFLT;
}

/***************************************************************
 *
 *  The user interface change notification callback.
 *
 ***************************************************************/
static void UIChangeNotify( LWXPanelID xpanel, unsigned int cid, unsigned int vid, int event )
{
    Specular_p	inst;
	
    if( !xpanel )
        return;
	
    inst = (Specular_p)xpanf->getData( xpanel, 0 );

    if( !inst )
        return;

    /*  If the user is tracking a control, we don't want to update the entire node flow.
        We just need to update this node, to make the previewer update faster. */
    if( event == LWXPEVENT_TRACK )
    {
        nodefuncs->UpdateNodePreview( inst->node );
        return;
    }
    // The value is set, so update the entire node flow.
    else if( event == LWXPEVENT_VALUE )
        lwupdate( LWNODE_HCLASS, inst );
}

/***************************************************************
 *
 *  The user interface control descriptions.
 *
 ***************************************************************/
static LWXPanelControl Spec_control[] = {
    { Spec_color,       "Color",        "color"     },
    { Spec_specular,    "Specularity",  "percent"   },
    { Spec_gloss,       "Glossiness",   "percent"   },
    { 0 }
};

static LWXPanelDataDesc Spec_data[] = {
    { Spec_color,       "Color",        "color"   },
    { Spec_specular,    "Specularity",  "float"   },
    { Spec_gloss,       "Glossiness",   "float"   },
    { 0 }
};

/***************************************************************
 *
 *  The user interface panel callback.
 *
 ***************************************************************/
XCALL_( static LWXPanelID )
Spec_Panel( Specular_p inst )
{
    if( !inst )
        return NULL;

    if( !inst->panel )
    {
        // We will use the name of the node as the the title for the panel.
        const char	*nname = nodefuncs->nodeName( inst->node );

        LWXPanelHint Spec_notify_hint[] = {
            XpCHGNOTIFY( UIChangeNotify ),
            XpLABEL( 0, nname ),
            XpDIVADD( Spec_color ),
            XpEND
        };

        if( NULL != ( inst->panel = xpanf->create( LWXP_VIEW, Spec_control ) ) )
        {
            xpanf->describe( inst->panel, Spec_data, Spec_Get, Spec_Set );
            xpanf->hint( inst->panel, 0, Spec_notify_hint );
            xpanf->viewInst( inst->panel, inst );
            xpanf->setData( inst->panel, 0, inst );
        }
        else
            return NULL;
    }
    return inst->panel;
}

/***************************************************************
 *
 *  Handler activation function.
 *
 ***************************************************************/
XCALL_( static int )
Handler( int version, GlobalFunc *global, LWNodeHandler *local,
   void *serverData)
{
    if( version != LWNODECLASS_VERSION )
        return AFUNC_BADVERSION;

    inputfuncs  = global( LWNODEINPUTFUNCS_GLOBAL,  GFUSE_TRANSIENT );
    outputfuncs = global( LWNODEOUTPUTFUNCS_GLOBAL, GFUSE_TRANSIENT );
    nodefuncs   = global( LWNODEFUNCS_GLOBAL,       GFUSE_TRANSIENT );
    iteminfo    = global( LWITEMINFO_GLOBAL,        GFUSE_TRANSIENT );
    lightinfo   = global( LWLIGHTINFO_GLOBAL,       GFUSE_TRANSIENT );
    objinfo     = global( LWOBJECTINFO_GLOBAL,       GFUSE_TRANSIENT );

    if( !inputfuncs || !outputfuncs || !nodefuncs || !iteminfo || !lightinfo || !objinfo )
        return AFUNC_BADGLOBAL;

    local->inst->priv       = global;
    local->inst->create     = Create;
    local->inst->destroy    = Destroy;
    local->inst->load       = Load;
    local->inst->save       = Save;
    local->inst->copy       = Copy;
    local->rend->init       = Init;
    local->rend->cleanup    = Cleanup;
    local->rend->newTime    = NewTime;
    local->evaluate         = Evaluate;

    return AFUNC_OK;
}

/***************************************************************
 *
 *  Interface activation function.
 *
 ***************************************************************/
XCALL_(int)
Interface( int version, GlobalFunc *global, LWInterface *local, void *serverdata )
{
    if( local == (LWInterface*)NULL )
        return AFUNC_BADLOCAL;

    lwupdate  = global( LWINSTUPDATE_GLOBAL,        GFUSE_TRANSIENT );
    xpanf     = global( LWXPANELFUNCS_GLOBAL,       GFUSE_TRANSIENT );

    if( !lwupdate || !xpanf )
        return AFUNC_BADGLOBAL;

    local->panel    = Spec_Panel( local->inst );
    local->options  = NULL;
    local->command  = NULL;

    return AFUNC_OK;
}

/***************************************************************
 *
 *  The server record
 *
 ***************************************************************/
ServerRecord ServerDesc[] = {
    { LWNODE_HCLASS, "Demo_SpecularNode", (ActivateFunc*)Handler },
    { LWNODE_ICLASS, "Demo_SpecularNode", (ActivateFunc*)Interface },
    { NULL }
};
