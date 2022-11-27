#include "prediction_system.h"

void engineprediction::fix_local_commands()
{
	auto GetCorrectionTicks = []() -> int
	{
		float v1; // st7
		float v4; // xmm0_4
		float v6; // [esp+0h] [ebp-10h]

		static auto sv_clockcorrection_msecs = m_cvar()->FindVar("sv_clockcorrection_msecs");
		if (!sv_clockcorrection_msecs || m_globals()->m_maxclients <= 1)
			return -1;

		v1 = sv_clockcorrection_msecs->GetFloat();
		v4 = 1.0f;
		v6 = v1 / 1000.0f;
		if (v6 <= 1.0f)
		{
			v4 = 0.0f;
			if (v6 >= 0.0f)
				v4 = v1 / 1000.0f;
		}

		return ((v4 / m_globals()->m_intervalpertick) + 0.5f);
	};

	if (!globals.local() || !globals.local()->is_alive())
		return;

	// game events are actually fired in OnRenderStart which is WAY later after they are received
	// effective delay by lerp time, now we call them right after theyre received (all receive proxies are invoked without delay).
	m_engine()->FireEvents();

	auto v18 = GetCorrectionTicks();
	if (v18 == -1)
		m_last_cmd_delta = 0;
	else
	{
		auto v19 = m_clientstate()->m_iServerTick; //g_csgo.m_engine->GetTick();
		int m_sim_ticks = TIME_TO_TICKS(globals.local()->m_flSimulationTime());

		if (m_sim_ticks > TIME_TO_TICKS(globals.local()->m_flOldSimulationTime()) && (std::fabs(m_sim_ticks - v19) <= v18))
			m_last_cmd_delta = m_sim_ticks - v19;
	}
}

void engineprediction::store(StoredData_t* ndata, CUserCmd* cmd, int m_tick)
{
	//if local invalid reset & return
	if (!globals.local() || !globals.local()->is_alive())
	{
		reset();
		return;
	}

	//get storing netvars for fix
	ndata->m_tickbase = globals.local()->m_nTickBase();
	ndata->m_command_number = cmd->m_command_number;
	ndata->m_punch = globals.local()->m_aimPunchAngle();
	ndata->m_punch_vel = globals.local()->m_aimPunchAngleVel();
	ndata->m_viewPunchAngle = globals.local()->m_viewPunchAngle();
	ndata->m_view_offset = globals.local()->m_vecViewOffset();
	ndata->m_view_offset.z = std::fminf(std::fmaxf(ndata->m_view_offset.z, 46.0f), 64.0f);
	ndata->m_vecVelocity = globals.local()->m_vecVelocity();
	ndata->m_vecOrigin = globals.local()->m_vecOrigin();
	ndata->m_flFallVelocity = globals.local()->m_flFallVelocity();
	ndata->m_flThirdpersonRecoil = globals.local()->m_flThirdpersonRecoil();
	ndata->m_duck_amount = globals.local()->m_flDuckAmount();
	ndata->m_velocity_modifier = globals.local()->m_flVelocityModifier();
	ndata->m_tick = m_tick;
	ndata->m_is_filled = true;
}

void engineprediction::apply(int time)
{
	//if local invalid reset & return
	if (!globals.local() || !globals.local()->is_alive())
	{
		reset();
		return;
	}

	// get current record and validate.
	StoredData_t* data = &m_data[time % MULTIPLAYER_BACKUP];
	if (!data || !data->m_is_filled || data->m_command_number != time || (time - data->m_command_number) > MULTIPLAYER_BACKUP)
		return;

	// get deltas. before, when you stop shooting, punch values would sit around 0.03125 and then goto 0 next update.
	// with this fix applied, values slowly decay under 0.03125.
	Vector aim_punch_delta = globals.local()->m_aimPunchAngle() - data->m_punch,
		aim_punch_vel_delta = globals.local()->m_aimPunchAngleVel() - data->m_punch_vel,
		view_punch_delta = globals.local()->m_viewPunchAngle() - data->m_viewPunchAngle;

	Vector view_offset_delta = globals.local()->m_vecViewOffset() - data->m_view_offset,
		velocity_diff = globals.local()->m_vecVelocity() - data->m_vecVelocity;

	float duck_amount_delta = globals.local()->m_flDuckAmount() - data->m_duck_amount,
		modifier_delta = globals.local()->m_flVelocityModifier() - data->m_velocity_modifier;

	// fixing netvar compressions.
	if (std::abs(aim_punch_delta.x) <= 0.03125f && std::abs(aim_punch_delta.y) <= 0.03125f && std::abs(aim_punch_delta.z) <= 0.03125f)
		globals.local()->m_aimPunchAngle() = data->m_punch;

	if (std::abs(aim_punch_vel_delta.x) <= 0.03125f && std::abs(aim_punch_vel_delta.y) <= 0.03125f && std::abs(aim_punch_vel_delta.z) <= 0.03125f)
		globals.local()->m_aimPunchAngleVel() = data->m_punch_vel;

	if (std::abs(view_offset_delta.z) <= 0.03125f)
		globals.local()->m_vecViewOffset().z = data->m_view_offset.z;

	if (std::abs(view_punch_delta.x) <= 0.03125f && std::abs(view_punch_delta.y) <= 0.03125f && std::abs(view_punch_delta.z) <= 0.03125f)
		globals.local()->m_viewPunchAngle() = data->m_viewPunchAngle;

	if (std::abs(velocity_diff.x) <= 0.03125f && std::abs(velocity_diff.y) <= 0.03125f && std::abs(velocity_diff.z) <= 0.03125f)
		globals.local()->m_vecVelocity() = data->m_vecVelocity;

	if (std::abs(globals.local()->m_flThirdpersonRecoil() - data->m_flThirdpersonRecoil) <= 0.03125f)
		globals.local()->m_flThirdpersonRecoil() = data->m_flThirdpersonRecoil;

	/*if (std::abs(g_local->m_flDuckSpeed() - data->m_flDuckSpeed) <= 0.03125f)
		g_local->m_flDuckSpeed() = data->m_flDuckSpeed;*/

	if (std::abs(duck_amount_delta) <= 0.03125f)
		globals.local()->m_flDuckAmount() = data->m_duck_amount;

	if (std::abs(globals.local()->m_flFallVelocity() - data->m_flFallVelocity) <= 0.03125f)
		globals.local()->m_flFallVelocity() = data->m_flFallVelocity;

	if (std::abs(modifier_delta) <= 0.00625f)
		globals.local()->m_flVelocityModifier() = data->m_velocity_modifier;
}

void engineprediction::detect_prediction_error(StoredData_t* data, int m_tick)
{
	if (!data || !globals.local() || !globals.local()->is_alive() || data->m_command_number != m_tick || !data->m_is_filled || data->m_tick > (m_globals()->m_tickcount + 8))
		return;

	static auto is_out_of_epsilon_float = [](float a, float b, float m_epsilon) -> bool {
		return std::fabsf(a - b) > m_epsilon;
	};

	static auto is_out_of_epsilon_vec = [](Vector a, Vector b, float m_epsilon) -> bool {
		return std::fabsf(a.x - b.x) > m_epsilon || std::fabsf(a.y - b.y) > m_epsilon || std::fabsf(a.z - b.z) > m_epsilon;
	};

	static auto is_out_of_epsilon_ang = [](Vector a, Vector b, float m_epsilon) -> bool {
		return std::fabsf(a.x - b.x) > m_epsilon || std::fabsf(a.y - b.y) > m_epsilon || std::fabsf(a.z - b.z) > m_epsilon;
	};

	if (is_out_of_epsilon_ang(globals.local()->m_aimPunchAngle(), data->m_punch, 0.5f))
	{
		data->m_punch = globals.local()->m_aimPunchAngle();
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
	else
		globals.local()->m_aimPunchAngle() = data->m_punch;

	if (is_out_of_epsilon_ang(globals.local()->m_aimPunchAngleVel(), data->m_punch_vel, 0.5f))
	{
		data->m_punch_vel = globals.local()->m_aimPunchAngleVel();
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
	else
		globals.local()->m_aimPunchAngleVel() = data->m_punch_vel;

	if (is_out_of_epsilon_float(globals.local()->m_vecViewOffset().z, data->m_view_offset.z, 0.5f))
	{
		data->m_view_offset.z = globals.local()->m_vecViewOffset().z;
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
	else
		globals.local()->m_vecViewOffset().z = data->m_view_offset.z;

	if (is_out_of_epsilon_vec(globals.local()->m_vecVelocity(), data->m_vecVelocity, 0.5f))
	{
		data->m_vecVelocity = globals.local()->m_vecVelocity();
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
	else
		globals.local()->m_vecVelocity() = data->m_vecVelocity;

	if (is_out_of_epsilon_float(globals.local()->m_flVelocityModifier(), data->m_velocity_modifier, 0.00625f))
	{
		data->m_velocity_modifier = globals.local()->m_flVelocityModifier();
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
	else
		globals.local()->m_flVelocityModifier() = data->m_velocity_modifier;

	if ((globals.local()->m_vecOrigin() - data->m_vecOrigin).LengthSqr() >= 1.f)
	{
		data->m_vecOrigin = globals.local()->m_vecOrigin();
		m_prediction()->IsFirstTimePredicted;
		//c_log->error(XOR("detected prediction error!"));
	}
}

void engineprediction::reset()
{
	m_data.fill(StoredData_t());
}

namespace engine_prediction
{
	void update()
	{
		//https://github.com/click4dylan/CSGO_GameMovement_Reversed/blob/24d68f47761e5635437aee473860f13e6adbfe6b/IGameMovement.cpp#L703 ?
		if (m_stored_variables.m_flVelocityModifier < 1.f)
		{
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = 1; //m_bPreviousAckHadErrors
			*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x1C)) = 0; //m_nCommandsPredicted
		}

		if (m_clientstate()->iDeltaTick > 0)
			m_prediction()->Update(m_clientstate()->iDeltaTick, m_clientstate()->iDeltaTick > 0, m_clientstate()->nLastCommandAck, m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands);
	}

	// Purpose: We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
//  This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
	void StartCommand(player_t* player, CUserCmd* cmd)
	{
#if !defined(NO_ENTITY_PREDICTION) && defined(USE_PREDICTABLEID)
		CPredictableId::ResetInstanceCounters();
#endif

		//if ( !stored_vars.move_data )
		memset(&m_stored_variables.m_MoveData, 0, sizeof(CMoveData));

		if (!m_stored_variables.prediction_player || !m_stored_variables.prediction_seed)
		{
			m_stored_variables.prediction_seed = *reinterpret_cast<std::uintptr_t*>(std::uintptr_t(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 47 40 A3"))) + 4);
			m_stored_variables.prediction_player = *reinterpret_cast<std::uintptr_t*>(std::uintptr_t(util::FindSignature(crypt_str("client.dll"), crypt_str("0F 5B C0 89 35"))) + 5);
		}

		*reinterpret_cast<int*>(m_stored_variables.prediction_seed) = cmd ? cmd->m_random_seed : -1;
		*reinterpret_cast<int*>(m_stored_variables.prediction_player) = std::uintptr_t(player);
		player->m_pCurrentCommand() = cmd;
		player->m_PlayerCommand() = cmd;

#if defined (HL2_DLL)
		// pull out backchannel data and move this out
		for (int i = 0; i < cmd->entitygroundcontact.Count(); i++)
		{
			int entindex = cmd->entitygroundcontact[i].entindex;
			player_t* pEntity = player_t::Instance(INDEXENT(entindex));
			if (pEntity)
			{
				CBaseAnimating* pAnimating = pEntity->GetBaseAnimating();
				if (pAnimating)
					pAnimating->SetIKGroundContactInfo(cmd->entitygroundcontact[i].minheight, cmd->entitygroundcontact[i].maxheight);
			}
		}
#endif
	}

	// Purpose: We've finished running a user's command
	void FinishCommand(player_t* player)
	{
		if (m_stored_variables.prediction_player && m_stored_variables.prediction_seed)
		{
			*reinterpret_cast<int*>(m_stored_variables.prediction_seed) = 0xffffffff;
			*reinterpret_cast<int*>(m_stored_variables.prediction_player) = 0;
		}

		player->m_pCurrentCommand() = 0;
	}

	//purpose: predicting buttons in realtime
	void UpdateButtonState(player_t* player, CUserCmd* cmd)
	{
#ifdef DEB
		const int original_buttons = cmd->m_buttons; //save cmd buttons
		int* nPlayerButtons = *player->get_buttons(); //get buttons on player
		const int buttons_changed = original_buttons ^ *nPlayerButtons; //different button getted

		player->get_buttons_last() = *nPlayerButtons; // synchronize m_afButtonLast
		player->get_buttons() = original_buttons; // synchronize m_nButtons
		player->get_buttons_pressed() = original_buttons & buttons_changed; // synchronize m_afButtonPressed
		player->get_buttons_released() = buttons_changed & ~original_buttons; // synchronize m_afButtonReleased
#endif

		/*.text:1032126F 8B 86 08 32 00 00                 mov     eax, [esi+3208h]
		.text:10321275 8B C8                             mov     ecx, eax
		.text:10321277 33 CA                             xor     ecx, edx*/
		static auto button_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ? ? ? ? 8B C8 33 CA")) + 0x2);

		/*.text:10321289 F7 D2                             not     edx
		.text:1032128B 89 86 00 32 00 00                 mov     [esi+3200h], eax
		.text:10321291 23 D1                             and     edx, ecx*/
		static auto buttons_pres_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("F7 D2 89 86 ? ? ? ? 23 D1")) + 0x4);

		/*.text:1032129C 89 96 04 32 00 00                 mov     [esi+3204h], edx
		.text:103212A2 F3 0F 10 40 14                    movss   xmm0, dword ptr [eax+14h]
		.text:103212A7 8B 11                             mov     edx, [ecx]*/
		static auto buttons_rel_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("89 96 ? ? ? ? F3 0F 10 40 ? 8B 11")) + 0x2);

		/*.text:10321279 89 86 FC 31 00 00                 mov     [esi+31FCh], eax
		.text:1032127F 8B C1                             mov     eax, ecx
		.text:10321281 89 96 08 32 00 00                 mov     [esi+3208h], edx*/
		static auto button_last_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("89 86 ?? ?? ?? ?? 8B C1 89 96 ?? ?? ?? ??")) + 0x2);

		const int original_buttons = cmd->m_buttons;
		int* player_buttons_last = reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_ptr);
		const int buttons_changed = original_buttons ^ *player_buttons_last;

		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_last_ptr) = *player_buttons_last;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + button_ptr) = original_buttons;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + buttons_pres_ptr) = original_buttons & buttons_changed;
		*reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(player) + buttons_rel_ptr) = buttons_changed & ~original_buttons;
	}

	void BuildDataAfterPredict(player_t* player, CUserCmd* cmd, weapon_t* m_weapon)
	{
		// get correctly max desync player delta
		//animations::calculate_desync_delta();

		// math cmd pitch
		float eye_pitch = math::normalize_pitch(cmd->m_viewangles.x);
		math::clamp(eye_pitch, -90.f, 90.f);

		// build new bones with fixed pitch pose
		//animations::FixLocalPosePitch(g_local, eye_pitch);

		// update weapon accuracy.
		m_weapon->update_accuracy_penality();

		// get actual spread and inaccuracy data
		m_stored_variables.m_flSpread = m_weapon->get_spread();
		m_stored_variables.m_flInaccuracy = m_weapon->get_inaccuracy();
	}

	// Purpose: Save data before prediction
	void BackupData(player_t* player, CUserCmd* cmd)
	{
		//backup data variables
		engineprediction::get().backup_data.flags = m_stored_variables.unpred_flags = m_stored_variables.m_fFlags = player->m_fFlags();
		engineprediction::get().backup_data.velocity = m_stored_variables.unpred_vel = m_stored_variables.m_vecVelocity = player->m_vecVelocity();
		m_stored_variables.m_vecOrigin = player->m_vecOrigin();
		m_stored_variables.tickbase = player->m_nTickBase();
		m_stored_variables.m_in_prediction = m_prediction()->InPrediction;
		m_stored_variables.m_is_first_time_predicted = m_prediction()->IsFirstTimePredicted;
		m_stored_variables.m_iButtons = cmd->m_buttons;

		// backup globals.
		m_stored_variables.m_cur_time = m_globals()->m_curtime;
		m_stored_variables.m_frame_time = m_globals()->m_frametime;
		m_stored_variables.m_fTickcount = m_globals()->m_tickcount;
	}

	void predict(CUserCmd* cmd, player_t* player)
	{
		weapon_t* m_weapon_local = player->m_hActiveWeapon().Get();
		if (!m_engine()->IsInGame() || !player || !m_weapon_local)
			return;

		// set pred cmd
		StartCommand(player, cmd);

		// backup shits
		BackupData(player, cmd);

		// set vars appropriatly 
		m_globals()->m_curtime = TICKS_TO_TIME(globals.g.fixed_tickbase); //TODO: need test //globals.globals.fixed_tickbase = TICKS_TO_TIME(player->m_nTickBase());
		m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.f : m_globals()->m_intervalpertick;
		m_globals()->m_tickcount = globals.g.fixed_tickbase;//player->m_nTickBase(); //TODO: need test

		/* anounce that we are predicting */
		m_prediction()->InPrediction = true;
		m_prediction()->IsFirstTimePredicted = false;

		/*.text:103211A9 8B 86 44 33 00 00                 mov     eax, [esi+3344h]
		.text:103211AF 09 47 30                          or      [edi+30h], eax
		.text:103211B2 8B 86 40 33 00 00                 mov     eax, [esi+3340h]
		.text:103211B8 F7 D0                             not     eax
		.text:103211BA 21 47 30                          and     [edi+30h], eax*/
		static auto bf_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ?? ?? ?? ?? 09 47 30 8B 86 ?? ?? ?? ??")) + 0x2);
		static auto bd_ptr = *reinterpret_cast<std::uint32_t*>(util::FindSignature(crypt_str("client.dll"), crypt_str("8B 86 ? ? ? ? F7 D0 21 47 30")) + 0x2);

		auto buttons_forced = *reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + bf_ptr);
		auto buttons_disabled = *reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + bd_ptr);

		// Add and subtract buttons we're forcing on the player
		cmd->m_buttons |= buttons_forced;
		cmd->m_buttons &= ~buttons_disabled;

		// Do weapon selection
		if (cmd->m_weaponselect)
		{
			if (const auto weapon = reinterpret_cast<weapon_t*>(m_entitylist()->GetClientEntity(cmd->m_weaponselect)))
			{
				if (const auto weapon_data = weapon->get_csweapon_info())
					player->select_item(weapon_data->szWeaponName, cmd->m_weaponsubtype);
			}
		}

		static auto using_standard_weapons_in_vehicle = reinterpret_cast <bool(__thiscall*)(void*)>(util::FindSignature(crypt_str("client.dll"), crypt_str("56 57 8B F9 8B 97 ? ? ? ? 83 FA FF 74 43")));
		const auto vehicle = reinterpret_cast<player_t*>(player->m_hVehicle().Get());
		static int offset_impulse = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_nImpulse"));

		// Latch in impulse.
		if (cmd->m_impulse)
		{
			// Discard impulse commands unless the vehicle allows them.
			// FIXME: UsingStandardWeapons seems like a bad filter for this. The flashlight is an impulse command, for example.
			if (!vehicle || using_standard_weapons_in_vehicle(player))
				*reinterpret_cast<std::uint32_t*>(std::uintptr_t(player) + offset_impulse) = cmd->m_impulse;
		}

		// predicting buttons
		UpdateButtonState(player, cmd);

		// check if the player is standing on a moving entity and adjusts velocity and basevelocity appropriately.
		m_prediction()->CheckMovingGround(player, m_globals()->m_frametime);

		// copy angles from command to player
		m_prediction()->SetLocalViewAngles(cmd->m_viewangles);

		// set target player ( host ).
		m_movehelper()->set_host(player);
		m_gamemovement()->StartTrackPredictionErrors(player);

		// Call standard client pre-think
		player->RunPreThink();

		// Call Think if one is set
		player->RunThink();

		// setup input.
		m_prediction()->SetupMove(player, cmd, m_movehelper(), &m_stored_variables.m_MoveData);

		//// re store move_data
		//m_stored_variables.m_MoveData.m_buttons = cmd->buttons;
		//m_stored_variables.m_MoveData.m_impulse_command = (unsigned __int8)cmd->impulse;
		//m_stored_variables.m_MoveData.m_forward_move = cmd->forwardmove;
		//m_stored_variables.m_MoveData.m_side_move = cmd->sidemove;
		//m_stored_variables.m_MoveData.m_up_move = cmd->upmove;
		//m_stored_variables.m_MoveData.m_angles.x = cmd->viewangles.x;
		//m_stored_variables.m_MoveData.m_angles.y = cmd->viewangles.y;
		//m_stored_variables.m_MoveData.m_angles.z = cmd->viewangles.z;
		//m_stored_variables.m_MoveData.m_view_angles.x = cmd->viewangles.x;
		//m_stored_variables.m_MoveData.m_view_angles.y = cmd->viewangles.y;
		//m_stored_variables.m_MoveData.m_view_angles.z = cmd->viewangles.z;

		// Let the game do the movement.
		if (!vehicle)
			m_gamemovement()->ProcessMovement(player, &m_stored_variables.m_MoveData);
		else
			call_virtual<void(__thiscall*)(void*, player_t*, CMoveData*)>(vehicle, 5)(vehicle, player, &m_stored_variables.m_MoveData);

		// Copy output
		m_prediction()->FinishMove(player, cmd, &m_stored_variables.m_MoveData);

		// Let server invoke any needed impact functions
		m_movehelper()->process_impacts();

		// run post think.
		player->RunPostThink();

		// reset target player ( host ). //этого нет здесь!
		m_gamemovement()->FinishTrackPredictionErrors(player);
		m_movehelper()->set_host(nullptr);
		m_gamemovement()->Reset();

		m_prediction()->IsFirstTimePredicted = m_stored_variables.m_is_first_time_predicted;
		m_prediction()->InPrediction = m_stored_variables.m_in_prediction;

		// update accuracy weapon
		/*m_stored_variables.m_flSpread = FLT_MAX;
		m_stored_variables.m_flInaccuracy = FLT_MAX;
		m_stored_variables.m_flCalculatedInaccuracy = 0.f;
		m_stored_variables.m_flRevolverAccuracy = .0f;*/

		// build correct local player data predicted
		BuildDataAfterPredict(globals.local(), cmd, m_weapon_local);

		auto viewmodel = globals.local()->m_hViewModel().Get();

		if (viewmodel)
		{
			engineprediction::get().viewmodel_data.weapon = viewmodel->m_hWeapon().Get();

			engineprediction::get().viewmodel_data.viewmodel_index = viewmodel->m_nViewModelIndex();
			engineprediction::get().viewmodel_data.sequence = viewmodel->m_nSequence();
			engineprediction::get().viewmodel_data.animation_parity = viewmodel->m_nAnimationParity();

			engineprediction::get().viewmodel_data.cycle = viewmodel->m_flCycle();
			engineprediction::get().viewmodel_data.animation_time = viewmodel->m_flAnimTime();
		}
	}

	void restore(player_t* player)
	{
		if (!m_engine()->IsInGame() || !player || !player->is_alive())
			return;

		// reset player commands
		FinishCommand(player);

		m_globals()->m_curtime = m_stored_variables.m_cur_time;
		m_globals()->m_frametime = m_stored_variables.m_frame_time;
		m_globals()->m_tickcount = m_stored_variables.m_fTickcount;
	}

	//void correct_viewmodel_data()
	//{
	//	if (!m_engine()->IsInGame() || !globals.local() || !globals.local()->is_alive() || globals.local()->m_hViewModel() == 0xFFFFFFFF)
	//		return;

	//	C_BaseViewModel* view_model = reinterpret_cast<C_BaseViewModel*>(i::entitylist->get_client_entity_from_handle((CBaseHandle)globals.local()->m_hViewModel()));
	//	if (!view_model)
	//		return;

	//	// onetap.su
	//	if (m_stored_viewmodel.m_viewmodel_Sequence == view_model->m_nSequence() && m_stored_viewmodel.m_viewmodel_Weapon == view_model->get_viewmodel_weapon()
	//		&& m_stored_viewmodel.m_viewmodel_AnimationParity == view_model->m_nAnimationParity())
	//	{
	//		view_model->m_flCycle() = m_stored_viewmodel.m_viewmodel_cycle;
	//		view_model->m_flModelAnimTime() = m_stored_viewmodel.m_viewmodel_anim_time;
	//		//m_stored_viewmodel.m_viewmodel_update_stage = CYCLE_NONE;
	//	}
	//}

	//void update_viewmodel_data()
	//{
	//	if (!m_engine()->IsInGame() || !globals.local() || !globals.local()->is_alive() || globals.local()->m_hViewModel() == 0xFFFFFFFF)
	//	{
	//		m_stored_viewmodel.m_viewmodel_Weapon = 0;
	//		return;
	//	}

	//	C_BaseViewModel* view_model = reinterpret_cast<C_BaseViewModel*>(i::entitylist->get_client_entity_from_handle((CBaseHandle)globals.local()->m_hViewModel()));
	//	if (!view_model)
	//	{
	//		m_stored_viewmodel.m_viewmodel_Weapon = 0;
	//		return;
	//	}

	//	m_stored_viewmodel.m_viewmodel_Weapon = view_model->get_viewmodel_weapon();
	//	m_stored_viewmodel.m_viewmodel_ViewModelIndex = view_model->m_nViewModelIndex();
	//	m_stored_viewmodel.m_viewmodel_Sequence = view_model->m_nSequence();
	//	m_stored_viewmodel.m_viewmodel_cycle = view_model->m_flCycle();
	//	m_stored_viewmodel.m_viewmodel_AnimationParity = view_model->m_nAnimationParity();
	//	m_stored_viewmodel.m_viewmodel_anim_time = view_model->m_flModelAnimTime();
	//}

	void patch_attack_packet(CUserCmd* cmd, bool restore)
	{
		static bool weaponAnimation = false;
		static float m_flLastCycle = 0.f;

		viewmodel_t* view_model = reinterpret_cast<viewmodel_t*>(m_entitylist()->GetClientEntityFromHandle((CBaseHandle)globals.local()->m_hViewModel()));
		if (!view_model)
			return;

		if (restore)
		{
			weaponAnimation = cmd->m_weaponselect || (cmd->m_buttons & (IN_ATTACK | IN_ATTACK2));
			m_flLastCycle = view_model->m_viewmodel_cycle;
		}
		else if (weaponAnimation /*&& m_stored_viewmodel.m_viewmodel_update_stage == CYCLE_NONE*/)
		{
			if (view_model->m_viewmodel_cycle == 0.f && m_flLastCycle > 0.f)
			{
				view_model->m_viewmodel_cycle = m_flLastCycle;
				//m_stored_viewmodel.m_viewmodel_update_stage = CYCLE_PRE_UPDATE;
			}
		}
	}

	////спиздил у лв, у вт гораздо проще
	//void FixRevolver(CUserCmd* cmd, player_t* player)
	//{
	//	if ((player->m_fFlags() & fl_frozen) || i::m_gamerules->m_bFreezePeriod())
	//		return;

	//	if (!g_core_engine.m_local_player_data.m_weapon || !g_core_engine.m_local_player_data.m_weapon_info)
	//		return;

	//	m_revolver.m_TickRecords[cmd->command_number % MULTIPLAYER_BACKUP] = player->m_nTickBase();
	//	m_revolver.m_FireStates[cmd->command_number % MULTIPLAYER_BACKUP] = player->CanFire();
	//	m_revolver.m_PrimaryAttack[cmd->command_number % MULTIPLAYER_BACKUP] = cmd->buttons & in_attack;

	//	int32_t nFireCommand = 0;
	//	int32_t nLowestCommand = cmd->command_number - (g_core_engine.m_tickrate >> 1);
	//	for (int32_t nAttackCommand = cmd->command_number - 1; nAttackCommand > nLowestCommand; nAttackCommand--)
	//	{
	//		if (!m_revolver.m_FireStates[nAttackCommand % MULTIPLAYER_BACKUP] || !m_revolver.m_PrimaryAttack[nAttackCommand % MULTIPLAYER_BACKUP])
	//			continue;

	//		nFireCommand = nAttackCommand;
	//	}

	//	if (nFireCommand)
	//	{
	//		if (g_core_engine.m_local_player_data.m_weapon->m_iItemDefinitionIndex() == weapon_revolver)
	//			g_core_engine.m_local_player_data.m_weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(m_revolver.m_TickRecords[nFireCommand % MULTIPLAYER_BACKUP]) + 0.2f;
	//	}

	//	// that ot v4
	//	/*if (g_core_engine.m_local_player_data.m_weapon->m_iItemDefinitionIndex() == weapon_revolver)
	//	{
	//		int nLowestCommand = cmd->command_number - (g_core_engine.m_tickrate >> 1);
	//		int nCheckCommand = cmd->command_number - 1;
	//		int nFireCommand = 0;

	//		while (nCheckCommand > nLowestCommand)
	//		{
	//			nFireCommand = nCheckCommand;
	//			if (!m_revolver.m_FireStates[nCheckCommand % MULTIPLAYER_BACKUP])
	//				break;

	//			if (!m_revolver.m_PrimaryAttack[nCheckCommand % MULTIPLAYER_BACKUP])
	//				break;

	//			nCheckCommand--;
	//		}

	//		if (nFireCommand)
	//		{
	//			int nOffset = 1 - (-0.03348f / i::globalvars->m_interval_per_tick);
	//			if (g_core_engine.m_local_player_data.m_weapon->m_iItemDefinitionIndex() == weapon_revolver)
	//			{
	//				if (cmd->command_number - nFireCommand >= nOffset)
	//					g_core_engine.m_local_player_data.m_weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(m_revolver.m_TickRecords[(nFireCommand + nOffset) % MULTIPLAYER_BACKUP]) + 0.2f;
	//			}
	//		}
	//	}*/
	//}

	void UpdateVelocityModifier()
	{
		if (!globals.local() || !globals.local()->is_alive() || !m_clientstate())
			return;

		static int m_iLastCmdAck = 0;
		static float m_flNextCmdTime = 0.f;

		if (m_iLastCmdAck != m_clientstate()->nLastCommandAck || m_flNextCmdTime != m_clientstate()->flNextCmdTime)
		{
			if (m_stored_variables.m_flVelocityModifier != globals.local()->m_flVelocityModifier())
			{
				*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = true;
				m_stored_variables.m_flVelocityModifier = globals.local()->m_flVelocityModifier();
			}

			m_iLastCmdAck = m_clientstate()->nLastCommandAck;
			m_flNextCmdTime = m_clientstate()->flNextCmdTime;
		}
	}

	void ProcessInterpolation(bool bPostFrame)
	{
		if (!globals.local() || !globals.local()->is_alive())
			return;

		auto net_channel_info = m_engine()->GetNetChannelInfo();
		if (!net_channel_info)
			return;

		auto outgoing = net_channel_info->GetLatency(FLOW_OUTGOING);
		auto incoming = net_channel_info->GetLatency(FLOW_INCOMING);

		// from lovenca
		/*int serverTickcount = g_core_engine.m_fake_duck ? (14 - i::clientstate->m_choked_commands) : i::globalvars->m_tick_count;
		serverTickcount += (g_core_engine.m_latency / g_csgo.m_globals->m_interval) + 3;
		g_local->m_nFinalPredictedTick() = serverTickcount;*/

		if (!bPostFrame)
		{
			// get backup final predicted tick
			m_stored_variables.m_FinalPredictedTick = globals.local()->m_nFinalPredictedTick();

			// set actualy final predicted tick
			int m_iTickCount = globals.g.fakeducking ? (14 - m_clientstate()->iChokedCommands) : m_globals()->m_tickcount;
			globals.local()->m_nFinalPredictedTick() = m_iTickCount + TIME_TO_TICKS(outgoing /*+ incoming*/);
			return;
		}

		// restore final predicted tick
		globals.local()->m_nFinalPredictedTick() = m_stored_variables.m_FinalPredictedTick;
	}
}