#include "imgui_menu.h"
#include "imgui/imgui_internal.h"
#include "fonts.h"
#include "imgui/bindsfont.h"
#include "imgui/imgui_freetype.h"
#include "imgui/bgtexture.h"
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "d3dx9.lib")

IDirect3DTexture9* imgxd = nullptr;
ImFont* tabs;
ImFont* def;
static int MenuTabs = 0;
static int RageSubTabs = 1, AntiAimsSubTabs = 1, LegitSubTabs = 1, VisualsSubTabs = 1, MiscSubTabs = 1, ScriptsSubTabs = 1;
static int lua_selected = 0;

std::vector<std::string> cfgList;
void ReadDirectory(const std::string& name, std::vector<std::string>& v)
{
    v.clear();
    auto pattern(name);
    pattern.append("\\*.ini");
    WIN32_FIND_DATAA data;
    HANDLE hFind;
    if ((hFind = FindFirstFileA(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            v.emplace_back(data.cFileName);
        } while (FindNextFileA(hFind, &data) != 0);
        FindClose(hFind);
    }
}

void c_menu::init(IDirect3DDevice9* device)
{
	if (this->initialized)
		return;

	ui::CreateContext();
	auto io = ui::GetIO();
	auto style = &ui::GetStyle();

	style->WindowRounding = 0.f;
	style->AntiAliasedLines = true;
	style->AntiAliasedFill = true;
	style->ScrollbarRounding = 0.f;
	style->ScrollbarSize = 6.f;
	style->WindowPadding = ImVec2(0, 0);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(45 / 255.f, 45 / 255.f, 45 / 255.f, 1.f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(65 / 255.f, 65 / 255.f, 65 / 255.f, 1.f);

	IDirect3DSwapChain9* pChain = nullptr;
	D3DPRESENT_PARAMETERS pp = {};
	D3DDEVICE_CREATION_PARAMETERS param = {};
	device->GetCreationParameters(&param);
	device->GetSwapChain(0, &pChain);

	if (pChain)
		pChain->GetPresentParameters(&pp);

	ImGui_ImplWin32_Init(param.hFocusWindow);
	ImGui_ImplDX9_Init(device);

	D3DXCreateTextureFromFileInMemoryEx(device, texture, sizeof(texture),
		4096, 4096, D3DUSAGE_DYNAMIC, 0, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &this->m_pTexture);

	ImFontConfig cfg;
	io.Fonts->AddFontFromFileTTF("C:/windows/fonts/verdana.ttf", 12.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontFromFileTTF("C:/windows/fonts/verdanab.ttf", 12.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontFromMemoryTTF(keybinds_font, 25600, 10.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	tabs = io.Fonts->AddFontFromMemoryCompressedTTF(interlude_compressed_data, 15400, 50.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	def = io.Fonts->AddFontFromMemoryTTF(skeet, 15400, 10.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
	io.Fonts->AddFontDefault();

	ImGuiFreeType::BuildFontAtlas(io.Fonts, 0x00);
	initialized = true;
}

void dev_mode(std::string key) {
	if (c_config::get()->b["developer_mode"] && ui::IsItemHovered()) {
		ui::BeginTooltip();
		ui::Text(key.c_str());
		ui::EndTooltip();
	}
}

void c_menu::menu_render()
{
	if (!this->menu_open && ui::GetStyle().Alpha > 0.f) {
		float fc = 255.f / 0.2f * ui::GetIO().DeltaTime;
		ui::GetStyle().Alpha = std::clamp(ui::GetStyle().Alpha - fc / 255.f, 0.f, 1.f);
	}

	if (this->menu_open && ui::GetStyle().Alpha < 1.f) {
		float fc = 255.f / 0.2f * ui::GetIO().DeltaTime;
		ui::GetStyle().Alpha = std::clamp(ui::GetStyle().Alpha + fc / 255.f, 0.f, 1.f);
	}

	if (!this->menu_open && ui::GetStyle().Alpha == 0.f)
		return;

	ui::SetNextWindowSizeConstraints(ImVec2(760, 660), ImVec2(4096, 4096));
	ui::Begin(XOR("øêèò ñèñè"), nullptr, ImGuiWindowFlags_NoTitleBar || ImGuiWindowFlags_NoScrollbar);
	ui::PushFont(tabs);
	ui::TabButton("0", &MenuTabs, 0, 7);
	ui::TabButton("7", &MenuTabs, 1, 7);
	ui::TabButton("5", &MenuTabs, 2, 7);
	ui::TabButton("6", &MenuTabs, 3, 7);
	ui::TabButton("1", &MenuTabs, 4, 7);
	ui::TabButton("8", &MenuTabs, 5, 7);
	ui::TabButton("8", &MenuTabs, 6, 7);
	ui::PopFont();


	static auto calculateChildWindowPosition = [](int num) -> ImVec2 {
		return ImVec2(ui::GetWindowPos().x + 120 + (ui::GetWindowSize().x / 3 - 31) * num + 20 * num, ui::GetWindowPos().y + 20);
	};

	static auto calculateChildWindowPositionDouble = [](int num) -> ImVec2 {
		return ImVec2(ui::GetWindowPos().x + 120 + (ui::GetWindowSize().x / 2 - 70) * num + 20 * num, ui::GetWindowPos().y + 20);
	};

	auto child_size = ImVec2(ui::GetWindowSize().x / 2 - 90, ui::GetWindowSize().y - 40);
	auto child_size_d = ImVec2(ui::GetWindowSize().x / 2 - 90, ui::GetWindowSize().y - 40);
	if (MenuTabs == 0) {
		ui::SetNextWindowPos(calculateChildWindowPositionDouble(0));

		ui::BeginChild("Aimbot", child_size_d);
		ui::Checkbox("Enable", &c_config::get()->b["rbot_enable"]); dev_mode("rbot_enable");
		ui::SingleSelect("Target selection", &c_config::get()->i["rbot_target_selection"], { "Distance", "Crosshair", "Damage", "Health", "Lag", "Height" }); dev_mode("rbot_target_selection");
		ui::Checkbox("Resolver", &c_config::get()->b["rbot_resolver"]); dev_mode("rbot_resolver");
		ui::SingleSelect("Autoscope", &c_config::get()->i["rbot_autoscope_mode"], { "None", "Always", "Hitchance fail" }); dev_mode("rbot_autoscope_mode");
		ui::MultiSelect("Target hitboxes", &c_config::get()->m["rage_hitboxes"], { "Head", "Chest", "Body", "Arms", "Legs" }); dev_mode("rage_hitboxes");
		ui::MultiSelect("Hitboxes history", &c_config::get()->m["rage_history_hitboxes"], { "Head", "Chest", "Body", "Arms", "Legs" }); dev_mode("rage_history_hitboxes");
		ui::MultiSelect("Multipoints", &c_config::get()->m["rage_multipoints"], { "Head", "Chest", "Body", "Legs" }); dev_mode("rage_multipoints");
		if (c_config::get()->m["rage_multipoints"][0])
			ui::SliderInt("Head scale", &c_config::get()->i["rbot_headscale"], 1, 100, "%d%%"); dev_mode("rbot_headscale");
		if (c_config::get()->m["rage_multipoints"][1])
			ui::SliderInt("Chest scale", &c_config::get()->i["rbot_chestscale"], 1, 100, "%d%%"); dev_mode("rbot_chestscale");

		if (c_config::get()->m["rage_multipoints"][2])
			ui::SliderInt("Body scale", &c_config::get()->i["rbot_bodyscale"], 1, 100, "%d%%"); dev_mode("rbot_bodyscale");

		if (c_config::get()->b["rbot_multipoints_legs"])
			ui::SliderInt("Legs scale", &c_config::get()->i["rbot_legsscale"], 1, 100, "%d%%"); dev_mode("rbot_legsscale");

		ui::MultiSelect("Baim prefers", &c_config::get()->m["rage_prefer_baim"], { "Always", "Lethal", "Lethal x2", "Air" }); dev_mode("rage_prefer_baim");
		ui::MultiSelect("Always baim", &c_config::get()->m["rage_always_baim"], { "Always", "Health", "Air" }); dev_mode("rage_always_baim");
		if (c_config::get()->m["rage_always_baim"][1])
			ui::SliderInt("Chest scale", &c_config::get()->i["rbot_baim_hp"], 1, 100, "%dhp"); dev_mode("rbot_baim_hp");

		ui::Text("Force body-aim");
		ui::Keybind("##forcebaim", &c_config::get()->i["rbot_baim_key"], &c_config::get()->i["rbot_baim_keystyle"]); dev_mode("rbot_baim_key");

		ui::Text("Exploit key");
		ui::Keybind("##exploitkey", &c_config::get()->i["exploits_key"], &c_config::get()->i["exploits_keystyle"]); dev_mode("exploits_key");
		ui::Checkbox("Wait for charge", &c_config::get()->b["rbot_wait_for_charge"]); dev_mode("rbot_wait_for_charge");
		ui::EndChild();

		ui::SetNextWindowPos(calculateChildWindowPositionDouble(1));
		ui::BeginChild("Accuracy", child_size_d);
		ui::SliderInt("Minimum damage", &c_config::get()->i["rbot_min_damage"], 1, 100, "%dhp"); dev_mode("rbot_min_damage");
		ui::SliderInt("Penetration minimum damage", &c_config::get()->i["rbot_penetration_min_damage"], 1, 100, "%dhp"); dev_mode("rbot_penetration_min_damage");
		ui::Checkbox("Scale minimum damage to hp", &c_config::get()->b["rbot_mindamage_scale"]); dev_mode("rbot_mindamage_scale");
		ui::SliderInt("Hit chance", &c_config::get()->i["rbot_hitchance"], 1, 100, "%d%%"); dev_mode("rbot_hitchance");
		ui::SingleSelect("Auto stop", &c_config::get()->i["rbot_autostop_mode"], { "Off", "Quick always", "Quick when hitchance fail", "Slow walk always", "Slow walk when hitchance fail", "Fake walk always", "Fake walk when hitchance fail" }); dev_mode("rbot_autostop_mode");
		ui::Checkbox("Stop between shots", &c_config::get()->b["rbot_between_shots"]); dev_mode("rbot_between_shots");
		ui::EndChild();
	}

	if (MenuTabs == 1) {
		ui::SetNextWindowPos(calculateChildWindowPositionDouble(0));
		ui::BeginChild("Main", child_size_d);
		ui::Checkbox("Enable", &c_config::get()->b["rbot_aa_enable"]); dev_mode("rbot_aa_enable");
		ui::SingleSelect("Pitch", &c_config::get()->i["rbot_aa_pitch"], { "None", "Down", "Up", "Random", "Ideal" }); dev_mode("rbot_aa_pitch");
		ui::SingleSelect("Yaw", &c_config::get()->i["rbot_aa_yaw"], { "None", "Direction", "Jitter", "Rotate" }); dev_mode("rbot_aa_yaw");
		if (c_config::get()->i["rbot_aa_yaw"] == 2) {
			ui::SliderInt("Jitter range", &c_config::get()->i["rbot_aa_jitter_range"], 1, 180.f, u8"%d°"); dev_mode("rbot_aa_jitter_range");
		}
		else if (c_config::get()->i["rbot_aa_yaw"] == 3) {
			ui::SliderInt("Rotate range", &c_config::get()->i["rbot_aa_rot_range"], 1, 360.f, u8"%d°"); dev_mode("rbot_aa_rot_range");
			ui::SliderInt("Rotate speed", &c_config::get()->i["rbot_aa_rot_speed"], 1, 100.f); dev_mode("rbot_aa_rot_speed");
		}

		ui::SingleSelect("Direction", &c_config::get()->i["rbot_aa_dir"], { "Auto", "Backwards", "Left", "Right", "Custom" }); dev_mode("rbot_aa_dir");
		if (c_config::get()->i["rbot_aa_dir"] == 0) {
			ui::SliderInt("Auto direction time", &c_config::get()->i["rbot_aa_auto_time"], 0, 10, "%ds"); dev_mode("rbot_aa_auto_time");
		}
		else if (c_config::get()->i["rbot_aa_dir"] == 4) {
			ui::SliderInt("Direction custom", &c_config::get()->i["rbot_aa_dir_custom"], -180, 180, u8"%d°"); dev_mode("rbot_aa_dir_custom");
		}
		ui::SingleSelect("Base angle", &c_config::get()->i["rbot_aa_base_angle"], { "Off", "Static", "Away crosshair", "Away distance" }); dev_mode("rbot_aa_base_angle");
		ui::Checkbox("Lock direction", &c_config::get()->b["rbot_aa_lock"]); dev_mode("rbot_aa_lock");
		ui::EndChild();
		
		ui::SetNextWindowPos(calculateChildWindowPositionDouble(1));
		ui::BeginChild("Additional", child_size_d);
		ui::Checkbox("Edge", &c_config::get()->b["rbot_aa_edge"]); dev_mode("rbot_aa_edge");
		ui::Checkbox("Desync", &c_config::get()->b["rbot_aa_desync"]); dev_mode("rbot_aa_desync");
		ui::Keybind("##desyncinvkeybind", &c_config::get()->i["desync_invert_key"], &c_config::get()->i["desync_invert_keystyle"]); dev_mode("desync_invert_key");

		ui::Text("Fakeduck key");
		ui::Keybind("##fakeduckkey", &c_config::get()->i["fakeduck_key"], &c_config::get()->i["fakeduck_keystyle"]); dev_mode("fakeduck_key");
		
		ui::Text("Slow walk key");
		ui::Keybind("Slow walk bind", &c_config::get()->i["slowwalk_key"], &c_config::get()->i["slowwalk_keystyle"]); dev_mode("slowwalk_key");
		ui::SingleSelect("Slow walk mode", &c_config::get()->i["rbot_slow_mode"], { "Slow Walk", "Fake walk", "Silent Walk" }); dev_mode("rbot_slow_mode");

		ui::Checkbox("Fakelag enable", &c_config::get()->b["rbot_aa_fakelag_enable"]); dev_mode("rbot_aa_fakelag_enable");
		ui::SingleSelect("Fakelag mode", &c_config::get()->i["rbot_aa_fakelag_mode"], { "Max", "Break", "Random", "Break step", "Flucate" }); dev_mode("rbot_aa_fakelag_mode");
		ui::SliderInt("Fakelag limit", &c_config::get()->i["rbot_fakelag_limit"], 2, 14, "%dticks"); dev_mode("rbot_fakelag_limit");

		if (c_config::get()->i["rbot_aa_fakelag_mode"] == 4) {
			ui::SliderInt("Fakelag variance", &c_config::get()->i["rbot_fakelag_variance"], 1, 100, "%d%%"); dev_mode("rbot_fakelag_variance");
		}

		ui::Checkbox("On land", &c_config::get()->b["rbot_aa_fakelag_land"]); dev_mode("rbot_aa_fakelag_land");
		ui::EndChild();
	}

	if (MenuTabs == 3) {
		ui::SetNextWindowPos(calculateChildWindowPositionDouble(0));
		ui::BeginChild("Players", child_size_d);
		ui::Checkbox("Box", &c_config::get()->b["visuals_box"]); dev_mode("visuals_box");
		ui::ColorEdit4("##eboxcol", c_config::get()->c["box_color"]);
		ui::Checkbox("Offscreen", &c_config::get()->b["visuals_offscreen"]); dev_mode("visuals_offscreen");
		ui::ColorEdit4("##offscreencolor", c_config::get()->c["offscreen_color"]);
		ui::Checkbox("Name", &c_config::get()->b["visuals_name"]); dev_mode("visuals_name");
		ui::Checkbox("Health", &c_config::get()->b["visuals_health"]); dev_mode("visuals_health");

		ui::Checkbox("Weapon", &c_config::get()->b["visuals_weapon"]); dev_mode("visuals_weapon");
		ui::Checkbox("Ammo", &c_config::get()->b["visuals_ammo"]); dev_mode("visuals_ammo");
		ui::Checkbox("Skeleton", &c_config::get()->b["visuals_skeleton"]); dev_mode("visuals_skeleton");

		ui::Checkbox("Chams", &c_config::get()->b["chams_enemy"]); dev_mode("chams_enemy");
		ui::Text("Enemy visible color");
		ui::ColorEdit4("##enemyvis", c_config::get()->c["chams_enemy_color"]);
		ui::Text("Enemy invisible color");
		ui::ColorEdit4("##enemyinvis", c_config::get()->c["chams_invis_enemy_color"]);
		ui::SliderInt("Enemy chams blend", &c_config::get()->i["chams_enemy_blend"], 0, 100, "%d%%"); dev_mode("chams_enemy_blend");
		ui::Checkbox("History chams", &c_config::get()->b["chams_enemy_history"]); dev_mode("chams_enemy_history");
		ui::ColorEdit4("##historychamscol", c_config::get()->c["chams_history_color"]);
		ui::SliderInt("History chams blend", &c_config::get()->i["chams_history_blend"], 0, 100, "%d%%");

		ui::Checkbox("Local chams", &c_config::get()->b["chams_local"]); dev_mode("chams_local");
		ui::ColorEdit4("##localclr", c_config::get()->c["local_chams_color"]);
		ui::SliderInt("Local chams blend", &c_config::get()->i["local_chams_blend"], 0, 100, "%d%%");
		ui::EndChild();

		ui::SetNextWindowPos(calculateChildWindowPositionDouble(1));
		ui::BeginChild("World", child_size_d);
		ui::Checkbox("Dropped items", &c_config::get()->b["visuals_items"]);
		ui::Checkbox("Disable temmates", &c_config::get()->b["visuals_disable_team"]);
		ui::SingleSelect("World modulation", &c_config::get()->i["visuals_color_modulation_mode"], { "Off", "Night", "Fullbright" });
		ui::Checkbox("Transperent props", &c_config::get()->b["no_props"]);
		if (ui::Button("Update world modulation")) {
			c_visuals::get()->ModulateWorld();
		}
		ui::Checkbox("No scope", &c_config::get()->b["visuals_noscope"]);
		ui::ColorEdit3("##noscopecolor", c_config::get()->c["noscope_color"]);
		ui::Checkbox("No fog", &c_config::get()->b["visuals_nofog"]);
		ui::Checkbox("No smoke", &c_config::get()->b["no_smoke"]);
		ui::Checkbox("Spread crosshair", &c_config::get()->b["visuals_spread_crosshair"]);
		ui::ColorEdit4("##spreadclr", c_config::get()->c["spread_crosshair_color"]);
		ui::Checkbox("Override fov", &c_config::get()->b["visuals_fov"]);
		ui::SliderInt("FOV", &c_config::get()->i["visuals_fov_amount"], 60, 140);
		ui::Checkbox("Override viewmodel fov", &c_config::get()->b["visuals_viewmodel_fov"]);
		ui::SliderInt("Viewmodel FOV", &c_config::get()->i["visuals_viewmodel_fov_amount"], 60, 140);
		ui::Checkbox("Force crosshair", &c_config::get()->b["force_crosshair"]);
		ui::Checkbox("Grenade prediction", &c_config::get()->b["grenade_tracer"]);
		ui::Checkbox("Ragdoll force", &c_config::get()->b["ragdoll_force"]);
		ui::Text("Third person key");
		ui::Keybind("##tpkey", &c_config::get()->i["thirdperson_key"], &c_config::get()->i["thirdperson_keystyle"]);
		ui::SliderInt("Thirdperson Distance", &c_config::get()->i["thirdperson_distance"], 0, 200);
		ui::Checkbox("Impact beams", &c_config::get()->b["visuals_impact_beams"]);
		ui::ColorEdit4("Impact beams color", c_config::get()->c["beams_color"]);
		/*ui::Checkbox("Fog", &c_config::get()->b["fog_override"]);
		ui::SliderInt("Fog start", &c_config::get()->i["fog_start"], 1, 3500);
		ui::SliderInt("Fog end", &c_config::get()->i["fog_end"], 1, 3500);*/
		ui::EndChild();
	}

	if (MenuTabs == 4) {
		ui::SetNextWindowPos(calculateChildWindowPositionDouble(0));
		ui::BeginChild("Misc", child_size_d);
		ui::Checkbox("Bhop", &c_config::get()->b["misc_bhop"]);
		ui::Checkbox("Duck in air", &c_config::get()->b["misc_air_duck"]);
		ui::Checkbox("Autostrafe", &c_config::get()->b["misc_autostrafe"]);
		ui::Checkbox("Fast Stop", &c_config::get()->b["misc_fast_stop"]);
		ui::MultiSelect("Notifications", &c_config::get()->m["misc_notifications"], { "Map", "Damage given", "Buy", "Bomb", "Defuse" });
		ui::Checkbox("Hitmarker", &c_config::get()->b["visuals_hitmarker"]);
		ui::Checkbox("Clantag", &c_config::get()->b["misc_clantag"]);
		ui::SingleSelect("Clantag mode", &c_config::get()->i["misc_clantag_mode"], { "Valve", "Static", "Animated" });
		ui::Checkbox("Developer mode", &c_config::get()->b["developer_mode"]);
		ui::EndChild();

		ui::SetNextWindowPos(calculateChildWindowPositionDouble(1));
		ui::BeginChild("Configuration", child_size_d);
		ui::Text("Cheat accent color");
		ui::ColorEdit4("##menucolor", c_config::get()->c["menu_color"]);

		ui::SingleSelect("Presets", &c_config::get()->i["_preset"], { "Rage", "Legit", "HvH", "Body-aim", "Secret", "Headshot", "Test" });
		if (ui::Button("Load"))
			c_config::get()->load();
		
		if (ui::Button("Save"))
			c_config::get()->save();

		if (ui::Button("Reset"))
			c_config::get()->load_defaults();

		ui::Text("Scripts");
		ui::EndChild();
	}
	ui::End();
}

void c_menu::keybind_list()
{
	
}   
