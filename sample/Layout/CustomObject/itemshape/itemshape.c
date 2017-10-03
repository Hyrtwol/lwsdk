// Custom Object Class Item Display
// Arnie Cachelin, Copyright 2001 NewTek, Inc.
// 

#include <stddef.h>
#include <lwhost.h>
#include <lwserver.h>
#include <lwhandler.h>
#include <lwenvel.h>
#include <lwchannel.h>
#include <lwrender.h>
#include <lwcustobj.h>
#include <lwtxtr.h>
#include <lwmath.h>
#include <lwcomring.h>
#include <lwitemshape.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include "custdraw.h"

void killItemTag(LWItemInfo *itinfo, LWItemID id, char *key);
int setItemTag(LWItemInfo *itInfo, LWItemID id, char *key, char *val);

static  LWComRing   *GlobalComRing;
//static    LWXPanelFuncs   *GlobalXPanFun;
static  LWMessageFuncs  *Gmessage;
//static    LWTextureFuncs  *GlobalTextureFuncs;
static  LWEnvelopeFuncs *GlobalEnvelopeFuncs;
static  LWItemInfo  *GlobalItemInfo;
static  LWChannelInfo   *GlobalChannelInfo;
//static    LWSceneInfo *GlobalSceneInfo;
//static    LWCameraInfo    *GlobalCameraInfo;
static  LWInstUpdate    *GlobalLWUpdate;
/*
static  float   Red[]   = {0.92f, 0.2f, 0.2f},
        Green[]     = {0.2f, 0.92f, 0.2f},
        Blue[]      = {0.2f, 0.2f, 0.92f},
        Yellow[]    = {0.92f, 0.92f, 0.2f},
        Orange[]    = {0.92f, 0.62f, 0.32f},
        White[]     = {0.92f, 0.92f, 0.92f},
        Black[]     = {0.1f, 0.1f, 0.1f};
*/
static char TagBuf[TAG_SIZE+1]="";
static char *JustList[] = {"Left","Center","Right",NULL};
static LWXPanelControl ItemShape_ctrl_list[] = {
    { CH_AXIS,       STR_Axis_TEXT,          "axis" },
    { CH_SHAPE,        STR_Shape_TEXT,          "iPopChoice" },
    { CH_FILL,        STR_Filled_TEXT,          "iBoolean" },
    { CH_USEC,        STR_UseC_TEXT,          "iBoolean" },
    { CH_COLR,        STR_Color_TEXT,          "color" },
    { CH_USEBC,        STR_UseBC_TEXT,          "iBoolean" },
    { CH_BCOLR,        STR_Color_TEXT,          "color" },
    { CH_USETC,        STR_UseTC_TEXT,          "iBoolean" },
    { CH_TCOLR,        STR_Color_TEXT,          "color" },
    { CH_LABL,        STR_Label_TEXT,          "string" },
    { CH_JUST,        STR_Just_TEXT,          "iPopChoice" },
    { CH_SCAL,        STR_Scale_TEXT,          "distance" },
    { CH_LEVL,        STR_Level_TEXT,          "percent" },
    { CH_TARG,        STR_Link_TEXT,          "iPopChoice" },
    { CH_USELB,        "",          "integer" },
    {0}
};
static LWXPanelDataDesc ItemShape_data_descrip[] = {
    { CH_AXIS,        STR_Axis_TEXT,          "integer" },
    { CH_SHAPE,        STR_Shape_TEXT,          "integer" },
    { CH_FILL,        STR_Filled_TEXT,          "integer" },
    { CH_USEC,        STR_UseC_TEXT,            "integer" },
    { CH_COLR,        STR_Color_TEXT,          "color" },
    { CH_USEBC,        STR_UseBC_TEXT,          "integer" },
    { CH_BCOLR,        STR_Color_TEXT,          "color" },
    { CH_USETC,        STR_UseTC_TEXT,          "integer" },
    { CH_TCOLR,        STR_Color_TEXT,          "color" },
    { CH_LABL,        STR_Label_TEXT,          "string" },
    { CH_JUST,        STR_Just_TEXT,            "integer" },
    { CH_SCAL,        STR_Scale_TEXT,          "distance" },
    { CH_LEVL,        STR_Level_TEXT,          "percent" },
    { CH_TARG,        STR_Link_TEXT,          "integer" },
    { CH_USELB,        "",          "integer" },
    {0},
};

/* ----------------- Plug-in Methods: LWInstanceFuncs  -----------------  */

static char *errAllocFailed ="Tiny Allocation Failed, I don't feel so good", *AxisName="XYZ";

static int xax[] = {1,2,0}, yax[] = {2,0,1};//, zax[] = {0,1,2};

static const char *ShapeList[] = {
        "Standard", 
        "Box", 
        "Ball", 
        "Pyramid", 
        "Diamond", 
        "Tetra", 
        "Ring",
        "Grid",
        "None",
        NULL
};  

// protos from proxypic
extern int popCnt_Item( LWItemInfo *ItemInfo );
extern const char *popName_Item( LWItemInfo *ItemInfo, int idx );
extern int popItemIdx( LWItemInfo *ItemInfo, LWItemID item );
extern LWItemID popIdxItem( LWItemInfo *ItemInfo, int idx );

// proto from this file:
static LWXPRefreshCode ItemShapeData_set ( void *myinst, unsigned int vid, void *value );
static int inalreadyreadlist( void *ctxt, AlreadyReadListMember *list );

// return index of item in list, 
int setItemTag(LWItemInfo *itInfo, LWItemID id, char *key, char *val)
{
    int t,n=0,i, blank=0;
    const char *tag;
    sprintf(TagBuf,"%s=", key);
    strncat(TagBuf, val, TAG_SIZE);
    for(t=0; t<MAX_TAGS; t++)   // store tag changes for current item
    {
        if( NULL != (tag=(*itInfo->getTag)(id,t+1)) )
        {
            n++;
            i = strlen(key);
            if(tag[0]==0)
                blank = t+1; // catch empty string tags for later
            if(!strncmp(tag,key,i) )
            {
                (*itInfo->setTag)(id,t+1,TagBuf);
                return n;
            }
        }
        else // end of existing tags
        {
            n++;
            (*itInfo->setTag)(id,blank,TagBuf); // add tag, or if blank, fill in""
            return blank ? blank:n;
        }
    }
    return n;
}

static int getItemTag(LWItemInfo *itInfo, LWItemID id, char *key, char *val, int len)
{
    int t,n=0,i;
    const char *tag;
    *val = 0;
    for(t=0; t<MAX_TAGS; t++)   // store tag changes for current item
    {
        if( NULL != (tag=(*itInfo->getTag)(id,t+1)) )
        {
            n++;
            i = strlen(key);
            if(!strncmp(tag,key,i))
            {
                //i++; // skip '='
                strncpy(val,&(tag[i]),len-i); // tag = "KEY="tag[i+1]
                return n;
            }
        }
    }
    return 0;
}

void killItemTag(LWItemInfo *itinfo, LWItemID id, char *key)
{
    int t=1,i;
    const char *tag;
    while( NULL != (tag=(*itinfo->getTag)(id,t)) )
    {
        i = strlen(key);
        if(!strncmp(tag,key,i))
        {
            (*itinfo->setTag)(id,t,"");
            return;
        }
        t++;
    }
    return;
}
// ********** Comring function added:  
static void ringEvent(void *clientData,void *portData,int eventCode,void *eventData)
{
    int error;
    //void *context;
    ItemShapeData     *isd, *edata;
    AlreadyReadListMember *oldlist;

    if( !eventData || !clientData ) return;

    isd = (ItemShapeData *)clientData;
    edata = (ItemShapeData *)eventData;
    
    if( edata->self != isd->self )
    {
        return;
    }
    if(eventCode == ITEMSHAPE_COMRING_READ ) 
    {  
        if( !inalreadyreadlist( isd->ctxt, edata->alreadyreadlist ) ){
            oldlist = edata->alreadyreadlist;
            memcpy( edata, isd, sizeof( ItemShapeData ) );
            edata->alreadyreadlist = oldlist;
        }
        return;
    }    
    else if( eventCode == ITEMSHAPE_COMRING_WRITE ) 
    {
        if( edata->ctxt == isd->ctxt ){
            memcpy( isd, edata, sizeof( ItemShapeData ) );
            error = ItemShapeData_set ( isd->ctxt, CH_ALLBYCOMRINGUPDATE, NULL ); 
        }
        else{
            return;
        }
    }
  
    return;
}

//**********
static int inalreadyreadlist( void *ctxt, AlreadyReadListMember *list )
{   
    AlreadyReadListMember* testlistmem;

    if ( !list ) return 0;

    testlistmem = list;
    while( testlistmem )
    {
        if( testlistmem->ctxt == ctxt )
        {
            return 1;
        }
        testlistmem = testlistmem->next;
    }

    return 0;
}

//**********************************

XCALL_(static LWInstance)ItemShapeCreate(void *data, LWItemID id, LWError *err)
{
    ItemShapeData *dat=NULL;
    XCALL_INIT;
    dat=malloc(sizeof(ItemShapeData));
    if( dat)
    {
        memset(dat,0,sizeof(*dat));
        dat->fill = 0;
        dat->axis = 1;
        dat->level = 1.0;
        dat->self = id;
        dat->scale = 1.0;
        dat->ctxt = dat;
        dat->justify = LWCUST_TEXT_C;
        getItemTag( GlobalItemInfo, id, "LINK=", dat->label, sizeof(dat->label) );
        if(dat->label[0])
        {
            unsigned int id32 = 0;
            if( 1 == sscanf(dat->label,"%x", &id32))
                dat->linkTo = UINT2LWITEMID(id32);
        }
        dat->label[0]=0;
        getItemTag( GlobalItemInfo, id, "LABEL=", dat->label, sizeof(dat->label) );
        if(dat->label[0])
            dat->shape = SHP_EMPTY; // just label for auto-apply
        sprintf(dat->desc," %s %s", ShapeList[dat->shape], dat->label);

        dat->alreadyreadlist = NULL;

        GlobalComRing->ringAttach(ITEMSHAPE_COMRING,(LWInstance)dat,ringEvent);
    }
    else
        *err = errAllocFailed;
    return dat;
}

XCALL_(static const char *)ItemShapeDescribe(ItemShapeData *dat)
{
    XCALL_INIT;
    if(dat->label[0])
        sprintf(dat->desc," : %s \"%s\"", ShapeList[dat->shape], dat->label);
    else
        sprintf(dat->desc," : %s", ShapeList[dat->shape]);
    return (dat->desc);
}

XCALL_(static void)ItemShapeDestroy(ItemShapeData *dat)
{
    XCALL_INIT;


    if(dat)
    {
        GlobalComRing->ringDetach( ITEMSHAPE_COMRING, (LWInstance)dat);

        free(dat);
    }
}

XCALL_(static LWError)ItemShapeCopy(ItemShapeData   *to, ItemShapeData  *from)
{
    XCALL_INIT;

    *to = *from;
    return (NULL);
}

XCALL_(static LWError)ItemShapeLoad(ItemShapeData *dat,const LWLoadState    *lState)
{
    int v;
    unsigned int id32;
    float sc = 1.0f;
    XCALL_INIT;
    LWLOAD_I4(lState,&v,1);
    if(v>=2)
    {
        LWLOAD_FP(lState,&sc,1);
        dat->scale = sc;
    }
    LWLOAD_I4(lState,&dat->axis,1);
    LWLOAD_I4(lState,&dat->shape,1);
    LWLOAD_I4(lState,&dat->fill,1);
    LWLOAD_I4(lState,&dat->flags,1);
    LWLOAD_FP(lState,dat->clrA,3);
    LWLOAD_FP(lState,dat->clrB,3);
    if(v>=3)
        LWLOAD_FP(lState,dat->clrT,3);
    if(v>=4)
    {
        LWLOAD_FP(lState,&sc,1);
        dat->level = sc;
    }

    LWLOAD_U4(lState,&id32,1);
    dat->linkTo = UINT2LWITEMID(id32);
    LWLOAD_STR(lState, dat->label,sizeof(dat->label));

    if(v>=5)
        LWLOAD_I4(lState,&dat->justify,1);

    dat->ctxt = dat;

    dat->alreadyreadlist = NULL;

    GlobalComRing->ringAttach( ITEMSHAPE_COMRING, (LWInstance)dat,ringEvent);

    return (NULL);
}

XCALL_(static LWError)ItemShapeSave(ItemShapeData *dat,const LWSaveState *sState)
{
    int v = 5;
    unsigned int id32;
    float sc = 1.0f;
    XCALL_INIT;

    LWSAVE_I4(sState,&v,1);
    sc = (float)dat->scale;
    LWSAVE_FP(sState,&sc,1);
    LWSAVE_I4(sState,&dat->axis,1);
    LWSAVE_I4(sState,&dat->shape,1);
    LWSAVE_I4(sState,&dat->fill,1);
    LWSAVE_I4(sState,&dat->flags,1);
    LWSAVE_FP(sState,dat->clrA,3);
    LWSAVE_FP(sState,dat->clrB,3);
    LWSAVE_FP(sState,dat->clrT,3); // v3
    sc = (float)dat->level;
    LWSAVE_FP(sState,&sc,1); // v4
    id32 = LWITEMID2UINT(dat->linkTo);
    LWSAVE_U4(sState,&id32,1);
    LWSAVE_STR(sState,dat->label);
    LWSAVE_I4(sState,&dat->justify,1); // v5
    return (NULL);
}


/* ----------------- Plug-in Methods: LWRenderFuncs  -----------------  */

XCALL_(static LWError)ItemShapeInit (LWInstance inst, int i)
{
    XCALL_INIT;
    return (NULL);
}

XCALL_(static LWError)ItemShapeNewTime (LWInstance inst, LWFrame f, LWTime t)
{
    ItemShapeData *dat = (ItemShapeData *)inst;
    XCALL_INIT;
    dat->time = t;
    if(dat->linkTo)
        GlobalItemInfo->param(dat->linkTo, LWIP_W_POSITION, t, dat->linkPos);
    return NULL;
}

XCALL_(static void)ItemShapeCleanup (LWInstance inst)
{
    XCALL_INIT;
    return;
}

/* ----------------- Plug-in Methods: LWItemFuncs  -----------------  */

XCALL_(static const LWItemID *)ItemShapeUseItems (LWInstance inst)
{
    ItemShapeData *dat = (ItemShapeData *)inst;
    static LWItemID items[] = {NULL, NULL};
    XCALL_INIT;
    items[0] = dat->linkTo;
    return items;
}

XCALL_(static void)ItemShapeChangeID (LWInstance inst, const LWItemID *items)
{
    ItemShapeData *dat = (ItemShapeData *)inst;
    int      i=0;
    XCALL_INIT;
    if(dat->linkTo)
        while(items[i])
            if(items[i]==dat->linkTo)
            {
                dat->linkTo = items[i+1];
                return;
            }
            else 
                i += 2;
    return;
}



/* ----------------- Plug-in Methods: LWCustomObjHandler  -----------------  */

XCALL_(static unsigned int)ItemShapeFlags (ItemShapeData *dat)
{
    XCALL_INIT;
    return 0;
}   
/*
// return true if sucessfully filled array of 3 envelopes for x,y,z
int     getMotionTriple(LWItemID id, LWEnvelopeID *env)
{
    LWChannelID     chan = NULL;
    LWChanGroupID   cgrp;
    env[0] = env[1] = env[2] = NULL;
    if(!id) 
        return 0;

    cgrp = GlobalItemInfo->chanGroup(id);
    if(chan = GlobalChannelInfo->nextChannel(cgrp, chan))
        env[0] = GlobalChannelInfo->channelEnvelope(chan);
    if(chan = GlobalChannelInfo->nextChannel(cgrp, chan))
        env[1] = GlobalChannelInfo->channelEnvelope(chan);
    if(chan = GlobalChannelInfo->nextChannel(cgrp, chan))
        env[2] = GlobalChannelInfo->channelEnvelope(chan);

    return (env[0] && env[2] && env[1]) ? 1:0;

}

static double dt = 1.0/30;
// need world xform matrix for each point!!!
void drawMotion(LWItemID id, const LWCustomObjAccess *cobjAcc)
{
    LWDVector       ptA, ptB;
    LWEnvelopeID    env[3] = {NULL, NULL, NULL};
    LWEnvKeyframeID keyA = NULL;
    double  t,ta, tmax;
    int     keys, step=0;

    if(!getMotionTriple(id,env)) 
        return;

    while(keyA = GlobalEnvelopeFuncs->nextKey(env[0], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[0], keyA,LWKEY_TIME,&ta);
        step++;
    }
    keys = step;
    step = 0;
    tmax = ta;
    keyA = NULL;
    while(keyA = GlobalEnvelopeFuncs->nextKey(env[1], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[1], keyA,LWKEY_TIME,&ta);
        step++;
    }
    if(step>keys)
        keys = step;
    if(ta>tmax)
        tmax = ta;
    keyA = NULL;
    step = 0;
    while(keyA = GlobalEnvelopeFuncs->nextKey(env[2], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[2], keyA,LWKEY_TIME,&ta);
        step++;
    }
    if(step>keys)
        keys = step;
    if(ta>tmax)
        tmax = ta;
    ptA[0] = GlobalEnvelopeFuncs->evaluate(env[0],0.0);
    ptA[1] = GlobalEnvelopeFuncs->evaluate(env[1],0.0);
    ptA[2] = GlobalEnvelopeFuncs->evaluate(env[2],0.0);
    for(t=dt; t<=tmax; t += dt)
    {
        ptB[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        ptB[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        ptB[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        cobjAcc->line(cobjAcc->dispData, ptA, ptB,LWCSYS_OBJECT);
        VCPY(ptA, ptB);
    }
    keyA = NULL;
    cobjAcc->setColor(cobjAcc->dispData, Red);
    while(keyA = GlobalEnvelopeFuncs->nextKey(env[0], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[0], keyA,LWKEY_TIME,&t);
        ptB[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        ptB[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        ptB[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        co_Rectangle(cobjAcc, ptB, 0.08,0.08, LWCSYS_ICON, 0);
        co_Rectangle(cobjAcc, ptB, 0.08,0.08, LWCSYS_ICON, 1);
        co_Rectangle(cobjAcc, ptB, 0.08,0.08, LWCSYS_ICON, 2);
    }
    keyA = NULL;
    cobjAcc->setColor(cobjAcc->dispData, Green);
    while(keyA = GlobalEnvelopeFuncs->nextKey(env[1], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[1], keyA,LWKEY_TIME,&t);
        ptB[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        ptB[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        ptB[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        co_Rectangle(cobjAcc, ptB, 0.10,0.1, LWCSYS_ICON, 0);
        co_Rectangle(cobjAcc, ptB, 0.10,0.1, LWCSYS_ICON, 1);
        co_Rectangle(cobjAcc, ptB, 0.10,0.1, LWCSYS_ICON, 2);
    }
    keyA = NULL;
    cobjAcc->setColor(cobjAcc->dispData, Blue);
    while(keyA = GlobalEnvelopeFuncs->nextKey(env[2], keyA))
    {
        GlobalEnvelopeFuncs->keyGet(env[2], keyA,LWKEY_TIME,&t);
        ptB[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        ptB[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        ptB[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        co_Rectangle(cobjAcc, ptB, 0.12,0.12, LWCSYS_ICON, 0);
        co_Rectangle(cobjAcc, ptB, 0.12,0.12, LWCSYS_ICON, 1);
        co_Rectangle(cobjAcc, ptB, 0.12,0.12, LWCSYS_ICON, 2);
    }
}

// count keys: nx: keys in x channel, ny: keys in y but not x, nz: keys in z but not x or y
int motionCountHandles(LWItemID id, int *nx, int *ny, int *nz)
{
    LWEnvelopeID    env[3] = {NULL, NULL, NULL};
    LWEnvKeyframeID key = NULL;
    double  t;

    if(!getMotionTriple(id,env)) 
        return 0;

    *nx = 0;
    *ny = 0;
    *nz = 0;

    while(key = GlobalEnvelopeFuncs->nextKey(env[0], key))
        *nx++;

    key = NULL;
    while(key = GlobalEnvelopeFuncs->nextKey(env[1], key))
    {
        GlobalEnvelopeFuncs->keyGet(env[1], key,LWKEY_TIME,&t);
        if(!GlobalEnvelopeFuncs->findKey(env[0],t)) // no X key!
            *ny++;
    }
    key = NULL;
    while(key = GlobalEnvelopeFuncs->nextKey(env[2], key))
    {
        GlobalEnvelopeFuncs->keyGet(env[2], key,LWKEY_TIME,&t);
        if(!GlobalEnvelopeFuncs->findKey(env[0],t)) // no X key!
            if(!GlobalEnvelopeFuncs->findKey(env[1],t)) // no Y key!
                *nz++;
    }
    return *nx + *ny + *nz;
}

int motionHandle(LWItemID id, int nx, int ny, int nz, LWDVector pos, int i)
{
    LWEnvelopeID    env[3] = {NULL, NULL, NULL};
    LWEnvKeyframeID key = NULL;
    double  t;
    int     k = 0;

    VCLR(pos);
    if(!getMotionTriple(id,env)) 
        return -1;

    if(i<nx)
    {
        while( (key=GlobalEnvelopeFuncs->nextKey(env[0], key)) && k<i)
            k++;
        GlobalEnvelopeFuncs->keyGet(env[0], key,LWKEY_TIME,&t);
        pos[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        pos[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        pos[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        return 1;
    }
    i -= nx;
    k = 0;
    if(i<ny)
    {
        key = NULL;
        while( (key=GlobalEnvelopeFuncs->nextKey(env[1], key)) && k<i )
        {
            GlobalEnvelopeFuncs->keyGet(env[1], key,LWKEY_TIME,&t);
            if(!GlobalEnvelopeFuncs->findKey(env[0],t)) // no X key!
                k++;
        }
        pos[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        pos[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        pos[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        return 2;
    }
    i -= ny;
    k = 0;
    if(i<nz)
    {
        key = NULL;
        while( (key=GlobalEnvelopeFuncs->nextKey(env[2], key)) && k<i )
        {
            GlobalEnvelopeFuncs->keyGet(env[2], key,LWKEY_TIME,&t);
            if(!GlobalEnvelopeFuncs->findKey(env[0],t)) // no X key!
                if(!GlobalEnvelopeFuncs->findKey(env[1],t)) // no Y key!
                    k++;
        }
        pos[0] = GlobalEnvelopeFuncs->evaluate(env[0],t);
        pos[1] = GlobalEnvelopeFuncs->evaluate(env[1],t);
        pos[2] = GlobalEnvelopeFuncs->evaluate(env[2],t);
        return 3;
    }
    return -1;
}
*/
#define root3over2  0.866025403784438646763723170752936

XCALL_(static void)ItemShapeEval (ItemShapeData *dat,   const LWCustomObjAccess *cobjAcc)
{
    double  orig[3] = {0.0,0.0,0.0}, vec[3], pt[3];
    int     i,ix,iy;
    float   rgba[4];

    XCALL_INIT; 
    ix = xax[dat->axis];
    iy = yax[dat->axis];
//  (*GlobalItemInfo->param)(dat->self, LWIP_SCALING, dat->time, scl);
    if(cobjAcc->flags&LWCOFL_SELECTED)
    {
        if(dat->flags&ITSHPF_COLOR)
        {
            VCPY(rgba,dat->clrA);
            rgba[3] = (float)dat->level;
            (*cobjAcc->setColor)(cobjAcc->dispData, rgba);
        }
    }
    else
        if(dat->flags&ITSHPF_BCOLOR)
        {
            VCPY(rgba,dat->clrB);
            rgba[3] = (float)dat->level;
            (*cobjAcc->setColor)(cobjAcc->dispData, rgba);
        }


/*  if(dat->linkTo)
    {
        drawMotion(dat->linkTo, cobjAcc);
        return;
    }
*/
    switch(dat->shape)
    {
        case SHP_BOX: 
            for(i=0; i<3; i++)
            {
                VCLR(vec);
                vec[i] -= 0.5*dat->scale;
                if(dat->fill)
                {
                    if(i==dat->axis)
                        co_Rectangle(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, i); // bottom open!
                    else
                        co_FillRect(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, i);
                    vec[i] += 1.0*dat->scale;
                    co_FillRect(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, i);
                    orig[dat->axis] = 0.505*dat->scale;
                }
                else
                {
                    co_Rectangle(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, i);
                    vec[i] += 1.0*dat->scale;
                    co_Rectangle(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, i);
                }
            }
            break;
        case SHP_BALL: 
            VCLR(vec);
            for(i=0; i<3; i++)
                co_Arc(cobjAcc, orig, 0.0,TWOPI, 0.5*dat->scale, LWCSYS_OBJECT, i);
            break;
        case SHP_PYRAMID:
            VCLR(vec);
            vec[dat->axis] = -0.50*root3over2*dat->scale;
            orig[dat->axis] = -vec[dat->axis];
            if(dat->fill)
            {
                co_FillRect(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, dat->axis);
                vec[ix] += 0.50*dat->scale;
                vec[iy] += 0.50*dat->scale;
                VCPY(pt,vec);
                pt[iy] -= 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, vec,orig,pt,LWCSYS_OBJECT);

                vec[ix] -= 1.0*dat->scale;
                vec[iy] -= 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, vec,orig,pt,LWCSYS_OBJECT);

                pt[ix] -= 1.0*dat->scale;
                pt[iy] += 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, vec,orig,pt,LWCSYS_OBJECT);

                vec[ix] += 1.0*dat->scale;
                vec[iy] += 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, vec,orig,pt,LWCSYS_OBJECT);

            }
            else
            {
                co_Rectangle(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, dat->axis);
                vec[ix] += 0.50*dat->scale;
                vec[iy] += 0.50*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, vec,orig,LWCSYS_OBJECT);
                vec[iy] -= 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, orig,vec,LWCSYS_OBJECT);
                vec[ix] -= 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, vec,orig,LWCSYS_OBJECT);
                vec[iy] += 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, orig,vec,LWCSYS_OBJECT);
            }
            break;
        case SHP_DIAMOND: 
            VCLR(vec);
            VCLR(pt);
            orig[dat->axis] -= 0.7072*dat->scale;//root3over2;
            pt[dat->axis] += 0.7072*dat->scale;//root3over2;
            vec[ix] += 0.5*dat->scale;
            vec[iy] += 0.5*dat->scale;
            if(dat->fill)
            {
                double v[3];
                VCPY(v,vec);
                v[iy] -= 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, pt,vec,v,LWCSYS_OBJECT);
                (*cobjAcc->triangle)(cobjAcc->dispData, orig,vec,v,LWCSYS_OBJECT);
                vec[iy] -= 1.0*dat->scale;
                vec[ix] -= 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, pt,vec,v,LWCSYS_OBJECT);
                (*cobjAcc->triangle)(cobjAcc->dispData, orig,vec,v,LWCSYS_OBJECT);
                v[iy] += 1.0*dat->scale;
                v[ix] -= 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, pt,vec,v,LWCSYS_OBJECT);
                (*cobjAcc->triangle)(cobjAcc->dispData, orig,vec,v,LWCSYS_OBJECT);
                vec[iy] += 1.0*dat->scale;
                vec[ix] += 1.0*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, pt,vec,v,LWCSYS_OBJECT);
                (*cobjAcc->triangle)(cobjAcc->dispData, orig,vec,v,LWCSYS_OBJECT);
            }
            else
            {
                (*cobjAcc->line)(cobjAcc->dispData, pt,vec,LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, vec,orig,LWCSYS_OBJECT);
                vec[iy] -= 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, orig,vec,LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_OBJECT);
                vec[ix] -= 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, pt,vec,LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, vec,orig,LWCSYS_OBJECT);
                vec[iy] += 1.0*dat->scale;
                (*cobjAcc->line)(cobjAcc->dispData, orig,vec,LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_OBJECT);
                VCLR(vec);
                co_Rectangle(cobjAcc, vec, 1.0*dat->scale, 1.0*dat->scale, LWCSYS_OBJECT, dat->axis);
            }
            orig[dat->axis] = 0.7072*dat->scale;//root3over2;
            break;
        case SHP_TETRA: 
            VCLR(vec);
            VCLR(pt);
            vec[dat->axis] = -0.25*dat->scale;
            orig[dat->axis] = 0.5*dat->scale;
            pt[dat->axis] = vec[dat->axis];
            vec[iy] = 0.5*dat->scale;
            (*cobjAcc->line)(cobjAcc->dispData, vec,orig,LWCSYS_OBJECT);
            pt[iy] = -0.25*dat->scale;
            pt[ix] = -0.5*root3over2*dat->scale;
            if(dat->fill)
                (*cobjAcc->triangle)(cobjAcc->dispData, orig, pt,vec, LWCSYS_OBJECT);
            else
            {
                (*cobjAcc->line)(cobjAcc->dispData, orig, pt, LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, pt,vec,LWCSYS_OBJECT);
            }
            pt[ix] = 0.5*root3over2*dat->scale;
            if(dat->fill)
                (*cobjAcc->triangle)(cobjAcc->dispData, vec, pt, orig, LWCSYS_OBJECT);
            else
            {
                (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_OBJECT);
                (*cobjAcc->line)(cobjAcc->dispData, pt,orig,LWCSYS_OBJECT);
            }
            VCPY(vec,pt);
            pt[ix] = -0.5*root3over2*dat->scale;
            if(dat->fill)
            {
                (*cobjAcc->triangle)(cobjAcc->dispData, vec, pt, orig, LWCSYS_OBJECT);
                VCPY(orig,pt);
                orig[ix] = 0.5*root3over2*dat->scale;
                (*cobjAcc->triangle)(cobjAcc->dispData, vec, pt, orig, LWCSYS_OBJECT); // bottom (optional!)
                VCLR(orig);
                orig[dat->axis] = 0.5*dat->scale;
            }
            else
                (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_OBJECT);
            break;
        case SHP_RING: 
        //  VSET(vec, 1.0);
            if(dat->fill)
            {
                co_FillArc(cobjAcc, orig, 0.0, TWOPI, 1.0*dat->scale, LWCSYS_OBJECT, dat->axis);
                orig[iy] += 1.0*dat->scale;
            }
            else
                co_Arc(cobjAcc, orig, 0.0, TWOPI, 1.0*dat->scale, LWCSYS_OBJECT, dat->axis);
            break;
        case SHP_GRID: 
            VSET(pt, -0.5 * dat->scale);
            pt[dat->axis] = 0.0;
            VSET(vec, dat->scale);

            co_Grid(cobjAcc, pt, vec, 10, LWCSYS_OBJECT, dat->axis);
        //  if(*dat->label)
            {
                char    pos[]="+ X", neg[]="- X";
                pt[ix] = 0.0;
                pt[dat->axis] = 0.005;
                neg[2] = AxisName[iy];
                pos[2] = AxisName[iy];
                (*cobjAcc->text)(cobjAcc->dispData, pt,neg, 1, LWCSYS_OBJECT);
                pt[iy] = 0.5 * dat->scale;
                (*cobjAcc->text)(cobjAcc->dispData, pt,pos, 1, LWCSYS_OBJECT);
                neg[2] = AxisName[ix];
                pos[2] = AxisName[ix];
                pt[iy] = 0.0;
                pt[ix] = 0.5 * dat->scale;
                (*cobjAcc->text)(cobjAcc->dispData, pt,pos, 1, LWCSYS_OBJECT);
                pt[ix] = -0.5 * dat->scale;
                (*cobjAcc->text)(cobjAcc->dispData, pt,neg, 1, LWCSYS_OBJECT);
            }
            break;
        case SHP_STANDARD:
            VCLR(vec);
            VCLR(pt);
            vec[0] = 0.15;
            pt[0] = -0.15;
            (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_ICON);
            vec[0] = pt[0] = 0.0;
            vec[1] = 0.15;
            pt[1] = -0.15;
            (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_ICON);
            vec[1] = pt[1] = 0.0;
            vec[2] = 0.15;
            pt[2] = -0.15;
            (*cobjAcc->line)(cobjAcc->dispData, vec,pt,LWCSYS_ICON);
        case SHP_EMPTY:
        default:
            break;
    }
    if(dat->linkTo)
    {
        GlobalItemInfo->param(dat->self, LWIP_W_POSITION, dat->time, pt);
        GlobalItemInfo->param(dat->linkTo, LWIP_W_POSITION, dat->time, dat->linkPos);
        cobjAcc->setPattern(cobjAcc->dispData, LWLPAT_DASH);
        cobjAcc->line(cobjAcc->dispData, dat->linkPos, pt, LWCSYS_WORLD);
    }
    // above switch leaves orig placed for label!!!
    if(*dat->label)
    {
        if(dat->flags&ITSHPF_TCOLOR)
        {
            VCPY(rgba,dat->clrT);
            rgba[3] = (float)dat->level;
            (*cobjAcc->setColor)(cobjAcc->dispData, rgba);
        }
        (*cobjAcc->text)(cobjAcc->dispData, orig, dat->label, dat->justify, LWCSYS_OBJECT);
    }

}

/* -----------------                 -----------------  */


XCALL_(static int) ItemShape (
    int              version,
    GlobalFunc          *global,
    LWCustomObjHandler  *local,
    void                *serverData)
{
    XCALL_INIT;
    if (version != LWCUSTOMOBJ_VERSION)
        return (AFUNC_BADVERSION);

    GlobalComRing = (*global) (LWCOMRING_GLOBAL,GFUSE_TRANSIENT);
    if (!GlobalComRing )
        return AFUNC_BADGLOBAL;

    Gmessage = (*global) (LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT);
    if (!Gmessage )
        return AFUNC_BADGLOBAL;

    GlobalItemInfo = (*global) (LWITEMINFO_GLOBAL, GFUSE_TRANSIENT);
    if (!GlobalItemInfo )
        return AFUNC_BADGLOBAL;

    GlobalEnvelopeFuncs = (*global) (LWENVELOPEFUNCS_GLOBAL, GFUSE_TRANSIENT);
    if (!GlobalEnvelopeFuncs )
        return AFUNC_BADGLOBAL;
    GlobalChannelInfo = (*global) (LWCHANNELINFO_GLOBAL, GFUSE_TRANSIENT);
    if (!GlobalChannelInfo )
        return AFUNC_BADGLOBAL;

    GlobalLWUpdate = (*global) (LWINSTUPDATE_GLOBAL, GFUSE_TRANSIENT);
    if(!GlobalLWUpdate)
    {
        (*Gmessage->error)("Can't get global",LWINSTUPDATE_GLOBAL);
        return AFUNC_BADGLOBAL;
    }

    local->inst->create  = ItemShapeCreate;
    local->inst->destroy = ItemShapeDestroy;
    local->inst->load    = ItemShapeLoad;
    local->inst->save    = ItemShapeSave;
    local->inst->copy    = ItemShapeCopy;
    local->inst->descln  = ItemShapeDescribe;

    local->item->useItems    = ItemShapeUseItems;
    local->item->changeID    = ItemShapeChangeID;

    local->rend->init       = ItemShapeInit;
    local->rend->newTime    = ItemShapeNewTime;
    local->rend->cleanup    = ItemShapeCleanup;

    local->evaluate     = ItemShapeEval;
    local->flags        = ItemShapeFlags;
    return (AFUNC_OK);
}

/* -----------------  User Interface  ----------------- */


static void *ItemShapeData_get ( void *myinst, unsigned int vid ) 
{
  ItemShapeData *dat = (ItemShapeData*)myinst;
  void *result = NULL;
  static int val = 0;
  static double rgba[4];

  if ( dat ) 
      switch ( vid ) {
        case CH_AXIS:
            result = &dat->axis;
            break;
        case CH_SHAPE:
            result = &dat->shape;
            break;
        case CH_FILL:
            result = &dat->fill;
            break;
        case CH_COLR:
            VCPY(rgba,dat->clrA);
            result = &rgba;
            break;
        case CH_BCOLR:
            VCPY(rgba,dat->clrB);
            result = &rgba;
            break;
        case CH_TCOLR:
            VCPY(rgba,dat->clrT);
            result = &rgba;
            break;
        case CH_USEC:
            val = dat->flags&ITSHPF_COLOR ? 1 : 0;
            result = &val;
            break;
        case CH_USEBC:
            val = dat->flags&ITSHPF_BCOLOR ? 1 : 0;
            result = &val;
            break;
        case CH_USETC:
            val = dat->flags&ITSHPF_TCOLOR ? 1 : 0;
            result = &val;
            break;
        case CH_LABL:
            result = &dat->label;
            break;
        case CH_JUST:
            val = dat->justify;
            result = &val;
            break;
        case CH_USELB:
            val = (int)dat->label[0];
            result = &val;
            break;
        case CH_LEVL:
            result = &dat->level;
            break;
        case CH_SCAL:
            result = &dat->scale;
            break;
        case CH_TARG:
            val = popItemIdx(GlobalItemInfo, dat->linkTo);
            result = &val;
            break;
      } 
  return result;
}

static LWXPRefreshCode ItemShapeData_set ( void *myinst, unsigned int vid, void *value ) 
{
    ItemShapeData *dat = (ItemShapeData*)myinst;
    double rgba[4]={0.0};
    int tempi = 0;
    LWXPRefreshCode rc = LWXPRC_NONE;
    if ( dat ) 
        switch ( vid ) {
            case CH_AXIS:
                dat->axis = *((int*)value);
                rc = LWXPRC_DFLT;
                break;
            case CH_COLR:
                VCPY(rgba,((double*)value));
                VCPY_F(dat->clrA, rgba);
                rc = LWXPRC_DFLT;
                break;
            case CH_BCOLR:
                VCPY(rgba,((double*)value));
                VCPY_F(dat->clrB, rgba);
                rc = LWXPRC_DFLT;
                break;
            case CH_TCOLR:
                VCPY(rgba,((double*)value));
                VCPY_F(dat->clrT, rgba);
                rc = LWXPRC_DFLT;
                break;
            case CH_USEC:
                tempi = *((int*)value);
                if(tempi)
                    dat->flags |= ITSHPF_COLOR;
                else
                    dat->flags &= ~ITSHPF_COLOR;
                rc = LWXPRC_DFLT;
                break;
            case CH_USEBC:
                tempi = *((int*)value);
                if(tempi)
                    dat->flags |= ITSHPF_BCOLOR;
                else
                    dat->flags &= ~ITSHPF_BCOLOR;
                rc = LWXPRC_DFLT;
                break;
            case CH_USETC:
                tempi = *((int*)value);
                if(tempi)
                    dat->flags |= ITSHPF_TCOLOR;
                else
                    dat->flags &= ~ITSHPF_TCOLOR;
                rc = LWXPRC_DFLT;
                break;
            case CH_FILL:
                dat->fill = *((int*)value);
                rc = LWXPRC_DFLT;
                break;
            case CH_SHAPE:
                dat->shape = *((int*)value);
                rc = LWXPRC_DFLT;
                break;
            case CH_LABL:
                strncpy(dat->label, ((char*)value), sizeof(dat->label)-1 );
                setItemTag(GlobalItemInfo, dat->self, "LABEL",dat->label);
                rc = LWXPRC_DFLT;
                break;
            case CH_JUST:
                tempi = *((int*)value);
                dat->justify = LWCUST_TEXT_C;
                if(tempi==0) // left
                    dat->justify = LWCUST_TEXT_L;
                else if(tempi==2) // right
                    dat->justify = LWCUST_TEXT_R;
                rc = LWXPRC_DFLT;
                break;
            case CH_USELB:
                rc = LWXPRC_DFLT;
                break;
            case CH_LEVL:
                dat->level = *((double*)value);
                rc = LWXPRC_DFLT;
                break;
            case CH_SCAL:
                dat->scale = *((double*)value);
                rc = LWXPRC_DFLT;
                break;
            case CH_TARG:
                tempi = *((int*)value);
                dat->linkTo = popIdxItem(GlobalItemInfo, tempi);
                rc = LWXPRC_DFLT;
                break;
            case CH_ALLBYCOMRINGUPDATE:
                rc = LWXPRC_DFLT;
                break;
        } 
    if(rc != LWXPRC_NONE)
        (*GlobalLWUpdate)(LWCUSTOMOBJ_HCLASS, dat);
    return rc;
}

//static int levEnable[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
static int fillEnable[] = {0,1,0,1,1,1,1,0,0,0,0,0,0};
static LWXPanelID ItemShapeXPanel(GlobalFunc *global, ItemShapeData *dat)
{
    LWXPanelFuncs *lwxpf = NULL;
    LWXPanelID     panID = NULL;
    static LWXPanelHint hint[] = {
        XpDELETE     (CH_USELB),
        XpLABEL(0,"ItemShape Object"),
        XpSTRLIST(CH_SHAPE,ShapeList),
        XpSTRLIST(CH_JUST,JustList),
        XpPOPFUNCS(CH_TARG,popCnt_Item, popName_Item),
        XpMIN(CH_LEVL, 0),
        XpMAX(CH_LEVL, 1),
        XpGROUP_(CH_GRP1),
            XpH(CH_SHAPE),
            XpH(CH_AXIS),
            XpH(CH_SCAL),
            XpH(CH_FILL),
            XpEND,
        XpGROUP_(CH_GRP2),
            XpH(CH_LABL),
            XpH(CH_JUST),
            XpH(CH_TARG),
            XpEND,
        XpGROUP_(CH_DISPGR),
            XpH(CH_USEC),
            XpH(CH_COLR),
            XpH(CH_USEBC),
            XpH(CH_BCOLR),
            XpH(CH_USETC),
            XpH(CH_TCOLR),
            XpH(CH_LEVL),
            XpEND,
        XpENABLE_(CH_USEC),
            XpH(CH_COLR),
            XpEND,
        XpENABLE_(CH_USEBC),
            XpH(CH_BCOLR),
            XpEND,
        XpENABLE_(CH_USETC),
            XpH(CH_TCOLR),
            XpEND,
        XpENABLE_MAP_(CH_SHAPE,fillEnable),
            XpH(CH_FILL),
            XpEND,
        XpENABLEMSG_(CH_USELB,"This control is disabled because there is no label"),
            XpH(CH_USETC),
            XpH(CH_TCOLR),
            XpH(CH_JUST),
            XpEND,
        XpNARROW(CH_USEC),
        XpNARROW(CH_COLR),
        XpLEFT(CH_USEC),
        XpNARROW(CH_USEBC),
        XpNARROW(CH_BCOLR),
        XpLEFT(CH_USEBC),
        XpNARROW(CH_USETC),
        XpNARROW(CH_TCOLR),
        XpLEFT(CH_USETC),
        XpNARROW(CH_FILL),
        XpNARROW(CH_LEVL),
//      XpLEFT(CH_FILL),
        XpEND
    };

    lwxpf = (LWXPanelFuncs*)(*global)( LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT);
    if ( lwxpf ) 
    {
        panID = (*lwxpf->create)( LWXP_VIEW, ItemShape_ctrl_list );
        if(panID) 
        {
        //  (*lwxpf->hint) ( panID, 0, common_hint );
            (*lwxpf->hint) ( panID, 0, hint );
            (*lwxpf->describe)( panID, ItemShape_data_descrip, ItemShapeData_get, ItemShapeData_set );
            (*lwxpf->viewInst)( panID, dat );
            (*lwxpf->setData)(panID, 0, dat);
            (*lwxpf->setData)(panID, CH_TARG, GlobalItemInfo);
        }
    }
    return panID;
}

XCALL_(static int) ItemShape_UI (
    int         version,
    GlobalFunc      *global,
    LWInterface     *UI,
    void            *serverData)
{
    XCALL_INIT;
    if (version != LWINTERFACE_VERSION)
        return (AFUNC_BADVERSION);
    UI->panel   = ItemShapeXPanel(global, UI->inst);
    UI->options = NULL;
    UI->command = NULL; 
    return AFUNC_OK;
}



static ServerTagInfo 
Shape_tags[] = { 
    {"Item Shape",SRVTAG_USERNAME|LANGID_USENGLISH}, 
    {NULL,0} },
Slide_tags[] = { 
    {"Sliders",SRVTAG_USERNAME|LANGID_USENGLISH}, 
    {"Tool_Sliders", SRVTAG_SELECTCMD},
    {NULL,0} },
pro_tags[] = { 
    {"Protractor",SRVTAG_USERNAME|LANGID_USENGLISH}, 
    {NULL,0} };
/*
static ServerTagInfo 
Lat_tags[] = { 
    {"Lattice",SRVTAG_USERNAME|LANGID_USENGLISH}, 
    {NULL,0} };
*/

#include <lwdisplce.h>
#include <lwmaster.h>
#include <lwlaytool.h>
XCALL_(extern int) Lattice_UI (int version, GlobalFunc *global, LWInterface *UI, void *serverData);
XCALL_(extern int) Lattice (int version, GlobalFunc *global, LWCustomObjHandler *local, void *serverData);
XCALL_(extern int) LatticeDisp (int version, GlobalFunc *global, LWCustomObjHandler *local, void *serverData);
XCALL_(extern int) Protract_UI (int version, GlobalFunc *global, LWInterface *UI, void *serverData);
XCALL_(extern int) Protract (int version, GlobalFunc *global, LWCustomObjHandler *local, void *serverData);
XCALL_(extern int) ProxyPick (int version, GlobalFunc *global, LWMasterHandler *local, void *serverData);
XCALL_(extern int) ProxyPick_UI (int version, GlobalFunc *global, LWInterface *UI, void *serverData);
XCALL_(extern int) Sliders_UI (int version, GlobalFunc *global, LWInterface *UI, void *serverData);
XCALL_(extern int) Sliders (int version, GlobalFunc *global, LWCustomObjHandler *local, void *serverData);
XCALL_(extern int) Sliders_Tool (int version, GlobalFunc *global, LWLayoutTool *local, void *serverData);

ServerRecord ServerDesc[] = {
    { LWCUSTOMOBJ_HCLASS,       LW_ITEMSHAPE_NAME,          ItemShape, Shape_tags },
    { LWCUSTOMOBJ_ICLASS,       LW_ITEMSHAPE_NAME,      ItemShape_UI, Shape_tags },
    { LWCUSTOMOBJ_HCLASS,       "LW_Protract",          Protract, pro_tags },
    { LWCUSTOMOBJ_ICLASS,       "LW_Protract",      Protract_UI,  pro_tags},
    { LWCUSTOMOBJ_HCLASS,       "Sliders",          Sliders, Slide_tags },
    { LWCUSTOMOBJ_ICLASS,       "Sliders",      Sliders_UI},
    { LWMASTER_HCLASS,      "ProxyPick",            ProxyPick },
    { LWMASTER_ICLASS,      "ProxyPick",        ProxyPick_UI},
    { LWLAYOUTTOOL_CLASS,       "Sliders",      Sliders_Tool},
#ifdef LATTICE
    { LWCUSTOMOBJ_HCLASS,       "LW_Lattice",           Lattice, Lat_tags },
    { LWCUSTOMOBJ_ICLASS,       "LW_Lattice",       Lattice_UI, Lat_tags },
    { LWDISPLACEMENT_HCLASS,        "LW_Lattice",           LatticeDisp, Lat_tags },
    { LWDISPLACEMENT_ICLASS,        "LW_Lattice",       Lattice_UI, Lat_tags },
#endif
    { NULL }
};

