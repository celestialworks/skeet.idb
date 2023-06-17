#include "includes.h"

DWORD dwOld_D3DRS_COLORWRITEENABLE;
IDirect3DVertexDeclaration9* vertDec;
IDirect3DVertexShader9* vertShader;

HRESULT __stdcall Hooks::Present(IDirect3DDevice9* pDevice, RECT* pRect1, const RECT* pRect2, HWND hWnd, const RGNDATA* pRGNData)
{
	c_menu::get()->init(pDevice);

	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &dwOld_D3DRS_COLORWRITEENABLE);
	pDevice->GetVertexDeclaration(&vertDec);
	pDevice->GetVertexShader(&vertShader);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
	pDevice->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ui::NewFrame();
	c_menu::get()->keybind_list();
	c_menu::get()->menu_render();
	ui::EndFrame();
	ui::Render();
	ImGui_ImplDX9_RenderDrawData(ui::GetDrawData());

	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, dwOld_D3DRS_COLORWRITEENABLE);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, true);
	pDevice->SetVertexDeclaration(vertDec);
	pDevice->SetVertexShader(vertShader);

	return g_hooks.m_device.GetOldMethod<decltype( &Present )>(17)(pDevice, pRect1, pRect2, hWnd, pRGNData);
}

HRESULT __stdcall Hooks::Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	auto result = g_hooks.m_device.GetOldMethod<decltype(&Reset)>(16)(pDevice, pPresentParameters);
	
	if(result >= 0)
		ImGui_ImplDX9_CreateDeviceObjects();

	return result;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI Hooks::WndProc( HWND wnd, uint32_t msg, WPARAM wParam, LPARAM lp ) {
	if (msg == WM_KEYDOWN && wParam == VK_INSERT)
		c_menu::get()->menu_open = !c_menu::get()->menu_open;

	if (c_menu::get()->menu_open) {
		ImGui_ImplWin32_WndProcHandler(wnd, msg, wParam, lp);
		if (wParam != 'W' && wParam != 'A' && wParam != 'S' && wParam != 'D' && wParam != VK_SHIFT && wParam != VK_CONTROL && wParam != VK_TAB && wParam != VK_SPACE || ui::GetIO().WantTextInput)
			return true;
	}

	return g_winapi.CallWindowProcA( g_hooks.m_old_wndproc, wnd, msg, wParam, lp );
}