#include "includes.h"

int Hooks::DebugSpreadGetInt() {
	if (c_client::get()->m_processing && !c_client::get()->m_local->m_bIsScoped() && c_config::get()->b["force_crosshair"])
		return 3;

	return g_hooks.m_debug_spread.GetOldMethod< GetInt_t >(ConVar::GETINT)(this);
}

int Hooks::CsmShadowGetInt() {
	// fuck the blue nigger shit on props with nightmode
	// :nauseated_face:
	return 0;
}