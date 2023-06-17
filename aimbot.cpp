#include "includes.h"

bool CanFireWithExploit(int m_iShiftedTick)
{
	// curtime before shift.
	float curtime = game::TICKS_TO_TIME(c_client::get()->m_local->m_nTickBase() - m_iShiftedTick);
	return c_client::get()->CanFireWeapon(curtime);
}

bool c_aimbot::CanUseExploit() {
	if (!c_client::get()->m_weapon)
		return false;
	int idx = c_client::get()->m_weapon->m_iItemDefinitionIndex();
	return c_client::get()->m_local->alive()
		&& c_csgo::get()->m_cl->m_choked_commands <= 1
		&& c_key_handler::get()->auto_check("exploits_key") && !c_hvh::get()->m_fakeduck;
}

void c_aimbot::RunExploit()
{
	static bool did_shift_before = false;
	static int prev_shift_ticks = 0;
	static bool reset = false;

	c_client::get()->m_tick_to_shift = 0;

	if (c_client::get()->can_use_exploit && !c_csgo::get()->m_gamerules->m_bFreezePeriod())
	{
		prev_shift_ticks = 0;

		auto can_shift_shot = CanFireWithExploit(c_client::get()->shift_value);
		auto can_shot = CanFireWithExploit(abs(-1 - prev_shift_ticks));

		if (can_shift_shot || !can_shot && !did_shift_before)
		{
			prev_shift_ticks = c_client::get()->shift_value;
		}
		else 
		{
			prev_shift_ticks = 0;
		}

		if (prev_shift_ticks > 0)
		{
			if (c_client::get()->m_weapon->DTable() && CanFireWithExploit(prev_shift_ticks))
			{
				if (c_client::get()->m_cmd->m_buttons & IN_ATTACK)
				{
					c_client::get()->m_tick_to_shift = prev_shift_ticks;
					reset = true;
				}
				else {
					if ((!(c_client::get()->m_cmd->m_buttons & IN_ATTACK) || !c_client::get()->m_shot) && reset
						&& fabsf(c_client::get()->m_weapon->m_fLastShotTime() - game::TICKS_TO_TIME(c_client::get()->m_local->m_nTickBase())) > 0.5f) {
						c_client::get()->m_charged = false;
						c_client::get()->m_tick_to_recharge = c_client::get()->shift_value;
						reset = false;
					}
				}
			}
		}
		did_shift_before = prev_shift_ticks != 0;
	}
}

void AimPlayer::UpdateAnimations(LagRecord* record) {
	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState();
	if (!state)
		return;

	// backup curtime.
	float curtime = c_csgo::get()->m_globals->m_curtime;
	float frametime = c_csgo::get()->m_globals->m_frametime;

	// set curtime to animtime.
	// set frametime to ipt just like on the server during simulation.
	c_csgo::get()->m_globals->m_curtime = record->m_anim_time;
	c_csgo::get()->m_globals->m_frametime = c_csgo::get()->m_globals->m_interval;

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = m_player->m_vecOrigin();
	backup.m_abs_origin = m_player->GetAbsOrigin();
	backup.m_velocity = m_player->m_vecVelocity();
	backup.m_abs_velocity = m_player->m_vecAbsVelocity();
	backup.m_flags = m_player->m_fFlags();
	backup.m_eflags = m_player->m_iEFlags();
	backup.m_duck = m_player->m_flDuckAmount();

	// set this fucker, it will get overriden.
	record->m_anim_velocity = record->m_velocity;

	// if using fake angles, correct angles.
	c_resolver::get()->ResolveAngles(m_player, record);

	// set stuff before animating.
	m_player->m_vecOrigin() = record->m_origin;
	m_player->m_vecVelocity() = m_player->m_vecAbsVelocity() = record->m_anim_velocity;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	m_player->m_iEFlags() &= ~0x1000;

	// fix animating in same frame.
	if (state->m_iLastClientSideAnimationUpdateFramecount == c_csgo::get()->m_globals->m_frame)
		state->m_iLastClientSideAnimationUpdateFramecount -= 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	m_player->m_bClientSideAnimation() = true;
	m_player->UpdateClientSideAnimation();
	m_player->m_bClientSideAnimation() = false;

	// restore backup data.
	m_player->m_vecOrigin() = backup.m_origin;
	m_player->m_vecVelocity() = backup.m_velocity;
	m_player->m_vecAbsVelocity() = backup.m_abs_velocity;
	m_player->m_fFlags() = backup.m_flags;
	m_player->m_iEFlags() = backup.m_eflags;
	m_player->m_flDuckAmount() = backup.m_duck;
	m_player->m_flLowerBodyYawTarget() = backup.m_body;
	m_player->SetAbsOrigin(backup.m_abs_origin);

	// IMPORTANT: do not restore poses here, since we want to preserve them for rendering.
	// also dont restore the render angles which indicate the model rotation.

	// restore globals.
	c_csgo::get()->m_globals->m_curtime = curtime;
	c_csgo::get()->m_globals->m_frametime = frametime;
}

void AimPlayer::OnNetUpdate(Player* player) {
	bool reset = (!c_config::get()->b["rbot_enable"] || !player->alive() || !player->enemy(c_client::get()->m_local));
	bool disable = (!reset && !c_client::get()->m_processing);

	// if this happens, delete all the lagrecords.
	if (reset) {
		player->m_bClientSideAnimation() = true;
		m_records.clear();
		return;
	}

	// just disable anim if this is the case.
	if (disable) {
		player->m_bClientSideAnimation() = true;
		return;
	}

	// update player ptr if required.
	// reset player if changed.
	if (m_player != player)
		m_records.clear();

	// update player ptr.
	m_player = player;

	// indicate that this player has been out of pvs.
	// insert dummy record to separate records
	// to fix stuff like animation and prediction.
	if (player->dormant()) {
		bool insert = true;

		// we have any records already?
		if (!m_records.empty()) {

			LagRecord* front = m_records.front().get();

			// we already have a dormancy separator.
			if (front->dormant())
				insert = false;
		}

		if (insert) {
			// add new record.
			m_records.emplace_front(std::make_shared< LagRecord >(player));

			// get reference to newly added record.
			LagRecord* current = m_records.front().get();

			// mark as dormant.
			current->m_dormant = true;
		}
	}

	bool update = (m_records.empty() || player->m_flSimulationTime() > m_records.front().get()->m_sim_time);

	if (!update && !player->dormant() && player->m_vecOrigin() != player->m_vecOldOrigin()) {
		update = true;

		// fix data.
		player->m_flSimulationTime() = game::TICKS_TO_TIME(c_csgo::get()->m_cl->clock_drift_mgr.m_server_tick);
	}

	// this is the first data update we are receving
	// OR we received data with a newer simulation context.
	if (update) {
		// add new record.
		m_records.emplace_front(std::make_shared< LagRecord >(player));

		// get reference to newly added record.
		LagRecord* current = m_records.front().get();

		// mark as non dormant.
		current->m_dormant = false;

		// update animations on current record.
		// call resolver.
		UpdateAnimations(current);

		// create bone matrix for this record.
		c_bones::get()->setup(m_player, nullptr, current);
	}

	// no need to store insane amt of data.
	while (m_records.size() > 256)
		m_records.pop_back();
}

void AimPlayer::OnRoundStart(Player *player) {
	m_player = player;
	m_walk_record = LagRecord{ };
	m_shots = 0;
	m_missed_shots = 0;

	m_records.clear();
	m_hitboxes.clear();

	// IMPORTANT: DO NOT CLEAR LAST HIT SHIT.
}

void AimPlayer::SetupHitboxes(LagRecord *record, bool history) {
	// reset hitboxes.
	m_hitboxes.clear();

	if (c_client::get()->m_weapon_id == ZEUS) {
		// hitboxes for the zeus.
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		return;
	}

	// prefer, always.
	if (c_config::get()->m["rage_prefer_baim"][0])
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });

	// prefer, lethal.
	if (c_config::get()->m["rage_prefer_baim"][1])
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL });

	// prefer, lethal x2.
	if (c_config::get()->m["rage_prefer_baim"][2])
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL2 });

	// prefer, in air.
	if (c_config::get()->m["rage_prefer_baim"][3] && !(record->m_pred_flags & FL_ONGROUND))
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });

	bool only{ false };

	// only, always.
	if (c_config::get()->m["rage_always_baim"][0]) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only, health.
	if (c_config::get()->m["rage_always_baim"][1] && m_player->m_iHealth() <= (int)c_config::get()->i["rbot_baim_hp"]) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only, in air.
	if (c_config::get()->m["rage_always_baim"][2] && !(record->m_pred_flags & FL_ONGROUND)) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only, on key.
	if (c_key_handler::get()->auto_check("rbot_baim_key")) {
		only = true;
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
	}

	// only baim conditions have been met.
	// do not insert more hitboxes.
	if (only)
		return;

	auto hitbox = history ? c_config::get()->m["rage_history_hitboxes"] : c_config::get()->m["rage_hitboxes"];

	if(hitbox[0])
		m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL });

	if (hitbox[1])
		m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL });

	if (hitbox[2])
	{
		m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL });
	}
	
	if (hitbox[3])
	{
		m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL });
	}
	
	if (hitbox[4])
	{
		m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL });
	}
}

void c_aimbot::init() {
	// clear old targets.
	m_targets.clear();

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max();
	m_best_fov = std::numeric_limits< float >::max();
	m_best_damage = 0.f;
	m_best_hp = std::numeric_limits< int >::max();
	m_best_lag = std::numeric_limits< float >::max();
	m_best_height = std::numeric_limits< float >::max();
}

void c_aimbot::StripAttack() {
	if (c_client::get()->m_weapon_id == REVOLVER)
		c_client::get()->m_cmd->m_buttons &= ~IN_ATTACK2;

	else
		c_client::get()->m_cmd->m_buttons &= ~IN_ATTACK;
}

void c_aimbot::think() {
	// do all startup routines.
	init();

	// sanity.
	if (!c_client::get()->m_weapon)
		return;

	// no grenades or bomb.
	if (c_client::get()->m_weapon->IsGrenade() || c_client::get()->m_weapon_type == WEAPONTYPE_EQUIPMENT)
		return;

	if (!c_client::get()->m_weapon_fire)
		StripAttack();

	// we have no aimbot enabled.
	if (!c_config::get()->b["rbot_enable"])
		return;

	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = c_client::get()->m_weapon_id == REVOLVER && c_client::get()->m_revolver_cock != 0;

	// one tick before being able to shoot.
	if (revolver && c_client::get()->m_revolver_cock > 0 && c_client::get()->m_revolver_cock == c_client::get()->m_revolver_query) {
		*c_client::get()->m_packet = false;
		return;
	}

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i) {
		Player *player = c_csgo::get()->m_entlist->GetClientEntity< Player * >(i);

		if (!IsValidTarget(player))
			continue;

		AimPlayer *data = &m_players[i - 1];
		if (!data)
			continue;

		// store player as potential target this tick.
		m_targets.emplace_back(data);
	}

	// run knifebot.
	if (c_client::get()->m_weapon_type == WEAPONTYPE_KNIFE && c_client::get()->m_weapon_id != ZEUS) {

		// no point in aimbotting if we cannot fire this tick.
		if (!c_client::get()->m_weapon_fire)
			return;
			
		knife();

		return;
	}

	// scan available targets... if we even have any.
	find();

	// finally set data when shooting.
	apply();
}

void c_aimbot::find() {
	struct BestTarget_t { Player* player; vec3_t pos; float damage; int hitbox; LagRecord* record; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.hitbox = -1;
	best.record = nullptr;

	if (m_targets.empty())
		return;

	if (!c_client::get()->m_weapon_id == ZEUS)
		return;

	// iterate all targets.
	for (const auto &t : m_targets) {
		if (t->m_records.empty())
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if (c_lag_comp::get()->StartPrediction(t)) {
			LagRecord *front = t->m_records.front().get();

			t->SetupHitboxes(front, false);
			if (t->m_hitboxes.empty())
				continue;

			// rip something went wrong..
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, front) && SelectTarget(front, tmp_pos, tmp_damage)) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = front;
			}
		}

		// player did not break lagcomp.
		// history aim is possible at this point.
		else {
			LagRecord *ideal = c_resolver::get()->FindIdealRecord(t);
			if (!ideal)
				continue;

			t->SetupHitboxes(ideal, false);
			if (t->m_hitboxes.empty())
				continue;

			// try to select best record as target.
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, ideal) && SelectTarget(ideal, tmp_pos, tmp_damage)) {
				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal;
			}

			LagRecord *last = c_resolver::get()->FindLastRecord(t);
			if (!last || last == ideal)
				continue;

			t->SetupHitboxes(last, true);
			if (t->m_hitboxes.empty())
				continue;

			// rip something went wrong..
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, best.hitbox, last) && SelectTarget(last, tmp_pos, tmp_damage)) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last;
			}
		}
	}

	// verify our target and set needed data.
	if (best.player && best.record) {
		// calculate aim angle.
		math::VectorAngles(best.pos - c_client::get()->m_shoot_pos, m_angle);

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_hitbox = best.hitbox;
		m_record = best.record;

		// write data, needed for traces / etc.
		m_record->cache();

		bool can_hit_on_fd = !c_hvh::get()->m_fakeduck || c_hvh::get()->m_fakeduck && c_client::get()->m_local->m_flDuckAmount() == 0.f;
		bool on = true;
		bool hit = (!c_client::get()->m_ground && c_client::get()->m_weapon_id == SSG08 && c_client::get()->m_weapon && c_client::get()->m_weapon->GetInaccuracy() < 0.009f) || (on && CheckHitchance(m_target, m_angle, m_record, best.hitbox) && can_hit_on_fd);

		// set autostop shit.
		if (c_config::get()->i["rbot_autostop_mode"] == 0) {
			m_stop = false;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = false;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 1) {
			m_stop = c_client::get()->m_ground;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = false;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 2) {
			m_stop = c_client::get()->m_ground && on && !hit;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = false;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 3) {
			m_stop = false;
			m_slow_motion_slowwalk = c_client::get()->m_ground;
			m_slow_motion_fakewalk = false;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 4) {
			m_stop = false;
			m_slow_motion_slowwalk = c_client::get()->m_ground && on && !hit;
			m_slow_motion_fakewalk = false;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 5) {
			m_stop = false;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = c_client::get()->m_ground;
		}
		else if (c_config::get()->i["rbot_autostop_mode"] == 6) {
			m_stop = false;
			m_slow_motion_slowwalk = false;
			m_slow_motion_fakewalk = c_client::get()->m_ground && on && !hit;
		}

		c_movement::get()->AutoStop();

		// if we can scope.
		bool can_scope = c_client::get()->m_weapon && c_client::get()->m_weapon->m_zoomLevel() == 0 && c_client::get()->m_weapon->IsZoomable(true);

		if (can_scope) {
			// always.
			if (c_config::get()->i["rbot_autoscope_mode"] == 1) {
				c_client::get()->m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}

			// hitchance fail.
			else if (c_config::get()->i["rbot_autoscope_mode"] == 2 && on && !hit) {
				c_client::get()->m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		// no point in aimbotting if we cannot fire this tick.
		if (!c_client::get()->m_weapon_fire)
			return;

		if (hit || !on) {
			c_client::get()->m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}

bool c_aimbot::CheckHitchance(Player* player, const ang_t& angle, LagRecord* record, int hitbox) {
	constexpr float HITCHANCE_MAX = 100.f;
	constexpr int   SEED_MAX = 255;

	vec3_t     start{ c_client::get()->m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	CGameTrace tr;
	size_t     total_hits{ }, needed_hits{ (size_t)std::ceil((c_config::get()->i["rbot_hitchance"] * SEED_MAX) / HITCHANCE_MAX) };

	// get needed directional vectors.
	math::AngleVectors(angle, &fwd, &right, &up);

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = c_client::get()->m_weapon->GetInaccuracy();
	spread = c_client::get()->m_weapon->GetSpread();

	// iterate all possible seeds.
	for (int i{ }; i <= SEED_MAX; ++i) {
		// get spread.
		wep_spread = c_client::get()->m_weapon->CalculateSpread(i, inaccuracy, spread);

		// get spread direction.
		dir = (fwd + (right * wep_spread.x) + (up * wep_spread.y)).normalized();

		// get end of trace.
		end = start + (dir * c_client::get()->m_weapon_info->flRange);

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (CanHit(start, end, record, hitbox, false, nullptr))
			++total_hits;

		// we made it.
		if (total_hits >= needed_hits)
			return true;

		// we cant make it anymore.
		if ((SEED_MAX - i + total_hits) < needed_hits)
			return false;
	}

	return false;
}

bool AimPlayer::SetupHitboxPoints(LagRecord *record, BoneArray *bones, int index, std::vector< vec3_t > &points) {
	// reset points.
	points.clear();

	const model_t *model = m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t *hdr = c_csgo::get()->m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t *set = hdr->GetHitboxSet(m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t *bbox = set->GetHitbox(index);
	if (!bbox)
		return false;

	// get hitbox scales.
	float hscale = c_config::get()->i["rbot_headscale"] / 100.f;
	float cscale = c_config::get()->i["rbot_chestscale"] / 100.f;
	float bscale = c_config::get()->i["rbot_bodyscale"] / 100.f;
	float lscale = c_config::get()->i["rbot_legsscale"] / 100.f;

	// these indexes represent boxes.
	if (bbox->m_radius <= 0.f) {
		// references: 
		//      https://developer.valvesoftware.com/wiki/Rotation_Tutorial
		//      CBaseAnimating::GetHitboxBonePosition
		//      CBaseAnimating::DrawServerHitboxes

		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		c_csgo::get()->AngleMatrix(bbox->m_angle, rot_matrix);

		// apply the rotation to the entity input space (local).
		matrix3x4_t matrix;
		math::ConcatTransforms(bones[bbox->m_bone], rot_matrix, matrix);

		// extract origin from matrix.
		vec3_t origin = matrix.GetOrigin();

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

		// the feet hiboxes have a side, heel and the toe.
		if (index == HITBOX_R_FOOT || index == HITBOX_L_FOOT) {
			float d1 = (bbox->m_mins.z - center.z) * 0.875f;

			// invert.
			if (index == HITBOX_L_FOOT)
				d1 *= -1.f;

			// side is more optimal then center.
			points.push_back({ center.x, center.y, center.z + d1 });

			if (c_config::get()->m["rage_multipoints"][3]) {
				// get point offset relative to center point
				// and factor in hitbox scale.
				float d2 = (bbox->m_mins.x - center.x) * lscale;
				float d3 = (bbox->m_maxs.x - center.x) * lscale;

				// heel.
				points.push_back({ center.x + d2, center.y, center.z });

				// toe.
				points.push_back({ center.x + d3, center.y, center.z });
			}
		}

		// nothing to do here we are done.
		if (points.empty())
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for (auto &p : points) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot(matrix[0]), p.dot(matrix[1]), p.dot(matrix[2]) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else {
		// factor in the pointscale.
		float hr = bbox->m_radius * hscale;
		float cr = bbox->m_radius * cscale;
		float br = bbox->m_radius * bscale;
		float lr = bbox->m_radius * lscale;

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

		// head has 5 points.
		if (index == HITBOX_HEAD) {
			// add center.
			points.push_back(center);

			if (c_config::get()->m["rage_multipoints"][0]) {
				// rotation matrix 45 degrees.
				// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
				// std::cos( deg_to_rad( 45.f ) )
				constexpr float rotation = 0.70710678f;

				// top/back 45 deg.
				// this is the best spot to shoot at.
				points.push_back({ bbox->m_maxs.x + (rotation * hr), bbox->m_maxs.y + (-rotation * hr), bbox->m_maxs.z });

				// right.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + hr });

				// left.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - hr });

				// back.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - hr, bbox->m_maxs.z });

				// get animstate ptr.
				CCSGOPlayerAnimState *state = record->m_player->m_PlayerAnimState();

				// add this point only under really specific circumstances.
				// if we are standing still and have the lowest possible pitch pose.
				if (state && record->m_anim_velocity.length() <= 0.1f && record->m_eye_angles.x <= state->m_min_pitch) {

					// bottom point.
					points.push_back({ bbox->m_maxs.x - hr, bbox->m_maxs.y, bbox->m_maxs.z });
				}
			}
		}

		// body has 5 points.
		else if (index == HITBOX_BODY) {
			// center.
			points.push_back(center);

			// back.
			if (c_config::get()->m["rage_multipoints"][2])
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
		}

		else if (index == HITBOX_PELVIS) {
			// back.
			points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
		}
		else if (index == HITBOX_UPPER_CHEST) {
			// back.
			points.push_back({ center.x, bbox->m_maxs.y - cr, center.z });
		}
		// other stomach/chest hitboxes have 2 points.
		else if (index == HITBOX_THORAX) {
			// add center.
			points.push_back(center);

			// add extra point on back.
			if (c_config::get()->m["rage_multipoints"][2])
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
		}
		else if (index == HITBOX_CHEST) {
			// add center.
			points.push_back(center);

			// add extra point on back.
			if (c_config::get()->m["rage_multipoints"][1])
				points.push_back({ center.x, bbox->m_maxs.y - cr, center.z });
		}
		else if (index == HITBOX_R_CALF || index == HITBOX_L_CALF) {
			// add center.
			points.push_back(center);

			// half bottom.
			if (c_config::get()->m["rage_multipoints"][3])
				points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
		}

		else if (index == HITBOX_R_THIGH || index == HITBOX_L_THIGH) {
			// add center.
			points.push_back(center);
		}

		// arms get only one point.
		else if (index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM) {
			// elbow.
			points.push_back({ bbox->m_maxs.x + bbox->m_radius, center.y, center.z });
		}

		// nothing left to do here.
		if (points.empty())
			return false;

		// transform capsule points.
		for (auto &p : points)
			math::VectorTransform(p, bones[bbox->m_bone], p);
	}

	return true;
}

bool AimPlayer::GetBestAimPosition(vec3_t& aim, float& damage, int& hitbox, LagRecord* record) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	// get player hp.
	int hp = m_player->m_iHealth();

	if (c_client::get()->m_weapon_id == ZEUS) {
		dmg = pendmg = hp;
		pen = true;
	}
	else {
		dmg = c_config::get()->i["rbot_min_damage"];
		if (c_config::get()->b["rbot_mindamage_scale"] && dmg > hp)
			dmg = hp;

		pendmg = c_config::get()->i["rbot_penetration_min_damage"];
		if (c_config::get()->b["rbot_mindamage_scale"] && pendmg > hp)
			pendmg = hp;

		pen = true;
	}

	// write all data of this record l0l.
	record->cache();

	// iterate hitboxes.
	for (const auto &it : m_hitboxes) {
		done = false;

		// setup points on hitbox.
		if (!SetupHitboxPoints(record, record->m_bones, it.m_index, points))
			continue;

		// iterate points on hitbox.
		for (const auto &point : points) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = pen;
			in.m_target = m_player;
			in.m_from = c_client::get()->m_local;
			in.m_pos = point;

			penetration::PenetrationOutput_t out;

			// we can hit p!
			if (penetration::run(&in, &out)) {

				// nope we did not hit head..
				if (it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD)
					continue;

				// prefered hitbox, just stop now.
				if (it.m_mode == HitscanMode::PREFER)
					done = true;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				else if (it.m_mode == HitscanMode::LETHAL && out.m_damage > m_player->m_iHealth())
					done = true;

				// 2 shots will be sufficient to kill.
				else if (it.m_mode == HitscanMode::LETHAL2 && (out.m_damage * 2.f) > m_player->m_iHealth())
					done = true;

				// this hitbox has normal selection, it needs to have more damage.
				else if (it.m_mode == HitscanMode::NORMAL) {
					// we did more damage.
					if (out.m_damage > scan.m_damage) {
						// save new best data.
						scan.m_damage = out.m_damage;
						scan.m_pos = point;
						scan.m_hitbox = it.m_index;

						// if the first point is lethal
						// screw the other ones.
						if (point == points.front() && out.m_damage > m_player->m_iHealth())
							break;
					}
				}

				// we found a preferred / lethal hitbox.
				if (done) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_hitbox = it.m_index;
					scan.m_pos = point;
					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if (done)
			break;
	}

	// we found something that we can damage.
	// set out vars.
	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;
		return true;
	}

	return false;
}

bool c_aimbot::SelectTarget(LagRecord *record, const vec3_t &aim, float damage) {
	float dist, fov, height;
	int   hp;

	switch (c_config::get()->i["rbot_target_selection"]) {

		// distance.
	case 0:
		dist = (record->m_pred_origin - c_client::get()->m_shoot_pos).length();

		if (dist < m_best_dist) {
			m_best_dist = dist;
			return true;
		}

		break;

		// crosshair.
	case 1:
		fov = math::GetFOV(c_client::get()->m_view_angles, c_client::get()->m_shoot_pos, aim);

		if (fov < m_best_fov) {
			m_best_fov = fov;
			return true;
		}

		break;

		// damage.
	case 2:
		if (damage > m_best_damage) {
			m_best_damage = damage;
			return true;
		}

		break;

		// lowest hp.
	case 3:
		hp = record->m_player->m_iHealth();

		if (hp < m_best_hp) {
			m_best_hp = hp;
			return true;
		}

		break;

		// least lag.
	case 4:
		if (record->m_lag < m_best_lag) {
			m_best_lag = record->m_lag;
			return true;
		}

		break;

		// height.
	case 5:
		height = record->m_pred_origin.z - c_client::get()->m_local->m_vecOrigin().z;

		if (height < m_best_height) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

void c_aimbot::apply() {
	bool attack, attack2;

	// attack states.
	attack = (c_client::get()->m_cmd->m_buttons & IN_ATTACK);
	attack2 = (c_client::get()->m_weapon_id == REVOLVER && c_client::get()->m_cmd->m_buttons & IN_ATTACK2);

	// ensure we're attacking.
	if (attack || attack2) {

		if (!c_hvh::get()->m_fakeduck) {
			*c_client::get()->m_packet = true;
		}

		if (m_target) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if (m_record && !m_record->m_broke_lc)
				c_client::get()->m_cmd->m_tick = game::TIME_TO_TICKS(m_record->m_sim_time + c_client::get()->m_lerp);

			// set angles to target.
			c_client::get()->m_cmd->m_view_angles = m_angle;

			// if not silent aim, apply the viewangles.

			//c_visuals::get()->DrawHitboxMatrix( m_record, colors::white, 10.f );
		}

		c_client::get()->m_cmd->m_view_angles -= c_client::get()->m_local->m_aimPunchAngle() * c_csgo::get()->weapon_recoil_scale->GetFloat();

		// store fired shot.
		c_shots::get()->OnShotFire(m_target ? m_target : nullptr, m_target ? m_damage : -1.f, c_client::get()->m_weapon_info->iBullets, m_target ? m_record : nullptr, m_hitbox);

		// set that we fired.
		c_client::get()->m_shot = true;
	}
}

void c_aimbot::NoSpread() {
	bool    attack2;
	vec3_t  spread, forward, right, up, dir;

	// revolver state.
	attack2 = (c_client::get()->m_weapon_id == REVOLVER && (c_client::get()->m_cmd->m_buttons & IN_ATTACK2));

	// get spread.
	spread = c_client::get()->m_weapon->CalculateSpread(c_client::get()->m_cmd->m_random_seed, attack2);

	// compensate.
	c_client::get()->m_cmd->m_view_angles -= { -math::rad_to_deg(std::atan(spread.length_2d())), 0.f, math::rad_to_deg(std::atan2(spread.x, spread.y)) };
}


bool c_aimbot::CanHit(vec3_t start, vec3_t end, LagRecord* record, int box, bool in_shot, BoneArray* bones)
{
	if (!record || !record->m_player)
		return false;

	// backup player
	const auto backup_origin = record->m_player->m_vecOrigin();
	const auto backup_abs_origin = record->m_player->GetAbsOrigin();
	const auto backup_abs_angles = record->m_player->GetAbsAngles();
	const auto backup_obb_mins = record->m_player->m_vecMins();
	const auto backup_obb_maxs = record->m_player->m_vecMaxs();
	const auto backup_cache = record->m_player->m_BoneCache2();

	// always try to use our aimbot matrix first.
	auto matrix = record->m_bones;

	// this is basically for using a custom matrix.
	if (in_shot)
		matrix = bones;

	if (!matrix)
		return false;

	const model_t* model = record->m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = c_csgo::get()->m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(record->m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(box);
	if (!bbox)
		return false;

	vec3_t min, max;
	const auto IsCapsule = bbox->m_radius != -1.f;

	if (IsCapsule) {
		math::VectorTransform(bbox->m_mins, matrix[bbox->m_bone], min);
		math::VectorTransform(bbox->m_maxs, matrix[bbox->m_bone], max);
		const auto dist = math::SegmentToSegment(start, end, min, max);

		if (dist < bbox->m_radius) {
			return true;
		}
	}
	else {
		CGameTrace tr;

		// setup trace data
		record->m_player->m_vecOrigin() = record->m_origin;
		record->m_player->SetAbsOrigin(record->m_origin);
		record->m_player->SetAbsAngles(record->m_abs_ang);
		record->m_player->m_vecMins() = record->m_mins;
		record->m_player->m_vecMaxs() = record->m_maxs;
		record->m_player->m_BoneCache2() = reinterpret_cast<matrix3x4_t**>(matrix);

		// setup ray and trace.
		c_csgo::get()->m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT, record->m_player, &tr);

		record->m_player->m_vecOrigin() = backup_origin;
		record->m_player->SetAbsOrigin(backup_abs_origin);
		record->m_player->SetAbsAngles(backup_abs_angles);
		record->m_player->m_vecMins() = backup_obb_mins;
		record->m_player->m_vecMaxs() = backup_obb_maxs;
		record->m_player->m_BoneCache2() = backup_cache;

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (tr.m_entity == record->m_player && game::IsValidHitgroup(tr.m_hitgroup))
			return true;
	}

	return false;
}