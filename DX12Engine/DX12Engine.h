#pragma once

#define DllExport   __declspec( dllexport )
class DllExport DX12Engine
{
public:
	static void OnInit();
	static void OnDispose();
	static void OnTick(bool appPaused);
	static void OnResize();
	static void OnMouseDown(int btnState, int x, int y);
	static void OnMouseUp(int btnState, int x, int y);
	static void OnMouseMove(int btnState, int x, int y);

};