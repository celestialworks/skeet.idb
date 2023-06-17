#include "includes.h"

void Hooks::PaintTraverse( VPANEL panel, bool repaint, bool force ) {
	static VPANEL tools{}, zoom{};

	// cache CHudZoom panel once.
	if( !zoom && FNV1a::get( c_csgo::get()->m_panel->GetName( panel ) ) == HASH( "HudZoom" ) )
		zoom = panel;

	// cache tools panel once.
	if( !tools && panel == c_csgo::get()->m_engine_vgui->GetPanel( PANEL_TOOLS ) )
		tools = panel;

	// render hack stuff.
	if( panel == tools )
		c_client::get()->OnPaint( );

	// don't call the original function if we want to remove the scope.
	if( panel == zoom && c_config::get()->b["visuals_noscope"])
		return;

	g_hooks.m_panel.GetOldMethod< PaintTraverse_t >( IPanel::PAINTTRAVERSE )( this, panel, repaint, force );
}