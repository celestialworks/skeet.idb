#include "includes.h"

bool Hooks::ShouldDrawParticles( ) {
	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawParticles_t >( IClientMode::SHOULDDRAWPARTICLES )( this );
}

bool Hooks::ShouldDrawFog( ) {
	// remove fog.
	if( c_config::get()->b["visuals_nofog"] )
		return false;

	return g_hooks.m_client_mode.GetOldMethod< ShouldDrawFog_t >( IClientMode::SHOULDDRAWFOG )( this );
}

void Hooks::OverrideView( CViewSetup* view ) {
	// damn son.
	c_client::get()->m_local = c_csgo::get()->m_entlist->GetClientEntity< Player* >( c_csgo::get()->m_engine->GetLocalPlayer( ) );

	// g_grenades.think( );
	c_visuals::get()->ThirdpersonThink( );

	if (c_client::get()->m_local && c_client::get()->m_local->alive() && c_key_handler::get()->auto_check("fakeduck_key"))
		view->m_origin.z = c_client::get()->m_local->GetAbsOrigin().z + 64.f;

    // call original.
	g_hooks.m_client_mode.GetOldMethod< OverrideView_t >( IClientMode::OVERRIDEVIEW )( this, view );
}

bool Hooks::CreateMove( float time, CUserCmd* cmd ) {
	Stack   stack;
	bool    ret;

	// let original run first.
	ret = g_hooks.m_client_mode.GetOldMethod< CreateMove_t >( IClientMode::CREATEMOVE )( this, time, cmd );

	// called from CInput::ExtraMouseSample -> return original.
	if( !cmd->m_command_number )
		return ret;

	// if we arrived here, called from -> CInput::CreateMove
	// call EngineClient::SetViewAngles according to what the original returns.
	if( ret )
		c_csgo::get()->m_engine->SetViewAngles( cmd->m_view_angles );

	// random_seed isn't generated in ClientMode::CreateMove yet, we must set generate it ourselves.
	cmd->m_random_seed = c_csgo::get()->MD5_PseudoRandom( cmd->m_command_number ) & 0x7fffffff;

	// get bSendPacket off the stack.
	c_client::get()->m_packet = stack.next( ).local( 0x1c ).as< bool* >( );

	// get bFinalTick off the stack.
	c_client::get()->m_final_packet = stack.next( ).local( 0x1b ).as< bool* >( );

	if (c_key_handler::get()->auto_check("exploits_key") && !c_client::get()->m_charged && c_config::get()->b["rbot_wait_for_charge"]) {
		// are we IN_ATTACK?
		if (cmd->m_buttons & IN_ATTACK) {
			// remove the flag :D!
			cmd->m_buttons &= ~IN_ATTACK;
		}
	}

	// invoke move function.
	c_client::get()->OnTick( cmd );

	if (c_key_handler::get()->auto_check("exploits_key") && !c_client::get()->m_charged && c_config::get()->b["rbot_wait_for_charge"]) {
		// are we IN_ATTACK?
		if (cmd->m_buttons & IN_ATTACK) {
			// remove the flag :D!
			cmd->m_buttons &= ~IN_ATTACK;
		}
	}

	if (c_hvh::get()->m_should_work) {
		if (*c_client::get()->m_packet)
			c_client::get()->m_angle = cmd->m_view_angles;
		else
			c_client::get()->m_real_angle = cmd->m_view_angles;
	}
	else {
		c_client::get()->m_angle = c_client::get()->m_real_angle = cmd->m_view_angles;
	}

	// disable all stop funct on next tick.
	c_aimbot::get()->m_stop = false;
	c_aimbot::get()->m_slow_motion_slowwalk = false;
	c_aimbot::get()->m_slow_motion_fakewalk = false;

	return false;
}

bool Hooks::DoPostScreenSpaceEffects( CViewSetup* setup ) {
	c_visuals::get()->RenderGlow( );

	return g_hooks.m_client_mode.GetOldMethod< DoPostScreenSpaceEffects_t >( IClientMode::DOPOSTSPACESCREENEFFECTS )( this, setup );
}