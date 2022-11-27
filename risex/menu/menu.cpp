// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <ShlObj_core.h>
#include <unordered_map>
#include "menu.h"
#include "imgui/code_editor.h"
#include "../constchars.h"
#include "../features/misc/logs.h"

#define ALPHA (ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)
#define NOALPHA (ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float)

std::vector <std::string> files;
std::vector <std::string> scripts;
std::string editing_script;

auto selected_script = 0;
auto loaded_editing_script = false;

static auto menu_setupped = false;
static auto should_update = true;

IDirect3DTexture9* all_skins[36];

std::string get_wep(int id, int custom_index = -1, bool knife = true)
{
	if (custom_index > -1)
	{
		if (knife)
		{
			switch (custom_index)
			{
			case 0: return crypt_str("weapon_knife");
			case 1: return crypt_str("weapon_bayonet");
			case 2: return crypt_str("weapon_knife_css");
			case 3: return crypt_str("weapon_knife_skeleton");
			case 4: return crypt_str("weapon_knife_outdoor");
			case 5: return crypt_str("weapon_knife_cord");
			case 6: return crypt_str("weapon_knife_canis");
			case 7: return crypt_str("weapon_knife_flip");
			case 8: return crypt_str("weapon_knife_gut");
			case 9: return crypt_str("weapon_knife_karambit");
			case 10: return crypt_str("weapon_knife_m9_bayonet");
			case 11: return crypt_str("weapon_knife_tactical");
			case 12: return crypt_str("weapon_knife_falchion");
			case 13: return crypt_str("weapon_knife_survival_bowie");
			case 14: return crypt_str("weapon_knife_butterfly");
			case 15: return crypt_str("weapon_knife_push");
			case 16: return crypt_str("weapon_knife_ursus");
			case 17: return crypt_str("weapon_knife_gypsy_jackknife");
			case 18: return crypt_str("weapon_knife_stiletto");
			case 19: return crypt_str("weapon_knife_widowmaker");
			}
		}
		else
		{
			switch (custom_index)
			{
			case 0: return crypt_str("ct_gloves"); //-V1037
			case 1: return crypt_str("studded_bloodhound_gloves");
			case 2: return crypt_str("t_gloves");
			case 3: return crypt_str("ct_gloves");
			case 4: return crypt_str("sporty_gloves");
			case 5: return crypt_str("slick_gloves");
			case 6: return crypt_str("leather_handwraps");
			case 7: return crypt_str("motorcycle_gloves");
			case 8: return crypt_str("specialist_gloves");
			case 9: return crypt_str("studded_hydra_gloves");
			}
		}
	}
	else
	{
		switch (id)
		{
		case 0: return crypt_str("knife");
		case 1: return crypt_str("gloves");
		case 2: return crypt_str("weapon_ak47");
		case 3: return crypt_str("weapon_aug");
		case 4: return crypt_str("weapon_awp");
		case 5: return crypt_str("weapon_cz75a");
		case 6: return crypt_str("weapon_deagle");
		case 7: return crypt_str("weapon_elite");
		case 8: return crypt_str("weapon_famas");
		case 9: return crypt_str("weapon_fiveseven");
		case 10: return crypt_str("weapon_g3sg1");
		case 11: return crypt_str("weapon_galilar");
		case 12: return crypt_str("weapon_glock");
		case 13: return crypt_str("weapon_m249");
		case 14: return crypt_str("weapon_m4a1_silencer");
		case 15: return crypt_str("weapon_m4a1");
		case 16: return crypt_str("weapon_mac10");
		case 17: return crypt_str("weapon_mag7");
		case 18: return crypt_str("weapon_mp5sd");
		case 19: return crypt_str("weapon_mp7");
		case 20: return crypt_str("weapon_mp9");
		case 21: return crypt_str("weapon_negev");
		case 22: return crypt_str("weapon_nova");
		case 23: return crypt_str("weapon_hkp2000");
		case 24: return crypt_str("weapon_p250");
		case 25: return crypt_str("weapon_p90");
		case 26: return crypt_str("weapon_bizon");
		case 27: return crypt_str("weapon_revolver");
		case 28: return crypt_str("weapon_sawedoff");
		case 29: return crypt_str("weapon_scar20");
		case 30: return crypt_str("weapon_ssg08");
		case 31: return crypt_str("weapon_sg556");
		case 32: return crypt_str("weapon_tec9");
		case 33: return crypt_str("weapon_ump45");
		case 34: return crypt_str("weapon_usp_silencer");
		case 35: return crypt_str("weapon_xm1014");
		default: return crypt_str("unknown");
		}
	}
}

IDirect3DTexture9* get_skin_preview(const char* weapon_name, const std::string& skin_name, IDirect3DDevice9* device)
{
	IDirect3DTexture9* skin_image = nullptr;
	std::string vpk_path;

	if (strcmp(weapon_name, crypt_str("unknown")) && strcmp(weapon_name, crypt_str("knife")) && strcmp(weapon_name, crypt_str("gloves"))) //-V526
	{
		if (skin_name.empty() || skin_name == crypt_str("default"))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");
		else
			vpk_path = crypt_str("resource/flash/econ/default_generated/") + std::string(weapon_name) + crypt_str("_") + std::string(skin_name) + crypt_str("_light_large.png");
	}
	else
	{
		if (!strcmp(weapon_name, crypt_str("knife")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_knife.png");
		else if (!strcmp(weapon_name, crypt_str("gloves")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else if (!strcmp(weapon_name, crypt_str("unknown")))
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/weapon_snowball.png");

	}
	const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));
	if (handle)
	{
		int file_len = m_basefilesys()->Size(handle);
		char* image = new char[file_len]; //-V121

		m_basefilesys()->Read(image, file_len, handle);
		m_basefilesys()->Close(handle);

		D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
		delete[] image;
	}

	if (!skin_image)
	{
		std::string vpk_path;

		if (strstr(weapon_name, crypt_str("bloodhound")) != NULL || strstr(weapon_name, crypt_str("hydra")) != NULL)
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/ct_gloves.png");
		else
			vpk_path = crypt_str("resource/flash/econ/weapons/base_weapons/") + std::string(weapon_name) + crypt_str(".png");

		const auto handle = m_basefilesys()->Open(vpk_path.c_str(), crypt_str("r"), crypt_str("GAME"));

		if (handle)
		{
			int file_len = m_basefilesys()->Size(handle);
			char* image = new char[file_len]; //-V121

			m_basefilesys()->Read(image, file_len, handle);
			m_basefilesys()->Close(handle);

			D3DXCreateTextureFromFileInMemory(device, image, file_len, &skin_image);
			delete[] image;
		}
	}

	return skin_image;
}

// setup some styles and colors, window size and bg alpha
// dpi setup
void c_menu::menu_setup(ImGuiStyle& style) //-V688
{
	ImGui::StyleColorsClassic(); // colors setup
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once); // window pos setup
	ImGui::SetNextWindowBgAlpha(min(style.Alpha, 0.94f)); // window bg alpha setup

	styles.WindowPadding = style.WindowPadding;
	styles.WindowRounding = style.WindowRounding;
	styles.WindowMinSize = style.WindowMinSize;
	styles.ChildRounding = style.ChildRounding;
	styles.PopupRounding = style.PopupRounding;
	styles.FramePadding = style.FramePadding;
	styles.FrameRounding = style.FrameRounding;
	styles.ItemSpacing = style.ItemSpacing;
	styles.ItemInnerSpacing = style.ItemInnerSpacing;
	styles.TouchExtraPadding = style.TouchExtraPadding;
	styles.IndentSpacing = style.IndentSpacing;
	styles.ColumnsMinSpacing = style.ColumnsMinSpacing;
	styles.ScrollbarSize = style.ScrollbarSize;
	styles.ScrollbarRounding = style.ScrollbarRounding;
	styles.GrabMinSize = style.GrabMinSize;
	styles.GrabRounding = style.GrabRounding;
	styles.TabRounding = style.TabRounding;
	styles.TabMinWidthForUnselectedCloseButton = style.TabMinWidthForUnselectedCloseButton;
	styles.DisplayWindowPadding = style.DisplayWindowPadding;
	styles.DisplaySafeAreaPadding = style.DisplaySafeAreaPadding;
	styles.MouseCursorScale = style.MouseCursorScale;

	// setup skins preview
	for (auto i = 0; i < g_cfg.skins.skinChanger.size(); i++)
		if (!all_skins[i])
			all_skins[i] = get_skin_preview(get_wep(i, (i == 0 || i == 1) ? g_cfg.skins.skinChanger.at(i).definition_override_vector_index : -1, i == 0).c_str(), g_cfg.skins.skinChanger.at(i).skin_name, device); //-V810

	menu_setupped = true; // we dont want to setup menu again
}

// resize current style sizes
void c_menu::dpi_resize(float scale_factor, ImGuiStyle& style) //-V688
{
	style.WindowPadding = (styles.WindowPadding * scale_factor);
	style.WindowRounding = (styles.WindowRounding * scale_factor);
	style.WindowMinSize = (styles.WindowMinSize * scale_factor);
	style.ChildRounding = (styles.ChildRounding * scale_factor);
	style.PopupRounding = (styles.PopupRounding * scale_factor);
	style.FramePadding = (styles.FramePadding * scale_factor);
	style.FrameRounding = (styles.FrameRounding * scale_factor);
	style.ItemSpacing = (styles.ItemSpacing * scale_factor);
	style.ItemInnerSpacing = (styles.ItemInnerSpacing * scale_factor);
	style.TouchExtraPadding = (styles.TouchExtraPadding * scale_factor);
	style.IndentSpacing = (styles.IndentSpacing * scale_factor);
	style.ColumnsMinSpacing = (styles.ColumnsMinSpacing * scale_factor);
	style.ScrollbarSize = (styles.ScrollbarSize * scale_factor);
	style.ScrollbarRounding = (styles.ScrollbarRounding * scale_factor);
	style.GrabMinSize = (styles.GrabMinSize * scale_factor);
	style.GrabRounding = (styles.GrabRounding * scale_factor);
	style.TabRounding = (styles.TabRounding * scale_factor);
	if (styles.TabMinWidthForUnselectedCloseButton != FLT_MAX) //-V550
		style.TabMinWidthForUnselectedCloseButton = (styles.TabMinWidthForUnselectedCloseButton * scale_factor);
	style.DisplayWindowPadding = (styles.DisplayWindowPadding * scale_factor);
	style.DisplaySafeAreaPadding = (styles.DisplaySafeAreaPadding * scale_factor);
	style.MouseCursorScale = (styles.MouseCursorScale * scale_factor);
}


std::string get_config_dir()
{
	std::string folder;
	static TCHAR path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL, 0x001a, NULL, NULL, path)))
		folder = std::string(path) + crypt_str("C:\\Risex\\Configs\\CSGO\\");

	CreateDirectory(folder.c_str(), NULL);
	return folder;
}

void load_config()
{
	if (cfg_manager->files.empty())
		return;

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), false);
	c_lua::get().unload_all_scripts();

	for (auto& script : g_cfg.scripts.scripts)
		c_lua::get().load_script(c_lua::get().get_script_id(script));

	scripts = c_lua::get().scripts;

	if (selected_script >= scripts.size())
		selected_script = scripts.size() - 1; //-V103

	for (auto& current : scripts)
	{
		if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
			current.erase(current.size() - 5, 5);
		else if (current.size() >= 4)
			current.erase(current.size() - 4, 4);
	}

	for (auto i = 0; i < g_cfg.skins.skinChanger.size(); ++i)
		all_skins[i] = nullptr;

	g_cfg.scripts.scripts.clear();

	cfg_manager->load(cfg_manager->files.at(g_cfg.selected_config), true);
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Loaded ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void save_config()
{
	if (cfg_manager->files.empty())
		return;

	g_cfg.scripts.scripts.clear();

	for (auto i = 0; i < c_lua::get().scripts.size(); ++i)
	{
		auto script = c_lua::get().scripts.at(i);

		if (c_lua::get().loaded.at(i))
			g_cfg.scripts.scripts.emplace_back(script);
	}

	cfg_manager->save(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	eventlogs::get().add(crypt_str("Saved ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);
}

void remove_config()
{
	if (cfg_manager->files.empty())
		return;

	eventlogs::get().add(crypt_str("Removed ") + files.at(g_cfg.selected_config) + crypt_str(" config"), false);

	cfg_manager->remove(cfg_manager->files.at(g_cfg.selected_config));
	cfg_manager->config_files();

	files = cfg_manager->files;

	if (g_cfg.selected_config >= files.size())
		g_cfg.selected_config = files.size() - 1; //-V103

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

void add_config()
{
	auto empty = true;

	for (auto current : g_cfg.new_config_name)
	{
		if (current != ' ')
		{
			empty = false;
			break;
		}
	}

	if (empty)
		g_cfg.new_config_name = crypt_str("config");

	eventlogs::get().add(crypt_str("Added ") + g_cfg.new_config_name + crypt_str(" config"), false);

	if (g_cfg.new_config_name.find(crypt_str(".rx")) == std::string::npos)
		g_cfg.new_config_name += crypt_str(".rx");

	cfg_manager->save(g_cfg.new_config_name);
	cfg_manager->config_files();

	g_cfg.selected_config = cfg_manager->files.size() - 1; //-V103
	files = cfg_manager->files;

	for (auto& current : files)
		if (current.size() > 2)
			current.erase(current.size() - 3, 3);
}

__forceinline void padding(float x, float y)
{
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x * c_menu::get().dpi_scale);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y * c_menu::get().dpi_scale);
}


// title of content child
void child_title(const char* label)
{
	ImGui::PushFont(c_menu::get().futura_large);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (300 * c_menu::get().dpi_scale));
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (100 * c_menu::get().dpi_scale));
	ImGui::Text(label);

	ImGui::PopStyleColor();
	ImGui::PopFont();
}

void draw_combo(const char* name, int& variable, const char* labels[], int count)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6 * c_menu::get().dpi_scale);
	ImGui::Text(name);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
	ImGui::Combo(std::string(crypt_str("##COMBO__") + std::string(name)).c_str(), &variable, labels, count);
}

void draw_combo(const char* name, int& variable, bool (*items_getter)(void*, int, const char**), void* data, int count)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6 * c_menu::get().dpi_scale);
	ImGui::Text(name);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
	ImGui::Combo(std::string(crypt_str("##COMBO__") + std::string(name)).c_str(), &variable, items_getter, data, count);
}

void draw_multicombo(std::string name, std::vector<int>& variable, const char* labels[], int count, std::string& preview)
{
	padding(-3, -6);
	ImGui::Text((crypt_str(" ") + name).c_str());
	padding(0, -5);

	auto hashname = crypt_str("##") + name; // we dont want to render name of combo

	for (auto i = 0, j = 0; i < count; i++)
	{
		if (variable[i])
		{
			if (j)
				preview += crypt_str(", ") + (std::string)labels[i];
			else
				preview = labels[i];

			j++;
		}
	}

	if (ImGui::BeginCombo(hashname.c_str(), preview.c_str())) // draw start
	{
		ImGui::Spacing();
		ImGui::BeginGroup();
		{

			for (auto i = 0; i < count; i++)
				ImGui::Selectable(labels[i], (bool*)&variable[i], ImGuiSelectableFlags_DontClosePopups);

		}
		ImGui::EndGroup();
		ImGui::Spacing();

		ImGui::EndCombo();
	}

	preview = crypt_str("None"); // reset preview to use later
}

bool LabelClick(const char* label, bool* v, const char* unique_id)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	// The concatoff/on thingies were for my weapon config system so if we're going to make that, we still need this aids.
	char Buf[64];
	_snprintf(Buf, 62, crypt_str("%s"), label);

	char getid[128];
	sprintf_s(getid, 128, crypt_str("%s%s"), label, unique_id);


	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(getid);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const ImRect check_bb(window->DC.CursorPos, ImVec2(label_size.y + style.FramePadding.y * 2 + window->DC.CursorPos.x, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2));
	ImGui::ItemSize(check_bb, style.FramePadding.y);

	ImRect total_bb = check_bb;

	if (label_size.x > 0)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y), ImVec2(window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y));

		ImGui::ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
		total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
	}

	if (!ImGui::ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	if (*v)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(126 / 255.f, 131 / 255.f, 219 / 255.f, 1.f));
	if (label_size.x > 0.0f)
		ImGui::RenderText(ImVec2(check_bb.GetTL().x + 12, check_bb.GetTL().y), Buf);
	if (*v)
		ImGui::PopStyleColor();

	return pressed;

}


void draw_keybind(const char* label, key_bind* key_bind, const char* unique_id)
{
	// reset bind if we re pressing esc
	if (key_bind->key == KEY_ESCAPE)
		key_bind->key = KEY_NONE;

	auto clicked = false;
	auto text = (std::string)m_inputsys()->ButtonCodeToString(key_bind->key);

	if (key_bind->key <= KEY_NONE || key_bind->key >= KEY_MAX)
		text = crypt_str("None");

	// if we clicked on keybind
	if (hooks::input_shouldListen && hooks::input_receivedKeyval == &key_bind->key)
	{
		clicked = true;
		text = crypt_str("...");
	}

	auto textsize = ImGui::CalcTextSize(text.c_str()).x + 8 * c_menu::get().dpi_scale;
	auto labelsize = ImGui::CalcTextSize(label);

	ImGui::Text(label);
	ImGui::SameLine();

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x - (ImGui::GetWindowSize().x - ImGui::CalcItemWidth()) - max(50 * c_menu::get().dpi_scale, textsize));
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3 * c_menu::get().dpi_scale);

	if (ImGui::KeybindButton(text.c_str(), unique_id, ImVec2(max(50 * c_menu::get().dpi_scale, textsize), 23 * c_menu::get().dpi_scale), clicked))
		clicked = true;


	if (clicked)
	{
		hooks::input_shouldListen = true;
		hooks::input_receivedKeyval = &key_bind->key;
	}

	static auto hold = false, toggle = false;

	switch (key_bind->mode)
	{
	case HOLD:
		hold = true;
		toggle = false;
		break;
	case TOGGLE:
		toggle = true;
		hold = false;
		break;
	}

	if (ImGui::BeginPopup(unique_id))
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6 * c_menu::get().dpi_scale);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Hold")).x / 2)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 11);

		if (LabelClick(crypt_str("Hold"), &hold, unique_id))
		{
			if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else
			{
				toggle = false;
				key_bind->mode = HOLD;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetCurrentWindow()->Size.x / 2) - (ImGui::CalcTextSize(crypt_str("Toggle")).x / 2)));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 11);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 9 * c_menu::get().dpi_scale);

		if (LabelClick(crypt_str("Toggle"), &toggle, unique_id))
		{
			if (toggle)
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}
			else if (hold)
			{
				toggle = false;
				key_bind->mode = HOLD;
			}
			else
			{
				hold = false;
				key_bind->mode = TOGGLE;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void draw_text(const char* name)
{
	ImGui::SetCursorPosX(4);
	ImGui::Text(name);
}
void draw_color(const char* name, Color* color, ImGuiColorEditFlags flags)
{
	ImGui::SetCursorPosX(4);
	ImGui::Text(name);
	ImGui::SameLine(0, 100);std::string nn = "##";nn += name;
	ImGui::ColorEdit(nn.c_str(), color, flags);

}
void child_start(const char* name)
{
	ImGui::SetCursorPosX(4);
	ImGui::Text(name);
	ImGui::Separator();
	ImGui::Spacing();
}

void draw_semitabs(const char* labels[], int count, int& tab, ImGuiStyle& style)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (2 * c_menu::get().dpi_scale));

	// center of main child
	float offset = 343 * c_menu::get().dpi_scale;

	// text size padding + frame padding
	for (int i = 0; i < count; i++)
		offset -= (ImGui::CalcTextSize(labels[i]).x) / 2 + style.FramePadding.x * 2;

	// set new padding
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	ImGui::BeginGroup();

	for (int i = 0; i < count; i++)
	{
		// switch current tab
		if (ImGui::ContentTab(labels[i], tab == i))
			tab = i;

		// continue drawing on same line 
		if (i + 1 != count)
		{
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.ItemSpacing.x);
		}
	}

	ImGui::EndGroup();
}

__forceinline void tab_start()
{
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (20 * c_menu::get().dpi_scale), ImGui::GetCursorPosY() + (5 * c_menu::get().dpi_scale)));
}

__forceinline void tab_end()
{
	ImGui::PopStyleVar();
	ImGui::SetWindowFontScale(c_menu::get().dpi_scale);
}

void lua_edit(std::string window_name)
{
	std::string file_path;

	auto get_dir = [&]() -> void
	{
		static TCHAR path[MAX_PATH];

		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
			file_path = std::string(path) + crypt_str("C:\\Risex\\Scripts\\CSGO\\");

		CreateDirectory(file_path.c_str(), NULL);
		file_path += window_name + crypt_str(".lua");
	};

	get_dir();
	const char* child_name = (window_name + window_name).c_str();

	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_Once);
	ImGui::Begin(window_name.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 5.f);

	static TextEditor editor;

	if (!loaded_editing_script)
	{
		static auto lang = TextEditor::LanguageDefinition::Lua();

		editor.SetLanguageDefinition(lang);
		editor.SetReadOnly(false);

		std::ifstream t(file_path);

		if (t.good()) // does while exist?
		{
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			editor.SetText(str); // setup script content
		}

		loaded_editing_script = true;
	}

	// dpi scale for font
	// we dont need to resize it for full scale
	ImGui::SetWindowFontScale(1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f));

	// new size depending on dpi scale
	ImGui::SetWindowSize(ImVec2(ImFloor(800 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))), ImFloor(700 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)))));
	editor.Render(child_name, ImGui::GetWindowSize() - ImVec2(0, 66 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))));

	// seperate code with buttons
	ImGui::Separator();

	// set cursor pos to right edge of window
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowSize().x - (16.f * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))) - (250.f * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f))));
	ImGui::BeginGroup();

	if (ImGui::CustomButton(crypt_str("Save"), (crypt_str("Save") + window_name).c_str(), ImVec2(125 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)), 0), true, c_menu::get().ico_bottom, crypt_str("S")))
	{
		std::ofstream out;

		out.open(file_path);
		out << editor.GetText() << std::endl;
		out.close();
	}

	ImGui::SameLine();

	// TOOD: close button will close window (return in start of function)
	if (ImGui::CustomButton(crypt_str("Close"), (crypt_str("Close") + window_name).c_str(), ImVec2(125 * (1.f + ((c_menu::get().dpi_scale - 1.0) * 0.5f)), 0)))
	{
		globals.g.focused_on_input = false;
		loaded_editing_script = false;
		editing_script.clear();
	}

	ImGui::EndGroup();

	ImGui::PopStyleVar();
	ImGui::End();
}

void draw_checkbox(const char* name, bool* v)
{
	ImGui::SetCursorPosX(4);
	ImGui::Checkbox(name, v);
}

void designers(float alpha, ImVec2 p)
{
	ImVec2 m_s = ImGui::GetWindowSize();
	auto d = ImGui::GetWindowDrawList();
	//d->AddRect(ImVec2(p), ImVec2(p + m_s), ImColor(vars.menu.menu_theme.r(), vars.menu.menu_theme.g(), vars.menu.menu_theme.b(), (int)(alpha * 255)));
	d->AddRectFilled(ImVec2(p.x + 4, p.y + 5), ImVec2(p.x + m_s.x - 4, p.y + m_s.y - 2), ImColor(15, 15, 15, (int)(alpha * 255)));
	d->AddLine(ImVec2(p.x + 140, p.y + 6), ImVec2(p.x + 140, p.y + m_s.y - 6), ImColor(54, 53, 54, (int)(alpha * 255)), 1.0f);
	//d->AddLine(ImVec2(p.x + 4, p.y + 42), ImVec2(p.x + 644, p.y + 42), ImColor(89, 87, 93, 255), 1.000000);
	//d->AddLine(ImVec2(p.x + 4, p.y + 41), ImVec2(p.x + 644, p.y + 41), ImColor(22, 21, 26, 255), 1.000000);
	//d->AddLine(ImVec2(p.x + 4, p.y + 43), ImVec2(p.x + 644, p.y + 43), ImColor(22, 21, 26, 255), 1.000000);
	d->AddRectFilledMultiColor(ImVec2(p.x + 4, p.y + 4), ImVec2(p.x + 325, p.y + 6), ImColor(24, 24, 24,(int)(alpha * 255)), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)), ImColor(24, 24, 24, (int)(alpha * 255)));
	d->AddRectFilledMultiColor(ImVec2(p.x + 325, p.y + 4), ImVec2(p.x + 646, p.y + 6), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)), ImColor(24, 24, 24, (int)(alpha * 255)), ImColor(24, 24, 24, (int)(alpha * 255)), ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)));
	//d->AddRectFilled(ImVec2(p.x + 7, p.y + 450), ImVec2(p.x + 7, p.y + 450), ImColor(17, 17, 23, 255));  //DOWN
	d->AddText(ImVec2(p.x + 350, p.y + 445), ImColor(255, 255, 255), "Risex for Counter-Strike: Global Offensive");
	//d->AddText(ImVec2(p.x + 640, p.y + 684), ImColor(255, 255, 255), "pandora.uno | 0 day left");
//	d->AddRectFilledMultiColor(ImVec2(p.x + 4, p.y + 2), ImVec2(p.x + m_s.x - 4, p.y + 12),
//		ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)),
//		ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)),
//		ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 25)),
//		ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 25)));
	//d->AddLine(ImVec2(p.x + 140, p.y + 40), ImVec2(p.x + m_s.x, p.y + 40), ImColor(54, 53, 54, (int)(alpha * 255)), 1.0f);
	//d->AddLine(ImVec2(p.x + 0, p.y + 410), ImVec2(p.x + 140, p.y + 410), ImColor(54, 53, 54, (int)(alpha * 255)), 1.0f);
	std::string names = "";
	std::string line;

	std::ifstream in("C:\\Risex\\Settings\\credentials.ini"); // окрываем файл для чтения
	if (in.is_open()) {
		while (getline(in, line))
			names = line;
	}

	in.close();
	//std::string usrname = "Name: ";
	//usrname += names;
	std::string version = "Version: v1.0.0";
	std::string build = "Build: Alpha";
	std::string time = crypt_str("Time: ") + globals.g.time;
	//auto ss = ImGui::CalcTextSize(usrname.c_str());
	ImGui::PushFont(c_menu::get().new_font);
	//d->AddText(ImVec2(p.x + 10, p.y + 390), ImColor(220, 220, 220, int(alpha * 255)), usrname.c_str());
	d->AddText(ImVec2(p.x + 10, p.y + 410), ImColor(220, 220, 220, int(alpha * 255)), build.c_str());
	d->AddText(ImVec2(p.x + 10, p.y + 430), ImColor(220, 220, 220, int(alpha * 255)), version.c_str());
	d->AddText(ImVec2(p.x + 10, p.y + 450), ImColor(220, 220, 220, int(alpha * 255)), time.c_str());
	ImGui::PopFont();
	ImGui::PushFont(c_menu::get().new_font);
	ImGui::PushFont(c_menu::get().futura_large);
	std::string name = "R";
	ImGui::GetWindowDrawList()->AddText({ p.x + 28, p.y + 16 }, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(alpha * 255)), name.c_str());
	ImGui::PopFont();

	ImGui::PushFont(c_menu::get().futura_large);
	std::string name_two = " I S E X";
	ImGui::GetWindowDrawList()->AddText({ p.x + 40, p.y + 16 }, ImColor(255, 255, 255), name_two.c_str());
	ImGui::PopFont();
	ImGui::PushFont(c_menu::get().futura_smallest);
	std::string name_three = " Developed by overlxrd";
	ImGui::GetWindowDrawList()->AddText({ p.x + 10, p.y + 40 }, ImColor(255, 255, 255), name_three.c_str());
	ImGui::PopFont();
}

#include "../features/misc/misc.h"

enum key_bind_num
{
	_AUTOFIRE,
	_LEGITBOT,
	_DOUBLETAP,
	_SAFEPOINT,
	_MIN_DAMAGE,
	_ANTI_BACKSHOT = 12,
	_M_BACK,
	_M_LEFT,
	_M_RIGHT,
	_DESYNC_FLIP,
	_THIRDPERSON,
	_AUTO_PEEK,
	_EDGE_JUMP,
	_FAKEDUCK,
	_SLOWWALK,
	_BODY_AIM,
	_RAGEBOT,
	_TRIGGERBOT,
	_L_RESOLVER_OVERRIDE,
	_FAKE_PEEK,
};
void c_menu::keys()
{


	static float main_alpha = 0.f;

	bool rage = g_cfg.ragebot.enable, legit = g_cfg.legitbot.enabled, aa = g_cfg.antiaim.enable, vis = g_cfg.player.enable;


	int pressed_binds = 0;


	static float keys_alpha[15] = {};
	if (key_binds::get().get_key_bind_state(_RAGEBOT) && rage) pressed_binds++;
	if (g_cfg.ragebot.weapon[globals.g.current_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[globals.g.current_weapon].damage_override_key.key < KEY_MAX && key_binds::get().get_key_bind_state(_MIN_DAMAGE) && rage) pressed_binds++;
	if (key_binds::get().get_key_bind_state(_LEGITBOT) && legit)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_AUTOFIRE) && legit)pressed_binds++;
	if (g_cfg.ragebot.double_tap && misc::get().double_tap_key || key_binds::get().get_key_bind_state(_DOUBLETAP) && rage)pressed_binds++;
	if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_BODY_AIM) && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_SAFEPOINT) && rage)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_THIRDPERSON) && vis)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_DESYNC_FLIP) && aa)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_FAKEDUCK) && aa)pressed_binds++;
	if (key_binds::get().get_key_bind_state(_AUTO_PEEK))pressed_binds++;
	if (hooks::menu_open) pressed_binds++;

	key_bind gui;
	gui.mode = (key_bind_mode)(1);

	if (!g_cfg.menu.keybinds)
		return;

	if (hooks::menu_open || pressed_binds > 0) {
		if (main_alpha < 1)
			main_alpha += 5 * ImGui::GetIO().DeltaTime;
	}
	else
		if (main_alpha > 0)
			main_alpha -= 5 * ImGui::GetIO().DeltaTime;

	if (main_alpha < 0.05f)
		return;
	ImGuiStyle* Style = &ImGui::GetStyle();
	ImDrawList* draw;
	Style->WindowRounding = 0;
	Style->WindowBorderSize = 1;
	Style->WindowMinSize = { 1,1 };
	static float alpha = 0.f;
	Style->Colors[ImGuiCol_WindowBg] = ImColor(33, 33, 33, 215);//zochem? ia zhe ubral bg?
	Style->Colors[ImGuiCol_ChildBg] = ImColor(21, 20, 21, 255);
	Style->Colors[ImGuiCol_ResizeGrip] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripHovered] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripActive] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_Border] = ImColor(38, 39, 55, 215);
	Style->Colors[ImGuiCol_Button] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonHovered] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonActive] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_PopupBg] = ImColor(18, 17, 18, 255);
	Style->FramePadding = ImVec2(4, 3);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, main_alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 3));
	ImVec2 p, s;



	ImGui::Begin("KEYS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{

		auto d = ImGui::GetWindowDrawList();
		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		ImGui::PushFont(c_menu::get().futura_small);
		//ImGui::SetWindowSize(ImVec2(400, 500 + 400 * (m_engine()->IsConnected() ? pressed_binds : hooks::menu_open ? 1 : 0)));
		//PostProcessing::performFullscreenBlur(d, main_alpha);
		//d->AddRectFilled(p, p + ImVec2(200, 21 + 18 * pressed_binds), ImColor(23, 23, 23, int(50 * main_alpha)));
		auto main_colf = ImColor(13, 13, 13, 200);
		auto main_coll = ImColor(13, 13, 13, 200);
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf, main_colf, main_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf, main_coll, main_coll, main_colf);
		//PostProcessing::performFullscreenBlur(d, 1.f);
		auto main_colf2 = ImColor(23, 23, 23, int(140 * min(main_alpha * 2, 1.f)));
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf2, main_colf2, main_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf2, main_coll, main_coll, main_colf2);
		auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		auto line_coll = ImColor(13, 13, 13, 200);
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 2), line_coll, line_colf, line_colf, line_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((200) / 2 - ImGui::CalcTextSize("keybinds").x / 2, (20) / 2 - ImGui::CalcTextSize("keybinds").y / 2), ImColor(250, 250, 250, int(230 * min(main_alpha * 3, 1.f))), "keybinds");

		ImGui::SetCursorPosY(20 + 2);
		ImGui::BeginGroup();//keys 
		add_key("Menu", hooks::menu_open, gui, 200, 300, true);
		if (m_engine()->IsConnected()) {
			add_key("Minimum damage", key_binds::get().get_key_bind_state(_MIN_DAMAGE), g_cfg.ragebot.weapon[globals.g.current_weapon].damage_override_key, 200, 200, rage, true);
			add_key("Legitbot key", key_binds::get().get_key_bind_state(_LEGITBOT), g_cfg.legitbot.key, 200, 200, legit);
			add_key("Trigger-bot", key_binds::get().get_key_bind_state(_AUTOFIRE), g_cfg.legitbot.autofire_key, 200, 200, legit);
			add_key("Double-tap", g_cfg.ragebot.double_tap && misc::get().double_tap_key || misc::get().double_tap_enabled || key_binds::get().get_key_bind_state(_DOUBLETAP), g_cfg.ragebot.double_tap_key, 200, min(main_alpha * 2, 1.f), rage, false);
			add_key("Hide-shots", g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key, g_cfg.antiaim.hide_shots_key, 200, min(main_alpha * 2, 1.f), rage&& aa, false);
			add_key("Force body-aim", key_binds::get().get_key_bind_state(_BODY_AIM), g_cfg.ragebot.body_aim_key, 200, 200, rage);
			add_key("Force safe-point", key_binds::get().get_key_bind_state(_SAFEPOINT), g_cfg.ragebot.safe_point_key, 200, rage);
			add_key("Third person", key_binds::get().get_key_bind_state(_THIRDPERSON), g_cfg.misc.thirdperson_toggle, 200, vis);
			add_key("Inverter", key_binds::get().get_key_bind_state(_DESYNC_FLIP), g_cfg.antiaim.flip_desync, 200, 200, aa);
			add_key("Fake-duck", key_binds::get().get_key_bind_state(_FAKEDUCK), g_cfg.misc.fakeduck_key, 200, 200, rage && aa);
			add_key("Automatic peek", key_binds::get().get_key_bind_state(_AUTO_PEEK), g_cfg.misc.automatic_peek, 200, 200, true);
		}
		ImGui::EndGroup();
		ImGui::PopFont();
	}
	ImGui::End();
	ImGui::PopStyleVar(2);
}



void c_menu::spectators()
{

	LPDIRECT3DTEXTURE9 photo[32];
	int specs = 0;
	int id[32];
	int modes = 0;
	std::string spect = "";
	std::string mode = "";
	if (m_engine()->IsInGame() && m_engine()->IsConnected())
	{

		int localIndex = m_engine()->GetLocalPlayer();
		player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
		if (pLocalEntity)
		{
			for (int i = 0; i < m_engine()->GetMaxClients(); i++)
			{
				player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
				if (!pBaseEntity)
					continue;
				if (pBaseEntity->m_iHealth() > 0)
					continue;
				if (pBaseEntity == pLocalEntity)
					continue;
				if (pBaseEntity->IsDormant())
					continue;
				if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
					continue;

				player_info_t pInfo;
				m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
				if (pInfo.ishltv)
					continue;
				spect += pInfo.szName;
				spect += "\n";
				specs++;
				id[i] = pInfo.steamID64;
				//photo[i] = steam_image(CSteamID((uint64)pInfo.steamID64));
				switch (pBaseEntity->m_iObserverMode())
				{
				case OBS_MODE_IN_EYE:
					mode += "Perspective";
					break;
				case OBS_MODE_CHASE:
					mode += "3rd Person";
					break;
				case OBS_MODE_ROAMING:
					mode += "No Clip";
					break;
				case OBS_MODE_DEATHCAM:
					mode += "Deathcam";
					break;
				case OBS_MODE_FREEZECAM:
					mode += "Freezecam";
					break;
				case OBS_MODE_FIXED:
					mode += "Fixed";
					break;
				default:
					break;
				}

				mode += "\n";
				modes++;

			}
		}
	}

	static float main_alpha = 0.f;
	if (!g_cfg.menu.spectators_list)
		return;

	if (hooks::menu_open || specs > 0) {
		if (main_alpha < 1)
			main_alpha += 5 * ImGui::GetIO().DeltaTime;
	}
	else
		if (main_alpha > 0)
			main_alpha -= 5 * ImGui::GetIO().DeltaTime;

	if (main_alpha < 0.05f)
		return;

	ImGuiStyle* Style = &ImGui::GetStyle();
	ImDrawList* draw;
	Style->WindowRounding = 0;
	Style->WindowBorderSize = 1;
	Style->WindowMinSize = { 1,1 };
	Style->Colors[ImGuiCol_WindowBg] = ImColor(33, 33, 33, 215);//zochem? ia zhe ubral bg?
	Style->Colors[ImGuiCol_ChildBg] = ImColor(21, 20, 21, 255);
	Style->Colors[ImGuiCol_ResizeGrip] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripHovered] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripActive] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_Border] = ImColor(38, 39, 55, 215);
	Style->Colors[ImGuiCol_Button] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonHovered] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonActive] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_PopupBg] = ImColor(18, 17, 18, 255);
	Style->FramePadding = ImVec2(1, 1);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, main_alpha);
	ImVec2 p, s;
	//ImGui::SetNextWindowSize(ImVec2(200, 20 + 15 * 15));

//	if (!vars.misc.spectators)
//		return;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 3));
	ImGui::Begin("SPECTATORS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoNav);
	{


		auto d = ImGui::GetWindowDrawList();
		int think_size = 200;
		int calced_size = think_size - 5;

		p = ImGui::GetWindowPos();
		s = ImGui::GetWindowSize();
		//ImGui::SetWindowSize(ImVec2(200, 21 + 20 * specs));
		ImGui::PushFont(c_menu::get().futura_small);
		////PostProcessing::performFullscreenBlur(d, main_alpha);
		//d->AddRectFilled(p, p + ImVec2(200, 21 + 20 * specs), ImColor(23, 23, 23, int(50 * main_alpha)));
		//auto main_colf = ImColor(13, 13, 13, 200);
		//auto main_coll = ImColor(13, 13, 13, 200);
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf, main_colf, main_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf, main_coll, main_coll, main_colf);
		////PostProcessing::performFullscreenBlur(d, 1.f);
		//auto main_colf2 = ImColor(23, 23, 23, int(140 * min(main_alpha * 2, 1.f)));
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 20), main_coll, main_colf2, main_colf2, main_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 20), main_colf2, main_coll, main_coll, main_colf2);
		//auto line_colf = ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), 255);
		//auto line_coll = ImColor(13, 13, 13, 200);
		//d->AddRectFilledMultiColor(p, p + ImVec2(100, 2), line_coll, line_colf, line_colf, line_coll);
		//d->AddRectFilledMultiColor(p + ImVec2(100, 0), p + ImVec2(200, 2), line_colf, line_coll, line_coll, line_colf);
		d->AddText(p + ImVec2((200) / 2 - ImGui::CalcTextSize("spectators").x / 2, (20) / 2 - ImGui::CalcTextSize("spectators").y / 2), ImColor(250, 250, 250, int(230 * min(main_alpha * 3, 1.f))), "spectators");
		if (specs > 0) spect += "\n";
		if (modes > 0) mode += "\n";


		ImGui::SetCursorPosY(22);
		if (m_engine()->IsInGame() && m_engine()->IsConnected())
		{
			int localIndex = m_engine()->GetLocalPlayer();
			player_t* pLocalEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(localIndex));
			if (pLocalEntity)
			{
				for (int i = 0; i < m_engine()->GetMaxClients(); i++)
				{
					player_t* pBaseEntity = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(i));
					if (!pBaseEntity)
						continue;
					if (pBaseEntity == pLocalEntity)
						continue;
					if (pBaseEntity->m_hObserverTarget() != pLocalEntity)
						continue;

					player_info_t pInfo;
					m_engine()->GetPlayerInfo(pBaseEntity->EntIndex(), &pInfo);
					if (pInfo.ishltv)
						continue;
					ImGui::SetCursorPosX(8);
					ImGui::Text(pInfo.szName);
					if (pInfo.fakeplayer)
					{
						ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(getAvatarTexture(pBaseEntity->m_iTeamNum()), ImVec2(15, 15), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}
					else
					{
						ImGui::SameLine(-1, s.x - 22);
						//ImGui::Image(steam_image(CSteamID((uint64)pInfo.steamID64)), ImVec2(15, 15), ImVec2(0,0), ImVec2(1,1), ImVec4(1.f, 1.f, 1.f, min(main_alpha * 2, 1.f)));
					}

				}
			}
		}

		ImGui::PopFont();
	}
	ImGui::End();
	ImGui::PopStyleVar(2);
}

std::string scomp_name() {

	char buff[MAX_PATH];
	GetEnvironmentVariableA("USERNAME", buff, MAX_PATH);

	return std::string(buff);
}

void c_menu::watermark()
{

	auto width = 0, height = 0;

	int pressed_binds = 0;
	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";
	static bool other_bind_pressed = false;

	m_engine()->GetScreenSize(width, height);

	auto watermark = crypt_str("risex | ") + scomp_name() + crypt_str(" | ") + globals.g.time;

	if (m_engine()->IsInGame())
	{
		auto nci = m_engine()->GetNetChannelInfo();

		if (nci)
		{
			auto server = nci->GetAddress();

			if (!strcmp(server, crypt_str("loopback")))
				server = crypt_str("local server");
			else if (m_gamerules()->m_bIsValveDS())
				server = crypt_str("valve server");

			auto tickrate = std::to_string((int)(1.0f / m_globals()->m_intervalpertick));
			watermark = crypt_str("risex | ") + scomp_name() + crypt_str(" | ") + server + crypt_str(" | ") + std::to_string(globals.g.ping) + crypt_str(" ms | ") + tickrate + crypt_str(" tick | ") + globals.g.time;
		}
	}

	if (pressed_binds > 0 || hooks::menu_open)
		other_bind_pressed = true;
	else
		other_bind_pressed = false;

	ImVec2 Pos;
	ImVec2 size_menu;
	static float alpha_menu = 0.1f;

	if (other_bind_pressed)
	{
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;

	}
	else
	{
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;

	}

	ImGuiStyle* Style = &ImGui::GetStyle();

	Style->WindowRounding = 0;
	Style->WindowBorderSize = 1;
	Style->WindowMinSize = { 1,1 };
	bool theme = true;
	Style->Colors[ImGuiCol_WindowBg] = ImColor(33, 33, 33, 215);//zochem? ia zhe ubral bg?
	Style->Colors[ImGuiCol_ChildBg] = ImColor(21, 20, 21, 255);
	Style->Colors[ImGuiCol_ResizeGrip] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripHovered] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_ResizeGripActive] = ImColor(42, 40, 43, 0);
	Style->Colors[ImGuiCol_Border] = ImColor(38, 39, 55, 215);
	Style->Colors[ImGuiCol_Button] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonHovered] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_ButtonActive] = ImColor(29, 125, 229, 5);
	Style->Colors[ImGuiCol_PopupBg] = ImColor(18, 17, 18, 255);
	Style->FramePadding = ImVec2(1, 1);
	//if ((g_cfg.menu.watermark)) {
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
	if (ImGui::Begin("Watermark", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
	{

		if (m_engine()->IsInGame())
		{
			auto b_alpha = alpha_menu;
			size_menu = ImGui::GetWindowSize();
			Pos = ImGui::GetWindowPos();
			auto Render = ImGui::GetWindowDrawList();
			//Render->AddRectFilled({ Pos.x + 0, Pos.y + 0 }, { Pos.x + 400, Pos.y + 23 }, ImColor(18, 17, 18, (int)(b_alpha * 255)), 4);
			//Render->AddRectFilledMultiColor({ Pos.x + 0, Pos.y + 2 }, { Pos.x + 370, Pos.y + 7 },
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 25)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 25)));
			//Render->AddRectFilled({ Pos.x + 0, Pos.y + 0 }, { Pos.x + 370, Pos.y + 4 }, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)), 6);
			ImGui::PushFont(c_menu::get().ico_menu);
			auto icon = ImGui::CalcTextSize("k");
			Render->AddText({ Pos.x + 5, Pos.y + 5 }, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)), "k");
			ImGui::PopFont();
			ImGui::PushFont(c_menu::get().new_font);
			Render->AddText({ Pos.x + 1 + icon.x + 0, Pos.y + 5 }, ImColor(255, 255, 255, (int)(alpha_menu * 255)), watermark.c_str());

			//Render->AddLine({ Pos.x + 0, Pos.y + 23 }, { Pos.x + 370, Pos.y + 23 }, ImColor(54, 53, 54, (int)(b_alpha * 255)), 1);
			ImVec2 size = ImGui::CalcTextSize(spect.c_str());
			ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
			ImGui::SetWindowSize(ImVec2(370, 23 + size.y));
			ImGui::SetCursorPosY(24);
			ImGui::Columns(2, "fart1", false);


			ImGui::SetColumnWidth(0, 190 - (size2.x + 8));
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::PopFont();
		}
		else
		{
			auto b_alpha = alpha_menu;
			size_menu = ImGui::GetWindowSize();
			Pos = ImGui::GetWindowPos();
			auto Render = ImGui::GetWindowDrawList();
			//Render->AddRectFilled({ Pos.x + 0, Pos.y + 0 }, { Pos.x + 220, Pos.y + 23 }, ImColor(18, 17, 18, (int)(b_alpha * 255)), 4);
			//Render->AddRectFilledMultiColor({ Pos.x + 0, Pos.y + 2 }, { Pos.x + 219020, Pos.y + 7 },
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 25)),
			//	ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 25)));
			//Render->AddRectFilled({ Pos.x + 0, Pos.y + 0 }, { Pos.x + 190, Pos.y + 4 }, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)), 6);
			ImGui::PushFont(c_menu::get().ico_menu);
			auto icon = ImGui::CalcTextSize("k");
			Render->AddText({ Pos.x + 5, Pos.y + 5 }, ImColor(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(b_alpha * 255)), "k");
			ImGui::PopFont();
			ImGui::PushFont(c_menu::get().new_font);
			Render->AddText({ Pos.x + 1 + icon.x + 0, Pos.y + 5 }, ImColor(255, 255, 255, (int)(alpha_menu * 255)), watermark.c_str());

			//Render->AddLine({ Pos.x + 0, Pos.y + 23 }, { Pos.x + 190, Pos.y + 23 }, ImColor(54, 53, 54, (int)(b_alpha * 255)), 1);
			ImVec2 size = ImGui::CalcTextSize(spect.c_str());
			ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
			ImGui::SetWindowSize(ImVec2(190, 23 + size.y));
			ImGui::SetCursorPosY(24);
			ImGui::Columns(2, "fart1", false);


			ImGui::SetColumnWidth(0, 190 - (size2.x + 8));
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::PopFont();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
	//}
}

void c_menu::draw(bool is_open)
{
	static auto w = 0, h = 0, current_h = 0;
	m_engine()->GetScreenSize(w, current_h);
	if (h != current_h)
	{
		if (h)
			update_scripts = true;

		h = current_h;
		update_dpi = true;
	}

	// animation related code
	static float m_alpha = 0.0002f;
	m_alpha = math::clamp(m_alpha + (3.f * ImGui::GetIO().DeltaTime * (is_open ? 1.f : -1.f)), 0.0001f, 1.f);

	// set alpha in class to use later in widgets
	public_alpha = m_alpha;

	if (m_alpha <= 0.0001f)
		return;

	// set new alpha
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_alpha);

	// setup colors and some styles
	if (!menu_setupped)
		menu_setup(ImGui::GetStyle());

	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].x, ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].y, ImGui::GetStyle().Colors[ImGuiCol_ScrollbarGrab].z, m_alpha));

	// default menu size
	const int x = 850, y = 560;

	// last active tab to switch effect & reverse alpha & preview alpha
	// IMPORTANT: DO TAB SWITCHING BY LAST_TAB!!!!!
	static int last_tab = active_tab;
	static bool preview_reverse = false;

	// start menu render

	ImGuiStyle* Styles = &ImGui::GetStyle();

	Styles->WindowRounding = 0;
	Styles->WindowBorderSize = 1;
	Styles->WindowMinSize = { 1,1 };
	bool theme = true;
	Styles->Colors[ImGuiCol_WindowBg] = ImColor(15, 15, 15, 115);//zochem? ia zhe ubral bg?
	Styles->Colors[ImGuiCol_ChildBg] = ImColor(20, 20, 20, 115);
	Styles->Colors[ImGuiCol_ScrollbarBg] = ImColor(20, 20, 20, 115);
	Styles->Colors[ImGuiCol_ResizeGrip] = ImColor(42, 40, 43, 0);
	Styles->Colors[ImGuiCol_ResizeGripHovered] = ImColor(42, 40, 43, 0);
	Styles->Colors[ImGuiCol_ResizeGripActive] = ImColor(42, 40, 43, 0);
	Styles->Colors[ImGuiCol_Border] = ImColor(38, 39, 55, 215);
	Styles->Colors[ImGuiCol_Button] = ImColor(29, 125, 229, 5);
	Styles->Colors[ImGuiCol_ButtonHovered] = ImColor(29, 125, 229, 5);
	Styles->Colors[ImGuiCol_ButtonActive] = ImColor(29, 125, 229, 5);
	Styles->Colors[ImGuiCol_PopupBg] = ImColor(18, 17, 18, 255);
	Styles->FramePadding = ImVec2(2, 2);
	Styles->PopupRounding = 2;
	static int rage_sub_tab;

	static float alpha_menu = 0;
	if (is_open && c_menu::get().menu_alpha <= 1.f)
		c_menu::get().menu_alpha += 0.01f;
	else if (!is_open && c_menu::get().menu_alpha >= 0.f)
		c_menu::get().menu_alpha -= 0.01f;
	ImVec2 size_menu;
	static int tab = 0;
	///menu
	ImVec2 posW;
	if (is_open) {
		ImVec2 m_s;
		//  mouse_pos 
		bool open = 0;
		static int tab = 0;
		static int sidesize = 0;
		static float alpha_bar = 0;
		static float alpha_logo = 0;
		float alpha = alpha_menu;

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, c_menu::get().menu_alpha);
		ImGui::SetNextWindowSizeConstraints(ImVec2(650, 470), ImVec2(650, 470));
		ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		{
			m_s = ImGui::GetWindowSize();
			ImVec2 p = ImGui::GetWindowPos();
			auto d = ImGui::GetWindowDrawList();
			static int t = tab;
			designers(c_menu::get().menu_alpha, p);
			ImGui::SetCursorPos(ImVec2(360, 16));
			if (ImGui::Tab("", "ragebot", t == 0 ? true : false, ImVec2(70, 30))) t = 0;
			ImGui::SetCursorPos(ImVec2(410, 16));
			if (ImGui::Tab("", "antiaim", t == 1 ? true : false, ImVec2(70, 30))) t = 1;
			ImGui::SetCursorPos(ImVec2(455, 16));
			if (ImGui::Tab("", "visuals", t == 2 ? true : false, ImVec2(70, 30))) t = 2;
			ImGui::SetCursorPos(ImVec2(495, 16));
			if (ImGui::Tab("", "skins", t == 3 ? true : false, ImVec2(70, 30))) t = 3;
			ImGui::SetCursorPos(ImVec2(530, 16));
			if (ImGui::Tab("", "misc", t == 4 ? true : false, ImVec2(70, 30))) t = 4;
			ImGui::SetCursorPos(ImVec2(580, 16));
			if (ImGui::Tab("", "configs", t == 5 ? true : false, ImVec2(70, 30))) t = 5;

			if (c_menu::get().menu_alpha > 0.9) {
				switch (t)
				{
				case 0://ragebot
				{

					ImGui::PushFont(c_menu::get().new_font);
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::PushItemWidth(210);
					ImGui::BeginChild("Aimbot", ImVec2(240, 395), false);
					{
						ImGui::PushItemWidth(210);
						child_start("General");
						draw_checkbox(crypt_str("Enable"), &g_cfg.ragebot.enable);
						if (g_cfg.ragebot.enable)
							g_cfg.legitbot.enabled = false;

						ImGui::SliderInt(crypt_str("Field of view"), &g_cfg.ragebot.field_of_view, 1, 180, false, crypt_str("%d°"));

						draw_checkbox(crypt_str("Silent aim"), &g_cfg.ragebot.silent_aim);

						draw_checkbox(crypt_str("Auto fire"), &g_cfg.ragebot.autoshoot);

						draw_checkbox(crypt_str("Auto wall"), &g_cfg.ragebot.autowall);

						draw_checkbox(crypt_str("Auto-Scope"), &g_cfg.ragebot.autoscope);

						draw_keybind(crypt_str("Force safe points"), &g_cfg.ragebot.safe_point_key, crypt_str("##HOKEY_FORCE_SAFE_POINTS"));

						draw_keybind(crypt_str("Force body aim"), &g_cfg.ragebot.body_aim_key, crypt_str("##HOKEY_FORCE_BODY_AIM"));

						draw_keybind(crypt_str("Double-tap"), &g_cfg.ragebot.double_tap_key, crypt_str("##HOTKEY_DT"));

						draw_keybind(crypt_str("Hide-shots"), &g_cfg.antiaim.hide_shots_key, crypt_str("##HOTKEY_HIDESHOTS"));

						ImGui::PopItemWidth();
					}
					ImGui::EndChild();

					ImGui::SetCursorPos({ 390, 45 });

					ImGui::PushItemWidth(210);
					ImGui::BeginChild("Aimbot2", ImVec2(240, 395), false);
					{
						child_start("Accuracy");
						ImGui::PushItemWidth(210);
						ImGui::PushFont(astrium);
						const char* rage_weapons[8] = { crypt_str("A"), crypt_str("D"), crypt_str("M"), crypt_str("S"), crypt_str("Y"), crypt_str("a"), crypt_str("Z"), crypt_str("f") };


						draw_combo(crypt_str(""), hooks::rage_weapon, rage_weapons, ARRAYSIZE(rage_weapons));

						ImGui::PopFont();

						draw_multicombo(crypt_str("Hitboxes"), g_cfg.ragebot.weapon[hooks::rage_weapon].hitboxes, hitboxes, ARRAYSIZE(hitboxes), preview);

						draw_checkbox(crypt_str("Static point scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].static_point_scale)
						{
							ImGui::SliderFloat(crypt_str("Head scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].head_scale, 0.0f, 100.0f, g_cfg.ragebot.weapon[hooks::rage_weapon].head_scale ? crypt_str("%.2f") : crypt_str("None"));
							ImGui::SliderFloat(crypt_str("Body scale"), &g_cfg.ragebot.weapon[hooks::rage_weapon].body_scale, 0.0f, 100.0f, g_cfg.ragebot.weapon[hooks::rage_weapon].body_scale ? crypt_str("%.2f") : crypt_str("None"));
						}

						draw_checkbox(crypt_str("Prefer safe points"), &g_cfg.ragebot.weapon[hooks::rage_weapon].prefer_safe_points);

						draw_checkbox(crypt_str("Prefer body aim"), &g_cfg.ragebot.weapon[hooks::rage_weapon].prefer_body_aim);

						draw_checkbox(crypt_str("Auto-stop"), &g_cfg.ragebot.weapon[hooks::rage_weapon].autostop);
						if (g_cfg.ragebot.weapon[hooks::rage_weapon].autostop) {
							draw_multicombo(crypt_str("Auto-stop settings"), g_cfg.ragebot.weapon[hooks::rage_weapon].autostop_modifiers, autostop_modifiers, ARRAYSIZE(autostop_modifiers), preview);
						}
						draw_checkbox(crypt_str("Hitchance"), &g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance);

						if (g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance) {
							ImGui::SliderInt(crypt_str("Amount##NOOOOOOOOOOOOOOO"), &g_cfg.ragebot.weapon[hooks::rage_weapon].hitchance_amount, 1, 100);
						}

						ImGui::SliderInt(crypt_str("Min damage"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_damage, 1, 120, true);

						draw_keybind(crypt_str("Damage override"), &g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key, crypt_str("##HOTKEY__DAMAGE_OVERRIDE"));

						if (g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key > KEY_NONE && g_cfg.ragebot.weapon[hooks::rage_weapon].damage_override_key.key < KEY_MAX)
						{
							ImGui::SliderInt(crypt_str("Amount##again?"), &g_cfg.ragebot.weapon[hooks::rage_weapon].minimum_override_damage, 1, 120, true);
						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();
					ImGui::PopFont();
				}break;
				case 1://anti-aim
				{
					ImGui::PushFont(c_menu::get().new_font);
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("aa", ImVec2(240, 395), false);
					{
						child_start("Anti-aim");
						ImGui::PushItemWidth(210);
						static auto type = 0;

						draw_checkbox(crypt_str("Enable"), &g_cfg.antiaim.enable);

						if (g_cfg.antiaim.enable)
						{
							draw_combo(crypt_str("Movement"), type, movement_type, ARRAYSIZE(movement_type));

							//draw_combo(crypt_str("Yaw"), g_cfg.antiaim.type[type].yaw, yaw, ARRAYSIZE(yaw));

							draw_combo(crypt_str("Eye Pos"), g_cfg.antiaim.type[type].base_angle, baseangle, ARRAYSIZE(baseangle));

							ImGui::SliderInt(crypt_str("Jitter"), & g_cfg.antiaim.type[type].range_jitter, 0, 180);

							padding(0, 3);


							draw_combo(crypt_str("Desync"), g_cfg.antiaim.type[type].desync, desync, ARRAYSIZE(desync));

							draw_combo(crypt_str("Lower Body Yaw"), g_cfg.antiaim.type[type].lby_type, lby_type, ARRAYSIZE(lby_type));

							if (g_cfg.antiaim.type[type].desync)
							{
								if (g_cfg.antiaim.type[type].lby_type == 0) {
									ImGui::SliderInt(crypt_str("Left Limit"), &g_cfg.antiaim.type[type].desync_range, 1, 60);

									ImGui::SliderInt(crypt_str("Right Limit"), &g_cfg.antiaim.type[type].inverted_desync_range, 1, 60);

								}

							}

							if (g_cfg.antiaim.type[type].desync == 1)
							{
								draw_keybind(crypt_str("Inverter"), &g_cfg.antiaim.flip_desync, crypt_str("##HOTKEY_INVERT_DESYNC"));
							}

							//draw_multicombo(crypt_str("Inverted condition"), g_cfg.antiaim.inverted_condition, condition_switch, ARRAYSIZE(condition_switch), preview);

							draw_keybind(crypt_str("Manual back"), &g_cfg.antiaim.manual_back, crypt_str("##HOTKEY_INVERT_BACK"));

							draw_keybind(crypt_str("Manual left"), &g_cfg.antiaim.manual_left, crypt_str("##HOTKEY_INVERT_LEFT"));

							draw_keybind(crypt_str("Manual right"), &g_cfg.antiaim.manual_right, crypt_str("##HOTKEY_INVERT_RIGHT"));

							if (g_cfg.antiaim.manual_back.key > KEY_NONE && g_cfg.antiaim.manual_back.key < KEY_MAX || g_cfg.antiaim.manual_left.key > KEY_NONE && g_cfg.antiaim.manual_left.key < KEY_MAX || g_cfg.antiaim.manual_right.key > KEY_NONE && g_cfg.antiaim.manual_right.key < KEY_MAX)
							{
								draw_checkbox(crypt_str("Manuals indicator"), &g_cfg.antiaim.flip_indicator);
								ImGui::SameLine();
								ImGui::ColorEdit(crypt_str("##invc"), &g_cfg.antiaim.flip_indicator_color, ALPHA);
							}
							ImGui::PopItemWidth();
						}
					}
					ImGui::EndChild();

					ImGui::SetCursorPos({ 390, 45 });

					ImGui::BeginChild("aa2", ImVec2(240, 395), false);
					{
						child_start("Fake-lag");
						ImGui::PushItemWidth(210);

						static auto type = 0;

						draw_checkbox(crypt_str("Fake lag"), &g_cfg.antiaim.fakelag);

						if (g_cfg.antiaim.fakelag)
						{
							ImGui::SliderInt(crypt_str("Limit"), &g_cfg.antiaim.fakelag_amount, 1, 14);

							draw_keybind(crypt_str("Fake-duck"), &g_cfg.misc.fakeduck_key, crypt_str("##FAKEDUCK__HOTKEY"));
							draw_keybind(crypt_str("Slow-walk"), &g_cfg.misc.slowwalk_key, crypt_str("##SLOWWALK__HOTKEY"));
							ImGui::PopItemWidth();
						}
					}
					ImGui::EndChild();
					ImGui::PopFont();
				}break;
				case 2://visuals
				{
					static int category = 0;
					static int category_w = 0;
					static int vis_subtab = 0;
					ImGui::SetCursorPos(ImVec2(30, 150 - 80));
					if (ImGui::SubTab("Players", "v1", vis_subtab == 0 ? true : false, ImVec2(85, 27))) vis_subtab = 0;
					ImGui::SetCursorPos(ImVec2(30, 180 - 80));
					if (ImGui::SubTab("World", "v2", vis_subtab == 1 ? true : false, ImVec2(85, 27))) vis_subtab = 1;
					ImGui::SetCursorPos(ImVec2(30, 210 - 80));
					if (ImGui::SubTab("View", "v3", vis_subtab == 2 ? true : false, ImVec2(85, 27))) vis_subtab = 2;
					ImGui::SetCursorPos(ImVec2(30, 240 - 80));
					if (ImGui::SubTab("Other", "v4", vis_subtab == 3 ? true : false, ImVec2(85, 27))) vis_subtab = 3;
					const char* first_name[] = { "ESP","Main","Camera", "Misc" };
					const char* second_name[] = { "Chams","Misc","Viewmodel", "Grenades" };
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("Visuals", ImVec2(240, 395), false);
					{
						ImGui::PushItemWidth(210);
						child_start(first_name[vis_subtab]);
						switch (vis_subtab)
						{
						case 0:
						{
							draw_checkbox(crypt_str("Enable"), &g_cfg.player.enable);

							if (category == ENEMY)
							{
								draw_checkbox(crypt_str("Out Of Fov Arrows"), &g_cfg.player.arrows);
								ImGui::SameLine(0, 200);
								ImGui::ColorEdit(crypt_str("##arrowscolor"), &g_cfg.player.arrows_color, ALPHA);

								if (g_cfg.player.arrows)
								{
									ImGui::SliderInt(crypt_str("Arrows distance"), &g_cfg.player.distance, 1, 100);
									ImGui::SliderInt(crypt_str("Arrows size"), &g_cfg.player.size, 1, 100);
								}
							}

							draw_checkbox(crypt_str("Bounding box"), &g_cfg.player.type[category].box);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##boxcolor"), &g_cfg.player.type[category].box_color, ALPHA);

							draw_checkbox(crypt_str("Name"), &g_cfg.player.type[category].name);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##namecolor"), &g_cfg.player.type[category].name_color, ALPHA);

							draw_checkbox(crypt_str("Health bar"), &g_cfg.player.type[category].health);
							draw_checkbox(crypt_str("Health color"), &g_cfg.player.type[category].custom_health_color);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##healthcolor"), &g_cfg.player.type[category].health_color, ALPHA);

							for (auto i = 0, j = 0; i < ARRAYSIZE(flags); i++)
							{
								if (g_cfg.player.type[category].flags[i])
								{
									if (j)
										preview += crypt_str(", ") + (std::string)flags[i];
									else
										preview = flags[i];

									j++;
								}
							}

							draw_multicombo(crypt_str("Flags"), g_cfg.player.type[category].flags, flags, ARRAYSIZE(flags), preview);
							draw_multicombo(crypt_str("Weapon"), g_cfg.player.type[category].weapon, weaponplayer, ARRAYSIZE(weaponplayer), preview);


							if (g_cfg.player.type[category].weapon[WEAPON_ICON] || g_cfg.player.type[category].weapon[WEAPON_TEXT])
							{
								draw_color(crypt_str("Color weapon"), &g_cfg.player.type[category].weapon_color, ALPHA);
							}

							draw_checkbox(crypt_str("Skeleton"), &g_cfg.player.type[category].skeleton);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##skeletoncolor"), &g_cfg.player.type[category].skeleton_color, ALPHA);

							draw_checkbox(crypt_str("Ammo bar"), &g_cfg.player.type[category].ammo);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##ammocolor"), &g_cfg.player.type[category].ammobar_color, ALPHA);

							draw_checkbox(crypt_str("Footsteps"), &g_cfg.player.type[category].footsteps);
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##footstepscolor"), &g_cfg.player.type[category].footsteps_color, ALPHA);

							if (g_cfg.player.type[category].footsteps)
							{
								ImGui::SliderInt(crypt_str("Thickness"), &g_cfg.player.type[category].thickness, 1, 10);
								ImGui::SliderInt(crypt_str("Radius"), &g_cfg.player.type[category].radius, 50, 500);
							}

							if (category == ENEMY || category == TEAM)
							{
								draw_checkbox(crypt_str("Snap lines"), &g_cfg.player.type[category].snap_lines);
								ImGui::SameLine();
								ImGui::ColorEdit(crypt_str("##snapcolor"), &g_cfg.player.type[category].snap_lines_color, ALPHA);
							}
						}break;
						case 1:
						{draw_checkbox(crypt_str("Enable"), &g_cfg.player.enable);
						if (g_cfg.player.enable) {
							draw_multicombo(crypt_str("Removals"), g_cfg.esp.removals, removals, ARRAYSIZE(removals), preview);
							if (g_cfg.esp.removals[REMOVALS_ZOOM]) {
								draw_checkbox(crypt_str("Fix zoom sensivity"), &g_cfg.esp.fix_zoom_sensivity);
							}
							draw_checkbox(crypt_str("Bomb indicator"), &g_cfg.esp.bomb_timer);
							draw_multicombo(crypt_str("Weapon ESP"), g_cfg.esp.weapon, weaponesp, ARRAYSIZE(weaponesp), preview);
							if (g_cfg.esp.weapon[WEAPON_ICON] || g_cfg.esp.weapon[WEAPON_TEXT] || g_cfg.esp.weapon[WEAPON_DISTANCE])
							{
								ImGui::Text(crypt_str("Color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##weaponcolor"), &g_cfg.esp.weapon_color, ALPHA);
							}
							if (g_cfg.esp.weapon[WEAPON_BOX])
							{
								ImGui::Text(crypt_str("Box color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##weaponboxcolor"), &g_cfg.esp.box_color, ALPHA);
							}
							if (g_cfg.esp.weapon[WEAPON_GLOW])
							{
								ImGui::Text(crypt_str("Glow color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##weaponglowcolor"), &g_cfg.esp.weapon_glow_color, ALPHA);
							}
							if (g_cfg.esp.weapon[WEAPON_AMMO])
							{
								ImGui::Text(crypt_str("Ammo bar color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##weaponammocolor"), &g_cfg.esp.weapon_ammo_color, ALPHA);
							}

							draw_checkbox(crypt_str("Taser range"), &g_cfg.esp.taser_range);
							draw_checkbox(crypt_str("Show spread"), &g_cfg.esp.show_spread);
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##spredcolor"), &g_cfg.esp.show_spread_color, ALPHA);
							draw_checkbox(crypt_str("Penetration crosshair"), &g_cfg.esp.penetration_reticle);
							const char* penet[] =
							{
								"Dot",
								"World rect"
							};
						}break;
						}
						case 2:
						{

							ImGui::SliderInt(crypt_str("View FOV"), &g_cfg.esp.fov, 0, 100);
							draw_keybind(crypt_str("Thirdperson"), &g_cfg.misc.thirdperson_toggle, crypt_str("##TPKEY__HOTKEY"));

							if (g_cfg.misc.thirdperson_toggle.key > KEY_NONE && g_cfg.misc.thirdperson_toggle.key < KEY_MAX)
							{
								ImGui::SliderInt(crypt_str("Thirdperson distance"), &g_cfg.misc.thirdperson_distance, 0, 300);
							}

							draw_checkbox(crypt_str("Thirdperson when spectating"), &g_cfg.misc.thirdperson_when_spectating);

							draw_checkbox(crypt_str("Aspect-ratio"), &g_cfg.misc.aspect_ratio);
							if (g_cfg.misc.aspect_ratio)
							{
								ImGui::SliderFloat("Amount", &g_cfg.misc.aspect_ratio_amount, 0.1, 2.5);
							}

						}break;
						case 3:
						{
							draw_checkbox(crypt_str("Hitsound"), &g_cfg.esp.hitsound);

							draw_checkbox(crypt_str("Hitmarker"), &g_cfg.esp.hitmarker);

							draw_checkbox(crypt_str("Damage marker"), &g_cfg.esp.damage_marker);
							//if (g_cfg.esp.hitmarker)
							//{
							//	ImGui::SameLine(210);
							//	ImGui::ColorEdit(crypt_str("##clientbulletimpacts"), &g_cfg.esp.hit, ALPHA);
							//}
							draw_checkbox(crypt_str("Kill effect"), &g_cfg.esp.kill_effect);
							if (g_cfg.esp.kill_effect) {
								ImGui::SliderFloat(crypt_str("Duration"), &g_cfg.esp.kill_effect_duration, 0.01f, 3.0f, "%.1f ms");
							}
							draw_checkbox(crypt_str("Client bullet impacts"), &g_cfg.esp.client_bullet_impacts);
							if (g_cfg.esp.client_bullet_impacts)
							{
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##clientbulletimpacts"), &g_cfg.esp.client_bullet_impacts_color, ALPHA);
							}
							draw_checkbox(crypt_str("Server bullet impacts"), &g_cfg.esp.server_bullet_impacts);
							if (g_cfg.esp.server_bullet_impacts)
							{
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##serverbulletimpacts"), &g_cfg.esp.server_bullet_impacts_color, ALPHA);
							}
							draw_checkbox(crypt_str("Local bullet tracers"), &g_cfg.esp.bullet_tracer);
							if (g_cfg.esp.bullet_tracer)
							{
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##bulltracecolor"), &g_cfg.esp.bullet_tracer_color, ALPHA);
							}
							draw_checkbox(crypt_str("Enemy bullet tracers"), &g_cfg.esp.enemy_bullet_tracer);
							if (g_cfg.esp.enemy_bullet_tracer)
							{
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##enemybulltracecolor"), &g_cfg.esp.enemy_bullet_tracer_color, ALPHA);
							}
						}break;
						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();

					ImGui::SetCursorPos({ 390, 45 });

					ImGui::BeginChild("Visuals2", ImVec2(240, 395), false);
					{child_start(second_name[vis_subtab]);
					ImGui::PushItemWidth(210);

					const char* category_char[] = {
						"Enemy",
						"Team",
						"Local"
					};
					ImGui::PushFont(c_menu::get().new_font);
					if (!vis_subtab) {
						ImGui::SetCursorPos(ImVec2(0, 40));
						ImGui::PushItemWidth(120);
						ImGui::Combo("Type", &category, category_char, 3);
						ImGui::PopItemWidth();
					}
					switch (vis_subtab)
					{
					case 0:
					{if (true)
					{
						if (category == LOCAL) {
							draw_combo(crypt_str("Type"), g_cfg.player.local_chams_type, local_chams_type, ARRAYSIZE(local_chams_type));
						}
						if (category == LOCAL && g_cfg.player.local_chams_type == 2)
						{
							draw_checkbox(crypt_str("Arms chams"), &g_cfg.esp.arms_chams);
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##armscolor"), &g_cfg.esp.arms_chams_color, ALPHA);

							draw_combo(crypt_str("Material"), g_cfg.esp.arms_chams_type, chamstype, ARRAYSIZE(chamstype));

						}
						if (category == LOCAL && g_cfg.player.local_chams_type == 3)
						{

							draw_checkbox(crypt_str("Weapon chams"), &g_cfg.esp.weapon_chams);
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##weaponchamscolors"), &g_cfg.esp.weapon_chams_color, ALPHA);
							draw_combo(crypt_str("Weapon chams material"), g_cfg.esp.weapon_chams_type, chamstype, ARRAYSIZE(chamstype));

						}
						if (category != LOCAL || !g_cfg.player.local_chams_type) {
							draw_multicombo(crypt_str("Chams"), g_cfg.player.type[category].chams, chamsvisact, ARRAYSIZE(chamsvisact), preview);
						}
						if (g_cfg.player.type[category].chams[PLAYER_CHAMS_VISIBLE] || category == LOCAL && g_cfg.player.local_chams_type) //-V648
						{
							if (category == LOCAL && g_cfg.player.local_chams_type == 1)
							{
								draw_checkbox(crypt_str("Enable desync chams"), &g_cfg.player.fake_chams_enable);
								draw_checkbox(crypt_str("Show lag"), &g_cfg.player.visualize_lag);
								draw_checkbox(crypt_str("Layered"), &g_cfg.player.layered);

								draw_combo(crypt_str("Material desync"), g_cfg.player.fake_chams_type, chamstype, ARRAYSIZE(chamstype));

								ImGui::Text(crypt_str("Color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##fakechamscolor"), &g_cfg.player.fake_chams_color, ALPHA);
	
							}
							else
							{
								draw_combo(crypt_str("Material"), g_cfg.player.type[category].chams_type, chamstype, ARRAYSIZE(chamstype));
	
								if (g_cfg.player.type[category].chams[PLAYER_CHAMS_VISIBLE])
								{
									ImGui::Text(crypt_str("Visible "));
									ImGui::SameLine(210);
									ImGui::ColorEdit(crypt_str("##chamsvisible"), &g_cfg.player.type[category].chams_color, ALPHA);
								}

								if (g_cfg.player.type[category].chams[PLAYER_CHAMS_INVISIBLE])
								{
									ImGui::Text(crypt_str("Hidden "));
									ImGui::SameLine(210);
									ImGui::ColorEdit(crypt_str("##chamsinvisible"), &g_cfg.player.type[category].xqz_color, ALPHA);
								}


								if (category == ENEMY)
								{
									draw_checkbox(crypt_str("Backtrack chams"), &g_cfg.player.backtrack_chams);

									if (g_cfg.player.backtrack_chams)
									{
										draw_combo(crypt_str("Material bactrack"), g_cfg.player.backtrack_chams_material, chamstype, ARRAYSIZE(chamstype));
										ImGui::Text(crypt_str("Color "));
										ImGui::SameLine(210);
										ImGui::ColorEdit(crypt_str("##backtrackcolor"), &g_cfg.player.backtrack_chams_color, ALPHA);
									}
								}
							}
						}

						if (category == ENEMY)
						{

							draw_checkbox(crypt_str("Shot record"), &g_cfg.player.lag_hitbox);
							if (g_cfg.player.lag_hitbox)
							{
								draw_combo(crypt_str("Shot chams material"), g_cfg.player.lag_hitbox_chams, chamstype, 4);
								ImGui::Text(crypt_str("Color "));
								ImGui::SameLine(210);
								ImGui::ColorEdit(crypt_str("##hitcolor"), &g_cfg.player.lag_hitbox_color, ALPHA);
							}
						}
						else if (!g_cfg.player.local_chams_type)
						{

							draw_checkbox(crypt_str("Transparency in scope"), &g_cfg.player.transparency_in_scope);
							if (g_cfg.player.transparency_in_scope)
								ImGui::SliderFloat(crypt_str("Alpha"), &g_cfg.player.transparency_in_scope_amount, 0.0f, 1.0f);

						}

						draw_checkbox(crypt_str("Glow"), &g_cfg.player.type[category].glow);
						if (g_cfg.player.type[category].glow)
						{

							draw_color(crypt_str("Color"), &g_cfg.player.type[category].glow_color, ALPHA);
						}
					}}break;
					case 1:
					{

						draw_checkbox(crypt_str("Full bright"), &g_cfg.esp.bright);

						draw_combo(crypt_str("Skybox"), g_cfg.esp.skybox, skybox, ARRAYSIZE(skybox));

						if (g_cfg.esp.skybox == 21)
						{
							static char sky_custom[64] = "\0";

							if (!g_cfg.esp.custom_skybox.empty())
								strcpy_s(sky_custom, sizeof(sky_custom), g_cfg.esp.custom_skybox.c_str());

							ImGui::Text(crypt_str("Enter the sky name"));
							ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

							if (ImGui::InputText(crypt_str("##customsky"), sky_custom, sizeof(sky_custom)))
								g_cfg.esp.custom_skybox = sky_custom;

							ImGui::PopStyleVar();
						}

						draw_checkbox(crypt_str("Night-mode"), &g_cfg.esp.nightmode);
						if (g_cfg.esp.nightmode)
						{

							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##worldcolor"), &g_cfg.esp.world_color, ALPHA);
							ImGui::SliderFloat(crypt_str("Brightness"), &g_cfg.esp.exposure, 0.0f, 2000.0f);
							ImGui::Text(crypt_str("Skybox color "));
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##skyboxcolor"), &g_cfg.esp.skybox_color, NOALPHA);
						}

						draw_checkbox(crypt_str("World modulation"), &g_cfg.esp.world_modulation);

						if (g_cfg.esp.world_modulation)
						{
							ImGui::SliderFloat(crypt_str("Bloom"), &g_cfg.esp.bloom, 0.0f, 750.0f);
							ImGui::SliderFloat(crypt_str("Ambient"), &g_cfg.esp.ambient, 0.0f, 1500.0f);
						}

						draw_checkbox(crypt_str("Fog modulation"), &g_cfg.esp.fog);
						if (g_cfg.esp.fog)
						{
							ImGui::SliderInt(crypt_str("Distance"), &g_cfg.esp.fog_distance, 0, 2500);
							ImGui::SliderInt(crypt_str("Density"), &g_cfg.esp.fog_density, 0, 100);
							ImGui::Text(crypt_str("Color "));
							ImGui::SameLine(210);
							ImGui::ColorEdit(crypt_str("##fogcolor"), &g_cfg.esp.fog_color, NOALPHA);
						}}break;
					case 2:
					{
						draw_checkbox(crypt_str("Rare weapon animations"), &g_cfg.skins.rare_animations);
						ImGui::SliderInt(crypt_str("Viewmodel FOV"), &g_cfg.esp.viewmodel_fov, 0, 89);
						ImGui::SliderInt(crypt_str("Viewmodel X"), &g_cfg.esp.viewmodel_x, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Y"), &g_cfg.esp.viewmodel_y, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel Z"), &g_cfg.esp.viewmodel_z, -50, 50);
						ImGui::SliderInt(crypt_str("Viewmodel roll"), &g_cfg.esp.viewmodel_roll, -180, 180);
					}break;
					case 3:
					{
						draw_checkbox(crypt_str("Grenade prediction"), &g_cfg.esp.grenade_prediction);

						if (g_cfg.esp.grenade_prediction) {
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##predictioncolor"), &g_cfg.esp.grenade_prediction_color, ALPHA);
							draw_checkbox(crypt_str("On click"), &g_cfg.esp.on_click);
						}

						draw_checkbox(crypt_str("Grenade warning"), &g_cfg.esp.grenade_proximity_warning);

						if (g_cfg.esp.grenade_proximity_warning)
						{
							draw_checkbox(crypt_str("Offscreen warning"), &g_cfg.esp.offscreen_proximity);

							ImGui::Text(crypt_str("Tracer color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color51235"), &g_cfg.esp.grenade_proximity_tracers_colors, ALPHA);

							ImGui::Text(crypt_str("Warning color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color_warning#12"), &g_cfg.esp.grenade_proximity_warning_progress_color, ALPHA);

							ImGui::Text(crypt_str("Warning circle color"));
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##grenade_color_warning#51"), &g_cfg.esp.grenade_proximity_warning_inner_color, ALPHA);
						}

						draw_checkbox(crypt_str("Molotov timer"), &g_cfg.esp.molotov_timer);

						if (g_cfg.esp.molotov_timer)
						{
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##firecolor"), &g_cfg.esp.molotov_timer_color, ALPHA);
						}

						draw_checkbox(crypt_str("Smoke timer"), &g_cfg.esp.smoke_timer);

						if (g_cfg.esp.smoke_timer)
						{
							ImGui::SameLine();
							ImGui::ColorEdit(crypt_str("##smokecolor"), &g_cfg.esp.smoke_timer_color, ALPHA);
						}
					}break;
					}
					ImGui::PopItemWidth();

					}
					ImGui::EndChild();
					ImGui::PopFont();
				}break;
				case 3://skins old
				{
					//static bool drugs = false;
					//static bool active_animation = false;
					//static bool preview_reverse = false;
					//static float switch_alpha = 1.f;
					//static int next_id = -1;
					//static int def_alp = 255;
					//static bool full_ready = false;
					//static int anim_speed = 5;
					ImGui::PushFont(c_menu::get().new_font);
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("skins", ImVec2(500, 395), false);
					{
					//	static bool leave;
					//	if (active_animation)
					//	{
					//		if (preview_reverse)
					//		{
					//			if (switch_alpha == 1.f) //-V550
					//			{
					//				preview_reverse = false;
					//				active_animation = false;
					//			}

					//			switch_alpha = math::clamp(switch_alpha + (4.f * ImGui::GetIO().DeltaTime), 0.01f, 1.f);
					//		}
					//		else
					//		{
					//			if (switch_alpha == 0.01f) //-V550
					//			{
					//				preview_reverse = true;
					//			}

					//			switch_alpha = math::clamp(switch_alpha - (4.f * ImGui::GetIO().DeltaTime), 0.01f, 1.f);
					//		}
					//	}
					//	else
					//		switch_alpha = math::clamp(switch_alpha + (4.f * ImGui::GetIO().DeltaTime), 0.0f, 1.f);

					//	// we need to count our items in 1 line
					//	auto same_line_counter = 0;
					//	static bool models[2];
					//	// if we didnt choose any weapon
					//	if (current_profile == -1)
					//	{
					//		for (auto i = 0; i < g_cfg.skins.skinChanger.size(); i++)
					//		{
					//			// do we need update our preview for some reasons?
					//			if (!all_skins[i])
					//			{
					//				g_cfg.skins.skinChanger.at(i).update();
					//				all_skins[i] = get_skin_preview(get_wep(i, (i == 0 || i == 1) ? g_cfg.skins.skinChanger.at(i).definition_override_vector_index : -1, i == 0).c_str(), g_cfg.skins.skinChanger.at(i).skin_name, device); //-V810
					//			}

					//			// we licked on weapon
					//			if (ImGui::ImageButton(all_skins[i], ImVec2(85, 55)))
					//			{
					//				next_id = i;
					//				active_animation = true;
					//			}



					//			// if our animation step is half from all - switch profile
					//			if (active_animation && preview_reverse)
					//			{
					//				ImGui::SetScrollY(0);
					//				current_profile = next_id;
					//			}

					//			if (same_line_counter < 4) { // continue push same-line
					//				ImGui::SameLine();
					//				same_line_counter++;
					//			}
					//			else { // we have maximum elements in 1 line
					//				same_line_counter = 0;
					//			}
					//		}
							ImGui::Spacing();
							draw_combo(crypt_str("Model T"), g_cfg.player.player_model_t, player_model_t, ARRAYSIZE(player_model_t));
							ImGui::Spacing();
							draw_combo(crypt_str("Model CT"), g_cfg.player.player_model_ct, player_model_ct, ARRAYSIZE(player_model_ct));
						//}
						//else
						//{
						//	// update skin preview bool
						//	static bool need_update[36];

						//	// we pressed crypt_str("Save & Close") button


						//	// update if we have nullptr texture or if we push force update
						//	if (!all_skins[current_profile] || need_update[current_profile])
						//	{
						//		all_skins[current_profile] = get_skin_preview(get_wep(current_profile, (current_profile == 0 || current_profile == 1) ? g_cfg.skins.skinChanger.at(current_profile).definition_override_vector_index : -1, current_profile == 0).c_str(), g_cfg.skins.skinChanger.at(current_profile).skin_name, device); //-V810
						//		need_update[current_profile] = false;
						//	}

						//	// get settings for selected weapon
						//	auto& selected_entry = g_cfg.skins.skinChanger[current_profile];
						//	selected_entry.itemIdIndex = current_profile;

						//	ImGui::BeginGroup();
						//	ImGui::PushItemWidth(220 * 1);

						//	// search input later
						//	static char search_skins[64] = "\0";
						//	static auto item_index = selected_entry.paint_kit_vector_index;

						//	if (!current_profile)
						//	{
						//		ImGui::Text(crypt_str("Knife"));
						//		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
						//		if (ImGui::Combo(crypt_str("##Knife_combo"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
						//			{
						//				*out_text = game_data::knife_names[idx].name;
						//				return true;
						//			}, nullptr, IM_ARRAYSIZE(game_data::knife_names)))
						//			need_update[current_profile] = true; // push force update
						//	}
						//	else if (current_profile == 1)
						//	{
						//		ImGui::Text(crypt_str("Gloves"));
						//		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
						//		if (ImGui::Combo(crypt_str("##Glove_combo"), &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
						//			{
						//				*out_text = game_data::glove_names[idx].name;
						//				return true;
						//			}, nullptr, IM_ARRAYSIZE(game_data::glove_names)))
						//		{
						//			item_index = 0; // set new generated paintkits element to 0;
						//			need_update[current_profile] = true; // push force update
						//		}
						//	}
						//	else
						//		selected_entry.definition_override_vector_index = 0;

						//	if (current_profile != 1)
						//	{
						//		ImGui::Text(crypt_str("Search"));
						//		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

						//		if (ImGui::InputText(crypt_str("##search"), search_skins, sizeof(search_skins)))
						//			item_index = -1;

						//		ImGui::PopStyleVar();
						//	}

						//	auto main_kits = current_profile == 1 ? SkinChanger::gloveKits : SkinChanger::skinKits;
						//	auto display_index = 0;

						//	SkinChanger::displayKits = main_kits;

						//	// we dont need custom gloves
						//	if (current_profile == 1)
						//	{
						//		for (auto i = 0; i < main_kits.size(); i++)
						//		{
						//			auto main_name = main_kits.at(i).name;

						//			for (auto i = 0; i < main_name.size(); i++)
						//				if (iswalpha((main_name.at(i))))
						//					main_name.at(i) = towlower(main_name.at(i));

						//			char search_name[64];

						//			if (!strcmp(game_data::glove_names[selected_entry.definition_override_vector_index].name, crypt_str("Hydra")))
						//				strcpy_s(search_name, sizeof(search_name), crypt_str("Bloodhound"));
						//			else
						//				strcpy_s(search_name, sizeof(search_name), game_data::glove_names[selected_entry.definition_override_vector_index].name);

						//			for (auto i = 0; i < sizeof(search_name); i++)
						//				if (iswalpha(search_name[i]))
						//					search_name[i] = towlower(search_name[i]);

						//			if (main_name.find(search_name) != std::string::npos)
						//			{
						//				SkinChanger::displayKits.at(display_index) = main_kits.at(i);
						//				display_index++;
						//			}
						//		}

						//		SkinChanger::displayKits.erase(SkinChanger::displayKits.begin() + display_index, SkinChanger::displayKits.end());
						//	}
						//	else
						//	{
						//		if (strcmp(search_skins, crypt_str(""))) //-V526
						//		{
						//			for (auto i = 0; i < main_kits.size(); i++)
						//			{
						//				auto main_name = main_kits.at(i).name;

						//				for (auto i = 0; i < main_name.size(); i++)
						//					if (iswalpha(main_name.at(i)))
						//						main_name.at(i) = towlower(main_name.at(i));

						//				char search_name[64];
						//				strcpy_s(search_name, sizeof(search_name), search_skins);

						//				for (auto i = 0; i < sizeof(search_name); i++)
						//					if (iswalpha(search_name[i]))
						//						search_name[i] = towlower(search_name[i]);

						//				if (main_name.find(search_name) != std::string::npos)
						//				{
						//					SkinChanger::displayKits.at(display_index) = main_kits.at(i);
						//					display_index++;
						//				}
						//			}

						//			SkinChanger::displayKits.erase(SkinChanger::displayKits.begin() + display_index, SkinChanger::displayKits.end());
						//		}
						//		else
						//			item_index = selected_entry.paint_kit_vector_index;
						//	}

						//	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
						//	if (!SkinChanger::displayKits.empty())
						//	{
						//		if (ImGui::ListBox(crypt_str("##PAINTKITS"), &item_index, [](void* data, int idx, const char** out_text) //-V107
						//			{
						//				while (SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")) != std::string::npos) //-V807
						//					SkinChanger::displayKits.at(idx).name.replace(SkinChanger::displayKits.at(idx).name.find(crypt_str("С‘")), 2, crypt_str("Рµ"));

						//				*out_text = SkinChanger::displayKits.at(idx).name.c_str();
						//				return true;
						//			}, nullptr, SkinChanger::displayKits.size(), SkinChanger::displayKits.size() > 9 ? 9 : SkinChanger::displayKits.size()) || !all_skins[current_profile])
						//		{
						//			SkinChanger::scheduleHudUpdate();
						//			need_update[current_profile] = true;

						//			auto i = 0;

						//			while (i < main_kits.size())
						//			{
						//				if (main_kits.at(i).id == SkinChanger::displayKits.at(item_index).id)
						//				{
						//					selected_entry.paint_kit_vector_index = i;
						//					break;
						//				}

						//				i++;
						//			}

						//		}
						//	}
						//	ImGui::PopStyleVar();

						//	if (ImGui::InputInt(crypt_str("Seed"), &selected_entry.seed, 1, 100))
						//		SkinChanger::scheduleHudUpdate();

						//	if (ImGui::InputInt(crypt_str("StatTrak"), &selected_entry.stat_trak, 1, 15))
						//		SkinChanger::scheduleHudUpdate();

						//	if (ImGui::SliderFloat(crypt_str("Wear"), &selected_entry.wear, 0.0f, 1.0f))
						//		drugs = true;
						//	else if (drugs)
						//	{
						//		SkinChanger::scheduleHudUpdate();
						//		drugs = false;
						//	}

						//	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6 * c_menu::get().dpi_scale);
						//	ImGui::Text(crypt_str("Quality"));
						//	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * c_menu::get().dpi_scale);
						//	if (ImGui::Combo(crypt_str("##Quality_combo"), &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text)
						//		{
						//			*out_text = game_data::quality_names[idx].name;
						//			return true;
						//		}, nullptr, IM_ARRAYSIZE(game_data::quality_names)))
						//		SkinChanger::scheduleHudUpdate();

						//		if (current_profile != 1)
						//		{
						//			if (!g_cfg.skins.custom_name_tag[current_profile].empty())
						//				strcpy_s(selected_entry.custom_name, sizeof(selected_entry.custom_name), g_cfg.skins.custom_name_tag[current_profile].c_str());

						//			ImGui::Text(crypt_str("Name Tag"));
						//			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

						//			if (ImGui::InputText(crypt_str("##nametag"), selected_entry.custom_name, sizeof(selected_entry.custom_name)))
						//			{
						//				g_cfg.skins.custom_name_tag[current_profile] = selected_entry.custom_name;
						//				SkinChanger::scheduleHudUpdate();
						//			}

						//			ImGui::PopStyleVar();
						//		}

						//		ImGui::PopItemWidth();

						//		ImGui::EndGroup();

						//		ImGui::SameLine();


						//		ImGui::BeginGroup();
						//		if (ImGui::ImageButton(all_skins[current_profile], ImVec2(200 * 1, 180 * 1)))
						//		{
						//			// maybe i will do smth later where, who knows :/
						//		}

						//		if (ImGui::CustomButton(crypt_str("Close"), crypt_str("##CLOSE__SKING"), ImVec2(200 * 1, 26 * 1)))
						//		{
						//			// start animation
						//			active_animation = true;
						//			next_id = -1;
						//			leave = true;
						//		}
						//		ImGui::EndGroup();

						//		// update element
						//		selected_entry.update();

						//		// we need to reset profile in the end to prevent render images with massive's index == -1
						//		if (leave && (preview_reverse || !active_animation))
						//		{
						//			ImGui::SetScrollY(0);
						//			current_profile = next_id;
						//			leave = false;
						//		}


						//}
						ImGui::EndChild();
					}
					//ImGui::EndChild();
					ImGui::PopFont();
				}break;
				/*case 94://skins new
				{
					static int index = 0;
					static wskin weaponSkin;
					auto same_line_counter = 0;
					auto same_line_counter2 = 0;
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("skins", ImVec2(500, 395), false);
					{
						// we need to count our items in 1 line
						static bool prev = false;
						if (!prev) {
							ImGui::BeginGroup();

							if (weaponSkin.wId == WEAPON_NONE)
								weaponSkin.wId = WEAPON_DEAGLE;

							for (const auto& weapon : k_inventory_names)
							{
								if (ImGui::Button(weapon.second, ImVec2(87, 76)))
								{
									weaponSkin.wId = weapon.first;

									weaponSkin.paintKit = 0;
									prev = true;
								}
								if (same_line_counter < 4) { // continue push same-line
									ImGui::SameLine();
									same_line_counter++;
								}
								else
									same_line_counter = 0;

							}

							ImGui::EndGroup();


							static int paintKit = 874;

							ImGui::Text("Medal skin");
							if (ImGui::BeginCombo("##PaintKit_yea", fosso[paintKit].name.c_str()))
							{
								int lastID = ImGui::GetItemID();

								for (auto skin : fosso)
								{
									{
										//ImGui::PushID(lastID++);
										if (ImGui::CustomSelectable(skin.second.name.c_str(), paintKit == skin.second.paintKit, 0, ImVec2(87, 76)))
											paintKit = skin.second.paintKit;
										//ImGui::PopID();
									}

								}
								ImGui::EndCombo();
							}
							if (ImGui::CustomButton("Add medal", "##ass300%", ImVec2(ImGui::GetWindowSize().x - 39, 26)))
								g_MedalSkins[math::random_int(200001, 1000000)] = { paintKit , 0 };



							if (ImGui::CustomButton("Apply medals", "##ass3s00%", ImVec2(ImGui::GetWindowSize().x - 39, 26)))
							{
								paintKit;
								m_engine()->ExecuteClientCmd("econ_clear_inventory_images");
								write.SendClientHello();
								write.SendMatchmakingClient2GCHello();
							}


						}
						if (prev) {
							auto weaponName = zweaponnames(weaponSkin.wId);
							ImGui::Text("Skin");

							if (ImGui::BeginCombo("##Paint Kit", weaponSkin.paintKit > 0 ? _inv.inventory.skinInfo[weaponSkin.paintKit].name.c_str() : "None"))
							{
								int lastID = ImGui::GetItemID();

								for (auto skin : _inv.inventory.skinInfo)
								{
									for (auto names : skin.second.weaponName)
									{
										if (weaponName != names)
											continue;

										ImGui::PushID(lastID++);

										if (ImGui::CustomSelectable(skin.second.name.c_str(), weaponSkin.paintKit == skin.first, 0, ImVec2()))
											weaponSkin.paintKit = skin.first;

										ImGui::PopID();
									}
								}
								ImGui::EndCombo();
							}


							ImGui::SliderFloat("Wear", &weaponSkin.wear, 0.f, 1.f);
							ImGui::SliderInt("Seed", &weaponSkin.seed, 0, 100);

							static const char* t[] = { "common","uncommon","rare","mythical","legendary","ancient","immortal" };
							//draw_combo("Rarity", weaponSkin.quality, t, ARRAYSIZE(t));

							static char skinname[64] = "\0";

							ImGui::InputText(("##skinname"), skinname, sizeof(skinname));
							ImGui::InputInt("Stattrak", &weaponSkin.stattrak);



							static int stickerkit[4] = { 0,0,0,0 };


							if (weaponSkin.wId <= 100 && weaponSkin.wId != 42 && weaponSkin.wId != 59)
							{

								if (ImGui::BeginCombo("Sticker 1", g_Stickers[stickerkit[0]].name.c_str()))
								{
									int lastID = ImGui::GetItemID();

									for (auto skin : fosso)
									{
										{
											ImGui::PushID(lastID++);
											if (ImGui::CustomSelectable(skin.second.name.c_str(), skin.second.paintKit == stickerkit[0], 0, ImVec2()))
												stickerkit[0] = skin.second.paintKit;
											ImGui::PopID();
										}
									}
									ImGui::EndCombo();
								}

								if (ImGui::BeginCombo("Sticker 2", g_Stickers[stickerkit[1]].name.c_str()))
								{
									int lastID = ImGui::GetItemID();

									for (auto skin : fosso)
									{
										{
											ImGui::PushID(lastID++);
											if (ImGui::CustomSelectable(skin.second.name.c_str(), skin.second.paintKit == stickerkit[1], 0, ImVec2()))
												stickerkit[1] = skin.second.paintKit;
											ImGui::PopID();
										}
									}
									ImGui::EndCombo();
								}

								if (ImGui::BeginCombo("Sticker 3", g_Stickers[stickerkit[2]].name.c_str()))
								{
									int lastID = ImGui::GetItemID();

									for (auto skin : fosso)
									{
										{
											ImGui::PushID(lastID++);
											if (ImGui::CustomSelectable(skin.second.name.c_str(), skin.second.paintKit == stickerkit[2], 0, ImVec2()))
												stickerkit[2] = skin.second.paintKit;
											ImGui::PopID();
										}
									}
									ImGui::EndCombo();
								}

								if (ImGui::BeginCombo("Sticker 4", g_Stickers[stickerkit[3]].name.c_str()))
								{
									int lastID = ImGui::GetItemID();

									for (auto skin : fosso)
									{
										{
											ImGui::PushID(lastID++);
											if (ImGui::CustomSelectable(skin.second.name.c_str(), skin.second.paintKit == stickerkit[3], 0, ImVec2()))
												stickerkit[3] = skin.second.paintKit;
											ImGui::PopID();
										}
									}
									ImGui::EndCombo();
								}


							}



							if (ImGui::CustomButton("Add", "##fuckyouasshole1", ImVec2(ImGui::GetWindowSize().x - 39, 26)))
							{
								weaponSkin.sicker[0] = stickerkit[0];
								weaponSkin.sicker[1] = stickerkit[1];
								weaponSkin.sicker[2] = stickerkit[2];
								weaponSkin.sicker[3] = stickerkit[3];

								std::string str(skinname);
								if (str.length() > 0)
									weaponSkin.name = str;
								g_InventorySkins.insert({ math::random_int(20000, 200000), weaponSkin });
								_inv.inventory.itemCount = g_InventorySkins.size();
								stickerkit[0] = 0;
								stickerkit[1] = 0;
								stickerkit[2] = 0;
								stickerkit[3] = 0;
								//index = 0;

								m_engine()->ExecuteClientCmd("econ_clear_inventory_images");
								if (!m_engine()->IsConnected())
									write.SendClientHello();
								write.SendMatchmakingClient2GCHello();
							}
							if (ImGui::CustomButton("back", "##fuckyouasshole2", ImVec2(ImGui::GetWindowSize().x - 39, 26)))
							{
								prev = false;
							}
							if (g_InventorySkins.size() > 0)
							{
								if (ImGui::CustomButton("Delete selected element", "##fuckyouasshole3", ImVec2(ImGui::GetWindowSize().x - 39, 26)))
								{
									// if (g_InventorySkins[index] != NULL)
									g_InventorySkins.erase(index);
									_inv.inventory.itemCount = g_InventorySkins.size();
								}
							}
						}
					}
					ImGui::EndChild();
				}break;*/
				case 4://misc
				{
					ImGui::PushFont(c_menu::get().new_font);
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("Misc", ImVec2(240, 395), false);
					{
						child_start("Movement");
						ImGui::PushItemWidth(210);
						draw_checkbox(crypt_str("Anti-untrusted"), &g_cfg.misc.anti_untrusted);
						draw_checkbox(crypt_str("Bunny-Hop"), &g_cfg.misc.bunnyhop);
						draw_checkbox(crypt_str("Auto strafe"), &g_cfg.misc.airstrafe);
						draw_checkbox(crypt_str("Crouch in air"), &g_cfg.misc.crouch_in_air);
						draw_checkbox(crypt_str("Fast-stop"), &g_cfg.misc.fast_stop);
						draw_checkbox(crypt_str("Slide-walk"), &g_cfg.misc.slidewalk);
						draw_checkbox(crypt_str("No duck cooldown"), &g_cfg.misc.noduck);
						draw_keybind(crypt_str("Auto-peek"), &g_cfg.misc.automatic_peek, crypt_str("##AUTOPEEK__HOTKEY"));
						//draw_color(crypt_str("Color"), &g_cfg.misc.automatic_peek_color, ALPHA);
						draw_keybind(crypt_str("Edge-jump"), &g_cfg.misc.edge_jump, crypt_str("##EDGEJUMP__HOTKEY"));
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();

					ImGui::SetCursorPos({ 390, 45 });

					ImGui::BeginChild("Misc2", ImVec2(240, 395), false);
					{
						child_start("Misc");
						ImGui::PushItemWidth(210);
						draw_multicombo(crypt_str("Events to log"), g_cfg.misc.events_to_log, events, ARRAYSIZE(events), preview);
						draw_checkbox(crypt_str("Logs output"), &g_cfg.misc.log_output);
						draw_color(crypt_str("Color"), &g_cfg.misc.log_color, ALPHA);
						draw_checkbox(crypt_str("Preserve killfeed"), &g_cfg.esp.preserve_killfeed);
						//draw_checkbox(crypt_str("Clantag spammer"), &g_cfg.misc.clantag_spammer);
						draw_checkbox(crypt_str("Unlock inventory access"), &g_cfg.misc.inventory_access);
						draw_checkbox(crypt_str("Gravity ragdolls"), &g_cfg.misc.ragdolls);
						draw_checkbox(crypt_str("Enable buybot"), &g_cfg.misc.buybot_enable);
						if (g_cfg.misc.buybot_enable)
						{
							draw_combo(crypt_str("Main"), g_cfg.misc.buybot1, mainwep, ARRAYSIZE(mainwep));
							draw_combo(crypt_str("Second"), g_cfg.misc.buybot2, secwep, ARRAYSIZE(secwep));
							draw_multicombo(crypt_str("Other"), g_cfg.misc.buybot3, grenades, ARRAYSIZE(grenades), preview);
						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();
					ImGui::PopFont();
				}break;
				case 5://configs
				{
					bool update = false;
					if (tab == 8)
						update = true;
					if (update)
					{
						cfg_manager->config_files();
						files = cfg_manager->files;

						for (auto& current : files)
							if (current.size() > 2)
								current.erase(current.size() - 3, 3);
					}
					ImGui::PushFont(c_menu::get().new_font);
					ImGui::SetCursorPos({ 145, 45 });
					ImGui::BeginChild("Menu", ImVec2(240, 395), false);
					{
						child_start("Configs");
						{
							ImGui::SetCursorPosX(5);
							if (ImGui::CustomButton(crypt_str("Refresh"), crypt_str("##CONFIG__REFRESH"), ImVec2(230, 26 * 1)))
							{
								cfg_manager->config_files();
								files = cfg_manager->files;

								for (auto& current : files)
									if (current.size() > 2)
										current.erase(current.size() - 3, 3);
							}
							static char config_name[64] = "\0";
							ImGui::SetCursorPosX(4);
							ImGui::PushItemWidth(230);
							ImGui::ListBoxConfigArray(crypt_str("##CONFIGS"), &g_cfg.selected_config, files, 6);
							ImGui::PopItemWidth();
							ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
							ImGui::SetCursorPosX(4);
							ImGui::PushItemWidth(230);
							ImGui::InputText(crypt_str("##configname"), config_name, sizeof(config_name));
							ImGui::PopItemWidth();
							ImGui::PopStyleVar();
							ImGui::SetCursorPosX(5);
							if (ImGui::CustomButton(crypt_str("Create"), crypt_str("##CONFIG__CREATE"), ImVec2(230 * 1, 26 * 1)))
							{
								g_cfg.new_config_name = config_name;
								add_config();
							}
							static auto next_save = false;
							static auto prenext_save = false;
							static auto clicked_sure = false;
							static auto save_time = m_globals()->m_realtime;
							static auto save_alpha = 1.0f;

							save_alpha = math::clamp(save_alpha + (4.f * ImGui::GetIO().DeltaTime * (!prenext_save ? 1.f : -1.f)), 0.01f, 1.f);


							if (!next_save)
							{
								ImGui::SetCursorPosX(5);
								if (ImGui::CustomButton(crypt_str("Save"), crypt_str("##CONFIG__SAVE"), ImVec2(230 * 1, 26 * 1)))
								{
									save_config();
									save_time = m_globals()->m_realtime;
									prenext_save = true;
								}
							}
							//else
							//{
							//	if (prenext_save && save_alpha <= 0.01f)
							//	{
							//		prenext_save = false;
							//		next_save = !clicked_sure;
							//	}
							//	ImGui::SetCursorPosX(5);
							//	if (ImGui::CustomButton(crypt_str("Click"), crypt_str("##AREYOUSURE__SAVE"), ImVec2(230 * 1, 26 * 1)))
							//	{
							//		save_config();
							//		prenext_save = true;
							//		clicked_sure = true;
							//	}

							//	if (!clicked_sure && m_globals()->m_realtime > save_time + 1.5f)
							//	{
							//		prenext_save = true;
							//		clicked_sure = true;
							//	}
							//}

							ImGui::SetCursorPosX(5);
							if (ImGui::CustomButton(crypt_str("Load"), crypt_str("##CONFIG__LOAD"), ImVec2(230 * 1, 26 * 1)))
								load_config();

							static auto next_delete = false;
							static auto prenext_delete = false;
							static auto clicked_sure_del = false;
							static auto delete_time = m_globals()->m_realtime;
							static auto delete_alpha = 1.0f;

							delete_alpha = math::clamp(delete_alpha + (4.f * ImGui::GetIO().DeltaTime * (!prenext_delete ? 1.f : -1.f)), 0.01f, 1.f);

							if (!next_delete)
							{
								ImGui::SetCursorPosX(5);
								if (ImGui::CustomButton(crypt_str("Delete"), crypt_str("##CONFIG__DELETE"), ImVec2(230 * 1, 26 * 1)))
								{
									delete_time = m_globals()->m_realtime;
									prenext_delete = true;
									remove_config();
								}
							}
						}
					}
					ImGui::EndChild();

					ImGui::SetCursorPos({ 390, 45 });
					ImGui::BeginChild("Menu2", ImVec2(240, 395), false);
					{
						child_start("Windows");
						draw_color(crypt_str("Menu Color"), &g_cfg.menu.menu_theme, ALPHA);
						draw_checkbox(crypt_str("Watermark"), &g_cfg.menu.watermark);
						draw_checkbox(crypt_str("Keystates"), &g_cfg.menu.keybinds);
						draw_checkbox(crypt_str("Spectators"), &g_cfg.menu.spectators_list);

						static bool debug = false;
						//draw_checkbox("Debug", &debug); ImGui::SameLine();
						ImGui::PushFont(c_menu::ico_menu);
						//if (ImGui::Button("n"))
						//	ImGui::OpenPopup("deb");
						ImGui::PopFont();
					}
					ImGui::EndChild();
					ImGui::PopFont();
				}break;
				//case 7://scripts
				//{
				//	ImGui::PushFont(c_menu::get().new_font);
				//	ImGui::SetCursorPos({ 145, 45 });
				//	ImGui::BeginChild("Scripts", ImVec2(240, 395), false);
				//	{
				//		child_start("Scripts");
				//		ImGui::PushItemWidth(230);
				//		static auto should_update = true;

				//		if (should_update)
				//		{
				//			should_update = false;
				//			scripts = c_lua::get().scripts;

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}
				//		}
				//		/*ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Open scripts folder"), crypt_str("##LUAS__FOLDER"), ImVec2(230, 26)))
				//		{
				//			std::string folder;

				//			auto get_dir = [&folder]() -> void
				//			{
				//				static TCHAR path[MAX_PATH];

				//				if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, NULL, path)))
				//					folder = std::string(path) + crypt_str("\\twotick\\Scripts\\");

				//				CreateDirectory(folder.c_str(), NULL);
				//			};

				//			get_dir();
				//			ShellExecute(NULL, crypt_str("open"), folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
				//		}*/

				//		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
				//		ImGui::SetCursorPosX(5);
				//		if (scripts.empty())
				//			ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
				//		else
				//		{
				//			auto backup_scripts = scripts;

				//			for (auto& script : scripts)
				//			{
				//				auto script_id = c_lua::get().get_script_id(script + crypt_str(".lua"));

				//				if (script_id == -1)
				//					continue;

				//				if (c_lua::get().loaded.at(script_id))
				//					scripts.at(script_id) += crypt_str(" [loaded]");
				//			}

				//			ImGui::ListBoxConfigArray(crypt_str("##LUAS"), &selected_script, scripts, 7);
				//			scripts = std::move(backup_scripts);
				//		}

				//		ImGui::PopStyleVar();
				//		ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Refresh scripts"), crypt_str("##LUA__REFRESH"), ImVec2(230, 26)))
				//		{
				//			c_lua::get().refresh_scripts();
				//			scripts = c_lua::get().scripts;

				//			if (selected_script >= scripts.size())
				//				selected_script = scripts.size() - 1; //-V103

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}
				//		}
				//		ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Load script"), crypt_str("##SCRIPTS__LOAD"), ImVec2(230, 26)))
				//		{
				//			c_lua::get().load_script(selected_script);
				//			c_lua::get().refresh_scripts();

				//			scripts = c_lua::get().scripts;

				//			if (selected_script >= scripts.size())
				//				selected_script = scripts.size() - 1; //-V103

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}

				//			eventlogs::get().add(crypt_str("Loaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
				//		}
				//		ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Unload script"), crypt_str("##SCRIPTS__UNLOAD"), ImVec2(230, 26)))
				//		{
				//			c_lua::get().unload_script(selected_script);
				//			c_lua::get().refresh_scripts();

				//			scripts = c_lua::get().scripts;

				//			if (selected_script >= scripts.size())
				//				selected_script = scripts.size() - 1; //-V103

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}

				//			eventlogs::get().add(crypt_str("Unloaded ") + scripts.at(selected_script) + crypt_str(" script"), false); //-V106
				//		}
				//		ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Reload all scripts"), crypt_str("##SCRIPTS__RELOAD"), ImVec2(230, 26)))
				//		{
				//			c_lua::get().reload_all_scripts();
				//			c_lua::get().refresh_scripts();

				//			scripts = c_lua::get().scripts;

				//			if (selected_script >= scripts.size())
				//				selected_script = scripts.size() - 1; //-V103

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}
				//		}
				//		ImGui::SetCursorPosX(5);
				//		if (ImGui::CustomButton(crypt_str("Unload all scripts"), crypt_str("##SCRIPTS__UNLOADALL"), ImVec2(230, 26)))
				//		{
				//			c_lua::get().unload_all_scripts();
				//			c_lua::get().refresh_scripts();

				//			scripts = c_lua::get().scripts;

				//			if (selected_script >= scripts.size())
				//				selected_script = scripts.size() - 1; //-V103

				//			for (auto& current : scripts)
				//			{
				//				if (current.size() >= 5 && current.at(current.size() - 1) == 'c')
				//					current.erase(current.size() - 5, 5);
				//				else if (current.size() >= 4)
				//					current.erase(current.size() - 4, 4);
				//			}
				//		}

				//		ImGui::PopItemWidth();
				//	}
				//	ImGui::EndChild();

				//	ImGui::SetCursorPos({ 390, 45 });

				//	ImGui::BeginChild("Scripts2", ImVec2(240, 395), false);
				//	{
				//		child_start("Scripts items");

				//		auto previous_check_box = false;

				//		for (auto& current : c_lua::get().scripts)
				//		{
				//			auto& items = c_lua::get().items.at(c_lua::get().get_script_id(current));

				//			for (auto& item : items)
				//			{
				//				std::string item_name;

				//				auto first_point = false;
				//				auto item_str = false;

				//				for (auto& c : item.first)
				//				{
				//					if (c == '.')
				//					{
				//						if (first_point)
				//						{
				//							item_str = true;
				//							continue;
				//						}
				//						else
				//							first_point = true;
				//					}

				//					if (item_str)
				//						item_name.push_back(c);
				//				}

				//				switch (item.second.type)
				//				{
				//				case NEXT_LINE:
				//					previous_check_box = false;
				//					break;
				//				case CHECK_BOX:
				//					previous_check_box = true;
				//					draw_checkbox(item_name.c_str(), &item.second.check_box_value);
				//					break;
				//				case COMBO_BOX:
				//					previous_check_box = false;
				//					draw_combo(item_name.c_str(), item.second.combo_box_value, [](void* data, int idx, const char** out_text)
				//						{
				//							auto labels = (std::vector <std::string>*)data;
				//							*out_text = labels->at(idx).c_str(); //-V106
				//							return true;
				//						}, &item.second.combo_box_labels, item.second.combo_box_labels.size());
				//					break;
				//				case SLIDER_INT:
				//					previous_check_box = false;
				//					ImGui::SliderInt(item_name.c_str(), &item.second.slider_int_value, item.second.slider_int_min, item.second.slider_int_max);
				//					break;
				//				case SLIDER_FLOAT:
				//					previous_check_box = false;
				//					ImGui::SliderFloat(item_name.c_str(), &item.second.slider_float_value, item.second.slider_float_min, item.second.slider_float_max);
				//					break;
				//				case COLOR_PICKER:
				//					if (previous_check_box)
				//						previous_check_box = false;
				//					else
				//						ImGui::Text((item_name + ' ').c_str());

				//					ImGui::SameLine();
				//					ImGui::ColorEdit((crypt_str("##") + item_name).c_str(), &item.second.color_picker_value, ALPHA, true);
				//					break;
				//				}
				//			}
				//		}
				//	}
				//	ImGui::EndChild();
				//	ImGui::PopFont();
				//}break;
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleVar(1);
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}