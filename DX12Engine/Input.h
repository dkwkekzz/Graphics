#pragma once
class Input
{
public:
	static void OnMouseDown(WPARAM btnState, int x, int y);
	static void OnMouseUp(WPARAM btnState, int x, int y);
	static void OnMouseMove(WPARAM btnState, int x, int y);
};

