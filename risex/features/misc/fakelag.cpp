// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\ragebot\ragebot.h"
#include "fakelag.h"
#include "misc.h"
#include "prediction_system.h"
#include "logs.h"

void fakelag::Fakelag(CUserCmd* m_pcmd)
{
	static auto fluctuate_ticks = 0;
	static auto switch_ticks = false;
	static auto random_factor = min(rand() % 14 + 1, g_cfg.antiaim.triggers_fakelag_amount);

	auto choked = m_clientstate()->iChokedCommands; //-V807
	auto flags = engineprediction::get().backup_data.flags; //-V807
	auto velocity = engineprediction::get().backup_data.velocity.Length(); //-V807

	auto max_speed = 320.0f;
	auto weapon_info = globals.g.weapon->get_csweapon_info();

	if (weapon_info)
		max_speed = globals.g.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

	if (m_gamerules()->m_bIsValveDS()) //-V807
		max_choke = m_engine()->IsVoiceRecording() ? 1 : min(max_choke, 4);

	if (misc::get().recharging_double_tap)
		max_choke = globals.g.weapon->get_max_tickbase_shift();

	max_choke = g_cfg.antiaim.fakelag_amount;

	if (choked < max_choke)
		*globals.send_packet = false;
	else
		*globals.send_packet = true;

	if (!globals.g.exploits && g_cfg.antiaim.fakelag)
	{
		max_choke = g_cfg.antiaim.fakelag_amount;

		if (m_gamerules()->m_bIsValveDS())
			max_choke = min(max_choke, 4);

		if (choked < max_choke)
			*globals.send_packet = false;
	}
	else if (globals.g.exploits || !antiaim::get().condition(m_pcmd, false) && g_cfg.antiaim.type[type].desync) //-V648
	{
		condition = true;
		started_peeking = false;


		if (choked < 1)
			*globals.send_packet = false;
		else
			*globals.send_packet = true;
	}
	else
		condition = true;
}

void fakelag::Createmove()
{
	if (FakelagCondition(globals.get_command()))
		return;

	Fakelag(globals.get_command());

	static auto Fn = util::FindSignature(crypt_str("engine.dll"), crypt_str("B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC")) + 0x1;
	DWORD old = 0;

	VirtualProtect((void*)Fn, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &old);
	*(uint32_t*)Fn = 17;
	VirtualProtect((void*)Fn, sizeof(uint32_t), old, &old);

}

bool fakelag::FakelagCondition(CUserCmd* m_pcmd)
{
	condition = false;

	if (globals.local()->m_bGunGameImmunity() || globals.local()->m_fFlags() & FL_FROZEN)
		condition = true;

	if (antiaim::get().freeze_check && !misc::get().double_tap_enabled && !misc::get().hide_shots_enabled)
		condition = true;

	return condition;
}