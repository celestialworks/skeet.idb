#include "includes.h"

// init routine.
ulong_t __stdcall c_client::init(void* arg) {
	// get user name.
	char user_name[257];
	DWORD user_name_size = sizeof(user_name);
	if (GetUserName(user_name, &user_name_size))
		c_client::get()->m_user = XOR(user_name);

	// stop here if we failed to acquire all the data needed from csgo.
	if (!c_csgo::get()->init())
		return 0;

	// clear the console of unnecessary shit.
	c_csgo::get()->m_engine->ExecuteClientCmd("clear");

	// set proper parameters for console.
	c_csgo::get()->m_cvar->FindVar(HASH("developer"))->SetValue(1);
	c_csgo::get()->m_cvar->FindVar(HASH("con_enable"))->SetValue(2);
	c_csgo::get()->m_cvar->FindVar(HASH("con_filter_enable"))->SetValue(1);
	c_csgo::get()->m_cvar->FindVar(HASH("con_filter_text"))->SetValue("[skeet.idb]");
	c_csgo::get()->m_cvar->FindVar(HASH("con_filter_text_out"))->SetValue(" ");
	c_csgo::get()->m_cvar->FindVar(HASH("contimes"))->SetValue(10);

	// welcome the user.
	std::string out = tfm::format(XOR("welcome back, %s\n"), c_client::get()->m_user);
	c_event_logs::get()->add(out, colors::white, 8.f, false);

	return 1;
}

void c_client::DrawHUD() {
	// constants for colors.
	const auto col_background = Color(17, 17, 17, 240);
	const auto col_accent = c_config::get()->imcolor_to_ccolor(c_config::get()->c["menu_color"]);
	const auto col_text = Color(255, 255, 255);

	// cheat variables.
	std::string logo = "skeet.idb";

#ifdef _DEBUG
	logo.append(" [debug]");
#endif

	// get time.
	time_t t = std::time(nullptr);
	std::ostringstream time;
	time << std::put_time(std::localtime(&t), XOR("%H:%M:%S"));

	std::string text = tfm::format(XOR("%s | %s | %s"), logo, c_client::get()->m_user, time.str().data());

	if (c_csgo::get()->m_engine->IsInGame())
	{
		// get round trip time in milliseconds.
		int ms = std::max(0, (int)std::round(c_client::get()->m_latency * 1000.f));
		text = tfm::format(XOR("%s | %s | delay: %ims | %itick | %s"), logo, c_client::get()->m_user, ms, c_client::get()->m_rate, time.str().data());
	}

	render::FontSize_t size = render::hud.size(text);

	// background.
	render::rect_filled(m_width - size.m_width - 18, 10, size.m_width + 8, size.m_height + 8, col_background);
	render::rect_filled(m_width - size.m_width - 18, 10, size.m_width + 8, 2, col_accent);

	// text.
	render::hud.string(m_width - 14, 14, col_text, text, render::ALIGN_RIGHT);
}

void c_client::KillFeed() {
	static bool clear_notices = false;
	static bool old_setting = c_config::get()->b["misc_killfeed"];
	static void(__thiscall * clear_death_notices_addr)(uintptr_t);
	static uintptr_t* death_notices_addr;

	if (!(c_csgo::get()->m_engine->IsConnected()) || !(c_csgo::get()->m_engine->IsInGame()))
		return;

	if (!c_client::get()->m_local) {

		// invalidate the clear death notice address if local goes invalid.
		if (clear_death_notices_addr) {
			clear_death_notices_addr = nullptr;
		}

		// invalidate the death notice address if local goes invalid.
		if (death_notices_addr) {
			death_notices_addr = nullptr;
		}

		// that's it, exit out.
		return;
	}

	// only do the following if local is alive.
	if (c_client::get()->m_local->alive()) {
		// get the death notice addr.
		if (!death_notices_addr) {
			death_notices_addr = game::FindElement(XOR("CCSGO_HudDeathNotice"));
		}

		// get the clear death notice addr.
		if (!clear_death_notices_addr) {
			clear_death_notices_addr = pattern::find(c_csgo::get()->m_client_dll, XOR("55 8B EC 83 EC 0C 53 56 8B 71 58")).as< void(__thiscall*)(uintptr_t) >();
		}

		// only do the following if both addresses were found and are valid.
		if (clear_death_notices_addr && death_notices_addr) {
			// grab the local death notice time.
			float* local_death_notice_time = (float*)((uintptr_t)death_notices_addr + 0x50);
			static float backup_local_death_notice_time = *local_death_notice_time;

			// extend killfeed time.
			if (c_client::get()->m_round_flags == 1) {
				if (local_death_notice_time && c_config::get()->b["misc_killfeed"]) {
					*local_death_notice_time = std::numeric_limits<float>::max();
				}
			}

			// if we disable the option, clear the death notices.
			if (old_setting != c_config::get()->b["misc_killfeed"]) {
				if (!c_config::get()->b["misc_killfeed"]) {
					if (local_death_notice_time) {
						// reset back to the regular death notice time.
						*local_death_notice_time = backup_local_death_notice_time;
					}

					clear_notices = true;
				}
				old_setting = c_config::get()->b["misc_killfeed"];
			}

			// if we have the option enabled and we need to clear the death notices.
			if (c_config::get()->b["misc_killfeed"]) {
				if (c_client::get()->m_round_flags == 0 && death_notices_addr - 0x14) {
					clear_notices = true;
				}
			}

			// clear the death notices.
			if (clear_notices) {
				clear_death_notices_addr(((uintptr_t)death_notices_addr - 0x14));
				clear_notices = false;
			}
		}
	}
	else {
		// invalidate clear death notice address.
		if (clear_death_notices_addr) {
			clear_death_notices_addr = nullptr;
		}

		// invalidate death notice address.
		if (death_notices_addr) {
			death_notices_addr = nullptr;
		}
	}
}

void c_client::ClanTag()
{
	// lambda function for setting our clantag.
	auto set_clantag = [&](std::string tag) -> void {
		using SetClanTag_t = int(__fastcall*)(const char*, const char*);
		static auto SetClanTagFn = pattern::find(c_csgo::get()->m_engine_dll, XOR("53 56 57 8B DA 8B F9 FF 15")).as<SetClanTag_t>();

		SetClanTagFn(tag.c_str(), XOR("skeet.idb"));
	};

	static auto is_enable = false;
	static int oldcurtime = 0;

	if (!c_config::get()->b["misc_clantag"])
	{
		if (is_enable)
		{
			if (oldcurtime != c_csgo::get()->m_globals->m_curtime) {
				set_clantag("");
			}
			oldcurtime = c_csgo::get()->m_globals->m_curtime;
		}
		is_enable = false;
		return;
	}

	is_enable = true;
	if (c_config::get()->i["misc_clantag_mode"] == 0)
	{
		if (oldcurtime != int(c_csgo::get()->m_globals->m_curtime)) {
			set_clantag("[VALV\xE1\xB4\xB1]");
		}
		oldcurtime = int(c_csgo::get()->m_globals->m_curtime);
	}
	else if (c_config::get()->i["misc_clantag_mode"] == 1)
	{
		if (oldcurtime != int(c_csgo::get()->m_globals->m_curtime)) {
			set_clantag("skeet.idb");
		}
		oldcurtime = int(c_csgo::get()->m_globals->m_curtime);
	}
	else if (c_config::get()->i["misc_clantag_mode"] == 2)
	{
		static auto is_freeze_period = false;
		if (c_csgo::get()->m_gamerules->m_bFreezePeriod())
		{
			if (is_freeze_period)
			{
				set_clantag("skeet.idb");
			}
			is_freeze_period = false;
			return;
		}

		is_freeze_period = true;

		if (oldcurtime != int(c_csgo::get()->m_globals->m_curtime * 3.6) % 10) {
			switch (int(c_csgo::get()->m_globals->m_curtime * 3.6) % 10) {
			case 0: { set_clantag("skeet.idb  "); break; }
			case 1: { set_clantag("keet.idb  s"); break; }
			case 2: { set_clantag("eet.idb  sk"); break; }
			case 3: { set_clantag("et.idb  ske"); break; }
			case 4: { set_clantag("t.idb  skee"); break; }
			case 5: { set_clantag(".idb  skeet"); break; }
			case 6: { set_clantag("idb  skeet."); break; }
			case 7: { set_clantag("db  skeet.i"); break; }
			case 8: { set_clantag("b  skeet.id"); break; }
			case 9: { set_clantag(" skeet.idb"); break; }

			}
			oldcurtime = int(c_csgo::get()->m_globals->m_curtime * 3.6) % 10;
		}
	}
}

void c_client::OnPaint() {
	// update screen size.
	c_csgo::get()->m_engine->GetScreenSize(m_width, m_height);

	// render stuff.
	c_visuals::get()->think();
	c_grenades::get()->paint();
	c_event_logs::get()->think();

	DrawHUD();
	c_visuals::get()->keybinds();
	KillFeed();
}

void c_client::OnMapload() {
	// store class ids.
	g_netvars.SetupClassData();

	if (!c_csgo::get()->m_engine)
		return;

	// createmove will not have been invoked yet.
	// but at this stage entites have been created.
	// so now we can retrive the pointer to the local player.
	m_local = c_csgo::get()->m_entlist->GetClientEntity< Player* >(c_csgo::get()->m_engine->GetLocalPlayer());

	// world materials.
	c_visuals::get()->ModulateWorld();

	m_sequences.clear();
	//m_cmds.clear();

	// if the INetChannelInfo pointer has changed, store it for later.
	c_csgo::get()->m_net = c_csgo::get()->m_engine->GetNetChannelInfo();

	if (c_csgo::get()->m_net) {
		g_hooks.m_net_channel.reset();
		g_hooks.m_net_channel.init(c_csgo::get()->m_net);
		g_hooks.m_net_channel.add(INetChannel::PROCESSPACKET, util::force_cast(&Hooks::ProcessPacket));
		g_hooks.m_net_channel.add(INetChannel::SENDDATAGRAM, util::force_cast(&Hooks::SendDatagram));
	}
}

void c_client::StartMove(CUserCmd* cmd) {

	// save some usercmd stuff.
	m_cmd = cmd;
	m_tick = cmd->m_tick;
	m_view_angles = cmd->m_view_angles;
	m_buttons = cmd->m_buttons;
	m_pressing_move = (m_buttons & (IN_LEFT) || m_buttons & (IN_FORWARD) || m_buttons & (IN_BACK) ||
		m_buttons & (IN_RIGHT) || m_buttons & (IN_MOVELEFT) || m_buttons & (IN_MOVERIGHT) ||
		m_buttons & (IN_JUMP));

	// get local ptr.
	m_local = c_csgo::get()->m_entlist->GetClientEntity< Player* >(c_csgo::get()->m_engine->GetLocalPlayer());
	if (!m_local)
		return;

	c_csgo::get()->m_net = c_csgo::get()->m_engine->GetNetChannelInfo();

	if (!c_csgo::get()->m_net)
		return;

	if (m_processing && m_tick_to_recharge > 0 && !m_charged) {
		m_tick_to_recharge--;
		if (m_tick_to_recharge == 0) {
			m_charged = true;
		}
		cmd->m_tick = INT_MAX;
		*m_packet = true;
	}

	// store some variables.
	m_max_lag = c_csgo::get()->m_gamerules->m_bIsValveDS() ? 6 : 14;
	m_lag = c_csgo::get()->m_cl->m_choked_commands;
	m_lerp = game::GetClientInterpAmount();
	m_latency = c_csgo::get()->m_net->GetLatency(INetChannel::FLOW_OUTGOING);
	math::clamp(m_latency, 0.f, 1.f);
	m_latency_ticks = game::TIME_TO_TICKS(m_latency);
	m_server_tick = c_csgo::get()->m_cl->clock_drift_mgr.m_server_tick;
	m_arrival_tick = m_server_tick + m_latency_ticks;
	can_use_exploit = c_aimbot::get()->CanUseExploit();
	shift_value = 15;

	// processing indicates that the localplayer is valid and alive.
	m_processing = m_local && m_local->alive();
	if (!m_processing)
		return;

	// make sure prediction has ran on all usercommands.
	// because prediction runs on frames, when we have low fps it might not predict all usercommands.
	// also fix the tick being inaccurate.
	g_inputpred.update();

	// store some stuff about the local player.
	m_flags = m_local->m_fFlags();

	// ...
	m_shot = false;
}

void c_client::BackupPlayers(bool restore) {
	if (restore) {
		// restore stuff.
		for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
			Player* player = c_csgo::get()->m_entlist->GetClientEntity< Player* >(i);

			if (!c_aimbot::get()->IsValidTarget(player))
				continue;

			c_aimbot::get()->m_backup[i - 1].restore(player);
		}
	}

	else {
		// backup stuff.
		for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
			Player* player = c_csgo::get()->m_entlist->GetClientEntity< Player* >(i);

			if (!c_aimbot::get()->IsValidTarget(player))
				continue;

			c_aimbot::get()->m_backup[i - 1].store(player);
		}
	}
}

void c_client::MouseDelta()
{
	// reason for this is to fix mousedx/dy
	// idk why it needs fixing but skeet, onetap and aimware do it.
	// the following code is from aimware

	if (!m_local || !m_cmd)
		return;

	static ang_t delta_viewangles{ };
	ang_t delta = m_cmd->m_view_angles - delta_viewangles;
	delta.clamp();

	static ConVar* sensitivity = c_csgo::get()->m_cvar->FindVar(HASH("sensitivity"));

	if (!sensitivity)
		return;

	if (delta.x != 0.f) {
		static ConVar* m_pitch = c_csgo::get()->m_cvar->FindVar(HASH("m_pitch"));

		if (!m_pitch)
			return;

		int final_dy = static_cast<int>((delta.x / m_pitch->GetFloat()) / sensitivity->GetFloat());
		if (final_dy <= 32767) {
			if (final_dy >= -32768) {
				if (final_dy >= 1 || final_dy < 0) {
					if (final_dy <= -1 || final_dy > 0)
						final_dy = final_dy;
					else
						final_dy = -1;
				}
				else {
					final_dy = 1;
				}
			}
			else {
				final_dy = 32768;
			}
		}
		else {
			final_dy = 32767;
		}

		m_cmd->m_mousedy = static_cast<short>(final_dy);
	}

	if (delta.y != 0.f) {
		static ConVar* m_yaw = c_csgo::get()->m_cvar->FindVar(HASH("m_yaw"));

		if (!m_yaw)
			return;

		int final_dx = static_cast<int>((delta.y / m_yaw->GetFloat()) / sensitivity->GetFloat());
		if (final_dx <= 32767) {
			if (final_dx >= -32768) {
				if (final_dx >= 1 || final_dx < 0) {
					if (final_dx <= -1 || final_dx > 0)
						final_dx = final_dx;
					else
						final_dx = -1;
				}
				else {
					final_dx = 1;
				}
			}
			else {
				final_dx = 32768;
			}
		}
		else {
			final_dx = 32767;
		}

		m_cmd->m_mousedx = static_cast<short>(final_dx);
	}

	delta_viewangles = m_cmd->m_view_angles;
}

void c_client::DoMove() {
	penetration::PenetrationOutput_t tmp_pen_data{ };

	// backup strafe angles (we need them for input prediction)
	m_strafe_angles = m_cmd->m_view_angles;

	// run movement code before input prediction.
	c_movement::get()->JumpRelated();
	c_movement::get()->Strafe();
	c_movement::get()->SlowMotion();
	c_movement::get()->FastStop();

	// predict input.
	g_inputpred.run();

	// restore original angles after input prediction
	m_cmd->m_view_angles = m_view_angles;

	// convert viewangles to directional forward vector.
	math::AngleVectors(m_view_angles, &m_forward_dir);

	// store stuff after input pred.
	m_shoot_pos = m_local->GetShootPosition();

	// reset shit.
	m_weapon = nullptr;
	m_weapon_info = nullptr;
	m_weapon_id = -1;
	m_weapon_type = WEAPONTYPE_UNKNOWN;
	m_player_fire = m_weapon_fire = false;

	// store weapon stuff.
	m_weapon = m_local->GetActiveWeapon();

	if (m_weapon) {
		m_weapon_info = m_weapon->GetWpnData();
		m_weapon_id = m_weapon->m_iItemDefinitionIndex();
		m_weapon_type = m_weapon_info->WeaponType;

		// ensure weapon spread values / etc are up to date.
		if (!m_weapon->IsGrenade())
			m_weapon->UpdateAccuracyPenalty();

		// run autowall once for penetration crosshair if we have an appropriate weapon.
		if (m_weapon_type != WEAPONTYPE_KNIFE && m_weapon_type != WEAPONTYPE_EQUIPMENT && !m_weapon->IsGrenade()) {
			penetration::PenetrationInput_t in;
			in.m_from = m_local;
			in.m_target = nullptr;
			in.m_pos = m_shoot_pos + (m_forward_dir * m_weapon_info->flRange);
			in.m_damage = 1.f;
			in.m_damage_pen = 1.f;
			in.m_can_pen = true;

			// run autowall.
			penetration::run(&in, &tmp_pen_data);
		}

		// set pen data for penetration crosshair.
		m_pen_data = tmp_pen_data;

		// can the player fire.
		m_player_fire = c_csgo::get()->m_globals->m_curtime >= m_local->m_flNextAttack() && !c_csgo::get()->m_gamerules->m_bFreezePeriod() && !(c_client::get()->m_flags & FL_FROZEN);

		UpdateRevolverCock();
		m_weapon_fire = CanFireWeapon(game::TICKS_TO_TIME(c_client::get()->m_local->m_nTickBase()));
	}

	// grenade prediction.
	c_grenades::get()->think();

	// run fakelag.
	c_hvh::get()->SendPacket();

	// run aimbot.
	c_aimbot::get()->think();

	// run antiaims.
	c_hvh::get()->AntiAim();
}

void c_client::EndMove(CUserCmd* cmd) {
	// update client-side animations.
	UpdateInformation();

	// if matchmaking mode, anti untrust clamp.
	m_cmd->m_view_angles.SanitizeAngle();

	// fix our movement.
	c_movement::get()->FixMove(cmd, m_strafe_angles);

	// this packet will be sent.
	if (*m_packet) {
		c_hvh::get()->m_step_switch = (bool)c_csgo::get()->RandomInt(0, 1);

		// we are sending a packet, so this will be reset soon.
		// store the old value.
		m_old_lag = m_lag;

		// get radar angles.
		m_radar = cmd->m_view_angles;
		m_radar.normalize();

		// get current origin.
		vec3_t cur = m_local->m_vecOrigin();

		// get prevoius origin.
		vec3_t prev = m_net_pos.empty() ? cur : m_net_pos.front().m_pos;

		// check if we broke lagcomp.
		m_lagcomp = (cur - prev).length_sqr() > 4096.f;

		// save sent origin and time.
		m_net_pos.emplace_front(c_csgo::get()->m_globals->m_curtime, cur);
	}

	// store some values for next tick.
	m_old_packet = *m_packet;
	m_old_shot = m_shot;
}

void c_client::OnTick(CUserCmd* cmd) {
	MouseDelta();

	// store some data and update prediction.
	StartMove(cmd);

	// not much more to do here.
	if (!m_processing)
		return;

	// save the original state of players.
	BackupPlayers(false);

	// run all movement related code.
	DoMove();

	// store stome additonal stuff for next tick
	// sanetize our usercommand if needed and fix our movement.
	EndMove(cmd);

	// restore the players.
	BackupPlayers(true);

	// restore curtime/frametime
	// and prediction seed/player.
	g_inputpred.restore();

	c_aimbot::get()->RunExploit();

	if (c_key_handler::get()->auto_check("exploits_key"))
	{
		if (!c_aimbot::get()->CanUseExploit())
			m_charged = false;
		else if (!m_charged && m_tick_to_recharge == 0) {
			m_tick_to_recharge = shift_value;
		}
	}
}

void c_client::UpdateInformation() {
	if (!c_client::get()->m_local || !c_client::get()->m_processing)
		return;

	CCSGOPlayerAnimState* state = c_client::get()->m_local->m_PlayerAnimState();
	if (!state)
		return;

	if (c_client::get()->m_lag > 0)
		return;

	// update time.
	m_anim_frame = c_csgo::get()->m_globals->m_curtime - m_anim_time;
	m_anim_time = c_csgo::get()->m_globals->m_curtime;

	// save updated data.
	m_speed = state->m_flSpeed;
	m_ground = state->m_bOnGround;
}

void c_client::UpdateAnimations() {
	if (!c_client::get()->m_local || !c_client::get()->m_processing)
		return;

	CCSGOPlayerAnimState* state = c_client::get()->m_local->m_PlayerAnimState();
	if (!state)
		return;

	if (!c_csgo::get()->m_cl)
		return;

	const auto backup_frametime = c_csgo::get()->m_globals->m_frametime;
	const auto backup_curtime = c_csgo::get()->m_globals->m_curtime;

	state->m_flGoalFeetYaw = c_client::get()->m_real_angle.y;

	if (state->m_iLastClientSideAnimationUpdateFramecount == c_csgo::get()->m_globals->m_frame)
		state->m_iLastClientSideAnimationUpdateFramecount -= 1.f;

	static float angle = state->m_flGoalFeetYaw;

	if (c_client::get()->m_local->m_flSimulationTime() != c_client::get()->m_local->m_flOldSimulationTime())
	{
		c_client::get()->m_local->GetAnimLayers(c_client::get()->m_real_layers);

		c_client::get()->m_update = true;
		game::UpdateAnimationState(state, c_client::get()->m_angle);
		c_client::get()->m_local->UpdateClientSideAnimation();
		c_client::get()->m_update = false;

		angle = state->m_flGoalFeetYaw;

		c_client::get()->m_local->SetAnimLayers(c_client::get()->m_real_layers);
		c_client::get()->m_local->GetPoseParameters(c_client::get()->m_real_poses);
	}
	state->m_flGoalFeetYaw = angle;
}

void c_client::print(const std::string text, ...) {
	va_list     list;
	int         size;
	std::string buf;

	if (text.empty())
		return;

	va_start(list, text);

	// count needed size.
	size = std::vsnprintf(0, 0, text.c_str(), list);

	// allocate.
	buf.resize(size);

	// print to buffer.
	std::vsnprintf(buf.data(), size + 1, text.c_str(), list);

	va_end(list);

	// print to console.
	c_csgo::get()->m_cvar->ConsoleColorPrintf(c_config::get()->imcolor_to_ccolor(c_config::get()->c["menu_color"]), XOR("[skeet.idb] "));
	c_csgo::get()->m_cvar->ConsoleColorPrintf(colors::white, buf.c_str());
}

bool c_client::CanFireWeapon(float curtime) {
	// the player cant fire.
	if (!m_player_fire)
		return false;

	if (m_weapon->IsGrenade())
		return false;

	// if we have no bullets, we cant shoot.
	if (m_weapon_type != WEAPONTYPE_KNIFE && m_weapon->m_iClip1() < 1)
		return false;

	// do we have any burst shots to handle?
	if ((m_weapon_id == GLOCK || m_weapon_id == FAMAS) && m_weapon->m_iBurstShotsRemaining() > 0) {
		// new burst shot is coming out.
		if (curtime >= m_weapon->m_fNextBurstShot())
			return true;
	}

	// r8 revolver.
	if (m_weapon_id == REVOLVER) {
		// mouse1.
		if (m_revolver_fire) {
			return true;
		}
		else {
			return false;
		}
	}

	// yeez we have a normal gun.
	if (curtime >= m_weapon->m_flNextPrimaryAttack())
		return true;

	return false;
}

bool c_client::IsFiring(float curtime) {
	const auto weapon = c_client::get()->m_weapon;
	if (!weapon)
		return false;

	const auto IsZeus = m_weapon_id == Weapons_t::ZEUS;
	const auto IsKnife = !IsZeus && m_weapon_type == WEAPONTYPE_KNIFE;

	if (weapon->IsGrenade())
		return !weapon->m_bPinPulled() && weapon->m_fThrowTime() > 0.f && weapon->m_fThrowTime() < curtime;
	else if (IsKnife)
		return (c_client::get()->m_cmd->m_buttons & (IN_ATTACK) || c_client::get()->m_cmd->m_buttons & (IN_ATTACK2)) && CanFireWeapon(curtime);
	else
		return c_client::get()->m_cmd->m_buttons & (IN_ATTACK) && CanFireWeapon(curtime);
}

void c_client::UpdateRevolverCock() {
	if (m_weapon_id != REVOLVER)
		return;

	static auto last_checked = 0;
	static auto last_spawn_time = 0.f;
	static auto tick_cocked = 0;
	static auto tick_strip = 0;

	const auto max_ticks = game::TIME_TO_TICKS(.25f) - 1;
	const auto tick_base = game::TIME_TO_TICKS(c_csgo::get()->m_globals->m_curtime);

	if (m_local->m_flSpawnTime() != last_spawn_time) {
		last_spawn_time = m_local->m_flSpawnTime();
		tick_cocked = tick_base;
		tick_strip = tick_base - max_ticks - 1;
	}

	if (m_weapon->m_flNextPrimaryAttack() > c_csgo::get()->m_globals->m_curtime) {
		m_cmd->m_buttons &= ~IN_ATTACK;
		m_revolver_fire = false;
		return;
	}

	if (last_checked == tick_base)
		return;

	last_checked = tick_base;
	m_revolver_fire = false;

	if (tick_base - tick_strip > 2 && tick_base - tick_strip < 14)
		m_revolver_fire = true;

	if (m_cmd->m_buttons & IN_ATTACK && m_revolver_fire)
		return;

	m_cmd->m_buttons |= IN_ATTACK;

	if (m_weapon->m_flNextSecondaryAttack() >= c_csgo::get()->m_globals->m_curtime)
		m_cmd->m_buttons |= IN_ATTACK2;

	if (tick_base - tick_cocked > max_ticks * 2 + 1) {
		tick_cocked = tick_base;
		tick_strip = tick_base - max_ticks - 1;
	}

	const auto cock_limit = tick_base - tick_cocked >= max_ticks;
	const auto after_strip = tick_base - tick_strip <= max_ticks;

	if (cock_limit || after_strip) {
		tick_cocked = tick_base;
		m_cmd->m_buttons &= ~IN_ATTACK;

		if (cock_limit)
			tick_strip = tick_base;
	}
}

void c_client::UpdateIncomingSequences() {
	if (!c_csgo::get()->m_net)
		return;

	if (m_sequences.empty() || c_csgo::get()->m_net->m_in_seq > m_sequences.front().m_seq) {
		// store new stuff.
		m_sequences.emplace_front(c_csgo::get()->m_globals->m_realtime, c_csgo::get()->m_net->m_in_rel_state, c_csgo::get()->m_net->m_in_seq);
	}

	// do not save too many of these.
	while (m_sequences.size() > 2048)
		m_sequences.pop_back();
}