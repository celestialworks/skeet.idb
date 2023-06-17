#include "includes.h"

void Hooks::DrawModelExecute(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {
	// do chams.
	c_chams::get()->init();

	if (c_csgo::get()->m_model_render->IsForcedMaterialOverride()) {
		g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(this, ctx, state, info, bone);
		return;
	}

	if (c_csgo::get()->m_engine->IsConnected() && c_csgo::get()->m_engine->IsInGame()) {
		if (strstr(info.m_model->m_name, XOR("player/contactshadow")) != nullptr) {
			return;
		}

		c_chams::get()->DrawChams(this, ctx, state, info, bone);
	}
	g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(this, ctx, state, info, bone);
	c_csgo::get()->m_model_render->ForcedMaterialOverride(nullptr);
}