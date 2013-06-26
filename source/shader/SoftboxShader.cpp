#include "c4d.h"
#include "c4d_symbols.h"
#include "xsoftbox.h"

#define CCOUNT 125

class SoftboxData : public ShaderData
{
	public:
		Real roundness;
		Real falloffWidth;
		Real border;
		Bool bulbs[4];
		Real hotspotRadius;
		Real hotspotStrength;
		Real hotspotFalloff;
		Vector lightColor;
		Vector darkColor;
		Gradient* goboU;
		Gradient* goboV;


		Real strength;
	public:
		virtual Bool Init		(GeListNode *node);
		virtual	Vector Output(BaseShader *chn, ChannelData *cd);

		virtual	INITRENDERRESULT InitRender(BaseShader *sh, const InitRenderStruct &irs);
		virtual	void FreeRender(BaseShader *sh);

		static NodeData *Alloc(void) { return gNew SoftboxData; }
};

Bool SoftboxData::Init(GeListNode *node)
{
	BaseContainer *data = ((BaseShader*)node)->GetDataInstance();
	data->SetReal(SOFTBOXSHADER_ROUNDNESS, 0.5);
	data->SetReal(SOFTBOXSHADER_FALLOFFWIDTH, 0.3);
	data->SetReal(SOFTBOXSHADER_BORDER, 0.2);
	data->SetBool(SOFTBOXSHADER_LEFT_LIGHT, TRUE);
	data->SetBool(SOFTBOXSHADER_RIGHT_LIGHT, TRUE);
	data->SetBool(SOFTBOXSHADER_TOP_LIGHT, TRUE);
	data->SetBool(SOFTBOXSHADER_BOTTOM_LIGHT, TRUE);
	data->SetReal(SOFTBOXSHADER_HOTSPOT_RADIUS, 0.0);
	data->SetReal(SOFTBOXSHADER_HOTSPOT_STRENGTH, 1.0);
	data->SetReal(SOFTBOXSHADER_HOTSPOT_FALLOFF, 1.3);
	data->SetVector(SOFTBOXSHADER_LIGHT_COLOR,Vector(1.));
	data->SetVector(SOFTBOXSHADER_DARK_COLOR,Vector(0.));
	data->SetReal(SOFTBOXSHADER_STRENGTH,.5);

	AutoAlloc<Gradient> gradient;
	if (!gradient) return FALSE;

	GradientKnot k1;
	k1.col=1.;
	k1.pos=0.;
	gradient->InsertKnot(k1);

	data->SetData(SOFTBOXSHADER_GOBO_U,GeData(CUSTOMDATATYPE_GRADIENT,gradient));
	data->SetData(SOFTBOXSHADER_GOBO_V,GeData(CUSTOMDATATYPE_GRADIENT,gradient));

	return TRUE;
}

INITRENDERRESULT SoftboxData::InitRender(BaseShader *sh, const InitRenderStruct &irs)
{
	BaseContainer *data = sh->GetDataInstance();

	roundness = FMax(data->GetReal(SOFTBOXSHADER_ROUNDNESS), 0.1);
	falloffWidth = 1./data->GetReal(SOFTBOXSHADER_FALLOFFWIDTH);
	border = data->GetReal(SOFTBOXSHADER_BORDER);
	bulbs[0] = data->GetBool(SOFTBOXSHADER_LEFT_LIGHT);
	bulbs[1] = data->GetBool(SOFTBOXSHADER_RIGHT_LIGHT);
	bulbs[2] = data->GetBool(SOFTBOXSHADER_TOP_LIGHT);
	bulbs[3] = data->GetBool(SOFTBOXSHADER_BOTTOM_LIGHT);
	hotspotRadius   = data->GetReal(SOFTBOXSHADER_HOTSPOT_RADIUS);
	hotspotStrength = data->GetReal(SOFTBOXSHADER_HOTSPOT_STRENGTH);
	hotspotFalloff = data->GetReal(SOFTBOXSHADER_HOTSPOT_FALLOFF);
	lightColor = data->GetVector(SOFTBOXSHADER_LIGHT_COLOR);
	darkColor = data->GetVector(SOFTBOXSHADER_DARK_COLOR);
	strength = data->GetReal(SOFTBOXSHADER_STRENGTH) * 0.5;

	goboU = (Gradient*)data->GetCustomDataType(SOFTBOXSHADER_GOBO_U,CUSTOMDATATYPE_GRADIENT); 
	if (!goboU || !goboU->InitRender(irs)) return INITRENDERRESULT_OUTOFMEMORY;

	goboV = (Gradient*)data->GetCustomDataType(SOFTBOXSHADER_GOBO_V,CUSTOMDATATYPE_GRADIENT); 
	if (!goboV || !goboV->InitRender(irs)) return INITRENDERRESULT_OUTOFMEMORY;
	
	return INITRENDERRESULT_OK;
}

void SoftboxData::FreeRender(BaseShader *sh)
{
}

static void GetContourCoords(Real dimension, Real c, Real &ex, Real &ey)
{
	ex = Pow(1./(Pow(c,dimension)+1),1./dimension);
	ey = ex*c;
	
}

Vector SoftboxData::Output(BaseShader *chn, ChannelData *cd)
{
	Real px=(cd->p.x - 0.5)*(2.+border);
	Real py=(cd->p.y - 0.5)*(2.+border);
	Real value = 0.;
	if(bulbs[0]) value += strength * 1. * (1.-px)*0.5;
	if(bulbs[1]) value += strength * 1. * (1.+px)*0.5;
	if(bulbs[2]) value += strength * 1. * (1.-py)*0.5;
	if(bulbs[3]) value += strength * 1. * (1.+py)*0.5;
	Real centerDist = Sqrt(px*px + py*py);
	value += (1-Smoothstep(hotspotRadius, hotspotRadius+hotspotFalloff, centerDist))*hotspotStrength*strength;
	px=Abs(px);
	py=Abs(py);
	Real c = py/px;
	if(px == 0.)
		c = py/0.01;
	Real dimension = 2.0/roundness;
	Real superellipse = Pow(Abs(px),dimension) + Pow(Abs(py),dimension);
	if(superellipse > 1.){
		Real ex,ey;
		GetContourCoords(dimension,c,ex,ey);
		Real tmpx = px-ex;
		Real tmpy = py-ey;
		Real edgeDist = Sqrt(tmpx*tmpx+tmpy*tmpy);
		value *= 1. - Smoothstep(0., 1., edgeDist*falloffWidth);
	}
	Vector goboValue = goboU->CalcGradientPixel(cd->p.x);
	goboValue ^= goboV->CalcGradientPixel(cd->p.y);
	return goboValue ^ (lightColor * value + darkColor*FMax(0., 1.-value));
}

#define ID_SOFTBOX	1030706

Bool RegisterSoftbox(void)
{
	return RegisterShaderPlugin(ID_SOFTBOX,GeLoadString(IDS_SOFTBOX),0,SoftboxData::Alloc,"Xsoftbox",0);
}