#include "Common.hlsl"
 
struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// ��ȡ��������
	MaterialData matData = gMaterialData[gMaterialIndex];
	
	vout.PosH = float4(vin.PosL, 1.0f);
    vout.PosW = vin.PosL;
    vout.NormalW = vin.NormalL;
	vout.TangentW = vin.TangentU;

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// ��ȡ��������
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	uint diffuseMapIndex = matData.DiffuseMapIndex;

	// �������ж�̬��������
	if(diffuseMapIndex != -1)
		diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
	// ��������alphaֵС��0.1������
	// ����ɫ���о������ò��ԣ��Ա��ڸ�����뿪��ɫ�����Ӷ�ʡȥ�����ļ���
	clip(diffuseAlbedo.a - 0.1f);
#endif

    return diffuseAlbedo;
}


