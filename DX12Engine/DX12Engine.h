#pragma once

#define DllExport   __declspec( dllexport )
class DllExport DX12Engine
{
public:
	static int Run(HINSTANCE hInstance);
};