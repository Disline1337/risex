#pragma once
#include "../includes.hpp"

class c_menu : public singleton<c_menu> {
public:
	void keys();
	void watermark();
	void spectators();
	void draw(bool is_open);
	void menu_setup(ImGuiStyle& style);

	float dpi_scale = 1.f;

	ImFont* futura;
	ImFont* esp_flags_name;
	ImFont* esp_weapon;
	ImFont* futura_large;
	ImFont* weapon_icons;
	ImFont* weapon_icons2;
	ImFont* font;
	ImFont* new_font;
	ImFont* futura_small;

	ImFont* gotham;

	ImFont* ico_menu;
	ImFont* astrium;
	ImFont* futura_smallest;
	ImFont* weapon_big;
	ImFont* ico_bottom;


	float menu_alpha = 0.1f;

	float public_alpha;
	IDirect3DDevice9* device;
	float color_buffer[4] = { 1.f, 1.f, 1.f, 1.f };
private:
	struct {
		ImVec2 WindowPadding;
		float  WindowRounding;
		ImVec2 WindowMinSize;
		float  ChildRounding;
		float  PopupRounding;
		ImVec2 FramePadding;
		float  FrameRounding;
		ImVec2 ItemSpacing;
		ImVec2 ItemInnerSpacing;
		ImVec2 TouchExtraPadding;
		float  IndentSpacing;
		float  ColumnsMinSpacing;
		float  ScrollbarSize;
		float  ScrollbarRounding;
		float  GrabMinSize;
		float  GrabRounding;
		float  TabRounding;
		float  TabMinWidthForUnselectedCloseButton;
		ImVec2 DisplayWindowPadding;
		ImVec2 DisplaySafeAreaPadding;
		float  MouseCursorScale;
	} styles;

	bool update_dpi = false;
	bool update_scripts = false;
	void dpi_resize(float scale_factor, ImGuiStyle& style);

	int active_tab_index;
	ImGuiStyle style;
	int width = 850, height = 560;
	float child_height;

	float preview_alpha = 1.f;
	int pressed_keys = 0;

	enum state
	{
		hold,
		toggle
	};

	std::string get_state(key_bind_mode mode)
	{
		if (mode == hold)
			return "hold";
		else if (mode == toggle)
			return "toggle";

	}
	void add_key(const char* name, bool main, key_bind key, int spacing, float alpha, bool condition = true, bool damage = false)
	{
		float animka = 1.f;
		auto p = ImGui::GetWindowPos() + ImGui::GetCursorPos() * animka;

		if (!condition || animka < 0)
			return;


		if (!main || animka < 0)
			return;

		if (key.mode == hold)
			ImGui::GetWindowDrawList()->AddCircle(ImVec2(p.x + 5, p.y + ImGui::CalcTextSize(name).y / 2), 3, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255));
		else if (key.mode == toggle)
			ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 5, p.y + ImGui::CalcTextSize(name).y / 2), 3, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255));

		spacing -= ImGui::CalcTextSize(damage ? std::to_string(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage).c_str() : get_state(key.mode).c_str()).x;
		ImGui::SetCursorPos(ImVec2(key.mode == -1 ? 5 : 15, ImGui::GetCursorPosY() * animka));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1));
		ImGui::Text(name, false); ImGui::SameLine(spacing - 5);
		ImGui::Text(damage ? std::to_string(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage).c_str() : get_state(key.mode).c_str(), false);
		ImGui::PopStyleColor(1);

		pressed_keys + 1;

	}

	int active_tab;

	int rage_section;
	int legit_section;
	int visuals_section;
	int players_section;
	int misc_section;
	int settings_section;

	// we need to use 'int child' to seperate content in 2 childs
	void draw_ragebot(int child);

	void draw_legit(int child);

	void draw_visuals(int child);
	int current_profile = -1;

	void draw_players(int child);
	void draw_tabs_players();

	void draw_misc(int child);

	void draw_settings(int child);

	void draw_lua(int child);

	std::string preview = crypt_str("None");
};
