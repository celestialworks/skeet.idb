#include "includes.h"

bool Hooks::IsConnected() {
	Stack stack;

	static Address IsLoadoutAllowed{ pattern::find(c_csgo::get()->m_client_dll, XOR("84 C0 75 04 B0 01 5F")) };

	if ( stack.ReturnAddress() == IsLoadoutAllowed)
		return false;

	return g_hooks.m_engine.GetOldMethod< IsConnected_t >(IVEngineClient::ISCONNECTED)(this);
}

void Hooks::SetViewAngles(vec3_t& ang) {
	g_hooks.m_engine.GetOldMethod< SetViewAngles_t >(IVEngineClient::SETVIEWANGLES)(this, ang);
}


bool Hooks::IsPaused() {
	static DWORD* return_to_extrapolation = (DWORD*)(pattern::find(c_csgo::get()->m_client_dll,
		XOR("FF D0 A1 ?? ?? ?? ?? B9 ?? ?? ?? ?? D9 1D ?? ?? ?? ?? FF 50 34 85 C0 74 22 8B 0D ?? ?? ?? ??")) + 0x29);

	if (_ReturnAddress() == (void*)return_to_extrapolation)
		return true;

	return g_hooks.m_engine.GetOldMethod< IsPaused_t >(IVEngineClient::ISPAUSED)(this);
}

bool Hooks::IsHLTV() {
	Stack stack;

	static Address SetupVelocity{ pattern::find(c_csgo::get()->m_client_dll, XOR("84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80")) };

	// AccumulateLayers
	if (c_bones::get()->m_running)
		return true;

	// fix for animstate velocity.
	if (stack.ReturnAddress() == SetupVelocity)
		return true;

	return g_hooks.m_engine.GetOldMethod< IsHLTV_t >(IVEngineClient::ISHLTV)(this);
}

void Hooks::EmitSound(IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const vec3_t* pOrigin, const vec3_t* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity) {
	if (strstr(pSample, "null")) {
		iFlags = (1 << 2) | (1 << 5);
	}

	g_hooks.m_engine_sound.GetOldMethod<EmitSound_t>(IEngineSound::EMITSOUND)(this, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, flAttenuation, nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);
}