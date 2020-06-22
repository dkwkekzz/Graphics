#pragma once

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define DllExport __declspec( dllexport )
class DllExport DX12Engine
{
public:
	static void OnInit(HWND mainWnd, int width, int height);
	static void OnDispose();
	static void OnTick(bool appPaused);
	static void OnDraw();
	static void OnResize(int width, int height);
	static void OnMouseDown(int btnState, int x, int y);
	static void OnMouseUp(int btnState, int x, int y);
	static void OnMouseMove(int btnState, int x, int y);

	static void OnRestart();
	static void OnPause();
	static float GetTotalTime();

	static void OnCreateActor(LPCSTR modelName, int& generatedId);
	static void OnTransformActor(int id, float translate[3], float rotate[3], float scale[3]);
};