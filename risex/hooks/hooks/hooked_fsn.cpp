// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\features\ragebot\ragebot.h"
#include "..\..\features\lagcompensation\animfix.h"
#include "..\..\features\visuals\nightmode.h"
#include "..\..\features\misc\misc.h"
#include "..\..\utils\nSkinz\SkinChanger.h"
#include "..\..\features\misc\fakelag.h"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\misc\prediction_system.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "../../features/misc/logger.hpp"

using FrameStageNotify_t = void(__stdcall*)(ClientFrameStage_t);

Vector flb_aim_punch;
Vector flb_view_punch;

Vector* aim_punch;
Vector* view_punch;

void remove_smoke()
{
	if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SMOKE])
	{
		static auto smoke_count = *reinterpret_cast<uint32_t**>(util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 57 8B CB")) + 0x1);
		*(int*)smoke_count = 0;
	}

	if (globals.g.should_remove_smoke == g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SMOKE])
		return;

	globals.g.should_remove_smoke = g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SMOKE];

	static std::vector <const char*> smoke_materials =
	{
		"effects/overlaysmoke",
		"particle/beam_smoke_01",
		"particle/particle_smokegrenade",
		"particle/particle_smokegrenade1",
		"particle/particle_smokegrenade2",
		"particle/particle_smokegrenade3",
		"particle/particle_smokegrenade_sc",
		"particle/smoke1/smoke1",
		"particle/smoke1/smoke1_ash",
		"particle/smoke1/smoke1_nearcull",
		"particle/smoke1/smoke1_nearcull2",
		"particle/smoke1/smoke1_snow",
		"particle/smokesprites_0001",
		"particle/smokestack",
		"particle/vistasmokev1/vistasmokev1",
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_nearcull",
		"particle/vistasmokev1/vistasmokev1_nearcull_fog",
		"particle/vistasmokev1/vistasmokev1_nearcull_nodepth",
		"particle/vistasmokev1/vistasmokev1_smokegrenade",
		"particle/vistasmokev1/vistasmokev4_emods_nocull",
		"particle/vistasmokev1/vistasmokev4_nearcull",
		"particle/vistasmokev1/vistasmokev4_nocull"
	};

	for (auto material_name : smoke_materials)
	{
		auto material = m_materialsystem()->FindMaterial(material_name, nullptr);

		if (!material)
			continue;

		material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, globals.g.should_remove_smoke);
	}
}

void __stdcall hooks::hooked_fsn(ClientFrameStage_t stage)
{
	static auto original_fn = client_hook->get_func_address <FrameStageNotify_t>(37);
	globals.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!globals.available())
	{
		nightmode::get().clear_stored_materials();
		return original_fn(stage);
	}

	if (stage == FRAME_START)
		key_binds::get().update_key_binds();

	aim_punch = nullptr;
	view_punch = nullptr;

	flb_aim_punch.Zero();
	flb_view_punch.Zero();

	if (globals.g.updating_skins && m_clientstate()->iDeltaTick > 0) //-V807
		globals.g.updating_skins = false;

	SkinChanger::run(stage);
	local_animations::get().run(stage);
	c_anim_fix::get().fsn(stage);
	original_fn(stage);

	if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && globals.local()->is_alive()) //-V522 //-V807
	{
		auto viewmodel = globals.local()->m_hViewModel().Get();

		if (viewmodel && engineprediction::get().viewmodel_data.weapon == viewmodel->m_hWeapon().Get() && engineprediction::get().viewmodel_data.sequence == viewmodel->m_nSequence() && engineprediction::get().viewmodel_data.animation_parity == viewmodel->m_nAnimationParity()) //-V807
		{
			viewmodel->m_flCycle() = engineprediction::get().viewmodel_data.cycle;
			viewmodel->m_flAnimTime() = engineprediction::get().viewmodel_data.animation_time;
		}
	}

	if (stage == FRAME_RENDER_START)
	{
		if (g_cfg.esp.client_bullet_impacts)
		{
			static auto last_count = 0;
			auto& client_impact_list = *(CUtlVector <client_hit_verify_t>*)((uintptr_t)globals.local() + 0xBC00);

			for (auto i = client_impact_list.Count(); i > last_count; --i)
				m_debugoverlay()->BoxOverlay(client_impact_list[i - 1].position, Vector(-1.5f, -1.5f, -1.5f), Vector(1.5f, 1.5f, 1.5f), QAngle(0.0f, 0.0f, 0.0f), g_cfg.esp.client_bullet_impacts_color.r(), g_cfg.esp.client_bullet_impacts_color.g(), g_cfg.esp.client_bullet_impacts_color.b(), g_cfg.esp.client_bullet_impacts_color.a(), 4.0f);

			if (client_impact_list.Count() != last_count)
				last_count = client_impact_list.Count();
		}

		remove_smoke();
		misc::get().ragdolls();

		if (g_cfg.esp.removals[REMOVALS_FLASH] && globals.local()->m_flFlashDuration() && g_cfg.player.enable) //-V807
			globals.local()->m_flFlashDuration() = 0.0f;

		if (*(bool*)m_postprocessing() != (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_POSTPROCESSING] && (!g_cfg.esp.world_modulation || !g_cfg.esp.exposure)))
			*(bool*)m_postprocessing() = g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_POSTPROCESSING] && (!g_cfg.esp.world_modulation || !g_cfg.esp.exposure);

		if (g_cfg.esp.removals[REMOVALS_RECOIL] && g_cfg.player.enable)
		{
			aim_punch = &globals.local()->m_aimPunchAngle();
			view_punch = &globals.local()->m_viewPunchAngle();

			flb_aim_punch = *aim_punch;
			flb_view_punch = *view_punch;

			(*aim_punch).Zero();
			(*view_punch).Zero(); //-V656
		}

		auto get_original_scope = false;

		if (globals.local()->is_alive())
		{
			globals.g.in_thirdperson = key_binds::get().get_key_bind_state(17);

			if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SCOPE])
			{
				auto weapon = globals.local()->m_hActiveWeapon().Get();

				if (weapon)
				{
					get_original_scope = true;

					globals.g.scoped = globals.local()->m_bIsScoped() && weapon->m_zoomLevel();
					globals.local()->m_bIsScoped() = weapon->m_zoomLevel();
				}
			}
		}

		if (!get_original_scope)
			globals.g.scoped = globals.local()->m_bIsScoped();
	}

	if (stage == FRAME_NET_UPDATE_END)
	{

		for (auto i = 1; i < m_globals()->m_maxclients; i++)
		{
			auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

			if (!e)
				break;
		}
	}

	if (stage == FRAME_RENDER_END)
	{
		static auto r_drawspecificstaticprop = m_cvar()->FindVar(crypt_str("r_drawspecificstaticprop")); //-V807

		if (r_drawspecificstaticprop->GetBool())
			r_drawspecificstaticprop->SetValue(FALSE);

		if (globals.g.change_materials)
		{
			if (g_cfg.esp.nightmode && g_cfg.player.enable)
				nightmode::get().apply();
			else
				nightmode::get().remove();

			globals.g.change_materials = false;
		}

		worldesp::get().skybox_changer();
		worldesp::get().fog_changer();

		misc::get().FullBright();
		misc::get().ViewModel();

		static auto cl_foot_contact_shadows = m_cvar()->FindVar(crypt_str("cl_foot_contact_shadows")); //-V807

		if (cl_foot_contact_shadows->GetBool())
			cl_foot_contact_shadows->SetValue(FALSE);

		static auto zoom_sensitivity_ratio_mouse = m_cvar()->FindVar(crypt_str("zoom_sensitivity_ratio_mouse"));

		if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_ZOOM] && g_cfg.esp.fix_zoom_sensivity && zoom_sensitivity_ratio_mouse->GetFloat() == 1.0f) //-V550
			zoom_sensitivity_ratio_mouse->SetValue(0.0f);
		else if ((!g_cfg.player.enable || !g_cfg.esp.removals[REMOVALS_ZOOM] || !g_cfg.esp.fix_zoom_sensivity) && !zoom_sensitivity_ratio_mouse->GetFloat())
			zoom_sensitivity_ratio_mouse->SetValue(1.0f);

		static auto r_modelAmbientMin = m_cvar()->FindVar(crypt_str("r_modelAmbientMin"));

		if (g_cfg.esp.world_modulation && g_cfg.esp.ambient && r_modelAmbientMin->GetFloat() != g_cfg.esp.ambient * 0.05f) //-V550
			r_modelAmbientMin->SetValue(g_cfg.esp.ambient * 0.05f);
		else if ((!g_cfg.esp.world_modulation || !g_cfg.esp.ambient) && r_modelAmbientMin->GetFloat())
			r_modelAmbientMin->SetValue(0.0f);
	}

	if (stage == FRAME_NET_UPDATE_END)
	{
		auto current_shot = globals.shots.end();

		auto net_channel = m_engine()->GetNetChannelInfo();
		auto latency = net_channel ? net_channel->GetLatency(FLOW_OUTGOING) + net_channel->GetLatency(FLOW_INCOMING) + 1.0f : 0.0f; //-V807

		for (auto& shot = globals.shots.begin(); shot != globals.shots.end(); ++shot)
		{
			if (shot->end)
			{
				current_shot = shot;
				break;
			}
			else if (shot->impacts && m_globals()->m_tickcount - 1 > shot->event_fire_tick)
			{
				current_shot = shot;
				current_shot->end = true;
				break;
			}
			else if (globals.g.backup_tickbase - TIME_TO_TICKS(latency) > shot->fire_tick)
			{
				current_shot = shot;
				current_shot->end = true;
				current_shot->latency = true;
				break;
			}
		}

		if (current_shot != globals.shots.end())
		{
			if (!current_shot->latency)
			{
				current_shot->shot_info.should_log = true; //-V807

				if (!current_shot->hurt_player)
				{
					auto reason = "";
					if (current_shot->impact_hit_player)
					{
						{
							reason = "resolver";
							current_shot->shot_info.result = crypt_str("R_MISS");
							if (current_shot->shot_info.hitchance > 80) {
								notify::add_log("Miss", crypt_str("Missed shot due to bad resolve"), Color(255, 10, 0));
							}
							else if (current_shot->shot_info.hitchance < 80) {
								notify::add_log("Miss", crypt_str("Missed shot due to desync animation"), Color(255, 10, 0));
							}
							else
								notify::add_log("Miss", "Missed shot due to resolver innacuracy", Color(255, 10, 0));
							globals.g.missed_shots[current_shot->last_target]++;
						}
					}
					else if (!current_shot->occlusion && current_shot->shot_info.hitchance < 100)
					{
						reason = "spread";
						current_shot->shot_info.result = crypt_str("SP_MISS");
						std::string m = "Missed shot due to ";
						notify::add_log("Miss", crypt_str(m + "spread"), Color(255, 10, 0));

					}
					else if (!current_shot->occlusion && current_shot->shot_info.hitchance > 100)
					{
						reason = "prediction error";
						current_shot->shot_info.result = crypt_str("PR_MISS");
						std::string m = "Missed shot due to ";
						if (current_shot->shot_info.client_damage > 1)
							notify::add_log("Miss", crypt_str(m + "prediction error"), Color(255, 10, 0));
						else
							notify::add_log("Miss", "Shot unregisted by server", Color(255, 10, 0));

					}
					else if (current_shot->occlusion)
					{
						reason = "occlusion";
						current_shot->shot_info.result = crypt_str("OC_MISS");
						std::string m = "Missed shot due to ";
						notify::add_log("Miss", crypt_str(m + "occlusion " + "| damage loss:" + (std::to_string(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage - current_shot->shot_info.client_damage).c_str())), Color(255, 10, 0));

					}
					else if (current_shot->occlusion && current_shot->shot_info.hitchance >= 100)
					{
						reason = "occlusion";
						current_shot->shot_info.result = crypt_str("OC_PR_MISS");
						std::string m = "Missed shot due to ";
						notify::add_log("Miss", crypt_str(m + "prediction error(incorrectly calculated damage)"), Color(255, 10, 0));

					}

					std::stringstream log;
					log << "Fired shot to " << current_shot->shot_info.target_name << "'s "
						<< current_shot->shot_info.client_hitbox << " for " << current_shot->shot_info.client_damage << " missed shot reason "
						<< reason << " backtracked for " << current_shot->shot_info.backtrack_ticks << " hitchance for " << current_shot->shot_info.hitchance
						<< " [not successful]";
					notify::add_log("Debug", crypt_str(log.str().c_str()), Color(35, 255, 25));

				}
				else
				{
					std::stringstream log;
					log << "Fired shot to " << current_shot->shot_info.target_name << "'s "
						<< current_shot->shot_info.client_hitbox << " for " << current_shot->shot_info.client_damage << " registed shot in "
						<< current_shot->shot_info.server_hitbox << " for " << current_shot->shot_info.server_damage << " backtracked for " << current_shot->shot_info.backtrack_ticks
						<< " hitchance for " << current_shot->shot_info.hitchance << " [successful]";
					notify::add_log("Debug", crypt_str(log.str().c_str()), Color(35, 255, 25));
				}
			}

			if (globals.g.loaded_script && current_shot->shot_info.should_log)
			{
				current_shot->shot_info.should_log = false;

				for (auto current : c_lua::get().hooks.getHooks(crypt_str("on_shot")))
					current.func(current_shot->shot_info);
			}

			globals.shots.erase(current_shot);
		}
	}

	if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_RECOIL] && globals.local()->is_alive() && aim_punch && view_punch)
	{
		*aim_punch = flb_aim_punch;
		*view_punch = flb_view_punch;
	}

	static DWORD* death_notice = nullptr;

	if (globals.local()->is_alive())
	{
		if (!death_notice)
			death_notice = util::FindHudElement <DWORD>(crypt_str("Cglobals_HudDeathNotice"));

		if (death_notice)
		{
			auto local_death_notice = (float*)((uintptr_t)death_notice + 0x50);

			if (local_death_notice)
				*local_death_notice = g_cfg.esp.preserve_killfeed ? FLT_MAX : 1.5f;

			if (globals.g.should_clear_death_notices)
			{
				globals.g.should_clear_death_notices = false;

				using Fn = void(__thiscall*)(uintptr_t);
				static auto clear_notices = (Fn)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 EC 0C 53 56 8B 71 58"));

				clear_notices((uintptr_t)death_notice - 0x14);
			}
		}
	}
	else
		death_notice = 0;
}