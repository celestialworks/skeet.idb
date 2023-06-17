#include "includes.h"

void c_movement::JumpRelated() {
	if (c_client::get()->m_local->m_MoveType() == MOVETYPE_NOCLIP)
		return;

	c_csgo::get()->m_cvar->FindVar(HASH("sv_autobunnyhopping"))->SetValue(c_config::get()->b["misc_bhop"]);
	c_csgo::get()->m_cvar->FindVar(HASH("sv_enablebunnyhopping"))->SetValue(c_config::get()->b["misc_bhop"]);

	if ((c_client::get()->m_cmd->m_buttons & IN_JUMP) && !(c_client::get()->m_flags & FL_ONGROUND)) {
		// bhop.
		if (c_config::get()->b["misc_bhop"])
			c_client::get()->m_cmd->m_buttons &= ~IN_JUMP;

		// duck jump ( crate jump ).
		if (c_config::get()->b["misc_air_duck"])
			c_client::get()->m_cmd->m_buttons |= IN_DUCK;
	}
}

void c_movement::Strafe() {
	if (!c_config::get()->b["misc_autostrafe"])
		return;

	// don't strafe while noclipping or on ladders..
	if (c_client::get()->m_local->m_MoveType() == MOVETYPE_NOCLIP || c_client::get()->m_local->m_MoveType() == MOVETYPE_LADDER)
		return;

	// disable strafing while pressing shift.
	// don't strafe if not holding primary jump key.
	if ((c_client::get()->m_buttons & IN_SPEED) || !(c_client::get()->m_buttons & IN_JUMP) || (c_client::get()->m_flags & FL_ONGROUND))
		return;

	float wish_dir = c_client::get()->m_strafe_angles.y;

	if (c_client::get()->m_pressing_move) {
		// took this idea from stacker, thank u !!!!
		enum EDirections {
			FORWARDS = 0,
			BACKWARDS = 180,
			LEFT = 90,
			RIGHT = -90,
			FORWARD_LEFT = 45,
			FORWARD_RIGHT = -45,
			BACK_LEFT = 135,
			BACK_RIGHT = -135
		};

		// get our key presses.
		bool holding_w = c_client::get()->m_buttons & IN_FORWARD;
		bool holding_a = c_client::get()->m_buttons & IN_MOVELEFT;
		bool holding_s = c_client::get()->m_buttons & IN_BACK;
		bool holding_d = c_client::get()->m_buttons & IN_MOVERIGHT;

		// move in the appropriate direction.
		if (holding_w) {
			//	forward left
			if (holding_a) {
				wish_dir += EDirections::FORWARD_LEFT;
			}
			//	forward right
			else if (holding_d) {
				wish_dir += EDirections::FORWARD_RIGHT;
			}
			//	forward
			else {
				wish_dir += EDirections::FORWARDS;
			}
		}
		else if (holding_s) {
			//	back left
			if (holding_a) {
				wish_dir += EDirections::BACK_LEFT;
			}
			//	back right
			else if (holding_d) {
				wish_dir += EDirections::BACK_RIGHT;
			}
			//	back
			else {
				wish_dir += EDirections::BACKWARDS;
			}
		}
		else if (holding_a) {
			//	left
			wish_dir += EDirections::LEFT;
		}
		else if (holding_d) {
			//	right
			wish_dir += EDirections::RIGHT;
		}
	}

	c_client::get()->m_cmd->m_forward_move = 0.f;
	c_client::get()->m_cmd->m_side_move = 0.f;

	const auto delta = remainderf(wish_dir -
		math::rad_to_deg(atan2(c_client::get()->m_local->m_vecVelocity().y, c_client::get()->m_local->m_vecVelocity().x)), 360.f);
	c_client::get()->m_cmd->m_side_move = delta > 0.f ? -450.f : 450.f;
	c_client::get()->m_strafe_angles.y = remainderf(wish_dir - delta, 360.f);
}

void c_movement::FixMove(CUserCmd *cmd, ang_t wish_angles) {
	vec3_t view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	math::AngleVectors(wish_angles, &view_fwd, &view_right, &view_up);
	math::AngleVectors(cmd->m_view_angles, &cmd_fwd, &cmd_right, &cmd_up);

	const auto v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const auto v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const auto v12 = sqrtf(view_up.z * view_up.z);

	const vec3_t norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const vec3_t norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const vec3_t norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const auto v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const auto v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const auto v18 = sqrtf(cmd_up.z * cmd_up.z);

	const vec3_t norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const vec3_t norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const vec3_t norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const auto v22 = norm_view_fwd.x * cmd->m_forward_move;
	const auto v26 = norm_view_fwd.y * cmd->m_forward_move;
	const auto v28 = norm_view_fwd.z * cmd->m_forward_move;
	const auto v24 = norm_view_right.x * cmd->m_side_move;
	const auto v23 = norm_view_right.y * cmd->m_side_move;
	const auto v25 = norm_view_right.z * cmd->m_side_move;
	const auto v30 = norm_view_up.x * cmd->m_up_move;
	const auto v27 = norm_view_up.z * cmd->m_up_move;
	const auto v29 = norm_view_up.y * cmd->m_up_move;

	cmd->m_forward_move = ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25))
		+ (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28)))
		+ (((norm_cmd_fwd.y * v30) + (norm_cmd_fwd.x * v29)) + (norm_cmd_fwd.z * v27));
	cmd->m_side_move = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25))
		+ (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28)))
		+ (((norm_cmd_right.x * v29) + (norm_cmd_right.y * v30)) + (norm_cmd_right.z * v27));
	cmd->m_up_move = ((((norm_cmd_up.x * v23) + (norm_cmd_up.y * v24)) + (norm_cmd_up.z * v25))
		+ (((norm_cmd_up.x * v26) + (norm_cmd_up.y * v22)) + (norm_cmd_up.z * v28)))
		+ (((norm_cmd_up.x * v30) + (norm_cmd_up.y * v29)) + (norm_cmd_up.z * v27));

	wish_angles = cmd->m_view_angles;

	if (c_client::get()->m_local->m_MoveType() != MOVETYPE_LADDER)
		cmd->m_buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVERIGHT | IN_MOVELEFT);
}

void c_movement::FastStop()
{
	if (c_client::get()->m_ground && c_config::get()->b["misc_fast_stop"])
	{
		if (!c_client::get()->m_pressing_move)
		{
			if (c_client::get()->m_speed > 15.f) {
				// convert velocity to angular momentum.
				ang_t angle;
				math::VectorAngles(c_client::get()->m_local->m_vecVelocity(), angle);

				// get our current speed of travel.
				float speed = c_client::get()->m_local->m_vecVelocity().length();

				// fix direction by factoring in where we are looking.
				angle.y = c_client::get()->m_view_angles.y - angle.y;

				// convert corrected angle back to a direction.
				vec3_t direction;
				math::AngleVectors(angle, &direction);

				vec3_t stop = direction * -speed;

				c_client::get()->m_cmd->m_forward_move = stop.x;
				c_client::get()->m_cmd->m_side_move = stop.y;
			}
			else {
				c_client::get()->m_cmd->m_forward_move = 0;
				c_client::get()->m_cmd->m_side_move = 0;
			}
			return;
		}
	}
}

void c_movement::AutoStop()
{
	if (c_client::get()->m_ground && c_config::get()->i["rbot_autostop_mode"] != 0) {

		vec3_t velocity{ c_client::get()->m_local->m_vecVelocity() };
		int    ticks{ }, max{ 7 };

		if (!c_client::get()->m_weapon_fire && !c_config::get()->b["rbot_between_shots"])
			return;

		if (c_key_handler::get()->auto_check("slowwalk_key"))
			return;

		if (c_aimbot::get()->m_stop) {
			if (c_client::get()->m_speed > 15.f) {
				// convert velocity to angular momentum.
				ang_t angle;
				math::VectorAngles(c_client::get()->m_local->m_vecVelocity(), angle);

				// get our current speed of travel.
				float speed = c_client::get()->m_local->m_vecVelocity().length();

				// fix direction by factoring in where we are looking.
				angle.y = c_client::get()->m_view_angles.y - angle.y;

				// convert corrected angle back to a direction.
				vec3_t direction;
				math::AngleVectors(angle, &direction);

				vec3_t stop = direction * -speed;

				c_client::get()->m_cmd->m_forward_move = stop.x;
				c_client::get()->m_cmd->m_side_move = stop.y;
			}
			else {
				c_client::get()->m_cmd->m_forward_move = 0;
				c_client::get()->m_cmd->m_side_move = 0;
			}
		}
		else if (c_aimbot::get()->m_slow_motion_slowwalk)
		{
			if (c_client::get()->m_weapon_info) {
				// get the max possible speed whilest we are still accurate.
				float flMaxSpeed = c_client::get()->m_local->m_bIsScoped() ? c_client::get()->m_weapon_info->flMaxPlayerSpeedAlt : c_client::get()->m_weapon_info->flMaxPlayerSpeed;
				float flDesiredSpeed = (flMaxSpeed * 0.33000001f);

				if (c_client::get()->m_speed > flDesiredSpeed) {
					// convert velocity to angular momentum.
					ang_t angle;
					math::VectorAngles(c_client::get()->m_local->m_vecVelocity(), angle);

					// get our current speed of travel.
					float speed = c_client::get()->m_local->m_vecVelocity().length();

					// fix direction by factoring in where we are looking.
					angle.y = c_client::get()->m_view_angles.y - angle.y;

					// convert corrected angle back to a direction.
					vec3_t direction;
					math::AngleVectors(angle, &direction);

					vec3_t stop = direction * -speed;

					c_client::get()->m_cmd->m_forward_move = stop.x;
					c_client::get()->m_cmd->m_side_move = stop.y;
				}
				else
				{
					ClampMovementSpeed(flDesiredSpeed);
				}
			}
		}
		else if (c_aimbot::get()->m_slow_motion_fakewalk)
		{
			// reference:
		// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp#L1612

		// calculate friction.
			float friction = c_csgo::get()->sv_friction->GetFloat() * c_client::get()->m_local->m_surfaceFriction();

			for (; ticks <= c_client::get()->m_max_lag; ++ticks) {
				// calculate speed.
				float speed = velocity.length();

				// if too slow return.
				if (speed <= 0.1f)
					break;

				// bleed off some speed, but if we have less than the bleed, threshold, bleed the threshold amount.
				float control = std::max(speed, c_csgo::get()->sv_stopspeed->GetFloat());

				// calculate the drop amount.
				float drop = control * friction * c_csgo::get()->m_globals->m_interval;

				// scale the velocity.
				float newspeed = std::max(0.f, speed - drop);

				if (newspeed != speed) {
					// determine proportion of old speed we are using.
					newspeed /= speed;

					// adjust velocity according to proportion.
					velocity *= newspeed;
				}
			}

			// zero forwardmove and sidemove.
			if (ticks > (max - c_client::get()->m_lag) || !c_client::get()->m_lag)
				c_client::get()->m_cmd->m_forward_move = c_client::get()->m_cmd->m_side_move = 0.f;

			// set bSendPacket.
			*c_client::get()->m_packet = !(c_client::get()->m_lag < max);
		}
	}
}

/*thanks onetap.com*/
void c_movement::ClampMovementSpeed(float speed) {
	float final_speed = speed;

	if (!c_client::get()->m_cmd || !c_client::get()->m_processing)
		return;

	c_client::get()->m_cmd->m_buttons |= IN_SPEED;

	float squirt = std::sqrtf((c_client::get()->m_cmd->m_forward_move * c_client::get()->m_cmd->m_forward_move) + (c_client::get()->m_cmd->m_side_move * c_client::get()->m_cmd->m_side_move));

	if (squirt > speed) {
		float squirt2 = std::sqrtf((c_client::get()->m_cmd->m_forward_move * c_client::get()->m_cmd->m_forward_move) + (c_client::get()->m_cmd->m_side_move * c_client::get()->m_cmd->m_side_move));

		float cock1 = c_client::get()->m_cmd->m_forward_move / squirt2;
		float cock2 = c_client::get()->m_cmd->m_side_move / squirt2;

		auto Velocity = c_client::get()->m_local->m_vecVelocity().length_2d();

		if (final_speed + 1.0 <= Velocity) {
			c_client::get()->m_cmd->m_forward_move = 0;
			c_client::get()->m_cmd->m_side_move = 0;
		}
		else {
			c_client::get()->m_cmd->m_forward_move = cock1 * final_speed;
			c_client::get()->m_cmd->m_side_move = cock2 * final_speed;
		}
	}
}

void c_movement::SlowMotion() {
	vec3_t velocity{ c_client::get()->m_local->m_vecVelocity() };
	int    ticks{ }, max{ 16 };

	if (!c_key_handler::get()->auto_check("slowwalk_key"))
		return;

	if (!c_client::get()->m_local->GetGroundEntity())
		return;

	if (c_config::get()->i["rbot_slow_mode"] == 0) {
		if (c_client::get()->m_weapon_info) {
			// get the max possible speed whilest we are still accurate.
			float flMaxSpeed = c_client::get()->m_local->m_bIsScoped() ? c_client::get()->m_weapon_info->flMaxPlayerSpeedAlt : c_client::get()->m_weapon_info->flMaxPlayerSpeed;
			float flDesiredSpeed = (flMaxSpeed * 0.33000001);

			ClampMovementSpeed(flDesiredSpeed);
		}
	}
	else if (c_config::get()->i["rbot_slow_mode"] == 1) {
		// reference:
		// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp#L1612

		// calculate friction.
		float friction = c_csgo::get()->sv_friction->GetFloat() * c_client::get()->m_local->m_surfaceFriction();

		for (; ticks <= c_client::get()->m_max_lag; ++ticks) {
			// calculate speed.
			float speed = velocity.length();

			// if too slow return.
			if (speed <= 0.1f)
				break;

			// bleed off some speed, but if we have less than the bleed, threshold, bleed the threshold amount.
			float control = std::max(speed, c_csgo::get()->sv_stopspeed->GetFloat());

			// calculate the drop amount.
			float drop = control * friction * c_csgo::get()->m_globals->m_interval;

			// scale the velocity.
			float newspeed = std::max(0.f, speed - drop);

			if (newspeed != speed) {
				// determine proportion of old speed we are using.
				newspeed /= speed;

				// adjust velocity according to proportion.
				velocity *= newspeed;
			}
		}

		// zero forwardmove and sidemove.
		if (ticks > (max - c_client::get()->m_lag) || !c_client::get()->m_lag)
			c_client::get()->m_cmd->m_forward_move = c_client::get()->m_cmd->m_side_move = 0.f;

		// set bSendPacket.
		*c_client::get()->m_packet = !(c_client::get()->m_lag < max);
	}
	else if (c_config::get()->i["rbot_slow_mode"] == 2) {
		vec3_t moveDir = vec3_t(0.f, 0.f, 0.f);
		float maxSpeed = 130.f; //can be 134 but sometimes I make a sound, 130 works perfectly.
		int movetype = c_client::get()->m_local->m_MoveType();
		bool InAir = !(c_client::get()->m_local->m_fFlags() & FL_ONGROUND);

		if (movetype == MOVETYPE_FLY || movetype == MOVETYPE_NOCLIP || InAir || c_client::get()->m_cmd->m_buttons & IN_DUCK || !(c_client::get()->m_cmd->m_buttons & IN_SPEED)) //IN_WALK doesnt work
			return;

		moveDir.x = c_client::get()->m_cmd->m_side_move;
		moveDir.y = c_client::get()->m_cmd->m_forward_move;
		if (sqrt((moveDir.x * moveDir.x) + (moveDir.y * moveDir.y)) > maxSpeed)
			moveDir = vec3_t((moveDir.x / sqrt((moveDir.x * moveDir.x) + (moveDir.y * moveDir.y))) * maxSpeed, (moveDir.y / sqrt((moveDir.x * moveDir.x) + (moveDir.y * moveDir.y))) * maxSpeed, 0);
		c_client::get()->m_cmd->m_side_move = moveDir.x;
		c_client::get()->m_cmd->m_forward_move = moveDir.y;
		if (!(velocity.length_2d() > maxSpeed + 1))
			c_client::get()->m_cmd->m_buttons &= ~IN_SPEED;
	}
}