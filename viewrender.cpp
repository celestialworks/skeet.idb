#include "includes.h"

float fov = 1.f;

void Hooks::OnRenderStart() {
	// call og.
	g_hooks.m_view_render.GetOldMethod< OnRenderStart_t >(CViewRender::ONRENDERSTART)(this);

	if (c_config::get()->b["visuals_fov"]) {
		if (c_client::get()->m_local && c_client::get()->m_local->m_bIsScoped()) {
			if (c_client::get()->m_weapon && c_client::get()->m_weapon->m_zoomLevel() != 2) {
				c_csgo::get()->m_view_render->m_view.m_fov = c_config::get()->i["visuals_fov_amount"];
			}
			else {
				c_csgo::get()->m_view_render->m_view.m_fov += 45.f;
			}
		}

		else c_csgo::get()->m_view_render->m_view.m_fov = c_config::get()->i["visuals_fov_amount"];
	}

	if (c_config::get()->b["visuals_viewmodel_fov"])
		c_csgo::get()->m_view_render->m_view.m_viewmodel_fov = c_config::get()->i["visuals_viewmodel_fov_amount"];

	fov = c_csgo::get()->m_view_render->m_view.m_fov;
}

void Hooks::RenderView( const CViewSetup &view, const CViewSetup &hud_view, int clear_flags, int what_to_draw ) {
	// ...

	g_hooks.m_view_render.GetOldMethod< RenderView_t >( CViewRender::RENDERVIEW )( this, view, hud_view, clear_flags, what_to_draw );
}

void Hooks::Render2DEffectsPostHUD( const CViewSetup &setup ) {
	if( !c_config::get()->b["no_smoke"])
		g_hooks.m_view_render.GetOldMethod< Render2DEffectsPostHUD_t >( CViewRender::RENDER2DEFFECTSPOSTHUD )( this, setup );
}

void Hooks::RenderSmokeOverlay( bool unk ) {
	// do not render smoke overlay.
	if( !c_config::get()->b["no_smoke"])
		g_hooks.m_view_render.GetOldMethod< RenderSmokeOverlay_t >( CViewRender::RENDERSMOKEOVERLAY )( this, unk );
}
