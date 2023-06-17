#include "includes.h"

#define dont

TickbaseSystem g_tickbase;

void TickbaseSystem::WriteUserCmd(bf_write* buf, CUserCmd* incmd, CUserCmd* outcmd) {
	// Generic WriteUserCmd code as seen in every hack ever
	using WriteUsercmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static WriteUsercmd_t fnWriteUserCmd = pattern::find(c_csgo::get()->m_client_dll, XOR("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D")).as<WriteUsercmd_t>();

	__asm {
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    fnWriteUserCmd
		add     esp, 4
	}
}

bool Hooks::WriteUsercmdDeltaToBuffer(int m_nSlot, void* m_pBuffer, int m_nFrom, int m_nTo, bool m_bNewCmd) {
	if (g_tickbase.m_shift_data.m_ticks_to_shift <= 0)
		return m_client.GetOldMethod<WriteUsercmdDeltaToBuffer_t>(24)(this, m_nSlot, m_pBuffer, m_nFrom, m_nTo, m_bNewCmd);

	if (m_nFrom != -1)
		return true;

	m_nFrom = -1;

	int m_nTickbase = g_tickbase.m_shift_data.m_ticks_to_shift;
	g_tickbase.m_shift_data.m_ticks_to_shift = 0;

	int* m_pnNewCmds = (int*)((uintptr_t)m_pBuffer - 0x2C);
	int* m_pnBackupCmds = (int*)((uintptr_t)m_pBuffer - 0x30);

	*m_pnBackupCmds = 0;

	int m_nNewCmds = *m_pnNewCmds;
	int m_nNextCmd = c_csgo::get()->m_cl->m_choked_commands + c_csgo::get()->m_cl->m_last_outgoing_command + 1;
	int m_nTotalNewCmds = std::min(m_nNewCmds + abs(m_nTickbase), 16);

	*m_pnNewCmds = m_nTotalNewCmds;

	for (m_nTo = m_nNextCmd - m_nNewCmds + 1; m_nTo <= m_nNextCmd; m_nTo++) {
		if (!m_client.GetOldMethod<WriteUsercmdDeltaToBuffer_t>(24)(this, m_nSlot, m_pBuffer, m_nFrom, m_nTo, true))
			return false;

		m_nFrom = m_nTo;
	}

	CUserCmd* m_pCmd = c_csgo::get()->m_input->GetUserCmd(m_nSlot, m_nFrom);
	if (!m_pCmd)
		return true;

	CUserCmd m_ToCmd = *m_pCmd, m_FromCmd = *m_pCmd;
	m_ToCmd.m_command_number++;
	m_ToCmd.m_tick += 3 * c_client::get()->m_rate;

	for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++) {
		g_tickbase.WriteUserCmd((bf_write*)m_pBuffer, &m_ToCmd, &m_FromCmd);
		m_FromCmd.m_buttons = m_ToCmd.m_buttons;
		m_FromCmd.m_view_angles.x = m_ToCmd.m_view_angles.x;
		m_FromCmd.m_impulse = m_ToCmd.m_impulse;
		m_FromCmd.m_weapon_select = m_ToCmd.m_weapon_select;
		m_FromCmd.m_aimdirection.y = m_ToCmd.m_aimdirection.y;
		m_FromCmd.m_weapon_subtype = m_ToCmd.m_weapon_subtype;
		m_FromCmd.m_up_move = m_ToCmd.m_up_move;
		m_FromCmd.m_random_seed = m_ToCmd.m_random_seed;
		m_FromCmd.m_mousedx = m_ToCmd.m_mousedx;
		m_FromCmd.m_command_number = m_ToCmd.m_command_number;
		m_FromCmd.m_tick = m_ToCmd.m_tick;
		m_FromCmd.m_mousedy = m_ToCmd.m_mousedy;
		m_FromCmd.m_predicted = m_ToCmd.m_predicted;
		// m_FromCmd = m_ToCmd;

		m_ToCmd.m_command_number++;
		m_ToCmd.m_tick++;
	}

	g_tickbase.m_shift_data.m_current_shift = m_nTickbase;
	return true;
}

void TickbaseSystem::PreMovement() {

	// Perform sanity checks to make sure we're able to shift
	if (!c_client::get()->m_processing) {
		return;
	}

	if (!c_client::get()->m_cmd || !c_client::get()->m_weapon) {
		return;
	}

	// Invalidate next shift amount and the ticks to shift prior to shifting
	g_tickbase.m_shift_data.m_next_shift_amount = g_tickbase.m_shift_data.m_ticks_to_shift = 0;
}

void TickbaseSystem::PostMovement() {
	// Perform sanity checks to make sure we're able to shift
	if (!c_client::get()->m_processing) {
		return;
	}

	if (!c_client::get()->m_cmd || !c_client::get()->m_weapon) {
		return;
	}

	if (c_client::get()->m_weapon_id == REVOLVER ||
		c_client::get()->m_weapon_id == C4 ||
		c_client::get()->m_weapon_type == WEAPONTYPE_KNIFE ||
		c_client::get()->m_weapon->IsGrenade())
	{
		g_tickbase.m_shift_data.m_did_shift_before = false;
		g_tickbase.m_shift_data.m_should_be_ready = false;
		g_tickbase.m_shift_data.m_should_disable = true;
		return;
	}

	// Don't attempt to shift if we're supposed to
	if (!g_tickbase.m_shift_data.m_should_attempt_shift) {
		g_tickbase.m_shift_data.m_did_shift_before = false;
		g_tickbase.m_shift_data.m_should_be_ready = false;
		return;
	}

	g_tickbase.m_shift_data.m_should_disable = false;

	// Setup variables we will later use in order to dermine if we're able to shift tickbase or not

	// This could be made into a checkbox 
	bool bFastRecovery = false;

	float flNonShiftedTime = game::TICKS_TO_TIME(c_client::get()->m_local->m_nTickBase() - c_client::get()->m_goal_shift);

	// Determine if we are able to shoot right now (at the time of the shift)
	bool bCanShootNormally = c_client::get()->CanFireWeapon(c_client::get()->m_local->m_nTickBase() - c_client::get()->m_goal_shift);

	// Determine if we are able to shoot in the previous iShiftAmount ticks (iShiftAmount ticks before we shifted)
	bool bCanShootIn12Ticks = c_client::get()->CanFireWeapon(flNonShiftedTime);

	bool bIsShooting = c_client::get()->IsFiring(game::TICKS_TO_TIME(c_client::get()->m_local->m_nTickBase()));

	// Determine if we are able to shift the tickbase respective to previously setup variables (rofl)
	g_tickbase.m_shift_data.m_can_shift_tickbase = bCanShootIn12Ticks || (!bCanShootNormally || bFastRecovery) && (g_tickbase.m_shift_data.m_did_shift_before);

	// If we can shift tickbase, shift enough ticks in order to double-tap
	// Always prioritise fake-duck if we wish to
	if (g_tickbase.m_shift_data.m_can_shift_tickbase && !c_hvh::get()->m_fakeduck) {
		// Tell the cheat to shift tick-base and disable fakelag
		g_tickbase.m_shift_data.m_next_shift_amount = c_client::get()->m_goal_shift;
	}
	else {
		g_tickbase.m_shift_data.m_next_shift_amount = 0;
		g_tickbase.m_shift_data.m_should_be_ready = false;
	}

	// we want to recharge after stopping fake duck.
	if (c_hvh::get()->m_fakeduck) {
		g_tickbase.m_shift_data.m_prepare_recharge = true;

		g_tickbase.m_shift_data.m_next_shift_amount = 0;
		g_tickbase.m_shift_data.m_should_be_ready = false;

		g_tickbase.m_shift_data.m_should_disable = true;

		return;
	}

	// Are we even supposed to shift tickbase?
	if (g_tickbase.m_shift_data.m_next_shift_amount > 0) {
		// Prevent m_iTicksAllowedForProcessing from being incremented.
		// g_cl.m_cmd->m_tick = INT_MAX;
		// Determine if we're able to double-tap  
		if (bCanShootIn12Ticks) {
			if (g_tickbase.m_shift_data.m_prepare_recharge && !bIsShooting) {
				g_tickbase.m_shift_data.m_needs_recharge = c_client::get()->m_goal_shift;
				g_tickbase.m_shift_data.m_prepare_recharge = false;
			}
			else {
				if (bIsShooting) {
					// Store shifted command
					g_tickbase.m_prediction.m_shifted_command = c_client::get()->m_cmd->m_command_number;

					// Store shifted ticks
					g_tickbase.m_prediction.m_shifted_ticks = abs(g_tickbase.m_shift_data.m_current_shift);

					// Store original tickbase
					g_tickbase.m_prediction.m_original_tickbase = c_client::get()->m_local->m_nTickBase();

					// Update our wish ticks to shift, and later shift tickbase
					g_tickbase.m_shift_data.m_ticks_to_shift = g_tickbase.m_shift_data.m_next_shift_amount;
				}
			}
		}
		else {
			g_tickbase.m_shift_data.m_prepare_recharge = true;
			g_tickbase.m_shift_data.m_should_be_ready = false;
		}
	}
	else {
		g_tickbase.m_shift_data.m_prepare_recharge = true;
		g_tickbase.m_shift_data.m_should_be_ready = false;
	}

	// Note DidShiftBefore state 
	g_tickbase.m_shift_data.m_did_shift_before = g_tickbase.m_shift_data.m_next_shift_amount > 0;
}