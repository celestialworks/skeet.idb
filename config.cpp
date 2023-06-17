#include "config.h"

std::string color_to_string(float col[4]) {
	return std::to_string((int)(col[0] * 255)) + "," + std::to_string((int)(col[1] * 255)) + "," + std::to_string((int)(col[2] * 255)) + "," + std::to_string((int)(col[3] * 255));
}

float* string_to_color(std::string s) {
	static auto split = [](std::string str, const char* del) -> std::vector<std::string>
	{
		char* pTempStr = _strdup(str.c_str());
		char* pWord = strtok(pTempStr, del);
		std::vector<std::string> dest;

		while (pWord != NULL)
		{
			dest.push_back(pWord);
			pWord = strtok(NULL, del);
		}

		free(pTempStr);

		return dest;
	};

	std::vector<std::string> col = split(s, ",");
	return new float[4]{
		(float)std::stoi(col.at(0)) / 255.f,
		(float)std::stoi(col.at(1)) / 255.f,
		(float)std::stoi(col.at(2)) / 255.f,
		(float)std::stoi(col.at(3)) / 255.f
	};
}

void c_config::save() {
	char file_path[MAX_PATH] = { 0 };
	sprintf(file_path, "C:/skeet.idb/csgo_%s.ini", presets.at(i["_preset"]));

	for (auto e : b) {
		if (!std::string(e.first).find("_")) continue;
		char buffer[8] = { 0 }; _itoa(e.second, buffer, 10);
		WritePrivateProfileStringA("b", e.first.c_str(), std::string(buffer).c_str(), file_path);
	}

	for (auto e : i) {
		if (!std::string(e.first).find("_")) continue;
		char buffer[32] = { 0 }; _itoa(e.second, buffer, 10);
		WritePrivateProfileStringA("i", e.first.c_str(), std::string(buffer).c_str(), file_path);
	}

	for (auto e : f) {
		if (!std::string(e.first).find("_")) continue;
		char buffer[64] = { 0 }; sprintf(buffer, "%f", e.second);
		WritePrivateProfileStringA("f", e.first.c_str(), std::string(buffer).c_str(), file_path);
	}

	for (auto e : c) {
		if (!std::string(e.first).find("_")) continue;
		WritePrivateProfileStringA("c", e.first.c_str(), color_to_string(e.second).c_str(), file_path);
	}

	for (auto e : m) {
		if (!std::string(e.first).find("_")) continue;

		std::string vs = "";
		for (auto v : e.second)
			vs += std::to_string(v.first) + ":" + std::to_string(v.second) + "|";

		WritePrivateProfileStringA("m", e.first.c_str(), vs.c_str(), file_path);
	}

	this->save_keys();
}

void c_config::load() {
	this->load_defaults();

	char file_path[MAX_PATH] = { 0 };
	sprintf(file_path, "C:/skeet.idb/csgo_%s.ini", presets.at(i["_preset"]));

	char b_buffer[65536], i_buffer[65536], f_buffer[65536], c_buffer[65536], m_buffer[65536] = { 0 };

	int b_read = GetPrivateProfileSectionA("b", b_buffer, 65536, file_path);
	int i_read = GetPrivateProfileSectionA("i", i_buffer, 65536, file_path);
	int f_read = GetPrivateProfileSectionA("f", f_buffer, 65536, file_path);
	int c_read = GetPrivateProfileSectionA("c", c_buffer, 65536, file_path);
	int m_read = GetPrivateProfileSectionA("m", m_buffer, 65536, file_path);

	if ((0 < b_read) && ((65536 - 2) > b_read)) {
		const char* pSubstr = b_buffer;

		while ('\0' != *pSubstr) {
			size_t substrLen = strlen(pSubstr);

			const char* pos = strchr(pSubstr, '=');
			if (NULL != pos) {
				char name[256] = "";
				char value[256] = "";

				strncpy_s(name, _countof(name), pSubstr, pos - pSubstr);
				strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pSubstr));

				b[name] = atoi(value);
			}

			pSubstr += (substrLen + 1);
		}
	}

	if ((0 < i_read) && ((65536 - 2) > i_read)) {
		const char* pSubstr = i_buffer;

		while ('\0' != *pSubstr) {
			size_t substrLen = strlen(pSubstr);

			const char* pos = strchr(pSubstr, '=');
			if (NULL != pos) {
				char name[256] = "";
				char value[256] = "";

				strncpy_s(name, _countof(name), pSubstr, pos - pSubstr);
				strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pSubstr));

				i[name] = atoi(value);
			}

			pSubstr += (substrLen + 1);
		}
	}

	if ((0 < f_read) && ((65536 - 2) > f_read)) {
		const char* pSubstr = f_buffer;

		while ('\0' != *pSubstr) {
			size_t substrLen = strlen(pSubstr);

			const char* pos = strchr(pSubstr, '=');
			if (NULL != pos) {
				char name[256] = "";
				char value[256] = "";

				strncpy_s(name, _countof(name), pSubstr, pos - pSubstr);
				strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pSubstr));

				f[name] = atof(value);
			}

			pSubstr += (substrLen + 1);
		}
	}

	if ((0 < c_read) && ((65536 - 2) > c_read)) {
		const char* pSubstr = c_buffer;

		while ('\0' != *pSubstr) {
			size_t substrLen = strlen(pSubstr);

			const char* pos = strchr(pSubstr, '=');
			if (NULL != pos) {
				char name[256] = "";
				char value[256] = "";

				strncpy_s(name, _countof(name), pSubstr, pos - pSubstr);
				strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pSubstr));

				auto col = string_to_color(value);
				c[name][0] = col[0];
				c[name][1] = col[1];
				c[name][2] = col[2];
				c[name][3] = col[3];
			}

			pSubstr += (substrLen + 1);
		}
	}

	static auto split = [](std::string str, const char* del) -> std::vector<std::string>
	{
		char* pTempStr = _strdup(str.c_str());
		char* pWord = strtok(pTempStr, del);
		std::vector<std::string> dest;

		while (pWord != NULL)
		{
			dest.push_back(pWord);
			pWord = strtok(NULL, del);
		}

		free(pTempStr);

		return dest;
	};

	if ((0 < m_read) && ((65536 - 2) > m_read)) {
		const char* pSubstr = m_buffer;

		while ('\0' != *pSubstr) {
			size_t substrLen = strlen(pSubstr);

			const char* pos = strchr(pSubstr, '=');
			if (NULL != pos) {
				char name[256] = "";
				char value[256] = "";

				strncpy_s(name, _countof(name), pSubstr, pos - pSubstr);
				strncpy_s(value, _countof(value), pos + 1, substrLen - (pos - pSubstr));

				std::vector<std::string> kvpa = split(value, "|");
				std::unordered_map<int, bool> vl = {};
				for (auto kvp : kvpa) {
					if (kvp == "")
						continue; // ало глухой

					std::vector<std::string> kv = split(kvp, ":");
					vl[std::stoi(kv.at(0))] = std::stoi(kv.at(1));
				}

				m[name] = vl;
			}

			pSubstr += (substrLen + 1);
		}
	}

	// хуй

	this->load_keys();
}

void c_config::load_defaults() {
	int _preset = this->i["_preset"];

	b = std::unordered_map<std::string, bool>();
	i = std::unordered_map<std::string, int>();
	f = std::unordered_map<std::string, float>();
	c = std::unordered_map<std::string, float[4]>();

	i["_preset"] = _preset;

	c["menu_color"][0] = 0.937f;
	c["menu_color"][1] = 0.035f;
	c["menu_color"][2] = 0.373f;
	c["menu_color"][3] = 1.00f;
	
	c["local_chams_color"][0] = 0.937f;
	c["local_chams_color"][1] = 0.035f;
	c["local_chams_color"][2] = 0.373f;
	c["local_chams_color"][3] = 1.00f;
	
	c["chams_history_color"][0] = 0.937f;
	c["chams_history_color"][1] = 0.035f;
	c["chams_history_color"][2] = 0.373f;
	c["chams_history_color"][3] = 1.00f;
	
	c["chams_enemy_color"][0] = 0.937f;
	c["chams_enemy_color"][1] = 0.035f;
	c["chams_enemy_color"][2] = 0.373f;
	c["chams_enemy_color"][3] = 1.00f;
	
	c["chams_invis_enemy_color"][0] = 0.937f;
	c["chams_invis_enemy_color"][1] = 0.035f;
	c["chams_invis_enemy_color"][2] = 0.373f;
	c["chams_invis_enemy_color"][3] = 1.00f;
	
	c["spread_crosshair_color"][0] = 0.937f;
	c["spread_crosshair_color"][1] = 0.035f;
	c["spread_crosshair_color"][2] = 0.373f;
	c["spread_crosshair_color"][3] = 1.00f;
	
	c["projecties_color"][0] = 0.937f;
	c["projecties_color"][1] = 0.035f;
	c["projecties_color"][2] = 0.373f;
	c["projecties_color"][3] = 1.00f;
	
	c["offscreen_color"][0] = 0.937f;
	c["offscreen_color"][1] = 0.035f;
	c["offscreen_color"][2] = 0.373f;
	c["offscreen_color"][3] = 1.00f;
	
	c["box_color"][0] = 0.937f;
	c["box_color"][1] = 0.035f;
	c["box_color"][2] = 0.373f;
	c["box_color"][3] = 1.00f;
	
	c["glow_color"][0] = 0.937f;
	c["glow_color"][1] = 0.035f;
	c["glow_color"][2] = 0.373f;
	c["glow_color"][3] = 1.00f;
	
	c["beams_color"][0] = 0.937f;
	c["beams_color"][1] = 0.035f;
	c["beams_color"][2] = 0.373f;
	c["beams_color"][3] = 1.00f;

	c["noscope_color"][0] = 0.f;
	c["noscope_color"][1] = 0.f;
	c["noscope_color"][2] = 0.f;
	c["noscope_color"][3] = 1.00f;

	i["rbot_fakelag_variance"] = 1;
	i["rbot_fakelag_limit"] = 2;
	i["rbot_baim_hp"] = 1;
	i["rbot_baim_key"] = 0;
	i["rbot_baim_keystyle"] = 1;
	i["rbot_autostop_mode"] = 0;
	i["rbot_autoscope_mode"] = 0;
	i["rbot_hitchance"] = 1;
	i["rbot_min_damage"] = 1;
	i["rbot_penetration_min_damage"] = 1;
	i["rbot_target_selection"] = 0;
	i["glow_blend"] = 100;

	i["rbot_headscale"] = 1;
	i["rbot_chestscale"] = 1;
	i["rbot_bodyscale"] = 1;
	i["rbot_legsscale"] = 1;

	i["local_chams_blend"] = 100;
	i["chams_history_blend"] = 100;
	i["chams_enemy_blend"] = 100;
	
	i["misc_clantag_mode"] = 0;
	i["visuals_color_modulation_mode"] = 0;

	i["fakeduck_key"] = 0;
	i["fakeduck_keystyle"] = 1;
	i["slowwalk_key"] = 0;
	i["slowwalk_keystyle"] = 1;

	i["rbot_aa_pitch"] = 0;
	i["rbot_aa_yaw"] = 0;
	i["rbot_aa_jitter_range"] = 0;
	i["rbot_aa_rot_range"] = 0;
	i["rbot_aa_rot_speed"] = 0;
	i["rbot_aa_rand_update"] = 0;
	i["rbot_aa_dir"] = 0;
	i["rbot_aa_dir_custom"] = 0;
	i["rbot_aa_base_angle"] = 0;
	i["rbot_aa_auto_time"] = 0;

	i["rbot_aa_fakelag_mode"] = 0;

	i["rbot_slow_mode"] = 0;
	i["visuals_fov_amount"] = 90;
	i["visuals_viewmodel_fov_amount"] = 90;

	i["thirdperson_distance"] = 0;

	i["exploits_key"] = 0;
	i["exploits_keystyle"] = 1;
	i["thirdperson_key"] = 0;
	i["thirdperson_keystyle"] = 1;

	i["desync_invert_key"] = 0;
	i["desync_invert_keystyle"] = 1;

	i["fog_start"] = 1;
	i["fog_end"] = 1;

	b["misc_killfeed"] = false;
	b["rbot_resolver"] = false;
	b["rbot_enable"] = false;
	b["rbot_mindamage_scale"] = false;

	b["chams_local_blend_in_scope"] = false;
	b["chams_local"] = false;
	b["chams_enemy_history"] = false;
	b["chams_enemy"] = false;

	b["misc_clantag"] = false;

	b["visuals_disable_team"] = false;
	b["force_crosshair"] = false;
	b["grenade_tracer"] = false;
	b["ragdoll_force"] = false;

	b["rbot_aa_enable"] = false;
	b["rbot_aa_edge"] = false;
	b["rbot_aa_lock"] = false;
	b["rbot_aa_desync"] = false;

	b["rbot_aa_fakelag_enable"] = false;
	b["rbot_aa_fakelag_land"] = false;

	b["visuals_impact_beams"] = false;
	b["visuals_hitmarker"] = false;

	b["misc_bhop"] = false;
	b["misc_air_duck"] = false;
	b["misc_autostrafe"] = false;
	b["misc_fast_stop"] = false;

	b["rbot_between_shots"] = false;

	b["visuals_nofog"] = false;
	b["visuals_noscope"] = false;
	b["rbot_wait_for_charge"] = false;
	b["visuals_fov"] = false;
	b["visuals_viewmodel_fov"] = false;

	b["no_smoke"] = false;
	b["no_props"] = false;

	b["visuals_spread_crosshair"] = false;
	b["visuals_items"] = false;
	b["visuals_projecties"] = false;
	b["visuals_ammo"] = false;
	b["visuals_offscreen"] = false;
	b["visuals_skeleton"] = false;
	b["visuals_box"] = false;
	b["visuals_name"] = false;
	b["visuals_health"] = false;
	b["visuals_weapon"] = false;
	b["visuals_glow"] = false;
	b["developer_mode"] = false;
	b["fog_override"] = false;
}

void c_config::load_keys() {
	for (int k = 0; k < presets.size(); k++) {
		char buffer[32] = { 0 }; sprintf(buffer, "_preset_%i", k);
		i[buffer] = GetPrivateProfileIntA("k", buffer, 0, "C:/skeet.idb/csgo_keys.ini");
	}
}

void c_config::save_keys() {
	for (int k = 0; k < presets.size(); k++) {
		char buffer[32] = { 0 }; sprintf(buffer, "_preset_%i", k);
		char value[32] = { 0 }; sprintf(value, "%i", i[buffer]);
		WritePrivateProfileStringA("k", buffer, value, "C:/skeet.idb/csgo_keys.ini");
	}
}