/**********************************************************************
 *<
	FILE: UVtex.cpp

	DESCRIPTION: A texture map that shows UVW texture coordinates

	CREATED BY: John Burnett

	HISTORY: 2.7.2000 - 1.0 - Created
			2.9.2000 - 1.1 - Change brightness/contrast to Tint Color and Amount
			3.15.2000 - 1.11 - Fixed cap on checker count

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include "UVtex.h"
#include "resource.h"
#include "stdmat.h"
#include "iparamm2.h"
#include "texutil.h"
#include "AboutRollup.h"

extern HINSTANCE hInstance;

#define UVTEX_CLASSID Class_ID(0x57583eeb, 0x6d523569)




enum { uvtex_params };  // pblock ID

enum {
	uv_uvchannel,
	uv_rtype, uv_gtype, uv_btype,
	uv_rcount, uv_gcount, uv_bcount,
	uv_brightness, uv_contrast,
	uv_tintAmount, uv_tintColor,
	uv_rAmount, uv_gAmount, uv_bAmount,
	uv_clampUVW,
};

enum {
	CHANTYPE_COLOR,
	CHANTYPE_CHECK,
};

class UVtex : public Texmap {
	IParamBlock2* pblock;		// ref 0

	TexHandle *texHandle;
	Interval texHandleValid;

	int		uvChannel;
	int		rType, gType, bType;
	int		rCount, gCount, bCount;
	float	rAmount, gAmount, bAmount;
	float	tintAmount;
	Color	tintColor;
	BOOL	clampUVW;

	Interval forever = FOREVER;
	// Caches
	Interval ivalid;
	CRITICAL_SECTION csect;

	public:
		UVtex();
		~UVtex();

//		static ParamDlg* xyzGenDlg;
		ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
		void Update(TimeValue t, Interval& valid);
		void Init();
		void Reset();
		Interval Validity(TimeValue t) {Update(t,forever); return ivalid;}

		// Evaluation
		ULONG LocalRequirements(int subMtlNum);
		void LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq);
		Color EvalUVtex(Point3 uvw);
		AColor EvalColor(ShadeContext& sc);
		float EvalMono(ShadeContext& sc);
		Point3 EvalNormalPerturb(ShadeContext& sc);

		// Viewport display
		void DiscardTexHandle();
		BOOL SupportTexDisplay() { return TRUE; }
		void ActivateTexDisplay(BOOL onoff);
#if (MAX_RELEASE >= 9000)
		DWORD_PTR GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker);
#else
		DWORD GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker);
#endif
		void GetUVTransform(Matrix3 &uvtrans) { uvtrans.IdentityMatrix(); }
		int GetTextureTiling() { return (U_WRAP|V_WRAP); }
		int GetUVWSource() { return ((uvChannel>0)?UVWSRC_EXPLICIT:UVWSRC_EXPLICIT2); }
		int GetMapChannel();
		Bitmap* BuildBitmap(int size);

		Class_ID ClassID() {return UVTEX_CLASSID;}
		SClass_ID SuperClassID() {return TEXMAP_CLASS_ID;}

		void DeleteThis() {delete this;}

		int NumSubs() {return 1;}
		Animatable* SubAnim(int i) { return pblock; }

#if MAX_RELEASE_R24
		void GetClassName(TSTR& s, BOOL isLocalized) { s = GetString(IDS_UVTEX_CLASSNAME); }
		TSTR SubAnimName(int subNum, BOOL isLocalized) { return TSTR(GetString(IDS_UVTEX_PARAMS)); }
#else
		void GetClassName(TSTR& s) { s = GetString(IDS_UVTEX_CLASSNAME); }
		TSTR SubAnimName(int subNum) { return TSTR(GetString(IDS_UVTEX_PARAMS)); }
#endif
		int SubNumToRefNum(int subNum) { return subNum; }

 		int NumRefs() {return 1;}
		RefTargetHandle GetReference(int i) { return pblock; }
		void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }

#if (MAX_RELEASE >= 9000) //max 9
		RefTargetHandle Clone(RemapDir &remap /*= DefaultRemapDir() */);
#else //max 8 and earlier
		RefTargetHandle Clone(RemapDir &remap = NoRemap());
#endif

#if MAX_VERSION_MAJOR < 17 //Max 2015
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message);
#else
		RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);
#endif

		int	NumParamBlocks() { return 1; }
		IParamBlock2* GetParamBlock(int i) { return pblock; }
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; }
};

class UVtexClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading) {return new UVtex;}
	const TCHAR *	ClassName() { return GetString(IDS_UVTEX_CLASSNAME); }
#if MAX_VERSION_MAJOR >= 24
	const TCHAR* NonLocalizedClassName() { return ClassName(); }
#endif

	SClass_ID		SuperClassID() {return TEXMAP_CLASS_ID;}
	Class_ID 		ClassID() {return UVTEX_CLASSID;}
	const TCHAR* 	Category() {return TEXMAP_CAT_3D;}
	const TCHAR*	InternalName() { return _T("uvTexture"); }
	HINSTANCE		HInstance() { return hInstance; }
};

static UVtexClassDesc uvTexCD;
ClassDesc2* GetUVtexDesc() {return &uvTexCD;}

class UVtexDlgProc:public ParamMap2UserDlgProc
{
public:
	UVtex *tex;
	UVtexDlgProc(UVtex *t) { tex = t; }
#if (MAX_RELEASE >= 9000)
	INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
#else
	BOOL DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
#endif
	void DeleteThis() { delete this; }
	void SetThing(ReferenceTarget *r) { tex = (UVtex*)r; }

	void SetChannelLabel(HWND hLabel, int chan);
};

void UVtexDlgProc::SetChannelLabel(HWND hLabel, int chan)
{
	switch(chan)
	{
		case 0: SetWindowText(hLabel, GetString(IDS_CHAN_0)); break;
		case -1: SetWindowText(hLabel, GetString(IDS_CHAN_1)); break;
		case -2: SetWindowText(hLabel, GetString(IDS_CHAN_2)); break;
		default: SetWindowText(hLabel, _T("")); break;
	}
}

#if (MAX_RELEASE >= 9000)
INT_PTR UVtexDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
#else
BOOL UVtexDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
#endif
{
	switch (msg)
	{
		case WM_INITDIALOG:
		case WM_PAINT:
			HWND hLabel = GetDlgItem(hWnd, IDC_CHANNEL_NAME);
			SetChannelLabel(hLabel, map->GetParamBlock()->GetInt(uv_uvchannel));
			break;
		}
	return FALSE;
}

#if MAX_VERSION_MAJOR >= 15
#define end p_end
#endif

static ParamBlockDesc2 uvtexpb ( uvtex_params, _T("parameters"),  0, &uvTexCD, P_AUTO_CONSTRUCT + P_AUTO_UI, 0,
	IDD_UVTEX, IDS_UVTEX_ROLLOUT, 0, 0, NULL,
	uv_uvchannel,	_T("uvwChannel"),	TYPE_INT,	0,	IDS_UVCHANNEL,
		p_default,		1,
#if MAX_RELEASE > 3100
		p_range,		-NUM_HIDDENMAPS, (MAX_MESHMAPS-1),
#else
		p_range,		0, (MAX_MESHMAPS-1),
#endif
		p_ui,			TYPE_SPINNER, EDITTYPE_INT, IDC_UVCHANNEL_EDIT, IDC_UVCHANNEL_SPIN, 1.0f,
		end,
	uv_rtype,	_T("redType"), 	TYPE_INT,	0,	IDS_R_TYPE,
		p_ui,			TYPE_RADIO, 2, IDC_RCHANNEL_UVW, IDC_RCHANNEL_CHECKER,
		p_default,		CHANTYPE_COLOR,
		p_range,		0,	1,
		end,
	uv_gtype,	_T("greenType"), 	TYPE_INT,	0,	IDS_G_TYPE,
		p_ui,			TYPE_RADIO, 2, IDC_GCHANNEL_UVW, IDC_GCHANNEL_CHECKER,
		p_default,		CHANTYPE_COLOR,
		p_range,		0,	1,
		end,
	uv_btype,	_T("blueType"), 	TYPE_INT,	0,	IDS_B_TYPE,
		p_ui,			TYPE_RADIO, 2, IDC_BCHANNEL_UVW, IDC_BCHANNEL_CHECKER,
		p_default,		CHANTYPE_CHECK,
		p_range,		0,	1,
		end,
	uv_rcount, _T("redCheckerCount"), TYPE_INT, P_ANIMATABLE, IDS_R_CHECKCOUNT,
		p_default,		10,
		p_range,		1, 1000,
		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_RCOUNT_EDIT, IDC_RCOUNT_SPIN, 1.0f,
		end,
	uv_gcount, _T("greenCheckerCount"), TYPE_INT, P_ANIMATABLE, IDS_G_CHECKCOUNT,
		p_default,		10,
		p_range,		1, 1000,
		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_GCOUNT_EDIT, IDC_GCOUNT_SPIN, 1.0f,
		end,
	uv_bcount, _T("blueCheckerCount"), TYPE_INT, P_ANIMATABLE, IDS_B_CHECKCOUNT,
		p_default,		10,
		p_range,		1, 1000,
		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_BCOUNT_EDIT, IDC_BCOUNT_SPIN, 1.0f,
		end,
	uv_tintAmount,	_T("tintAmount"), TYPE_FLOAT, P_ANIMATABLE, IDS_TINTAMOUNT,
		p_default,		0.0f,
		p_range,		0.0f, 1.0f,
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TINTAMOUNT_EDIT, IDC_TINTAMOUNT_SPIN, 0.01f,
		end,
	uv_tintColor,	_T("tintColor"), TYPE_RGBA, P_ANIMATABLE, IDS_TINTCOLOR,
		p_default,		Color(0.5f,0.5f,0.5f),
		p_ui,			TYPE_COLORSWATCH, IDC_TINTCOLOR,
		end,
	uv_rAmount,	_T("redAmount"), TYPE_FLOAT, P_ANIMATABLE, IDS_R_AMOUNT,
		p_default,		1.0f,
		p_range,		0.0f, 1.0f,
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAMOUNT_EDIT, IDC_RAMOUNT_SPIN, 0.01f,
		end,
	uv_gAmount,	_T("greenAmount"), TYPE_FLOAT, P_ANIMATABLE, IDS_G_AMOUNT,
		p_default,		1.0f,
		p_range,		0.0f, 1.0f,
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_GAMOUNT_EDIT, IDC_GAMOUNT_SPIN, 0.01f,
		end,
	uv_bAmount,	_T("blueAmount"), TYPE_FLOAT, P_ANIMATABLE, IDS_B_AMOUNT,
		p_default,		1.0f,
		p_range,		0.0f, 1.0f,
		p_ui,			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BAMOUNT_EDIT, IDC_BAMOUNT_SPIN, 0.01f,
		end,
	uv_clampUVW, _T("clampUVW"), TYPE_BOOL, 0, IDS_CLAMPUVW,
		p_default,		FALSE,
		p_ui,			TYPE_SINGLECHEKBOX,	IDC_CLAMPUVW,
		end,
	end
);

UVtex::UVtex() {
	pblock   = NULL;
	texHandle = NULL;
	uvTexCD.MakeAutoParamBlocks(this);	// make and intialize paramblock2
	Init();
	InitializeCriticalSection(&csect);
	ivalid.SetEmpty();
}

UVtex::~UVtex() {
	DiscardTexHandle();
	DeleteCriticalSection(&csect);
}

ParamDlg* UVtex::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) {
	IAutoMParamDlg* masterDlg = uvTexCD.CreateParamDlgs(hwMtlEdit, imp, this);
	uvtexpb.SetUserDlgProc(new UVtexDlgProc(this));
	imp->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_ABOUT), aboutDlgProc, _T("About"));
	return masterDlg;
}

int UVtex::GetMapChannel()
{
#if MAX_RELEASE > 3100
	if (uvChannel <= 0)
		return 1;
	else
		return uvChannel;
#else
	return uvChannel;
#endif
}

ULONG UVtex::LocalRequirements(int subMtlNum)
{
	if (uvChannel == 0) return (MTLREQ_UV2);
	if (uvChannel == 1) return (MTLREQ_UV);
	return 0;
}

void UVtex::LocalMappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq)
{
	if(uvChannel >= 0)
		mapreq.Set(uvChannel);
}

static inline float Clamp(float x, float a=0.0f, float b=1.0f) {
	return (x < a) ? a : ((x > b) ? b : x);
}

static inline int Clamp(int x, int a=0, int b=99) {
	return (x < a) ? a : ((x > b) ? b : x);
}

void UVtex::Update(TimeValue t, Interval& valid) {
	EnterCriticalSection(&csect);
	if (!ivalid.InInterval(t)) {
		ivalid = FOREVER;
		pblock->GetValue(uv_uvchannel,t,uvChannel,ivalid);
#if MAX_RELEASE > 3100
		uvChannel = Clamp(uvChannel, -NUM_HIDDENMAPS, (MAX_MESHMAPS-1));
#else
		uvChannel = Clamp(uvChannel);
#endif
		pblock->GetValue(uv_rtype,t,rType,ivalid);
		rType = Clamp(rType, 0, 1);
		pblock->GetValue(uv_gtype,t,gType,ivalid);
		gType = Clamp(gType, 0, 1);
		pblock->GetValue(uv_btype,t,bType,ivalid);
		bType = Clamp(bType, 0, 1);
		pblock->GetValue(uv_rcount,t,rCount,ivalid);
		rCount = Clamp(rCount, 0, 1000);
		pblock->GetValue(uv_gcount,t,gCount,ivalid);
		gCount = Clamp(gCount, 0, 1000);
		pblock->GetValue(uv_bcount,t,bCount,ivalid);
		bCount = Clamp(bCount, 0, 1000);
		pblock->GetValue(uv_tintColor,t,tintColor,ivalid);
		pblock->GetValue(uv_tintAmount,t,tintAmount,ivalid);
		tintAmount = Clamp(tintAmount);
		pblock->GetValue(uv_rAmount,t,rAmount,ivalid);
		rAmount = Clamp(rAmount);
		pblock->GetValue(uv_gAmount,t,gAmount,ivalid);
		gAmount = Clamp(gAmount);
		pblock->GetValue(uv_bAmount,t,bAmount,ivalid);
		bAmount = Clamp(bAmount);
		pblock->GetValue(uv_clampUVW,t,clampUVW,ivalid);
	}
	valid &= ivalid;
	LeaveCriticalSection(&csect);
}

//TODO: see what happens if pblock stuff isn't reset?  should be handled by CD.reset below
void UVtex::Init() {
	ivalid.SetEmpty();
}

void UVtex::Reset() {
	uvTexCD.Reset(this, TRUE);	// reset all pb2's
	Init();
}

static inline float EvalCheck (float u, float v, float count) {
	float umod = mod(u*(count/2.0f), 1.0f);
	float vmod = mod(v*(count/2.0f), 1.0f);
	return ( (umod>0.5f) ^ (vmod>0.5f) ) ? 1.0f : 0.0f;
}

static Color grey (0.5f, 0.5f, 0.5f);
static Color white (1,1,1);

Color UVtex::EvalUVtex(Point3 uvw) {
	Color c;
	c.r = (rType == CHANTYPE_COLOR) ? uvw.x : EvalCheck (uvw.y, uvw.z, (float)rCount);
	c.g = (gType == CHANTYPE_COLOR) ? uvw.y : EvalCheck (uvw.x, uvw.z, (float)gCount);
	c.b = (bType == CHANTYPE_COLOR) ? uvw.z : EvalCheck (uvw.x, uvw.y, (float)bCount);

	c.r *= rAmount;
	c.g *= gAmount;
	c.b *= bAmount;

	c = (c * (1.0f - tintAmount)) + (tintColor * tintAmount);

	return c;
}

AColor UVtex::EvalColor(ShadeContext& sc) {
	if (gbufID) sc.SetGBufferID(gbufID);

#if MAX_RELEASE > 3100
	Point3 uvw;
	if (uvChannel < 0)
	{
		if (sc.InMtlEditor())
		{
			Point2 a, b;
			sc.ScreenUV(a, b);
			uvw = Point3(a.x, a.y, 0.0f);
		} else if (sc.globContext != NULL && sc.NodeID() >= 0)
		{
			RenderInstance* ri = sc.globContext->GetRenderInstance(sc.NodeID());
			Mesh* m = ri->mesh;
			if (m->mapSupport(uvChannel))
			{
				Point3 bc = sc.BarycentricCoords();
				int i = sc.FaceNumber();

				UVVert* v = m->mapVerts(uvChannel);
				TVFace* f = m->mapFaces(uvChannel);

				uvw =	v[f[i].t[0]] * bc.x +
						v[f[i].t[1]] * bc.y +
						v[f[i].t[2]] * bc.z;
			} else {
				uvw = Point3(0.0,0.0,0.0);
			}
		} else {
			uvw = Point3(0.0,0.0,0.0);
		}
	} else {
		uvw = sc.UVW(uvChannel);
	}
#else
	Point3 uvw = sc.UVW(uvChannel);
#endif

	if (clampUVW) {
		uvw.x = Clamp(uvw.x);
		uvw.y = Clamp(uvw.y);
		uvw.z = Clamp(uvw.z);
	} else {
		uvw.x = mod(uvw.x, 1.0000001f);
		uvw.y = mod(uvw.y, 1.0000001f);
		uvw.z = mod(uvw.z, 1.0000001f);
	}

	return EvalUVtex(uvw);
}

float UVtex::EvalMono(ShadeContext& sc) {
	return Intens(EvalColor(sc));
}

Point3 UVtex::EvalNormalPerturb(ShadeContext& sc) {
	return Point3(0,0,0);
}

void UVtex::DiscardTexHandle() {
	if (texHandle) {
		texHandle->DeleteThis();
		texHandle = NULL;
	}
}

void UVtex::ActivateTexDisplay(BOOL onoff) {
	if (!onoff)
		DiscardTexHandle();
}

inline UWORD FlToWord(float r) {
	return (UWORD)(65535.0f*r);
}

Bitmap* UVtex::BuildBitmap(int size) {
	float u,v;
	BitmapInfo bi;
	bi.SetName(_T("uvTexTemp"));
	bi.SetWidth(size);
	bi.SetHeight(size);
	bi.SetType(BMM_TRUE_32);
	Bitmap *bm = TheManager->Create(&bi);
	if (bm==NULL) return NULL;
	PixelBuf l64(size);
	float d = 1.0f/float(size);
	v = 0.0f;
	for (int y=0; y<size; y++) {
		BMM_Color_64 *p64=l64.Ptr();
		u = 0.0f;
		for (int x=0; x<size; x++, p64++) {
			Color c = EvalUVtex( Point3(u,(1.0f-v),0.0f) );
			p64->r = FlToWord(c.r);
			p64->g = FlToWord(c.g);
			p64->b = FlToWord(c.b);
			p64->a = 0xffff;
			u += d;
		}
		bm->PutPixels(0,y, size, l64.Ptr());
		v += d;
	}
	return bm;
}

#if (MAX_RELEASE >= 9000)
DWORD_PTR UVtex::GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker) {
#else
DWORD UVtex::GetActiveTexHandle(TimeValue t, TexHandleMaker& thmaker) {
#endif
	if (texHandle) {
		if (texHandleValid.InInterval(t))
			return texHandle->GetHandle();
		else DiscardTexHandle();
	}

	Interval v;
	Update(t,v);

	Bitmap *bm;
	bm = BuildBitmap(thmaker.Size());
	texHandle = thmaker.CreateHandle(bm);
	bm->DeleteThis();

	texHandleValid.SetInfinite();
	int i;
	pblock->GetValue(uv_uvchannel, t, i, texHandleValid);
	pblock->GetValue(uv_rtype, t, i, texHandleValid);
	pblock->GetValue(uv_gtype, t, i, texHandleValid);
	pblock->GetValue(uv_btype, t, i, texHandleValid);
	pblock->GetValue(uv_rcount, t, i, texHandleValid);
	pblock->GetValue(uv_gcount, t, i, texHandleValid);
	pblock->GetValue(uv_bcount, t, i, texHandleValid);
	float f;
	pblock->GetValue(uv_tintAmount, t, f, texHandleValid);
	pblock->GetValue(uv_rAmount, t, f, texHandleValid);
	pblock->GetValue(uv_gAmount, t, f, texHandleValid);
	pblock->GetValue(uv_bAmount, t, f, texHandleValid);
	Color c;
	pblock->GetValue(uv_tintColor, t, c, texHandleValid);
	BOOL b;
	pblock->GetValue(uv_clampUVW, t, b, texHandleValid);
	return texHandle->GetHandle();
}

RefTargetHandle UVtex::Clone(RemapDir &remap) {
	UVtex *mnew = new UVtex();
	*((MtlBase*)mnew) = *((MtlBase*)this);  // copy superclass stuff
	mnew->ReplaceReference(0,remap.CloneRef(pblock));
#if MAX_RELEASE > 3100
	BaseClone(this, mnew, remap);
#endif
	return mnew;
}

#if MAX_VERSION_MAJOR < 17 //Max 2015
RefResult UVtex::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message)
#else
RefResult UVtex::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate)
#endif
	{
	switch (message) {
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget == pblock) {
				// see if this message came from a changing parameter in the pblock,
				// if so, limit rollout update to the changing item and update any active viewport texture
				ParamID changing_param = pblock->LastNotifyParamID();
				uvtexpb.InvalidateUI();//changing_param
				DiscardTexHandle();
				// notify our dependents that we've changed
				// NotifyChanged();  //DS this is redundant
			}
			break;
		}
	return REF_SUCCEED;
}
