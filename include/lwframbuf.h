/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWFRAMBUF.H -- LightWave Framebuffers
 */
#ifndef LWSDK_FRAMBUF_H
#define LWSDK_FRAMBUF_H

#include <lwrender.h>

#define LWFRAMEBUFFER_HCLASS	"FrameBufferHandler"
#define LWFRAMEBUFFER_ICLASS	"FrameBufferInterface"
#define LWFRAMEBUFFER_GCLASS	"FrameBufferGizmo"
#define LWFRAMEBUFFER_VERSION	5

typedef struct st_LWFrameBufferHandler {
	LWInstanceFuncs	 *inst;
	LWItemFuncs	 *item;
	int		  type;
	LWError         (*open) (LWInstance, int, int );
	void            (*close) (LWInstance);
	LWError         (*begin) (LWInstance);
	LWError         (*write) (LWInstance, const void *, const void *, const void *, const void *);
	void            (*pause) (LWInstance);
	LWPixmapID      (*getPixelMap) (LWInstance);
} LWFrameBufferHandler;

#define LWFBT_UBYTE	0
#define LWFBT_FLOAT	1

#endif
