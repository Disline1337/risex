// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\fakelag.h"

using DoPostScreenEffects_t = void(__thiscall*)(void*, CViewSetup*);

void __fastcall hooks::hooked_postscreeneffects(void* thisptr, void* edx, CViewSetup* setup)
{
	static auto original_fn = clientmode_hook->get_func_address <DoPostScreenEffects_t>(44);

	if (!g_cfg.player.enable)
		return original_fn(thisptr, setup);

	if (!globals.local())
		return original_fn(thisptr, setup);

	for (auto i = 0; i < m_glow()->m_GlowObjectDefinitions.Count(); i++) //-V807
	{
		if (m_glow()->m_GlowObjectDefinitions[i].IsUnused())
			continue;

		auto object = &m_glow()->m_GlowObjectDefinitions[i];
		auto entity = object->GetEnt();

		if (!entity)
			continue;

		auto client_class = entity->GetClientClass();

		if (!client_class)
			continue;

		if (entity->is_player())
		{
			auto e = (player_t*)entity;
			auto should_glow = false;

			if (!e->valid(false, false))
				continue;


			auto type = ENEMY;

			if (e == globals.local())
				type = LOCAL;
			else if (e->m_iTeamNum() == globals.local()->m_iTeamNum())
				type = TEAM;

			//if (g_cfg.player.type[type].glow)
			//	return;

			float color[4];

			if (g_cfg.player.type[ENEMY].glow && type == ENEMY)
			{
				should_glow = true;

				color[0] = g_cfg.player.type[ENEMY].glow_color[0] / 255.0f;
				color[1] = g_cfg.player.type[ENEMY].glow_color[1] / 255.0f;
				color[2] = g_cfg.player.type[ENEMY].glow_color[2] / 255.0f;
				color[3] = g_cfg.player.type[ENEMY].glow_color[3] / 255.0f;
			}
			else if (g_cfg.player.type[TEAM].glow && type == TEAM)
			{
				should_glow = true;

				color[0] = g_cfg.player.type[TEAM].glow_color[0] / 255.0f;
				color[1] = g_cfg.player.type[TEAM].glow_color[1] / 255.0f;
				color[2] = g_cfg.player.type[TEAM].glow_color[2] / 255.0f;
				color[3] = g_cfg.player.type[TEAM].glow_color[3] / 255.0f;
			}
			else if (g_cfg.player.type[LOCAL].glow && type == LOCAL)
			{
				should_glow = true;

				color[0] = g_cfg.player.type[LOCAL].glow_color[0] / 255.0f;
				color[1] = g_cfg.player.type[LOCAL].glow_color[1] / 255.0f;
				color[2] = g_cfg.player.type[LOCAL].glow_color[2] / 255.0f;
				color[3] = g_cfg.player.type[LOCAL].glow_color[3] / 255.0f;
			}

			if (!should_glow)
				continue;

			object->Set
			(
				color[0],
				color[1],
				color[2],
				g_cfg.misc.anti_screenshot && m_engine()->IsTakingScreenshot() ? 0.0f : color[3],
				1.0f,
				0
			);
		}
		else if (g_cfg.esp.weapon[WEAPON_GLOW] && (client_class->m_ClassID == CAK47 || client_class->m_ClassID == CDEagle || client_class->m_ClassID >= CWeaponAug && client_class->m_ClassID <= CWeaponZoneRepulsor)) //-V648
		{
			float color[4] =
			{
				color[0] = g_cfg.esp.weapon_glow_color[0] / 255.0f,
				color[1] = g_cfg.esp.weapon_glow_color[1] / 255.0f,
				color[2] = g_cfg.esp.weapon_glow_color[2] / 255.0f,
				color[3] = g_cfg.esp.weapon_glow_color[3] / 255.0f
			};

			object->Set
			(
				color[0],
				color[1],
				color[2],
				g_cfg.misc.anti_screenshot && m_engine()->IsTakingScreenshot() ? 0.0f : color[3],
				1.0f,
				0
			);
		}
	}

	original_fn(thisptr, setup);
}