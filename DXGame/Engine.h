#pragma once
#include "config.h"

class Engine
{
public:
	Engine(HINSTANCE hInstance);
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	~Engine();

	static Engine* GetApp();

	inline HINSTANCE		AppInst()const { return mhAppInst; }
	inline HWND				MainWnd()const { return mhMainWnd; }
	inline float			AspectRatio()const { return static_cast<float>(mClientWidth) / mClientHeight; }
	inline int				Width()const { return mClientWidth; }
	inline int				Height()const { return mClientHeight; }

	bool Init();
	int Run();
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void CalculateFrameStats();

private:
	static Engine* mApp;

	HINSTANCE	mhAppInst = nullptr; // application instance handle
	HWND		mhMainWnd = nullptr; // main window handle

	bool		mInitialized = false;
	bool		mAppPaused = false;  // is the application paused?
	bool		mMinimized = false;  // is the application minimized?
	bool		mMaximized = false;  // is the application maximized?
	bool		mResizing = false;   // are the resize bars being dragged?
	bool		mFullscreenState = false;// fullscreen enabled

	int				mClientWidth = Config::SCREEN_WIDTH;
	int				mClientHeight = Config::SCREEN_HEIGHT;
	std::wstring	mMainWndCaption = Config::APPLICATION_NAME;

};

