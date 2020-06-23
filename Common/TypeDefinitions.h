#pragma once
#include <vector>

namespace Common
{
	enum class RenderLayer : int
	{
		Opaque = 0,
		Mirrors,
		Reflected,
		Transparent,
		Shadow,
		AlphaTested,
		AlphaTestedTreeSprites,
		HorzBlur,
		VertBlur,
		Count
	};

	enum class PrimitiveType : int
	{
		PointList,
		TriangleList,
	};

	struct float3
	{
		float x, y, z;
	};

	struct float4
	{
		float x, y, z, w;
	};

	struct DrawDesc
	{
		const char* modelName;
		int submesh;
		int materialIndex;
		RenderLayer layer;
		PrimitiveType primitiveType;
		float3 translate;
		float4 rotate;
		float3 scale;
	};

	struct InstanceDesc
	{
		int submesh;
		int materialIndex;
		float3 translate;
		float4 rotate;
		float3 scale;
	};

	struct DrawInstancedDesc
	{
		const char* modelName;
		RenderLayer layer;
		PrimitiveType primitiveType;
		std::vector<InstanceDesc> instances;
	};

	constexpr float3 zero3 = float3{ 0.0f, 0.0f, 0.0f };
	constexpr float3 one3 = float3{ 1.0f, 1.0f, 1.0f };
	constexpr float4 zero4 = float4{ 0.0f, 0.0f, 0.0f, 0.0f };
	constexpr float4 one4 = float4{ 1.0f, 1.0f, 1.0f, 1.0f };
}