// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\menu\menu.h"
#include "..\..\features\lagcompensation\animfix.h"
#include "..\..\features\visuals\player_esp.h"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\misc.h"
#include "..\..\features\visuals\GrenadePrediction.h"
#include "..\..\features\visuals\bullet_tracers.h"
#include "..\..\features\visuals\dormant_esp.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "..\..\features\visuals\hitmarker.h"

using PaintTraverse_t = void(__thiscall*)(void*, vgui::VPANEL, bool, bool);

bool reload_fonts()
{
	static int old_width, old_height;
	static int width, height;

	m_engine()->GetScreenSize(width, height);

	if (width != old_width || height != old_height)
	{
		old_width = width;
		old_height = height;

		return true;
	}

	return false;
}

void __fastcall hooks::hooked_painttraverse(void* ecx, void* edx, vgui::VPANEL panel, bool force_repaint, bool allow_force)
{
	static auto original_fn = panel_hook->get_func_address <PaintTraverse_t> (41);
	globals.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true); //-V807

	static auto set_console = true;

	if (set_console)
	{
		set_console = false;

		m_cvar()->FindVar(crypt_str("developer"))->SetValue(FALSE); //-V807
		m_cvar()->FindVar(crypt_str("con_filter_enable"))->SetValue(TRUE);
		m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		m_engine()->ExecuteClientCmd(crypt_str("clear"));
		m_cvar()->ConsoleColorPrintf(Color(100, 200, 50), "Cheat injection success! \n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "\n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "Hello dear, user! \n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "\n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "\n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|------ Thanks for -----| \n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|                       | \n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|         Disline       | \n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|         Overlxrd      | \n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|                       | \n");
		//m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "|-----------------------| \n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "\n");
		m_cvar()->ConsoleColorPrintf(Color(255, 255, 255), "\n");
	}

	if (set_console)
	{
		set_console = false;

		m_cvar()->FindVar(crypt_str("developer"))->SetValue(FALSE); //-V807
		m_cvar()->FindVar(crypt_str("con_filter_enable"))->SetValue(TRUE);
		m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		m_engine()->ExecuteClientCmd(crypt_str("clear"));
	}

	static auto log_value = true;

	if (log_value != g_cfg.misc.show_default_log)
	{
		log_value = g_cfg.misc.show_default_log;

		if (log_value)
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str(""));
		else
			m_cvar()->FindVar(crypt_str("con_filter_text"))->SetValue(crypt_str("IrWL5106TZZKNFPz4P4Gl3pSN?J370f5hi373ZjPg%VOVh6lN"));
	}

	static vgui::VPANEL panel_id = 0;
	static auto in_game = false;

	if (!in_game && m_engine()->IsInGame()) //-V807
	{
		in_game = true;

		for (auto i = 1; i < 65; i++)
		{
			globals.g.fired_shots[i] = 0;
			globals.g.missed_shots[i] = 0;
			player_records[i].clear();
			c_anim_fix::get().is_dormant[i] = false;
			playeresp::get().esp_alpha_fade[i] = 0.0f;
			playeresp::get().health[i] = 100;
			c_dormant_esp::get().m_cSoundPlayers[i].reset();
		}

		antiaim::get().freeze_check = false;
		globals.g.next_lby_update = FLT_MIN;
		globals.g.last_lby_move = FLT_MIN;
		globals.g.last_aimbot_shot = 0;
		globals.g.bomb_timer_enable = true;
		globals.g.backup_model = false;
		globals.g.should_remove_smoke = false;
		globals.g.should_update_beam_index = true;
		globals.g.should_update_playerresource = true;
		globals.g.should_update_gamerules = true;
		globals.g.kills = 0;
		globals.shots.clear();
		globals.g.commands.clear();
		SkinChanger::model_indexes.clear();
		SkinChanger::player_model_indexes.clear();
	}
	else if (in_game && !m_engine()->IsInGame())
	{
		in_game = false;

		globals.g.m_networkable = nullptr;

		misc::get().double_tap_enabled = false;
		misc::get().double_tap_key = false;

		misc::get().hide_shots_enabled = false;
		misc::get().hide_shots_key = false;
	}

	if (m_engine()->IsTakingScreenshot() && g_cfg.misc.anti_screenshot)
		return;

	static uint32_t HudZoomPanel = 0;
	
	if (!HudZoomPanel)
		if (!strcmp(crypt_str("HudZoom"), m_panel()->GetName(panel)))
			HudZoomPanel = panel;

	if (HudZoomPanel == panel && g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SCOPE])
		return;

	original_fn(ecx, panel, force_repaint, allow_force);

	if (!panel_id)
	{
		auto panelName = m_panel()->GetName(panel);

		if (!strcmp(panelName, crypt_str("MatSystemTopPanel")))
			panel_id = panel;
	}

	if (reload_fonts())
	{
		static auto create_font = [](const char* name, int size, int weight, DWORD flags) -> vgui::HFont
		{
			globals.last_font_name = name;

			auto font = m_surface()->FontCreate();
			m_surface()->SetFontGlyphSet(font, name, size, weight, NULL, NULL, flags);

			return font;
		};

		fonts[LOGS] = create_font(crypt_str("Lucida Console"), 10, FW_MEDIUM, FONTFLAG_DROPSHADOW);
		fonts[ESP] = create_font(crypt_str("Small Fonts"), 8, FW_NORMAL, FONTFLAG_OUTLINE);
		fonts[ESP_TWO] = create_font(crypt_str("undefeated"), 10, FW_NORMAL, FONTFLAG_OUTLINE);
		fonts[NAME] = create_font(crypt_str("Verdana"), 12, FW_MEDIUM, FONTFLAG_OUTLINE);
		fonts[SUBTABWEAPONS] = create_font(crypt_str("undefeated"), 13, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[KNIFES] = create_font(crypt_str("undefeated"), 13, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[GRENADES] = create_font(crypt_str("undefeated"), 20, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[INDICATORFONT] = create_font(crypt_str("Verdana"), 12, FW_MEDIUM, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW);
		fonts[DAMAGE_MARKER] = create_font(crypt_str("Small Fonts"), 14, FW_MEDIUM, FONTFLAG_OUTLINE);

		globals.last_font_name.clear();
	}

	if (panel_id == panel)
	{
		if (globals.available())
		{
			static auto alive = false;

			if (!alive && globals.local()->is_alive())
			{
				alive = true;
				globals.g.should_clear_death_notices = true;
			}
			else if (alive && !globals.local()->is_alive())
			{
				alive = false;

				for (auto i = 1; i < m_globals()->m_maxclients; i++)
				{
					globals.g.fired_shots[i] = 0;
					globals.g.missed_shots[i] = 0;
				}

				local_animations::get().local_data.prediction_animstate = nullptr;
				local_animations::get().local_data.animstate = nullptr;

				globals.g.weapon = nullptr;
				globals.g.should_choke_packet = false;
				globals.g.should_send_packet = false;
				globals.g.kills = 0;
				globals.g.should_buy = 3;
			}

			globals.g.bomb_carrier = -1;

			if (g_cfg.player.enable)
			{
				worldesp::get().paint_traverse();
				playeresp::get().paint_traverse();
			}

			misc::get().zeus_range();
			misc::get().desync_arrows();

			auto weapon = globals.local()->m_hActiveWeapon().Get();

			if (weapon->is_grenade() && g_cfg.esp.grenade_prediction && g_cfg.player.enable)
				GrenadePrediction::get().Paint();

			if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SCOPE] && globals.g.scoped && weapon->is_sniper())
			{
				static int w, h;
				m_engine()->GetScreenSize(w, h);

				render::get().line(w / 2, 0, w / 2, h, Color::Black);
				render::get().line(0, h / 2, w, h / 2, Color::Black);
			}

			if (globals.local()->is_alive())
			{
				if (c_menu::get().public_alpha > 0.15f && g_cfg.legitbot.enabled)
				{
					int x, y;
					m_engine()->GetScreenSize(x, y);

					if (g_cfg.legitbot.weapon[globals.g.current_weapon].fov)
					{
						float radius = tanf(DEG2RAD(g_cfg.legitbot.weapon[globals.g.current_weapon].fov) / 2) / tanf(DEG2RAD(90 + g_cfg.esp.fov) / 2) * x;
						render::get().circle_filled(x / 2, y / 2, 60, radius, Color(235, 235, 235, c_menu::get().public_alpha * 0.68));
						render::get().circle(x / 2, y / 2, 60, radius, Color(235, 235, 235, c_menu::get().public_alpha * 0.8));
					}

					if (g_cfg.legitbot.weapon[globals.g.current_weapon].silent_fov)
					{
						float silent_radius = tanf(DEG2RAD(g_cfg.legitbot.weapon[globals.g.current_weapon].silent_fov) / 2) / tanf(DEG2RAD(90 + g_cfg.esp.fov) / 2) * x;
						render::get().circle_filled(x / 2, y / 2, 60, silent_radius, Color(15, 235, 15, c_menu::get().public_alpha * 0.68));
						render::get().circle(x / 2, y / 2, 60, silent_radius, Color(15, 235, 15, c_menu::get().public_alpha * 0.8));
					}
				}
			}
			
			worldesp::get().penetration_reticle();
			worldesp::get().automatic_peek_indicator();


			bullettracers::get().draw_beams();
		}

		static auto framerate = 0.0f;
		framerate = 0.9f * framerate + 0.1f * m_globals()->m_absoluteframetime;

		if (framerate <= 0.0f)
			framerate = 1.0f;

		globals.g.framerate = (int)(1.0f / framerate);
		auto nci = m_engine()->GetNetChannelInfo();

		if (nci)
		{
			auto latency = m_engine()->IsPlayingDemo() ? 0.0f : nci->GetAvgLatency(FLOW_OUTGOING);

			if (latency) //-V550
			{
				static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
				latency -= 0.5f / cl_updaterate->GetFloat();
			}

			globals.g.ping = (int)(max(0.0f, latency) * 1000.0f);
		}

		time_t lt;
		struct tm* t_m;

		lt = time(nullptr);
		t_m = localtime(&lt);

		auto time_h = t_m->tm_hour;
		auto time_m = t_m->tm_min;
		auto time_s = t_m->tm_sec;

		std::string time;

		if (time_h < 10)
			time += "0";

		time += std::to_string(time_h) + ":";

		if (time_m < 10)
			time += "0";

		time += std::to_string(time_m) + ":";

		if (time_s < 10)
			time += "0";

		time += std::to_string(time_s);
		globals.g.time = std::move(time);

		static int w, h;
		m_engine()->GetScreenSize(w, h);

		static auto alpha = 0;
		auto speed = 800.0f * m_globals()->m_frametime;

		eventlogs::get().paint_traverse();

		misc::get().NightmodeFix();
		if (globals.g.loaded_script)
			for (auto current : c_lua::get().hooks.getHooks(crypt_str("on_paint")))
				current.func();
	}
}