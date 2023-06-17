#include "includes.h"
#include "studiorender.h"

void c_chams::SetColor(Color col, IMaterial* mat) {
	if (mat)
	{
		mat->ColorModulate(col);
		bool found = false;
		auto var = mat->FindVar("$envmaptint", &found);
		if (found)
			var->set_vec_value(col.r() / 255, col.g() / 255, col.b() / 255);
	}
	c_csgo::get()->m_render_view->SetColorModulation(col);
}

void c_chams::SetAlpha(float alpha, IMaterial* mat) {
	if (mat)
		mat->AlphaModulate(alpha);
	c_csgo::get()->m_render_view->SetBlend(alpha);
}

void c_chams::SetupMaterial(IMaterial* mat, Color col, bool z_flag) {
	SetColor(col, mat);

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, flags );
	mat->SetFlag(MATERIAL_VAR_ZNEARER, z_flag);
	mat->SetFlag(MATERIAL_VAR_NOFOG, z_flag);
	mat->SetFlag(MATERIAL_VAR_IGNOREZ, z_flag);

	c_csgo::get()->m_model_render->ForcedMaterialOverride(mat);
}

void c_chams::init() {
	// find stupid materials.
	debugambientcube = c_csgo::get()->m_material_system->FindMaterial(XOR("debug/debugambientcube"), nullptr);
	debugambientcube->IncrementReferenceCount();

	debugdrawflat = c_csgo::get()->m_material_system->FindMaterial(XOR("debug/debugdrawflat"), nullptr);
	debugdrawflat->IncrementReferenceCount();

	glow = c_csgo::get()->m_material_system->FindMaterial(XOR("dev/glow_armsrace.vmt"), nullptr);
	glow->IncrementReferenceCount();
}

bool c_chams::GenerateLerpedMatrix(int index, BoneArray* out) {
	LagRecord* current_record;
	AimPlayer* data;

	Player* ent = c_csgo::get()->m_entlist->GetClientEntity< Player* >(index);
	if (!ent)
		return false;

	if (!c_aimbot::get()->IsValidTarget(ent))
		return false;

	data = &c_aimbot::get()->m_players[index - 1];
	if (!data || data->m_records.empty())
		return false;

	if (data->m_records.size() < 2)
		return false;

	auto* channel_info = c_csgo::get()->m_engine->GetNetChannelInfo();
	if (!channel_info)
		return false;

	static float max_unlag = 0.2f;
	static auto sv_maxunlag = c_csgo::get()->m_cvar->FindVar(HASH("sv_maxunlag"));
	if (sv_maxunlag) {
		max_unlag = sv_maxunlag->GetFloat();
	}

	for (auto it = data->m_records.rbegin(); it != data->m_records.rend(); it++) {
		current_record = it->get();

		bool end = it + 1 == data->m_records.rend();

		if (current_record && current_record->valid() && (!end && ((it + 1)->get()))) {
			if (current_record->m_origin.dist_to(ent->GetAbsOrigin()) < 1.f) {
				return false;
			}

			vec3_t next = end ? ent->GetAbsOrigin() : (it + 1)->get()->m_origin;
			float  time_next = end ? ent->m_flSimulationTime() : (it + 1)->get()->m_sim_time;

			float total_latency = channel_info->GetAvgLatency(0) + channel_info->GetAvgLatency(1);
			total_latency = std::clamp(total_latency, 0.f, max_unlag);

			float correct = total_latency + c_client::get()->m_lerp;
			float time_delta = time_next - current_record->m_sim_time;
			float add = end ? 1.f : time_delta;
			float deadtime = current_record->m_sim_time + correct + add;

			float curtime = c_csgo::get()->m_globals->m_curtime;
			float delta = deadtime - curtime;

			float mul = 1.f / add;
			vec3_t lerp = math::Interpolate(next, current_record->m_origin, std::clamp(delta * mul, 0.f, 1.f));

			matrix3x4_t ret[128];

			std::memcpy(ret,
				current_record->m_bones,
				sizeof(ret));

			for (size_t i{ }; i < 128; ++i) {
				vec3_t matrix_delta = current_record->m_bones[i].GetOrigin() - current_record->m_origin;
				ret[i].SetOrigin(matrix_delta + lerp);
			}

			std::memcpy(out,
				ret,
				sizeof(ret));

			return true;
		}
	}

	return false;
}

void c_chams::DrawChams(void* ecx, uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {

	AimPlayer* data;
	LagRecord* record;

	if (strstr(info.m_model->m_name, XOR("models/player")) != nullptr) {
		Player* m_entity = c_csgo::get()->m_entlist->GetClientEntity<Player*>(info.m_index);
		if (!m_entity)
			return;

		c_csgo::get()->m_model_render->ForcedMaterialOverride(nullptr);
		c_csgo::get()->m_render_view->SetColorModulation(colors::white);
		c_csgo::get()->m_render_view->SetBlend(1.f);

		if (m_entity->m_bIsLocalPlayer()) {
			if (c_config::get()->b["chams_local_blend_in_scope"] && m_entity->m_bIsScoped())
				SetAlpha(0.5f);

			else if (c_config::get()->b["chams_local"]) {
				// override blend.
				SetAlpha(c_config::get()->i["local_chams_blend"] / 100.f);

				// set material and color.
				SetupMaterial(glow, c_config::get()->imcolor_to_ccolor(c_config::get()->c["local_chams_color"]), false);
				g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
			}
		}

		bool enemy = c_client::get()->m_local && m_entity->enemy(c_client::get()->m_local);

		if (enemy && c_config::get()->b["chams_enemy_history"]) {
			if (c_aimbot::get()->IsValidTarget(m_entity)) {
				if (c_config::get()->b["chams_enemy_history"]) {
					data = &c_aimbot::get()->m_players[m_entity->index() - 1];
					if (!data->m_records.empty())
					{
						record = c_resolver::get()->FindLastRecord(data);
						if (record) {
							// was the matrix properly setup?
							BoneArray arr[128];
							if (GenerateLerpedMatrix(m_entity->index(), arr)) {
								// override blend.
								SetAlpha(c_config::get()->i["chams_history_blend"] / 100.f);

								// set material and color.
								SetupMaterial(debugdrawflat, c_config::get()->imcolor_to_ccolor(c_config::get()->c["chams_history_color"]), true);

								g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, arr);
							}
						}
					}
				}
			}
		}

		if (enemy && c_config::get()->b["chams_enemy"]) {
			SetAlpha(c_config::get()->i["chams_enemy_blend"] / 100.f);
			SetupMaterial(debugambientcube, c_config::get()->imcolor_to_ccolor(c_config::get()->c["chams_invis_enemy_color"]), true);

			g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);

			SetAlpha(c_config::get()->i["chams_enemy_blend"] / 100.f);
			SetupMaterial(debugambientcube, c_config::get()->imcolor_to_ccolor(c_config::get()->c["chams_enemy_color"]), false);

			g_hooks.m_model_render.GetOldMethod< Hooks::DrawModelExecute_t >(IVModelRender::DRAWMODELEXECUTE)(ecx, ctx, state, info, bone);
		}
	}
}
