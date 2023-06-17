#include "includes.h"

void c_hvh::IdealPitch() {
	CCSGOPlayerAnimState *state = c_client::get()->m_local->m_PlayerAnimState();
	if (!state)
		return;

	c_client::get()->m_cmd->m_view_angles.x = state->m_min_pitch;
}

void c_hvh::AntiAimPitch() {
	bool safe = true;

	switch (m_pitch) {
	case 1:
		// down.
		c_client::get()->m_cmd->m_view_angles.x = safe ? 89.f : 720.f;
		break;

	case 2:
		// up.
		c_client::get()->m_cmd->m_view_angles.x = safe ? -89.f : -720.f;
		break;

	case 3:
		// random.
		c_client::get()->m_cmd->m_view_angles.x = c_csgo::get()->RandomFloat(safe ? -89.f : -720.f, safe ? 89.f : 720.f);
		break;

	case 4:
		// ideal.
		IdealPitch();
		break;

	default:
		break;
	}
}

void c_hvh::AutoDirection() {
	// constants.
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 32.f };

	// best target.
	struct AutoTarget_t { float fov; Player *player; };
	AutoTarget_t target{ 180.f + 1.f, nullptr };

	// iterate players.
	for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
		Player *player = c_csgo::get()->m_entlist->GetClientEntity< Player * >(i);

		// validate player.
		if (!c_aimbot::get()->IsValidTarget(player))
			continue;

		// skip dormant players.
		if (player->dormant())
			continue;

		// get best target based on fov.
		float fov = math::GetFOV(c_client::get()->m_view_angles, c_client::get()->m_shoot_pos, player->WorldSpaceCenter());

		if (fov < target.fov) {
			target.fov = fov;
			target.player = player;
		}
	}

	if (!target.player) {
		// we have a timeout.
		if (m_auto_last > 0.f && m_auto_time > 0.f && c_csgo::get()->m_globals->m_curtime < (m_auto_last + m_auto_time))
			return;

		// set angle to backwards.
		m_auto = math::NormalizedAngle(m_view - 180.f);
		m_auto_dist = -1.f;
		return;
	}

	/*
	* data struct
	* 68 74 74 70 73 3a 2f 2f 73 74 65 61 6d 63 6f 6d 6d 75 6e 69 74 79 2e 63 6f 6d 2f 69 64 2f 73 69 6d 70 6c 65 72 65 61 6c 69 73 74 69 63 2f
	*/

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back(m_view - 180.f);
	angles.emplace_back(m_view + 90.f);
	angles.emplace_back(m_view - 90.f);

	// start the trace at the enemy shoot pos.
	vec3_t start = target.player->GetShootPosition();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ c_client::get()->m_shoot_pos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			c_client::get()->m_shoot_pos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			c_client::get()->m_shoot_pos.z };

		// draw a line for debugging purposes.
		//c_csgo::get()->m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = c_csgo::get()->m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid) {
		// set angle to backwards.
		m_auto = math::NormalizedAngle(m_view - 180.f);
		m_auto_dist = -1.f;
		return;
	}

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle &a, const AdaptiveAngle &b) {
		return a.m_dist > b.m_dist;
	});

	// the best angle should be at the front now.
	AdaptiveAngle *best = &angles.front();

	// check if we are not doing a useless change.
	if (best->m_dist != m_auto_dist) {
		// set yaw to the best result.
		m_auto = math::NormalizedAngle(best->m_yaw);
		m_auto_dist = best->m_dist;
		m_auto_last = c_csgo::get()->m_globals->m_curtime;
	}
}

void c_hvh::GetAntiAimDirection() {
	// edge aa.
	if (c_config::get()->b["rbot_aa_edge"] && c_client::get()->m_local->m_vecVelocity().length() < 320.f) {

		ang_t ang;
		if (DoEdgeAntiAim(c_client::get()->m_local, ang)) {
			m_direction = ang.y;
			return;
		}
	}

	// lock while standing..
	bool lock = c_config::get()->b["rbot_aa_lock"];

	// save view, depending if locked or not.
	if ((lock && c_client::get()->m_speed > 0.1f) || !lock)
		m_view = c_client::get()->m_cmd->m_view_angles.y;

	if (m_base_angle > 0) {
		// 'static'.
		if (m_base_angle == 1)
			m_view = 0.f;

		// away options.
		else {
			float  best_fov{ std::numeric_limits< float >::max() };
			float  best_dist{ std::numeric_limits< float >::max() };
			float  fov, dist;
			Player *target, *best_target{ nullptr };

			for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
				target = c_csgo::get()->m_entlist->GetClientEntity< Player * >(i);

				if (!c_aimbot::get()->IsValidTarget(target))
					continue;

				if (target->dormant())
					continue;

				// 'away crosshair'.
				if (m_base_angle == 2) {

					// check if a player was closer to our crosshair.
					fov = math::GetFOV(c_client::get()->m_view_angles, c_client::get()->m_shoot_pos, target->WorldSpaceCenter());
					if (fov < best_fov) {
						best_fov = fov;
						best_target = target;
					}
				}

				// 'away distance'.
				else if (m_base_angle == 3) {

					// check if a player was closer to us.
					dist = (target->m_vecOrigin() - c_client::get()->m_local->m_vecOrigin()).length_sqr();
					if (dist < best_dist) {
						best_dist = dist;
						best_target = target;
					}
				}
			}

			if (best_target) {
				// todo - dex; calculate only the yaw needed for this (if we're not going to use the x component that is).
				ang_t angle;
				math::VectorAngles(best_target->m_vecOrigin() - c_client::get()->m_local->m_vecOrigin(), angle);
				m_view = angle.y;
			}
		}
	}

	// switch direction modes.
	switch (m_dir) {

		// auto.
	case 0:
		AutoDirection();
		m_direction = m_auto;
		break;

		// backwards.
	case 1:
		m_direction = m_view + 180.f;
		break;

		// left.
	case 2:
		m_direction = m_view + 90.f;
		break;

		// right.
	case 3:
		m_direction = m_view - 90.f;
		break;

		// custom.
	case 4:
		m_direction = m_view + m_dir_custom;
		break;

	default:
		break;
	}

	// normalize the direction.
	math::NormalizeAngle(m_direction);
}

bool c_hvh::DoEdgeAntiAim(Player *player, ang_t &out) {
	CGameTrace trace;
	static CTraceFilterSimple_game filter{ };

	if (player->m_MoveType() == MOVETYPE_LADDER)
		return false;

	// skip this player in our traces.
	filter.SetPassEntity(player);

	// get player bounds.
	vec3_t mins = player->m_vecMins();
	vec3_t maxs = player->m_vecMaxs();

	// make player bounds bigger.
	mins.x -= 20.f;
	mins.y -= 20.f;
	maxs.x += 20.f;
	maxs.y += 20.f;

	// get player origin.
	vec3_t start = player->GetAbsOrigin();

	// offset the view.
	start.z += 56.f;

	c_csgo::get()->m_engine_trace->TraceRay(Ray(start, start, mins, maxs), CONTENTS_SOLID, (ITraceFilter *)&filter, &trace);
	if (!trace.m_startsolid)
		return false;

	float  smallest = 1.f;
	vec3_t plane;

	// trace around us in a circle, in 20 steps (anti-degree conversion).
	// find the closest object.
	for (float step{ }; step <= math::twopi; step += (math::pi / 10.f)) {
		// extend endpoint x units.
		vec3_t end = start;

		// set end point based on range and step.
		end.x += std::cos(step) * 32.f;
		end.y += std::sin(step) * 32.f;

		c_csgo::get()->m_engine_trace->TraceRay(Ray(start, end, { -1.f, -1.f, -8.f }, { 1.f, 1.f, 8.f }), CONTENTS_SOLID, (ITraceFilter *)&filter, &trace);

		// we found an object closer, then the previouly found object.
		if (trace.m_fraction < smallest) {
			// save the normal of the object.
			plane = trace.m_plane.m_normal;
			smallest = trace.m_fraction;
		}
	}

	// no valid object was found.
	if (smallest == 1.f || plane.z >= 0.1f)
		return false;

	// invert the normal of this object
	// this will give us the direction/angle to this object.
	vec3_t inv = -plane;
	vec3_t dir = inv;
	dir.normalize();

	// extend point into object by 24 units.
	vec3_t point = start;
	point.x += (dir.x * 24.f);
	point.y += (dir.y * 24.f);

	// check if we can stick our head into the wall.
	if (c_csgo::get()->m_engine_trace->GetPointContents(point, CONTENTS_SOLID) & CONTENTS_SOLID) {
		// trace from 72 units till 56 units to see if we are standing behind something.
		c_csgo::get()->m_engine_trace->TraceRay(Ray(point + vec3_t{ 0.f, 0.f, 16.f }, point), CONTENTS_SOLID, (ITraceFilter *)&filter, &trace);

		// we didnt start in a solid, so we started in air.
		// and we are not in the ground.
		if (trace.m_fraction < 1.f && !trace.m_startsolid && trace.m_plane.m_normal.z > 0.7f) {
			// mean we are standing behind a solid object.
			// set our angle to the inversed normal of this object.
			out.y = math::rad_to_deg(std::atan2(inv.y, inv.x));
			return true;
		}
	}

	// if we arrived here that mean we could not stick our head into the wall.
	// we can still see if we can stick our head behind/asides the wall.

	// adjust bounds for traces.
	mins = { (dir.x * -3.f) - 1.f, (dir.y * -3.f) - 1.f, -1.f };
	maxs = { (dir.x * 3.f) + 1.f, (dir.y * 3.f) + 1.f, 1.f };

	// move this point 48 units to the left 
	// relative to our wall/base point.
	vec3_t left = start;
	left.x = point.x - (inv.y * 48.f);
	left.y = point.y - (inv.x * -48.f);

	c_csgo::get()->m_engine_trace->TraceRay(Ray(left, point, mins, maxs), CONTENTS_SOLID, (ITraceFilter *)&filter, &trace);
	float l = trace.m_startsolid ? 0.f : trace.m_fraction;

	// move this point 48 units to the right 
	// relative to our wall/base point.
	vec3_t right = start;
	right.x = point.x + (inv.y * 48.f);
	right.y = point.y + (inv.x * -48.f);

	c_csgo::get()->m_engine_trace->TraceRay(Ray(right, point, mins, maxs), CONTENTS_SOLID, (ITraceFilter *)&filter, &trace);
	float r = trace.m_startsolid ? 0.f : trace.m_fraction;

	// both are solid, no edge.
	if (l == 0.f && r == 0.f)
		return false;

	// set out to inversed normal.
	out.y = math::rad_to_deg(std::atan2(inv.y, inv.x));

	// left started solid.
	// set angle to the left.
	if (l == 0.f) {
		out.y += 90.f;
		return true;
	}

	// right started solid.
	// set angle to the right.
	if (r == 0.f) {
		out.y -= 90.f;
		return true;
	}

	return false;
}

void c_hvh::DoRealAntiAim() {
	// if we have a yaw antaim.
	if (m_yaw > 0) {

		// if we have a yaw active, which is true if we arrived here.
		// set the yaw to the direction before applying any other operations.
		c_client::get()->m_cmd->m_view_angles.y = m_direction;

		switch (m_yaw) {

			// direction.
		case 1:
			// do nothing, yaw already is direction.
			break;

			// jitter.
		case 2: {

			// get the range from the menu.
			float range = m_jitter_range / 2.f;

			// set angle.
			c_client::get()->m_cmd->m_view_angles.y += c_csgo::get()->RandomFloat(-range, range);
			break;
		}

				// rotate.
		case 3: {
			// set base angle.
			c_client::get()->m_cmd->m_view_angles.y = (m_direction - m_rot_range / 2.f);

			// apply spin.
			c_client::get()->m_cmd->m_view_angles.y += std::fmod(c_csgo::get()->m_globals->m_curtime * (m_rot_speed * 20.f), m_rot_range);

			break;
		}

				// random.
		case 4:
			// check update time.
			if (c_csgo::get()->m_globals->m_curtime >= m_next_random_update) {

				// set new random angle.
				m_random_angle = c_csgo::get()->RandomFloat(-180.f, 180.f);

				// set next update time
				m_next_random_update = c_csgo::get()->m_globals->m_curtime + m_rand_update;
			}

			// apply angle.
			c_client::get()->m_cmd->m_view_angles.y = m_random_angle;
			break;

		default:
			break;
		}
	}
}

void c_hvh::DoFakeAntiAim() {
	// run desync.
	if (c_config::get()->b["rbot_aa_desync"]) {
		// move in the sent commands.
		if (!c_csgo::get()->m_cl->m_choked_commands) {
			// make sure we don't move if we are wanting to move manually.
			if (!c_client::get()->m_pressing_move) {
				static bool negate = false;
				const float velocity = c_client::get()->m_local->m_flDuckAmount() ? 3.25f : 1.01f;

				// set sidemove to make sure lby is always being updated.
				// only do this if we aren't pressing any move keys, or
				// else we won't be able to move and stuff wont go nicely.

				c_client::get()->m_cmd->m_side_move = negate ? velocity : -velocity;

				// move in the negative direction next tick.
				negate = !negate;
			}
		}
		if (!*c_client::get()->m_packet) {
			c_client::get()->m_cmd->m_view_angles.y += c_key_handler::get()->auto_check("desync_invert_key") ? 60.f : -60.f;
		}
	}
}

void c_hvh::AntiAim() {
	bool attack, attack2;

	m_should_work = false;

	if (!c_config::get()->b["rbot_aa_enable"])
		return;

	if ((c_client::get()->m_local->m_fFlags() & FL_ONGROUND) && c_key_handler::get()->auto_check("fakeduck_key")) {
		m_fakeduck = true;

		c_client::get()->m_cmd->m_buttons |= IN_BULLRUSH;

		if (c_csgo::get()->m_cl->m_choked_commands <= (c_client::get()->m_max_lag / 2)) {
			c_client::get()->m_cmd->m_buttons &= ~IN_DUCK;
		}
		// duck if we are choking more than 7 ticks.
		else {
			c_client::get()->m_cmd->m_buttons |= IN_DUCK;
		}
	}
	else
		m_fakeduck = false;

	attack = c_client::get()->m_cmd->m_buttons & IN_ATTACK;
	attack2 = c_client::get()->m_cmd->m_buttons & IN_ATTACK2;

	if (c_client::get()->m_weapon && c_client::get()->m_weapon_fire) {
		bool knife = c_client::get()->m_weapon_type == WEAPONTYPE_KNIFE && c_client::get()->m_weapon_id != ZEUS;
		bool revolver = c_client::get()->m_weapon_id == REVOLVER;

		// if we are in attack and can fire, do not anti-aim.
		if (attack || (attack2 && (knife || revolver)))
			return;
	}

	// disable conditions.
	if (c_csgo::get()->m_gamerules->m_bFreezePeriod() || (c_client::get()->m_flags & FL_FROZEN) || c_client::get()->m_round_end || (c_client::get()->m_cmd->m_buttons & IN_USE))
		return;

	// grenade throwing
	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if (c_client::get()->m_weapon && c_client::get()->m_weapon->IsGrenade()
		&& (!c_client::get()->m_weapon->m_bPinPulled() || attack || attack2)
		&& c_client::get()->m_weapon->m_fThrowTime() > 0.f && c_client::get()->m_weapon->m_fThrowTime() < c_csgo::get()->m_globals->m_curtime)
		return;

	m_should_work = true;

	m_mode = AntiAimMode::STAND;

	if ((c_client::get()->m_buttons & IN_JUMP) || !(c_client::get()->m_flags & FL_ONGROUND))
		m_mode = AntiAimMode::AIR;

	else if (c_key_handler::get()->auto_check("slowwalk_key"))
		m_mode = AntiAimMode::SLOWMOTION;

	else if (c_client::get()->m_speed > 6.f)
		m_mode = AntiAimMode::WALK;

	m_pitch = c_config::get()->i["rbot_aa_pitch"];
	m_yaw = c_config::get()->i["rbot_aa_yaw"];
	m_jitter_range = c_config::get()->i["rbot_aa_jitter_range"];
	m_rot_range = c_config::get()->i["rbot_aa_rot_range"];
	m_rot_speed = c_config::get()->i["rbot_aa_rot_speed"];
	m_rand_update = c_config::get()->i["rbot_aa_rand_update"];
	m_dir = c_config::get()->i["rbot_aa_dir"];
	m_dir_custom = c_config::get()->i["rbot_aa_dir_custom"];
	m_base_angle = c_config::get()->i["rbot_aa_base_angle"];
	m_auto_time = c_config::get()->i["rbot_aa_auto_time"];

	// set pitch.
	AntiAimPitch();

	// if we have any yaw.
	if (m_yaw > 0) {
		// set direction.
		GetAntiAimDirection();
	}

	// we have no real, but we desync animations.
	else if (c_config::get()->b["rbot_aa_desync"])
		m_direction = c_client::get()->m_cmd->m_view_angles.y;

	DoRealAntiAim();
	DoFakeAntiAim();

	// normalize angle.
	math::NormalizeAngle(c_client::get()->m_cmd->m_view_angles.y);
}

void c_hvh::SendPacket() {
	// if not the last packet this shit wont get sent anyway.
	// fix rest of hack by forcing to false.
	if (!*c_client::get()->m_final_packet)
		*c_client::get()->m_packet = false;

	if (c_client::get()->can_use_exploit) {
		*c_client::get()->m_packet = c_csgo::get()->m_cl->m_choked_commands >= 1;
	}
	else {
		// fake-lag enabled.
		if (c_config::get()->b["rbot_aa_fakelag_enable"] && !c_csgo::get()->m_gamerules->m_bFreezePeriod() && !(c_client::get()->m_flags & FL_FROZEN)) {
			// limit of lag.
			int limit = std::min((int)c_config::get()->i["rbot_fakelag_limit"], c_client::get()->m_max_lag);
			// type of lag.
			int mode = c_config::get()->i["rbot_aa_fakelag_mode"];
			// variance of fluctuate lag.
			int variance = std::clamp((int)c_config::get()->i["rbot_fakelag_variance"], 1, 100);

			// get current origin.
			vec3_t cur = c_client::get()->m_local->m_vecOrigin();

			// get prevoius origin.
			vec3_t prev = c_client::get()->m_net_pos.empty() ? c_client::get()->m_local->m_vecOrigin() : c_client::get()->m_net_pos.front().m_pos;

			// delta between the current origin and the last sent origin.
			float delta = (cur - prev).length_sqr();

			// max.
			if (mode == 0)
				*c_client::get()->m_packet = false;

			// break.
			else if (mode == 1 && delta <= 4096.f)
				*c_client::get()->m_packet = false;

			// random.
			else if (mode == 2) {
				// compute new factor.
				if (c_client::get()->m_lag >= m_random_lag)
					m_random_lag = c_csgo::get()->RandomInt(2, limit);

				// factor not met, keep choking.
				else *c_client::get()->m_packet = false;
			}

			// break step.
			else if (mode == 3) {
				// normal break.
				if (m_step_switch) {
					if (delta <= 4096.f)
						*c_client::get()->m_packet = false;
				}

				// max.
				else *c_client::get()->m_packet = false;
			}
			// fluctuate.
			else if (mode == 4) {
				if (c_client::get()->m_cmd->m_command_number % variance >= limit)
					limit = 1;

				*c_client::get()->m_packet = false;
			}

			if (c_client::get()->m_lag >= limit)
				*c_client::get()->m_packet = true;
		}

		if (!c_config::get()->b["rbot_aa_fakelag_land"]) {
			vec3_t                start = c_client::get()->m_local->m_vecOrigin(), end = start, vel = c_client::get()->m_local->m_vecVelocity();
			CTraceFilterWorldOnly filter;
			CGameTrace            trace;

			// gravity.
			vel.z -= (c_csgo::get()->sv_gravity->GetFloat() * c_csgo::get()->m_globals->m_interval);

			// extrapolate.
			end += (vel * c_csgo::get()->m_globals->m_interval);

			// move down.
			end.z -= 2.f;

			c_csgo::get()->m_engine_trace->TraceRay(Ray(start, end), MASK_SOLID, &filter, &trace);

			// check if landed.
			if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f && !(c_client::get()->m_flags & FL_ONGROUND))
				*c_client::get()->m_packet = true;
		}

		// force fake-lag to 14 when use fakeduck.
		if (m_fakeduck) {
			*c_client::get()->m_packet = false;
		}

		// do not lag while shooting.
		if (!m_fakeduck && c_client::get()->m_old_shot)
			*c_client::get()->m_packet = true;
	}

	// we somehow reached the maximum amount of lag.
	// we cannot lag anymore and we also cannot shoot anymore since we cant silent aim.
	if (c_client::get()->m_lag >= c_client::get()->m_max_lag) {
		// set bSendPacket to true.
		*c_client::get()->m_packet = true;

		// disable firing, since we cannot choke the last packet.
		c_client::get()->m_weapon_fire = false;
	}
}