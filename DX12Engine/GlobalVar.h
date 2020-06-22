#pragma once

namespace Global
{
	constexpr float SCREEN_DEPTH = 1000.0f;
	constexpr float SCREEN_NEAR = 0.1f;
	constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;
	constexpr int FRAME_RESOURCE_COUNT = 3;
	constexpr int MAIN_PASS_COUNT = 2;
	constexpr int MAX_OBJECT_COUNT = 128;
	constexpr int MAX_MATERIAL_COUNT = 32;
	constexpr int MAX_TEXTURE_COUNT = 128;
	constexpr int MAX_LIGHTS = 16;
	constexpr int OBJECTCB_PARAMETER_INDEX = 1;
	constexpr int PASSCB_PARAMETER_INDEX = 2;
	constexpr int MATCB_PARAMETER_INDEX = 3;
}