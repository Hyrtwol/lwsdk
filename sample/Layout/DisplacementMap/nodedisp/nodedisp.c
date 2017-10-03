/* NodeDisp.c
 *
 *   Node Editor for a displacement class plugin.
 *
 *	 Antti Järvelä.
 */

#include <lwserver.h>
#include <lwhost.h>
#include <lwhandler.h>
#include <lwmath.h>
#include <lwdisplce.h>
#include <lwrender.h>
#include <lwnodeeditor.h>
#include <lwxpanel.h>

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <float.h>

/*****************************************************************************
 *  
 *  Globals.
 *
 *****************************************************************************/
static LWNodeEditorFuncs	*nodeedf = NULL;
static LWItemInfo			*iteminfo = NULL;
static LWNodeInputFuncs		*inputfuncs = NULL;
static LWNodeFuncs			*nodefuncs = NULL;
static LWXPanelFuncs		*xpanf = NULL;
static LWInstUpdate			*lwupdate = NULL;
static LWObjectInfo			*objinfo = NULL;
static LWSceneInfo			*sceneinfo = NULL;

enum { ID_AMP = 0x8001, ID_USEAXIS, ID_AXIS };

/*****************************************************************************
 *  
 *  Instance structure.
 *
 *****************************************************************************/
typedef struct NodeDisp_t {

	LWXPanelID		xpanel;
	NodeID			node;

	int				axis;
	int				use_axis;
	LWItemID		item, camera;

	NodeEditorID	node_editor;
	NodeInputID		amp_inp, vector_inp;
	double			amp;

	double			wXfrm[9];
	double			oXfrm[9];

	LWDVector		camera_wpos;

} NodeDisp, *NodeDisp_p;

/*****************************************************************************
 *  
 *  Structure for autosize function.
 *
 *****************************************************************************/
typedef struct BBox_t {
	double				bounds[6];
	LWMeshInfo			*mesh;
} BBox, *BBox_p;

/*****************************************************************************
 *  
 *  Blocks for saving and loading.
 *
 *****************************************************************************/
#define	IDS_NODES				LWID_( 'N','D','N','O' )
#define	IDS_AMP					LWID_( 'N','D','A','P' )
#define	IDS_USEAXIS				LWID_( 'N','D','U','A' )
#define	IDS_AXIS				LWID_( 'N','D','A','X' )

static LWBlockIdent NodeDisp_Block[] = {
	IDS_NODES,		"Nodes",
	IDS_AMP,		"Amplitude",
	IDS_USEAXIS,	"UseAxis",
	IDS_AXIS,		"Axis",
	0
};

/*****************************************************************************
 *  
 *  Get function for the xpanels.
 *
 *****************************************************************************/
static void *RootNode_Get( LWInstance vdata, unsigned int vid )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	switch( vid )
	{
		case ID_AMP:
			return &inst->amp;

		case ID_USEAXIS:
			return &inst->use_axis;

		case ID_AXIS:
			return &inst->axis;

		default:
			return NULL;
	}
	return NULL;
}

/*****************************************************************************
 *  
 *  Set function for the xpanels.
 *
 *****************************************************************************/
static LWXPRefreshCode RootNode_Set( LWInstance vdata, unsigned int vid, void *value )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	switch( vid )
	{
		case ID_AMP:
			inst->amp = *(double*)value;
			break;

		case ID_USEAXIS:
			inst->use_axis = *(int*)value;
			break;

		case ID_AXIS:
			inst->axis = *(int*)value;
			break;

		default:
			return LWXPRC_NONE;
	}
	return LWXPRC_DFLT;
}

/*****************************************************************************
 *  
 *  Notify UI change.
 *
 *****************************************************************************/
static void UIChangeNotify( LWXPanelID xpanel, unsigned int cid, unsigned int vid, int event )
{
	NodeDisp_p	inst;

	inst = (NodeDisp_p)xpanf->getData( xpanel, 0 );
	if( !inst ) return;

	if( event == LWXPEVENT_TRACK )
	{
		nodefuncs->UpdateNodePreview( inst->node );
		return;
	}
	else if( event == LWXPEVENT_VALUE )
	{
		lwupdate( LWDISPLACEMENT_HCLASS, inst );
		lwupdate( LWNODE_HCLASS, inst );
	}
}

/*****************************************************************************
 *  
 *  Panel controls.
 *
 *****************************************************************************/
static LWXPanelControl RootNode_control[] = {
	{ ID_AMP,		"Amplitude",	"float" },
	{ ID_USEAXIS,	"Use Axis",		"iBoolean" },
	{ ID_AXIS,		"Axis",			"axis" },
	{ 0 }
};

static LWXPanelDataDesc RootNode_data[] = {
	{ ID_AMP,		"Amplitude",	"float" },
	{ ID_USEAXIS,	"Use Axis",		"integer" },
	{ ID_AXIS,		"Axis",			"integer" },
	{ 0 }
};

/*****************************************************************************
 *  
 *  Panel function.
 *	Called when the user selects "Edit node" for the root node.
 *
 *****************************************************************************/
XCALL_( static LWXPanelID ) RootNode_Panel( NodeEditorID editor )
{
	NodeDisp_p	inst = (NodeDisp_p)nodeedf->getUserData( editor );

	if( !inst ) return NULL;

	if( !inst->xpanel )
	{
		LWXPanelHint RootNode_notify_hint[] = {
			XpCHGNOTIFY( UIChangeNotify ),
			XpLABEL( 0, "Displacement" ),
			XpENABLE_( ID_USEAXIS ),
				XpH( ID_AXIS ),
			XpEND,
			XpEND
		};

        inst->xpanel = xpanf->create( LWXP_VIEW, RootNode_control );
		if( inst->xpanel ) {
			xpanf->describe( inst->xpanel, RootNode_data, RootNode_Get, RootNode_Set );
			xpanf->hint( inst->xpanel, 0, RootNode_notify_hint );
			xpanf->viewInst( inst->xpanel, inst );
			xpanf->setData( inst->xpanel, 0, inst );
		} else return NULL;
	}
	return inst->xpanel;
}

/*****************************************************************************
 *  
 *  Root node preview.
 *
 *****************************************************************************/
XCALL_( static void ) rootNodePreview( NodeEditorID editor, LWNodalAccess* na, LWDVector pcolor )
{
	NodeDisp_p	inst = (NodeDisp_p)nodeedf->getUserData( editor );
	double		amp = inst->amp;
	LWDVector	vector;

	VCLR( vector );

	inputfuncs->evaluate( inst->amp_inp, na, &amp );
	inputfuncs->evaluate( inst->vector_inp, na, vector );

	if( inst->use_axis )
	{
		switch( inst->axis )
		{
		case 0:
			vector[1] = 0.0;
			vector[2] = 0.0;
			break;

		case 1:
			vector[0] = 0.0;
			vector[2] = 0.0;
			break;

		case 2:
			vector[0] = 0.0;
			vector[1] = 0.0;
			break;

		default:
			break;
		}
	}

	VSCL( vector, amp );
	VCPY( pcolor, vector );
}

/*****************************************************************************
 *  
 *  polyScan function for autosize.
 *
 *****************************************************************************/
static int polyScan( void *vbox, LWPolID id )
{
	BBox_p		bbox = (BBox_p)vbox;
	int			i, nverts;
	
	nverts = bbox->mesh->polSize( bbox->mesh, id );
	for( i = 0; i < nverts; i++ )
	{
		LWFVector	pos;

		bbox->mesh->pntBasePos( bbox->mesh, bbox->mesh->polVertex( bbox->mesh, id, i % nverts ), pos );
		bbox->bounds[0] = MIN( bbox->bounds[0], pos[0] );
		bbox->bounds[1] = MAX( bbox->bounds[1], pos[0] );
		bbox->bounds[2] = MIN( bbox->bounds[2], pos[1] );
		bbox->bounds[3] = MAX( bbox->bounds[3], pos[1] );
		bbox->bounds[4] = MIN( bbox->bounds[4], pos[2] );
		bbox->bounds[5] = MAX( bbox->bounds[5], pos[2] );
	}
	return 0;
}

/*****************************************************************************
 *  
 *  Node Editor autosize function.
 *
 *****************************************************************************/
XCALL_( static void ) NodeDisp_Autosize( NodeEditorID nodal, void *userData, double bboxa[3][2] )
{
	NodeDisp_p		inst = (NodeDisp_p)userData;
	BBox_p			bbox;

	if( !objinfo || !inst )
		return;

	bbox = (BBox_p)calloc( 1, sizeof( BBox ) );
	if( !bbox ) return;

	bbox->bounds[0] = bbox->bounds[2] = bbox->bounds[4] = DBL_MAX;
	bbox->bounds[1] = bbox->bounds[3] = bbox->bounds[5] = DBL_MIN;

	bbox->mesh = objinfo->meshInfo( inst->item, 0 );
	
	if( !bbox->mesh )
	{
		free( bbox );
		return;
	}

	bbox->mesh->scanPolys( bbox->mesh, (LWPolScanFunc*)polyScan, bbox );

	bboxa[0][0] = bbox->bounds[0];
	bboxa[0][1] = bbox->bounds[1];

	bboxa[1][0] = bbox->bounds[2];
	bboxa[1][1] = bbox->bounds[3];

	bboxa[2][0] = bbox->bounds[4];
	bboxa[2][1] = bbox->bounds[5];

	if( bbox->mesh->destroy ) bbox->mesh->destroy( bbox->mesh );

	free( bbox );
}

/*****************************************************************************
 *  
 *  Node Editor event function.
 *
 *****************************************************************************/
XCALL_( static void ) NodeDisp_EventFunc( NodeEditorID editor )
{
	NodeDisp_p	inst = (NodeDisp_p)nodeedf->getUserData( editor );

	if( lwupdate && inst )
		lwupdate( LWDISPLACEMENT_HCLASS, inst );
}

/*****************************************************************************
 *  
 *  Create.
 *
 *****************************************************************************/
XCALL_( static LWInstance ) NodeDisp_Create( void *priv, LWItemID item, LWError *err )
{
	NodeDisp_p		inst = NULL;
	LWRootNode		rootnode;
	LWChanGroupID	cgroup = NULL;

	inst = (NodeDisp_p)calloc( 1, sizeof( NodeDisp ) );
	if( !inst )
	{
		*err = "Instance creation failed!";
		return NULL;
	}

	inst->axis = 1;
	inst->use_axis = 1;
	inst->amp = 1.0;
	inst->item = item;

	memset( &rootnode, 0, sizeof( LWRootNode ) );
	rootnode.rootPanel = RootNode_Panel;
	rootnode.rootPreview = rootNodePreview;

	inst->node_editor = nodeedf->create( "Displacement", "Node Displacement", &rootnode, inst );
	if( !inst->node_editor )
	{
		*err = "Node editor creation failed!";
		return NULL;
	}

	cgroup = iteminfo->chanGroup( item );
	nodeedf->setEnvGroup( inst->node_editor, cgroup );
	inst->node = nodeedf->getRootNodeID( inst->node_editor );
	nodeedf->setUpdateFunc( inst->node_editor, NodeDisp_EventFunc );
	nodeedf->setAutosize( inst->node_editor, NodeDisp_Autosize );

	inst->amp_inp = nodeedf->addInput( inst->node_editor, NOT_SCALAR, "Amplitude", NULL );
	inst->vector_inp = nodeedf->addInput( inst->node_editor, NOT_VECTOR, "Vector", NULL );

	return inst;
}

/*****************************************************************************
 *  
 *  Destroy.
 *
 *****************************************************************************/
XCALL_( static void ) NodeDisp_Destroy( LWInstance vdata )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	if( inst->xpanel ) xpanf->destroy( inst->xpanel );
	if( inst->node_editor ) nodeedf->destroy( inst->node_editor );
	if( inst ) free( inst );
}

/*****************************************************************************
 *  
 *  Copy.
 *
 *****************************************************************************/
XCALL_( static LWError ) NodeDisp_Copy( LWInstance vto, LWInstance vfrom )
{
	NodeDisp_p	to_inst = (NodeDisp_p)vto;
	NodeDisp_p	from_inst = (NodeDisp_p)vfrom;

	if( !to_inst || !from_inst )
		return "Couldn't copy data!";

	nodeedf->copy( to_inst->node_editor, from_inst->node_editor );
	to_inst->axis = from_inst->axis;
	to_inst->use_axis = from_inst->axis;
	to_inst->amp = from_inst->amp;

	return NULL;
}

/*****************************************************************************
 *  
 *  Load.
 *
 *****************************************************************************/
XCALL_( static LWError ) NodeDisp_Load( LWInstance vdata, const LWLoadState *load )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;
	LWError		err;
	LWID		id;
	LWFVector	f;

	VCLR( f );

	while( id = LWLOAD_FIND( load, NodeDisp_Block ) )
	{
		switch( id )
		{
		
		case IDS_NODES:
			err = nodeedf->load( inst->node_editor, load );
			break;

		case IDS_AMP:
			LWLOAD_FP( load, f, 1 );
			inst->amp = f[0];
			break;

		case IDS_USEAXIS:
			LWLOAD_I4( load, &inst->use_axis, 1 );
			break;

		case IDS_AXIS:
			LWLOAD_I4( load, &inst->axis, 1 );
			break;
		}

		LWLOAD_END( load );
	}

	if( err ) return err;
	return NULL;
}

/*****************************************************************************
 *  
 *  Save.
 *
 *****************************************************************************/
XCALL_( static LWError ) NodeDisp_Save( LWInstance vdata, const LWSaveState *save )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;
	LWError		err;
	LWFVector	f;

	VCLR( f );

	LWSAVE_BEGIN( save, &NodeDisp_Block[ 0 ], 0 );
	err = nodeedf->save( inst->node_editor, save );
	LWSAVE_END( save );

	LWSAVE_BEGIN( save, &NodeDisp_Block[ 1 ], 1 );
	f[0] = inst->amp;
	LWSAVE_FP( save, f, 1 );
	LWSAVE_END( save );

	LWSAVE_BEGIN( save, &NodeDisp_Block[ 2 ], 1 );
	LWSAVE_I4( save, &inst->use_axis, 1 );
	LWSAVE_END( save );

	LWSAVE_BEGIN( save, &NodeDisp_Block[ 3 ], 1 );
	LWSAVE_I4( save, &inst->axis, 1 );
	LWSAVE_END( save );

	if( err ) return err;
	return NULL;
}

/*****************************************************************************
 *  
 *  Init.
 *
 *****************************************************************************/
XCALL_( static LWError) NodeDisp_Init( LWInstance vdata, int mode )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	nodeedf->init( inst->node_editor, mode );

	return NULL;
}

/*****************************************************************************
 *  
 *  newTime.
 *
 *****************************************************************************/
XCALL_( static LWError) NodeDisp_NewTime( LWInstance vdata, LWFrame f, LWTime t )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	nodeedf->newTime( inst->node_editor, f, t );

	if( inst->item )
	{
		iteminfo->param( inst->item, LWIP_W_RIGHT,		t, &inst->wXfrm[0] );
		iteminfo->param( inst->item, LWIP_W_UP,			t, &inst->wXfrm[3] );
		iteminfo->param( inst->item, LWIP_W_FORWARD,	t, &inst->wXfrm[6] );

		iteminfo->param( inst->item, LWIP_RIGHT,		t, &inst->oXfrm[0] );
		iteminfo->param( inst->item, LWIP_UP,			t, &inst->oXfrm[3] );
		iteminfo->param( inst->item, LWIP_FORWARD,		t, &inst->oXfrm[6] );
	}

	VCLR( inst->camera_wpos );
	inst->camera = sceneinfo->renderCamera( t );
	if( inst->camera )
		iteminfo->param( inst->camera, LWIP_W_POSITION,	t, inst->camera_wpos );

	return NULL;
}

/*****************************************************************************
 *  
 *  Cleanup.
 *
 *****************************************************************************/
XCALL_( static void ) NodeDisp_Cleanup( LWInstance vdata )
{
	NodeDisp_p	inst = (NodeDisp_p)vdata;

	nodeedf->cleanup( inst->node_editor );
}

/*****************************************************************************
 *  
 *  Evaluate.
 *
 *****************************************************************************/
XCALL_( static void ) NodeDisp_Evaluate( LWInstance vdata, LWDisplacementAccess *da )
{
	NodeDisp_p		inst = (NodeDisp_p)vdata;
	LWDVector		vector;
	double			amp = inst->amp;
	LWNodalAccess	na;
	int				i;

	memset( &na, 0, sizeof( LWNodalAccess ) );

	VCPY( na.oPos, da->oPos );
	VCPY( na.wPos, da->source );
	for( i = 0; i < 9; i++ )
	{
		na.wXfrm[i] = inst->wXfrm[i];
		na.oXfrm[i] = inst->oXfrm[i];
	}

	na.verts[0] = da->point;
	na.weights[0] = 1.0f;
	na.gNorm[inst->axis] = na.wNorm[inst->axis] = na.wNorm0[inst->axis] = 1;
	na.spotSize = 0.001;
	VCPY( na.vertsWPos[0], da->source );
	VCPY( na.raySource, inst->camera_wpos );
	VSUB3( vector, na.wPos, na.raySource );
	na.rayLength = VLEN( vector );
	na.objID = inst->item;
	na.sourceID = inst->camera;
	VCLR( vector );

	inputfuncs->evaluate( inst->amp_inp, &na, &amp );
	inputfuncs->evaluate( inst->vector_inp, &na, vector );

	if( inst->use_axis )
	{
		switch( inst->axis )
		{
		case 0:
			vector[1] = 0.0;
			vector[2] = 0.0;
			break;

		case 1:
			vector[0] = 0.0;
			vector[2] = 0.0;
			break;

		case 2:
			vector[0] = 0.0;
			vector[1] = 0.0;
			break;

		default:
			break;
		}
	}

	VSCL( vector, amp );
	VADD( da->source, vector );
}

/*****************************************************************************
 *  
 *  Flags.
 *
 *****************************************************************************/
XCALL_( static unsigned int ) NodeDisp_Flags( LWInstance vdata )
{
/*	NodeDisp_p	inst = (NodeDisp_p)vdata; */
	return LWDMF_WORLD;
}

/*****************************************************************************
 *  
 *  Handler activation.
 *
 *****************************************************************************/
XCALL_( static int ) NodeDisp_Handler( int version, GlobalFunc *global, LWDisplacementHandler *local, void *serverData )
{
	nodeedf = global( LWNODEEDITORFUNCS_GLOBAL, GFUSE_TRANSIENT );
	iteminfo = global( LWITEMINFO_GLOBAL, GFUSE_TRANSIENT );
	inputfuncs = global( LWNODEINPUTFUNCS_GLOBAL, GFUSE_TRANSIENT );
	nodefuncs = global( LWNODEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	sceneinfo = global( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );

	if( !nodeedf || !iteminfo || !inputfuncs || !nodefuncs || !sceneinfo ) return AFUNC_BADGLOBAL;

	local->inst->create =	NodeDisp_Create;
	local->inst->destroy =	NodeDisp_Destroy;
	local->inst->load =		NodeDisp_Load;
	local->inst->save =		NodeDisp_Save;
	local->inst->copy =		NodeDisp_Copy;

	local->rend->init =		NodeDisp_Init;
	local->rend->newTime =	NodeDisp_NewTime;
	local->rend->cleanup =	NodeDisp_Cleanup;

	local->evaluate =		NodeDisp_Evaluate;
	local->flags =			NodeDisp_Flags;
	local->inst->descln =	NULL;

	return AFUNC_OK;
}

/*****************************************************************************
 *  
 *  Interface activation.
 *
 *****************************************************************************/
XCALL_( static int ) NodeDisp_Interface( int version, GlobalFunc *global, LWInterface *local, void *serverData )
{
	NodeDisp_p	inst = NULL;

	if( version != LWINTERFACE_VERSION )
		return AFUNC_BADVERSION;

	xpanf = global( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT );
	lwupdate = (LWInstUpdate*)global( LWINSTUPDATE_GLOBAL, GFUSE_TRANSIENT );
	objinfo = global( LWOBJECTINFO_GLOBAL, GFUSE_TRANSIENT );
	if( !xpanf || !lwupdate ) return AFUNC_BADGLOBAL;

	inst = (NodeDisp_p)local->inst;

	if( inst->node_editor )
		nodeedf->OpenNodeEditor( inst->node_editor );

	return AFUNC_OK;
}

/*****************************************************************************
 *  
 *  Server tag.
 *
 *****************************************************************************/
static ServerTagInfo NodeDisp_tag[] = { 
	{ "Node Displacement", SRVTAG_USERNAME },
	{ "", 0 }
};

/*****************************************************************************
 *  
 *  Server record.
 *
 *****************************************************************************/
ServerRecord ServerDesc[] = {
	{ LWDISPLACEMENT_HCLASS, "LW_NodeDisplacement", (ActivateFunc*)NodeDisp_Handler, NodeDisp_tag },
	{ LWDISPLACEMENT_ICLASS, "LW_NodeDisplacement", (ActivateFunc*)NodeDisp_Interface, NodeDisp_tag },
	{ NULL }
};

