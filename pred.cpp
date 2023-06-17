#include "includes.h"
#include "pred.h"

InputPrediction g_inputpred{};;

void InputPrediction::update( ) {
	bool        valid{ c_csgo::get()->m_cl->m_delta_tick > 0 };

	// render start was not called.
	if( c_client::get()->m_stage == FRAME_NET_UPDATE_END ) {
		int start = c_csgo::get()->m_cl->m_last_command_ack;
		int stop  = c_csgo::get()->m_cl->m_last_outgoing_command + c_csgo::get()->m_cl->m_choked_commands;

		// call CPrediction::Update.
		c_csgo::get()->m_prediction->Update( c_csgo::get()->m_cl->m_delta_tick, valid, start, stop );
	}
}

void InputPrediction::run( ) {
	static CMoveData data{};

	c_csgo::get()->m_prediction->m_in_prediction = true;

	// CPrediction::StartCommand
	c_client::get()->m_local->m_pCurrentCommand( ) = c_client::get()->m_cmd;
	c_client::get()->m_local->m_PlayerCommand( )   = *c_client::get()->m_cmd;

	*c_csgo::get()->m_nPredictionRandomSeed = c_client::get()->m_cmd->m_random_seed;
	c_csgo::get()->m_pPredictionPlayer      = c_client::get()->m_local;

	// backup globals.
	m_curtime   = c_csgo::get()->m_globals->m_curtime;
	m_frametime = c_csgo::get()->m_globals->m_frametime;

	// CPrediction::RunCommand

	// set globals appropriately.
	c_csgo::get()->m_globals->m_curtime   = game::TICKS_TO_TIME( c_client::get()->m_local->m_nTickBase( ) );
	c_csgo::get()->m_globals->m_frametime = c_csgo::get()->m_prediction->m_engine_paused ? 0.f : c_csgo::get()->m_globals->m_interval;

	// set target player ( host ).
	c_csgo::get()->m_move_helper->SetHost( c_client::get()->m_local );
	c_csgo::get()->m_game_movement->StartTrackPredictionErrors( c_client::get()->m_local );

	// setup input.
	c_csgo::get()->m_prediction->SetupMove( c_client::get()->m_local, c_client::get()->m_cmd, c_csgo::get()->m_move_helper, &data );

	// run movement.
	c_csgo::get()->m_game_movement->ProcessMovement( c_client::get()->m_local, &data );
	c_csgo::get()->m_prediction->FinishMove( c_client::get()->m_local, c_client::get()->m_cmd, &data );
	c_csgo::get()->m_game_movement->FinishTrackPredictionErrors( c_client::get()->m_local );

	// reset target player ( host ).
	c_csgo::get()->m_move_helper->SetHost( nullptr );
}

void InputPrediction::restore( ) {
	c_csgo::get()->m_prediction->m_in_prediction = false;

	*c_csgo::get()->m_nPredictionRandomSeed = -1;
	c_csgo::get()->m_pPredictionPlayer      = nullptr;

	// restore globals.
	c_csgo::get()->m_globals->m_curtime   = m_curtime;
	c_csgo::get()->m_globals->m_frametime = m_frametime;
}