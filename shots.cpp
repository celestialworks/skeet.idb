#include "includes.h"

void c_shots::OnShotFire(Player* target, float damage, int bullets, LagRecord* record, int hitbox) {
	// setup new shot data.
	ShotRecord shot;
	shot.m_target = target;
	shot.m_record = record;
	shot.m_time = c_csgo::get()->m_globals->m_curtime;
	shot.m_lat = c_client::get()->m_latency;
	shot.m_damage = damage;
	shot.m_pos = c_client::get()->m_shoot_pos;
	shot.m_hitbox = hitbox;
	shot.m_impacted = false;
	shot.m_confirmed = false;
	shot.m_hurt = false;
	shot.m_weapon_range = c_client::get()->m_weapon_info->flRange;

	// we are not shooting manually.
	// and this is the first bullet, only do this once.
	if (target && record) {
		// increment total shots on this player.
		AimPlayer* data = &c_aimbot::get()->m_players[target->index() - 1];
		if (data) {
			++data->m_shots;

			auto matrix = record->m_bones;

			if (matrix)
				shot.m_matrix = matrix;
		}
	}

	// add to tracks.
	m_shots.push_front(shot);

	// no need to keep an insane amount of shots.
	while (m_shots.size() > 128)
		m_shots.pop_back();
}

void c_shots::OnImpact(IGameEvent* evt) {
	int        attacker;
	vec3_t     pos, dir, start, end;
	float      time;

	// screw this.
	if (!evt || !c_client::get()->m_local)
		return;

	// get attacker, if its not us, screw it.
	attacker = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (attacker != c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	// decode impact coordinates and convert to vec3.
	pos = {
		evt->m_keys->FindKey(HASH("x"))->GetFloat(),
		evt->m_keys->FindKey(HASH("y"))->GetFloat(),
		evt->m_keys->FindKey(HASH("z"))->GetFloat()
	};

	// get prediction time at this point.
	time = c_csgo::get()->m_globals->m_curtime;

	// add to visual impacts if we have features that rely on it enabled.
	// todo - dex; need to match shots for this to have proper Weapon_ShootPosition, don't really care to do it anymore.
	if (c_config::get()->b["visuals_impact_beams"])
		m_vis_impacts.push_back({ pos, c_client::get()->m_local->GetShootPosition(), c_client::get()->m_local->m_nTickBase() });

	// we did not take a shot yet.
	if (m_shots.empty())
		return;

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'bullet_impact' event.
		if (s.m_impacted)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(time - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	// this shot was matched.
	shot->m_impacted = true;
	shot->m_server_pos = pos;

	// create new impact instance that we can match with a player hurt.
	ImpactRecord impact;
	impact.m_shot = shot;
	impact.m_tick = c_client::get()->m_local->m_nTickBase();
	impact.m_pos = pos;

	// add to track.
	m_impacts.push_front(impact);

	// no need to keep an insane amount of impacts.
	while (m_impacts.size() > 128)
		m_impacts.pop_back();
}

void c_shots::OnHurt(IGameEvent* evt) {
	int         attacker, victim, group, hp;
	float       damage;
	std::string name;

	if (!evt || !c_client::get()->m_local)
		return;

	attacker = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("attacker"))->GetInt());
	victim = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());

	// skip invalid player indexes.
	// should never happen? world entity could be attacker, or a nade that hits you.
	if (attacker < 1 || attacker > 64 || victim < 1 || victim > 64)
		return;

	// we were not the attacker or we hurt ourselves.
	else if (attacker != c_csgo::get()->m_engine->GetLocalPlayer() || victim == c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	// get hitgroup.
	// players that get naded ( DMG_BLAST ) or stabbed seem to be put as HITGROUP_GENERIC.
	group = evt->m_keys->FindKey(HASH("hitgroup"))->GetInt();

	// invalid hitgroups ( note - dex; HITGROUP_GEAR isn't really invalid, seems to be set for hands and stuff? ).
	if (group == HITGROUP_GEAR)
		return;

	// get the player that was hurt.
	Player* target = c_csgo::get()->m_entlist->GetClientEntity< Player* >(victim);
	if (!target)
		return;

	// get player info.
	player_info_t info;
	if (!c_csgo::get()->m_engine->GetPlayerInfo(victim, &info))
		return;

	// get player name;
	name = std::string(info.m_name).substr(0, 24);

	// get damage reported by the server.
	damage = (float)evt->m_keys->FindKey(HASH("dmg_health"))->GetInt();

	// get remaining hp.
	hp = evt->m_keys->FindKey(HASH("health"))->GetInt();

	// hitmarker.
	if (c_config::get()->b["visuals_hitmarker"]) {
		c_visuals::get()->m_hit_duration = 1.f;
		c_visuals::get()->m_hit_start = c_csgo::get()->m_globals->m_curtime;
		c_visuals::get()->m_hit_end = c_visuals::get()->m_hit_start + c_visuals::get()->m_hit_duration;

		c_csgo::get()->m_sound->EmitAmbientSound(XOR("buttons/arena_switch_press_02.wav"), 1.f);
	}

	// print this shit.
	if (c_config::get()->m["misc_notifications"][1]) {
		std::string out = tfm::format(XOR("Hit %s in the %s for %i damage (%i health remaining)\n"), name, m_groups[group], (int)damage, hp);
		c_event_logs::get()->add(out);
	}

	if (group == HITGROUP_GENERIC)
		return;

	// if we hit a player, mark vis impacts.
	if (!m_vis_impacts.empty()) {
		for (auto& i : m_vis_impacts) {
			if (i.m_tickbase == c_client::get()->m_local->m_nTickBase())
				i.m_hit_player = true;
		}
	}

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'player_hurt' event.
		if (s.m_hurt)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(c_csgo::get()->m_globals->m_curtime - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	shot->m_hurt = true;
}

void c_shots::OnFire(IGameEvent* evt)
{
	int attacker;

	// screw this.
	if (!evt || !c_client::get()->m_local)
		return;

	// get attacker, if its not us, screw it.
	attacker = c_csgo::get()->m_engine->GetPlayerForUserID(evt->m_keys->FindKey(HASH("userid"))->GetInt());
	if (attacker != c_csgo::get()->m_engine->GetLocalPlayer())
		return;

	struct ShotMatch_t { float delta; ShotRecord* shot; };
	ShotMatch_t match;
	match.delta = std::numeric_limits< float >::max();
	match.shot = nullptr;

	// iterate all shots.
	for (auto& s : m_shots) {

		// this shot was already matched
		// with a 'weapon_fire' event.
		if (s.m_confirmed)
			continue;

		// add the latency to the time when we shot.
		// to predict when we would receive this event.
		float predicted = s.m_time + s.m_lat;

		// get the delta between the current time
		// and the predicted arrival time of the shot.
		float delta = std::abs(c_csgo::get()->m_globals->m_curtime - predicted);

		// fuck this.
		if (delta > 1.f)
			continue;

		// store this shot as being the best for now.
		if (delta < match.delta) {
			match.delta = delta;
			match.shot = &s;
		}
	}

	// no valid shotrecord was found.
	ShotRecord* shot = match.shot;
	if (!shot)
		return;

	shot->m_confirmed = true;
}

void c_shots::OnFrameStage()
{
	if (!c_client::get()->m_processing || m_shots.empty()) {
		if (!m_shots.empty())
			m_shots.clear();

		return;
	}

	for (auto it = m_shots.begin(); it != m_shots.end();) {
		if (it->m_time + 1.f < c_csgo::get()->m_globals->m_curtime)
			it = m_shots.erase(it);
		else
			it = next(it);
	}

	for (auto it = m_shots.begin(); it != m_shots.end();) {
		if (it->m_impacted && !it->m_hurt && it->m_confirmed) {
			// not in nospread mode, see if the shot missed due to spread.
			Player* target = it->m_target;
			if (!target) {
				it = m_shots.erase(it);
				continue;
			}

			// not gonna bother anymore.
			if (!target->alive()) {
				it = m_shots.erase(it);
				continue;
			}

			AimPlayer* data = &c_aimbot::get()->m_players[target->index() - 1];
			if (!data) {
				it = m_shots.erase(it);
				continue;
			}

			// this record was deleted already.
			if (!it->m_record->m_bones) {
				c_event_logs::get()->add(XOR("Missed shot due to invalid target\n"));
				it = m_shots.erase(it);
				continue;
			}

			// start position of trace is where we took the shot.
			vec3_t start = it->m_pos;

			// the impact pos contains the spread from the server
			// which is generated with the server seed, so this is where the bullet
			// actually went, compute the direction of this from where the shot landed
			// and from where we actually took the shot.
			vec3_t dir = (it->m_server_pos - start).normalized();

			// get end pos by extending direction forward.
			// todo; to do this properly should save the weapon range at the moment of the shot, cba..
			vec3_t end = start + dir * start.dist_to(it->m_record->m_origin) * 6.6f;

			if (!c_aimbot::get()->CanHit(start, end, it->m_record, it->m_hitbox, true, it->m_matrix)) {
				c_event_logs::get()->add(XOR("Missed shot due to spread\n"));

				//c_csgo::get()->m_debug_overlay->AddLineOverlay(start, end, 255, 255, 255, false, 3.f);

				it = m_shots.erase(it);

				continue;
			}

			// is player a bot?
			bool bot = game::IsFakePlayer(target->index());

			// let's not increment this if this is a shot record.
			if (!it->m_record->m_shot && !bot)
			{
				c_event_logs::get()->add(XOR("Missed shot due to resolver\n"));
				++data->m_missed_shots;

				it = m_shots.erase(it);

				continue;
			}

			c_event_logs::get()->add(XOR("Missed shot due to wrong angles calculation\n"));

			// we processed this shot, let's delete it.
			it = m_shots.erase(it);
		}
		else {
			it = next(it);
		}
	}
}