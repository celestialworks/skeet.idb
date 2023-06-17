#pragma once
#include "includes.h"
#include <d3dx9.h>
#include "singleton.h"

class c_menu : public singleton<c_menu>
{
public:
	void init(IDirect3DDevice9* device);
	void menu_render();
	void keybind_list();
	bool initialized;
	bool menu_open = false;
	IDirect3DTexture9* m_pTexture = nullptr;
};