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

	// nointerpolationʹ���������ᱻ��ֵ
	nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;

	// ��ȡʵ������
	InstanceData instData = gInstanceData[instanceID];
	float4x4 world = instData.World;
	float4x4 invTraWorld = instData.InvTraWorld;
	float4x4 texTransform = instData.TexTransform;
	uint matIndex = instData.MaterialIndex;

	vout.MatIndex = matIndex;

	// ��ȡ��������
	MaterialData matData = gMaterialData[matIndex];
	
    // �任������ռ�
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;

	// �ٶ���������������ģ�������Ҫ������ת�þ���
	// ����ֱ��ʹ����ת�þ���
    vout.NormalW = mul(vin.NormalL, (float3x3)invTraWorld);
	vout.TangentW = mul(vin.TangentU, (float3x3)invTraWorld);

    // �任����μ��ÿռ�
    vout.PosH = mul(posW, gViewProj);

	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), matIndex);
	vout.TexC = mul(texC, matData.MatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// ��ȡ��������
	MaterialData matData = gMaterialData[pin.MatIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseMapIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;

	// �������ж�̬��������
	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
	// ��������alphaֵС��0.1������
	// ����ɫ���о������ò��ԣ��Ա��ڸ�����뿪��ɫ�����Ӷ�ʡȥ�����ļ���
	clip(diffuseAlbedo.a - 0.1f);
#endif

	// ��ֵ����������ɷǵ�λ�������������Ҫ����
	pin.NormalW = normalize(pin.NormalW);

	float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

	// ȡ��ע���Թرշ�������ͼӳ��
	bumpedNormalW = pin.NormalW;

	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // ��λ��

	// ������
    float4 ambient = gAmbientLight* diffuseAlbedo;

    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// ���淴��
	//float3 r = reflect(-toEyeW, bumpedNormalW);
	////r = BoxCubeMapLookup(pin.PosW, normalize(r), float3(0.0f, 0.0f, 0.0f), float3(2500.0f, 2500.0f, 2500.0f));
	//float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	//float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
	//litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

#ifdef FOG
	// ������
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // ͨ�����������������ȡalphaֵ
    litColor.a = diffuseAlbedo.a;

    return litColor;
}


