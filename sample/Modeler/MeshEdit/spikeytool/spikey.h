/*
 * SPIKEY.H -- Spikey modeler plug-in header.
 *
 * Copyright 1999, NewTek, Inc.
 * written by Stuart Ferguson
 * last revision  8/30/99
 */
#include <lwmeshedt.h>
#include <lwpolygon.h>
#include <lwxpanel.h>
#include <lwdyna.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

extern DynaMonitorFuncs *globFun_mon;
extern LWXPanelFuncs *globFun_pan;

extern int      Spikey (MeshEditOp *, double);

