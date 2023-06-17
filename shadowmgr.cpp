#include "includes.h"

void Hooks::ComputeShadowDepthTextures( const CViewSetup &view, bool unk ) {
	if( !unk )
		return g_hooks.m_shadow_mgr.GetOldMethod< ComputeShadowDepthTextures_t >( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES )( this, view, unk );

	if(c_config::get()->b["visuals_disable_team"]) {
		for( int i{ 1 }; i <= c_csgo::get()->m_globals->m_max_clients; ++i ) {
			Player* player = c_csgo::get()->m_entlist->GetClientEntity< Player* >( i );

			if( !player )
				continue;

			if( player->m_bIsLocalPlayer( ) )
				continue;

			if( !player->enemy( c_client::get()->m_local ) ) {
				// disable all rendering at the root.
				player->m_bReadyToDraw( ) = false;

				// now stop rendering their weapon.
				Weapon* weapon = player->GetActiveWeapon( );
				if( weapon )
					weapon->m_bReadyToDraw( ) = false;
			}
		}
	}

	g_hooks.m_shadow_mgr.GetOldMethod< ComputeShadowDepthTextures_t >( IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES )( this, view, unk );
}