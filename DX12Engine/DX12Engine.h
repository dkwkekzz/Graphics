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

	static void OnCreateTexture(LPCSTR name, LPCWSTR filename);
	static void OnCreateModel(LPCSTR name, LPCWSTR filename);
	static void OnCreateShader(LPCSTR name, LPCWSTR filename);
};