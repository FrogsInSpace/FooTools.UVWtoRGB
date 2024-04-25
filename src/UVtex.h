/**********************************************************************
 *<
	FILE: UVtexture.h

	DESCRIPTION:	Template Utility

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __UVTEXTURE__H
#define __UVTEXTURE__H

#include "Max.h"

#if (MAX_RELEASE >= 9000)
#include "maxheapdirect.h"	//max 9
#else
#include "max_mem.h"		//max 8 and earlier
#endif

#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"

#include "stdmat.h"
#include "imtl.h"
#include "macrorec.h"

extern TCHAR *GetString(int id);
extern ClassDesc2* GetUVtexDesc();

extern HINSTANCE hInstance;

#endif // __UVTEXTURE__H
