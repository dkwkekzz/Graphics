#pragma once
class Engine
{
public:
	Engine(HINSTANCE hInstance);
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	~Engine();

	void Run(const wchar_t* className);

private:
	bool m_AppPaused{ false };

};

