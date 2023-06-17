#include "includes.h"

void Hooks::LevelInitPreEntity( const char* map ) {
	float rate{ 1.f / c_csgo::get()->m_globals->m_interval };

	// set rates when joining a server.
	c_csgo::get()->cl_updaterate->SetValue( rate );
	c_csgo::get()->cl_cmdrate->SetValue( rate );

	c_aimbot::get()->reset( );
	c_visuals::get()->m_hit_start = c_visuals::get()->m_hit_end = c_visuals::get()->m_hit_duration = 0.f;

	if (map)
	{
		std::string out = tfm::format(XOR("now playing on: %s\n"), map);
		c_event_logs::get()->add(out);
	}

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t >( CHLClient::LEVELINITPREENTITY )( this, map );
}

void Hooks::LevelInitPostEntity( ) {
	c_client::get()->OnMapload( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t >( CHLClient::LEVELINITPOSTENTITY )( this );
}

void Hooks::LevelShutdown( ) {
	c_aimbot::get()->reset( );

	c_client::get()->m_local       = nullptr;
	c_client::get()->m_weapon      = nullptr;
	c_client::get()->m_processing  = false;
	c_client::get()->m_weapon_info = nullptr;
	c_client::get()->m_round_end   = false;

	c_client::get()->m_sequences.clear( );
	//c_client::get()->m_cmds.clear( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t >( CHLClient::LEVELSHUTDOWN )( this );
}

void WriteUsercmd(bf_write* buf, CUserCmd* in, CUserCmd* out)
{
	static auto WriteUsercmdF = pattern::find(c_csgo::get()->m_client_dll, XOR("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

	__asm
	{
		mov ecx, buf
		mov edx, in
		push out
		call WriteUsercmdF
		add esp, 4
	}
}

bool Hooks::WriteUsercmdDeltaToBuffer(int slot, bf_write* buf, int from, int to, bool isnewcommand)
{
	if (c_client::get()->m_processing && c_csgo::get()->m_engine->IsConnected() && c_csgo::get()->m_engine->IsInGame()) {
		if (c_csgo::get()->m_gamerules->m_bFreezePeriod())
			return g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(CHLClient::USRCMDTODELTABUFFER)(this, slot, buf, from, to, isnewcommand);

		if (c_client::get()->m_tick_to_shift <= 0 || c_csgo::get()->m_cl->m_choked_commands > 3)
			return g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(CHLClient::USRCMDTODELTABUFFER)(this, slot, buf, from, to, isnewcommand);

		if (from != -1)
			return true;

		uintptr_t stackbase;
		__asm mov stackbase, ebp;
		CCLCMsg_Move_t* msg = reinterpret_cast<CCLCMsg_Move_t*>(stackbase + 0xFCC);
		auto net_channel = c_csgo::get()->m_cl->m_net_channel;
		int32_t new_commands = msg->new_commands;

		int32_t next_cmdnr = c_csgo::get()->m_cl->m_last_outgoing_command + c_csgo::get()->m_cl->m_choked_commands + 1;
		int32_t total_new_commands = std::min(c_client::get()->m_tick_to_shift, 62);
		c_client::get()->m_tick_to_shift -= total_new_commands;

		from = -1;
		msg->new_commands = total_new_commands;
		msg->backup_commands = 0;

		for (to = next_cmdnr - new_commands + 1; to <= next_cmdnr; to++) {
			if (!g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(CHLClient::USRCMDTODELTABUFFER)(this, slot, buf, from, to, isnewcommand))
				return false;

			from = to;
		}

		CUserCmd* last_realCmd = c_csgo::get()->m_input->GetUserCmd(slot, from);
		CUserCmd fromCmd;

		if (last_realCmd)
			fromCmd = *last_realCmd;

		CUserCmd toCmd = fromCmd;
		toCmd.m_command_number++;
		//toCmd.m_tick += c_client::get()->m_rate * 3;
		// note: simv0l - tickrate * 3 is not accurate.
		toCmd.m_tick++;

		for (int i = new_commands; i <= total_new_commands; i++) {
			WriteUsercmd(buf, &toCmd, &fromCmd);
			fromCmd = toCmd;
			toCmd.m_command_number++;
			toCmd.m_tick++;
		}
		return true;

	}
	else
		return g_hooks.m_client.GetOldMethod< WriteUsercmdDeltaToBuffer_t >(CHLClient::USRCMDTODELTABUFFER)(this, slot, buf, from, to, isnewcommand);
}

void Hooks::FrameStageNotify( Stage_t stage ) {
	// save stage.
	if( stage != FRAME_START )
		c_client::get()->m_stage = stage;

	// damn son.
	c_client::get()->m_local = c_csgo::get()->m_entlist->GetClientEntity< Player* >( c_csgo::get()->m_engine->GetLocalPlayer( ) );

	if ( stage == FRAME_NET_UPDATE_END ) {
		c_client::get()->UpdateInformation();
		c_client::get()->UpdateAnimations();
	}
	else if ( stage == FRAME_RENDER_START ) {
		if (c_client::get()->m_local && c_client::get()->m_local->alive()) {
			if (c_csgo::get()->m_input->m_camera_in_third_person)
				c_csgo::get()->m_prediction->SetLocalViewAngles(c_client::get()->m_angle);

			c_client::get()->m_local->SetPoseParameters(c_client::get()->m_real_poses);

			// note: simv0l - idk why we need this
			//if (g_menu.main.config.mode.get() == 0 && (c_client::get()->m_local->m_fFlags() & FL_ONGROUND)) {
			//	c_client::get()->m_local->m_PlayerAnimState()->m_bOnGround = true;
			//	c_client::get()->m_local->m_PlayerAnimState()->m_szInHitGroundAnimation = false;
			//}
			c_client::get()->m_local->SetAbsAngles(ang_t(0, c_client::get()->m_local->m_PlayerAnimState()->m_flGoalFeetYaw, 0));
		}

		// draw our custom beams.
		c_visuals::get()->DrawBeams();

		// handle our shots.
		c_shots::get()->OnFrameStage();
	}

	if (c_config::get()->b["fog_override"]) {
		c_csgo::get()->m_cvar->FindVar(HASH("fog_enable"))->SetValue("1");
		c_csgo::get()->m_cvar->FindVar(HASH("fog_override"))->SetValue("0");
		c_csgo::get()->m_cvar->FindVar(HASH("fog_start"))->SetValue(c_config::get()->i["fog_start"]);
		c_csgo::get()->m_cvar->FindVar(HASH("fog_end"))->SetValue(c_config::get()->i["fog_end"]);
		c_csgo::get()->m_cvar->FindVar(HASH("fog_maxdensity"))->SetValue(1);
		c_csgo::get()->m_cvar->FindVar(HASH("fog_color"))->SetValue("56 45 255");
	}
	else {
		c_csgo::get()->m_cvar->FindVar(HASH("fog_enable"))->SetValue("1");
		c_csgo::get()->m_cvar->FindVar(HASH("fog_override"))->SetValue("1");
	}

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	if (stage == FRAME_RENDER_START) {
		// get tickrate.
		c_client::get()->m_rate = (int)std::round(1.f / c_csgo::get()->m_globals->m_interval);

	}

	else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START) {
		// run our clantag changer.
		c_client::get()->ClanTag();
	}

	else if( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END ) {
		c_visuals::get()->NoSmoke( );
	}

	else if( stage == FRAME_NET_UPDATE_END ) {
        // restore non-compressed netvars.
		g_netdata.apply( );

		// update all players.
		for( int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i ) {
			Player* player = c_csgo::get()->m_entlist->GetClientEntity< Player* >( i );
			if( !player || player->m_bIsLocalPlayer( ) )
				continue;

			AimPlayer* data = &c_aimbot::get()->m_players[ i - 1 ];
			data->OnNetUpdate( player );
		}
	}
}