#include "legitbot.h"

Player* c_legitbot::calcTarget()
{
	float ofov = 10;
	float nfov = 0;

	Player* player = nullptr;

	for (int i = 0; i < 64; i++)
	{
		auto ent = c_csgo::get()->m_entlist->GetClientEntity<Player*>(i);

		if (ent && ent->alive() && !ent->dormant() && ent->enemy(c_client::get()->m_local))
		{
			vec3_t eyepos;
			c_client::get()->m_local->GetEyePos(&eyepos);
			vec3_t hitboxpos;
			ent->GetEyePos(&hitboxpos);

			vec3_t AngleTo = { math::CalcAngle(eyepos, hitboxpos).x, math::CalcAngle(eyepos, hitboxpos).y, math::CalcAngle(eyepos, hitboxpos).z };

			vec3_t view = { c_client::get()->m_cmd->m_view_angles.x, c_client::get()->m_cmd->m_view_angles.y, c_client::get()->m_cmd->m_view_angles.z };

			nfov = view.dist_to(AngleTo);

			if (nfov < ofov)
			{
				ofov = nfov;
				player = ent;
			}
		}
	}
	return player;
}

void c_legitbot::Aim()
{
	if (!c_config::get()->b["legit_enabled"])
		return;

	if (!c_csgo::get()->m_engine->IsConnected() || !c_csgo::get()->m_engine->IsInGame())
		return;

	if (!c_client::get()->m_local)
		return;

	if (!c_client::get()->m_local->alive())
		return;

	Player* target = calcTarget();

	if (target && c_client::get()->m_cmd->m_buttons & IN_ATTACK)
	{
		vec3_t eyepos;
		c_client::get()->m_local->GetEyePos(&eyepos);
		vec3_t targethit;
		target->GetEyePos(&targethit);
		vec3_t viewangles = math::CalcAngle(eyepos, targethit);

		float smooth = g_Options.legitaim_smooth;

		/*QAngle delta = cmd->viewangles - viewangles;

		QAngle finalAng = cmd->viewangles - delta / (smooth * 20);

		if (g_Options.legitaim_visibleCheck && g_LocalPlayer->CanSeePlayer(target, target->GetEyePos()))
		{
			cmd->viewangles = finalAng;
			g_EngineClient->SetViewAngles(&cmd->viewangles);
		}
		else if (!g_Options.legitaim_visibleCheck)
		{
			cmd->viewangles = finalAng;
			g_EngineClient->SetViewAngles(&cmd->viewangles);
		}*/
	}
}
