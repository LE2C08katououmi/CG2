#include "Basic.hlsli"

VSOutput main(float4 pos : POSITION,float3 normal:NORMAL, float2 uv : TEXCOORD)
{
    VSOutput output; // ピクセルシェーダーに渡す値
    output.svpos = mul(mat, pos);
    // output.svpos = pos;
    // output.uv = uv;
    output.normal = normal;
    return output;
}
