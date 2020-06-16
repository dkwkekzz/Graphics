#pragma once
#include "GlobalVar.h"
#include "GameTimer.h"
#include "World.h"


class SystemBase
{
public:
	SystemBase(HINSTANCE hInstance);
	SystemBase(const SystemBase&) = delete;
	SystemBase& operator=(const SystemBase&) = delete;
	~SystemBase();

	static SystemBase* GetApp();

	inline HINSTANCE		AppInst()const { return mhAppInst; }
	inline HWND				MainWnd()const { return mhMainWnd; }
	inline float			AspectRatio()const { return static_cast<float>(mClientWidth) / mClientHeight; }
	inline int				Width()const { return mClientWidth; }
	inline int				Height()const { return mClientHeight; }
	inline const GameTimer& GetTimer()const { return m_timer; }

	bool Init();
	int Run();
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void CalculateFrameStats();

private:
	static SystemBase* mApp;
	
	HINSTANCE	mhAppInst = nullptr; // application instance handle
	HWND		mhMainWnd = nullptr; // main window handle

	bool		mAppPaused = false;  // is the application paused?
	bool		mMinimized = false;  // is the application minimized?
	bool		mMaximized = false;  // is the application maximized?
	bool		mResizing = false;   // are the resize bars being dragged?
	bool		mFullscreenState = false;// fullscreen enabled

	int				mClientWidth = Global::SCREEN_WIDTH;
	int				mClientHeight = Global::SCREEN_HEIGHT;
	std::wstring	mMainWndCaption = Global::APPLICATION_NAME;

	GameTimer	m_timer;
	World		m_world;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static SystemBase* ApplicationHandle = 0;