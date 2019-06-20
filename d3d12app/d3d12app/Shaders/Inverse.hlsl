Texture2D gMap : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static const float2 gTexCoords[6] = 
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

static const float4 gPosCoords[6] =
{
	float4(-1.0f, -1.0f, 0.0f, 1.0f),
	float4(-1.0f, 1.0f, 0.0f, 1.0f),
	float4(1.0f, 1.0f, 0.0f, 1.0f),
	float4(-1.0f, -1.0f, 0.0f, 1.0f),
	float4(1.0f, 1.0f, 0.0f, 1.0f),
	float4(1.0f, -1.0f, 0.0f, 1.0f)
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;
	
	vout.TexC = gTexCoords[vid];

	vout.PosH = gPosCoords[vid];

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
	//return gMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f);
}


