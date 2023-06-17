#include "includes.h"

Listener g_listener{};;

void events::round_start(IGameEvent* evt) {
	// new round has started. no longer round end.
	c_client::get()->m_round_end = false;

	// remove notices.
	c_client::get()->m_round_flags = 0;

	// reset hvh / aa stuff.
	c_hvh::get()->m_next_random_update = 0.f;
	c_hvh::get()->m_auto_last = 0.f;

	// reset bomb stuff.
	c_visuals::get()->m_c4_planted = false;
	c_visuals::get()->m_planted_c4 = nullptr;

	// reset dormant esp.
	c_visuals::get()->m_draw.fill(false);
	c_visuals::get()->m_opacities.fill(0.f);
	c_visuals::get()->m_offscreen_damage.fill(OffScreenDamageData_t());

	//// buybot.
	//{
	//	auto buy1 = g_menu.main.misc.buy1.GetActiveItems( );
	//	auto buy2 = g_menu.main.misc.buy2.GetActiveItems( );
	//	auto buy3 = g_menu.main.misc.buy3.GetActiveItems( );

	//	for( auto it = buy1.begin( ); it != buy1.end( ); ++it )
	//		c_csgo::get()->m_engine->ExecuteClientCmd( tfm::format( XOR( "buy %s" ), *it ).data( ) );

	//	for( auto it = buy2.begin( ); it != buy2.end( ); ++it )
	//		c_csgo::get()->m_engine->ExecuteClientCmd( tfm::format( XOR( "buy %s" ), *it ).data( ) );

	//	for( auto it = buy3.begin( ); it != buy3.end( ); ++it )
	//		c_csgo::get()->m_engine->ExecuteClientCmd( tfm::format( XOR( "buy %s" ), *it ).data( ) );
	//}c_client::get()->m_cmds

	// update all players.
	for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
		Player* player = c_csgo::get()->m_entlist->GetClientEntity< Player* >(i);
		if (!player || player->m_bIsLocalPlayer())
			continue;

		AimPlayer* data = &c_aimbot::get()->m_players[i - 1];
		data->OnRoundStart(player);
	}

	// reset cmds for packet start
	//c_client::get()->m_cmds.clear();

	c_shots::get()->m_hits.clear();
	c_shots::get()->m_impacts.clear();
	c_shots::get()->m_shots.clear();

	// clear origins.
	c_client::get()->m_net_pos.clear();
}

void events::round_end(IGameEvent* evt) {
	if (!c_client::get()->m_local)
		return;

	// get the reason for the round end.
	int reason = evt->m_keys->FindKey(HASH("reason"))->GetInt();

	// reset.
	c_client::get()->m_round_end = false;

	if (c_client::get()->m_local->m_iTeamNum() == TEAM_COUNTERTERRORISTS && reason == CSRoundEndReason::CT_WIN)
		c_client::get()->m_round_end = true;

	else if (c_client::get()->m_local->m_iTeamNum() == TEAM_TERRORISTS && reason == CSRoundEndReason::T_WIN)
		c_client::get()->m_round_end = true;
}

void events::player_hurt(IGameEvent* evt) {
	int attacker, victim;

	// forward event to resolver / shots hurt processing.
	// c_resolver::get()->hurt( evt );
	c_shots::get()->OnHurt(evt);

	// offscreen esp damage stuff.
	if (evt) {
		attacker = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("attacker"))->GetInt());
		victim = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

		// a player damaged the local player.
		if (attacker > 0 && attacker < 64 && victim == c_csgo::get()->m_engine->GetLocalPlayer())
			c_visuals::get()->m_offscreen_damage[attacker] = { 3.f, 0.f, colors::red };
	}
}

void events::bullet_impact(IGameEvent* evt) {
	// forward event to resolver impact processing.
	c_shots::get()->OnImpact(evt);
}

void events::item_purchase(IGameEvent* evt) {
	int           team, purchaser;
	player_info_t info;

	if (!c_client::get()->m_local || !evt)
		return;

	if (!c_config::get()->m["misc_notifications"][2])
		return;

	// only log purchases of the enemy team.
	team = evt->m_keys->FindKey(HASH("team"))->GetInt();
	if (team == c_client::get()->m_local->m_iTeamNum())
		return;

	// get the player that did the purchase.
	purchaser = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

	// get player info of purchaser.
	if (!c_csgo::get()->m_engine->GetPlayerInfo(purchaser, &info))
		return;

	std::string weapon = evt->m_keys->FindKey(HASH("weapon"))->m_string;
	if (weapon == XOR("weapon_unknown"))
		return;

	std::string out = tfm::format(XOR("%s bought %s\n"), std::string{ info.m_name }.substr(0, 24), weapon);
	c_event_logs::get()->add(out);
}

void events::player_death(IGameEvent* evt) {
	// get index of player that died.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

	// reset opacity scale.
	c_visuals::get()->m_opacities[index] = 0.f;
	c_visuals::get()->m_draw[index] = false;
}

void events::player_given_c4(IGameEvent* evt) {
	player_info_t info;

	if (!c_config::get()->m["misc_notifications"][3])
		return;

	// get the player who received the bomb.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (index == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	if (!c_csgo::get()->m_engine->GetPlayerInfo(index, &info))
		return;

	std::string out = tfm::format(XOR("%s received the bomb\n"), std::string{ info.m_name }.substr(0, 24));
	c_event_logs::get()->add(out);
}

void events::bomb_beginplant(IGameEvent* evt) {
	player_info_t info;

	if (!c_config::get()->m["misc_notifications"][3])
		return;

	// get the player who played the bomb.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (index == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	// get player info of purchaser.
	if (!c_csgo::get()->m_engine->GetPlayerInfo(index, &info))
		return;

	std::string out = tfm::format(XOR("%s started planting the bomb\n"), std::string{ info.m_name }.substr(0, 24));
	c_event_logs::get()->add(out);
}

void events::bomb_abortplant(IGameEvent* evt) {
	player_info_t info;

	if (!c_config::get()->m["misc_notifications"][3])
		return;

	// get the player who stopped planting the bomb.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (index == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	// get player info of purchaser.
	if (!c_csgo::get()->m_engine->GetPlayerInfo(index, &info))
		return;

	std::string out = tfm::format(XOR("%s stopped planting the bomb\n"), std::string{ info.m_name }.substr(0, 24));
	c_event_logs::get()->add(out);
}

void events::bomb_planted(IGameEvent* evt) {
	Entity* bomb_target;
	std::string   site_name;
	int           player_index;
	player_info_t info;
	std::string   out;

	// get the func_bomb_target entity and store info about it.
	bomb_target = c_csgo::get()->m_entlist->GetClientEntity(evt->m_keys->FindKey(HASH("site"))->GetInt());
	if (bomb_target) {
		site_name = bomb_target->GetBombsiteName();
		c_visuals::get()->m_last_bombsite = site_name;
	}

	if (!c_config::get()->m["misc_notifications"][3])
		return;

	player_index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (player_index == c_csgo::get()->m_engine->GetLocalPlayer())
		out = tfm::format(XOR("you planted the bomb at %s\n"), site_name.c_str());

	else {
		c_csgo::get()->m_engine->GetPlayerInfo(player_index, &info);

		out = tfm::format(XOR("the bomb was planted at %s by %s\n"), site_name.c_str(), std::string(info.m_name).substr(0, 24));
	}

	c_event_logs::get()->add(out);
}

void events::bomb_beep(IGameEvent* evt) {
	Entity* c4;
	vec3_t             explosion_origin, explosion_origin_adjusted;
	CTraceFilterSimple filter;
	CGameTrace         tr;

	// we have a bomb ent already, don't do anything else.
	if (c_visuals::get()->m_c4_planted)
		return;

	// bomb_beep is called once when a player plants the c4 and contains the entindex of the C4 weapon itself, we must skip that here.
	c4 = c_csgo::get()->m_entlist->GetClientEntity(evt->m_keys->FindKey(HASH("entindex"))->GetInt());
	if (!c4 || !c4->is(HASH("CPlantedC4")))
		return;

	// planted bomb is currently active, grab some extra info about it and set it for later.
	c_visuals::get()->m_c4_planted = true;
	c_visuals::get()->m_planted_c4 = c4;
	c_visuals::get()->m_planted_c4_explode_time = c4->m_flC4Blow();

	// the bomb origin is adjusted slightly inside CPlantedC4::C4Think, right when it's about to explode.
	// we're going to do that here.
	explosion_origin = c4->GetAbsOrigin();
	explosion_origin_adjusted = explosion_origin;
	explosion_origin_adjusted.z += 8.f;

	// setup filter and do first trace.
	filter.SetPassEntity(c4);

	c_csgo::get()->m_engine_trace->TraceRay(
		Ray(explosion_origin_adjusted, explosion_origin_adjusted + vec3_t(0.f, 0.f, -40.f)),
		MASK_SOLID,
		&filter,
		&tr
	);

	// pull out of the wall a bit.
	if (tr.m_fraction != 1.f)
		explosion_origin = tr.m_endpos + (tr.m_plane.m_normal * 0.6f);

	// this happens inside CCSGameRules::RadiusDamage.
	explosion_origin.z += 1.f;

	// set all other vars.
	c_visuals::get()->m_planted_c4_explosion_origin = explosion_origin;

	// todo - dex;  get this radius dynamically... seems to only be available in map bsp file, search string: "info_map_parameters"
	//              info_map_parameters is an entity created on the server, it doesnt seem to have many useful networked vars for clients.
	//
	//              swapping maps between de_dust2 and de_nuke and scanning for 500 and 400 float values will leave you one value.
	//              need to figure out where it's written from.
	//
	// server.dll uses starting 'radius' as damage... the real radius passed to CCSGameRules::RadiusDamage is actually multiplied by 3.5.
	c_visuals::get()->m_planted_c4_damage = 500.f;
	c_visuals::get()->m_planted_c4_radius = c_visuals::get()->m_planted_c4_damage * 3.5f;
	c_visuals::get()->m_planted_c4_radius_scaled = c_visuals::get()->m_planted_c4_radius / 3.f;
}

void events::bomb_begindefuse(IGameEvent* evt) {
	player_info_t info;

	if (!c_config::get()->m["misc_notifications"][4])
		return;

	// get index of player that started defusing the bomb.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (index == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	if (!c_csgo::get()->m_engine->GetPlayerInfo(index, &info))
		return;

	bool kit = evt->m_keys->FindKey(HASH("haskit"))->GetBool();

	if (kit) {
		std::string out = tfm::format(XOR("%s started defusing with a kit\n"), std::string(info.m_name).substr(0, 24));
		c_event_logs::get()->add(out);
	}

	else {
		std::string out = tfm::format(XOR("%s started defusing without a kit\n"), std::string(info.m_name).substr(0, 24));
		c_event_logs::get()->add(out);
	}
}

void events::bomb_abortdefuse(IGameEvent* evt) {
	player_info_t info;

	if (!c_config::get()->m["misc_notifications"][4])
		return;

	// get index of player that stopped defusing the bomb.
	int index = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (index == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	if (!c_csgo::get()->m_engine->GetPlayerInfo(index, &info))
		return;

	std::string out = tfm::format(XOR("%s stopped defusing\n"), std::string(info.m_name).substr(0, 24));
	c_event_logs::get()->add(out);
}

void events::bomb_exploded(IGameEvent* evt) {
	c_visuals::get()->m_c4_planted = false;
	c_visuals::get()->m_planted_c4 = nullptr;
}

void events::bomb_defused(IGameEvent* evt) {
	c_visuals::get()->m_c4_planted = false;
	c_visuals::get()->m_planted_c4 = nullptr;
}

void events::round_freeze_end(IGameEvent* evt) {
	c_client::get()->m_round_flags = 1;
}

void events::client_disconnect(IGameEvent* evt) {
	c_client::get()->m_round_flags = 0;
}

void events::weapon_fire(IGameEvent* evt)
{
	c_shots::get()->OnFire(evt);
}

void Listener::init() {
	// link events with callbacks.
	add(XOR("round_start"), events::round_start);
	add(XOR("round_end"), events::round_end);
	add(XOR("player_hurt"), events::player_hurt);
	add(XOR("bullet_impact"), events::bullet_impact);
	add(XOR("item_purchase"), events::item_purchase);
	add(XOR("player_death"), events::player_death);
	add(XOR("player_given_c4"), events::player_given_c4);
	add(XOR("bomb_beginplant"), events::bomb_beginplant);
	add(XOR("bomb_abortplant"), events::bomb_abortplant);
	add(XOR("bomb_planted"), events::bomb_planted);
	add(XOR("bomb_begindefuse"), events::bomb_begindefuse);
	add(XOR("bomb_abortdefuse"), events::bomb_abortdefuse);
	add(XOR("bomb_exploded"), events::bomb_exploded);
	add(XOR("bomb_defused"), events::bomb_defused);
	add(XOR("round_freeze_end"), events::round_freeze_end);
	add(XOR("client_disconnect"), events::client_disconnect);
	add(XOR("weapon_fire"), events::weapon_fire);

	register_events();
}