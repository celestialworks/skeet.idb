#include "includes.h"

bool Hooks::OverrideConfig( MaterialSystem_Config_t* config, bool update ) {
	if(c_config::get()->i["visuals_color_modulation_mode"] == 2 )
		config->m_nFullbright = true;

	if (c_client::get()->m_update_night) {
		c_visuals::get()->ModulateWorld();

		c_client::get()->m_update_night = false;
	}

	return g_hooks.m_material_system.GetOldMethod< OverrideConfig_t >( IMaterialSystem::OVERRIDECONFIG )( this, config, update );
}