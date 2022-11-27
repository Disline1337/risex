// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "animfix.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];

void c_anim_fix::fsn(ClientFrameStage_t stage)
{
	if (stage != FRAME_NET_UPDATE_END)
		return;

	if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled)
		return;

	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == globals.local())
			continue;

		if (!valid(i, e))
			continue;

		auto update = e->m_flSimulationTime() != e->m_flOldSimulationTime(); //-V550

		if (update) //-V550
		{
			if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f) // fix break_lc
				for (auto& record : player_records[i])
					record.invalid = true;

			player_records[i].emplace_front(adjust_data());
			update_player_animations(e);

			while (player_records[i].size() > 32) // max size lagcomp ticks
				player_records[i].pop_back();
		}
	}
}

bool c_anim_fix::valid(int i, player_t* e)
{
	if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled || !e->valid(false))
	{
		if (!e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].Reset();

			globals.g.fired_shots[i] = 0;
			globals.g.missed_shots[i] = 0;
		}
		else if (e->IsDormant())
			is_dormant[i] = true;

		player_records[i].clear();
		return false;
	}

	return true;
}

void c_anim_fix::post_anim_update(player_t* player, adjust_data* record)
{
	memcpy(player->get_animlayers(), record->layers, 13 * sizeof AnimationLayer);
	memcpy(player_resolver[record->player->EntIndex()].previous_layers, record->layers, sizeof(AnimationLayer) * 13);
	record->player->get_animlayers()[0]; player_resolver[record->player->EntIndex()].m_flAbsRotationFake = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[0][6].m_flPlaybackRate);
	record->player->get_animlayers()[1]; player_resolver[record->player->EntIndex()].m_flAbsRotationRight = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[1][6].m_flPlaybackRate);
	record->player->get_animlayers()[2]; player_resolver[record->player->EntIndex()].m_flAbsRotationLeft = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[2][6].m_flPlaybackRate);
	record->player->get_animlayers()[3]; player_resolver[record->player->EntIndex()].m_flAbsRotationRightLow = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[3][6].m_flPlaybackRate);
	record->player->get_animlayers()[4]; player_resolver[record->player->EntIndex()].m_flAbsRotationLeftLow = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[4][6].m_flPlaybackRate);
}

void c_anim_fix::pre_anim_update(player_t* player, adjust_data* record)
{
	memcpy(player->get_animlayers(), record->layers, 13 * sizeof AnimationLayer);
	memcpy(player_resolver[record->player->EntIndex()].previous_layers, record->layers, sizeof(AnimationLayer) * 13);
	record->player->get_animlayers()[0]; player_resolver[record->player->EntIndex()].m_flAbsRotationFake = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[0][6].m_flPlaybackRate);
	record->player->get_animlayers()[1]; player_resolver[record->player->EntIndex()].m_flAbsRotationRight = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[1][6].m_flPlaybackRate);
	record->player->get_animlayers()[2]; player_resolver[record->player->EntIndex()].m_flAbsRotationLeft = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[2][6].m_flPlaybackRate);
	record->player->get_animlayers()[3]; player_resolver[record->player->EntIndex()].m_flAbsRotationRightLow = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[3][6].m_flPlaybackRate);
	record->player->get_animlayers()[4]; player_resolver[record->player->EntIndex()].m_flAbsRotationLeftLow = abs(record->layers[6].m_flPlaybackRate - player_resolver[record->player->EntIndex()].resolver_layers[4][6].m_flPlaybackRate);
}

void c_anim_fix::anim_update(player_t* player)
{
	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;
	auto backup_absolute_frametime = m_globals()->m_absoluteframetime;

	auto backup_lower_body_yaw_target = player->m_flLowerBodyYawTarget();
	auto backup_duck_amount = player->m_flDuckAmount();
	auto backup_flags = player->m_fFlags();
	auto backup_eflags = player->m_iEFlags();
	auto backup_sim_time = player->m_flSimulationTime();
	auto backup_pose_parametr = player->m_flPoseParameter();
	auto backup_animlayers = player->get_animlayers();

	auto simulation_ticks = TIME_TO_TICKS(player->m_flSimulationTime() - m_globals()->m_intervalpertick);

	// сетаем клиентского глобалварс в серверный
	m_globals()->m_realtime = backup_sim_time;
	m_globals()->m_curtime = backup_sim_time;
	m_globals()->m_frametime = backup_sim_time;
	m_globals()->m_absoluteframetime = backup_sim_time;
	m_globals()->m_framecount = simulation_ticks;
	m_globals()->m_tickcount = simulation_ticks;
	m_globals()->m_interpolation_amount = 0.0f;

	globals.g.updating_animation = true;
	player->update_clientside_animation();
	globals.g.updating_animation = false;

	m_globals()->m_realtime = backup_realtime;
	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;
	m_globals()->m_absoluteframetime = backup_absolute_frametime;
	m_globals()->m_framecount = backup_framecount;
	m_globals()->m_tickcount = backup_tickcount;
	m_globals()->m_interpolation_amount = backup_interpolation_amount;
	// после апдейта возвращаем обратно
}

__forceinline float c_anim_fix::GetMaxPlayerSpeed(player_t* pl)
{
	auto weapon = pl->m_hActiveWeapon().Get();

	if (weapon)
	{
		auto weaponData = weapon->GetWeaponData();
		auto maxSpeed = weapon->m_weaponMode() == 0 ? weaponData->flMaxPlayerSpeed : weaponData->flMaxPlayerSpeedAlt;
		maxSpeed = std::clamp(maxSpeed * 0.34f, maxSpeed, 1.0f - pl->m_flDuckAmount());
		return maxSpeed;
	}

	return 260.f;
}

void c_anim_fix::update_player_animations(player_t* e)
{
	// сетапим информацию
	auto animstate = e->get_animation_state();

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();

	AnimationLayer animlayers[13];
	float pose_parametrs[24];

	memcpy(pose_parametrs, &e->m_flPoseParameter(), 24 * sizeof(float));
	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();
	auto backup_sim_time = e->m_flSimulationTime();

	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;
	auto backup_absolute_frametime = m_globals()->m_absoluteframetime;

	record->store_data(e, true); // устанавливаем дату - тобеж Lby = LowerBodyYaw()

	if (previous_record) // привёрсим запись
	{
		auto velocity = e->m_vecVelocity();
		auto was_in_air = e->m_fFlags() & FL_ONGROUND && previous_record->flags & FL_ONGROUND;

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[11].m_flWeight > 0.0f && animlayers[11].m_flWeight < 1.0f && animlayers[11].m_flCycle > previous_record->layers[11].m_flCycle)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = std::fmax(GetMaxPlayerSpeed(e), 0.001);
					auto weapon_info = e->m_hActiveWeapon().Get()->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					auto modifier = 0.35f * (1.0f - animlayers[11].m_flWeight);

					if (modifier > 0.0f && modifier < 1.0f)
						animation_speed = max_speed * (modifier + 0.55f);
				}
			}

			if (animation_speed > 0.0f)
			{
				animation_speed /= e->m_vecVelocity().Length2D();

				e->m_vecVelocity().x *= animation_speed;
				e->m_vecVelocity().y *= animation_speed;
			}

			if (records->size() >= 3 && e->m_vecVelocity().Length2D() > 0 && e->m_fFlags() & FL_ONGROUND && record->flags & FL_ONGROUND)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					const auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto real_velocity = record->velocity.Length2D();

					float delta = current_direction - previous_direction;

					if (delta <= 180.0f)
						if (delta <= -180.0f)
							delta = delta + 360;
						else
							delta = delta - 360;

					float v63 = delta * 0.5f + current_direction;

					auto direction = (v63 + 90.f) * 0.017453292f;

					e->m_vecVelocity().x = sinf(direction) * real_velocity;
					e->m_vecVelocity().y = cosf(direction) * real_velocity;
				}
			}

			if (!(record->flags & FL_ONGROUND))
			{
				velocity = (record->origin - previous_record->origin) / record->simulation_time;

				float_t flWeight = 1.0f - record->layers[ANIMATION_LAYER_ALIVELOOP].m_flWeight;

				if (flWeight > 0.0f)
				{
					float_t flPreviousRate = previous_record->layers[ANIMATION_LAYER_ALIVELOOP].m_flPlaybackRate;
					float_t flCurrentRate = record->layers[ANIMATION_LAYER_ALIVELOOP].m_flPlaybackRate;

					if (flPreviousRate == flCurrentRate)
					{
						int32_t iPreviousSequence = previous_record->layers[ANIMATION_LAYER_ALIVELOOP].m_nSequence;
						int32_t iCurrentSequence = record->layers[ANIMATION_LAYER_ALIVELOOP].m_nSequence;

						if (iPreviousSequence == iCurrentSequence)
						{
							float_t flSpeedNormalized = (flWeight / 2.8571432f) + 0.55f;

							if (flSpeedNormalized > 0.0f)
							{
								float_t flSpeed = flSpeedNormalized * e->GetMaxPlayerSpeed();

								if (flSpeed > 0.0f)
								{
									if (velocity.Length2D() > 0.0f)
									{
										velocity.x /= velocity.Length2D() / flSpeed;
										velocity.y /= velocity.Length2D() / flSpeed;
									}
								}
							}
						}
					}
				}

				static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));
				velocity.z -= sv_gravity->GetFloat() * 0.5f * TICKS_TO_TIME(record->simulation_time);
			}
			else
				velocity.z = 0.0f;
		}
	}

	e->m_iEFlags() &= ~0x1800;

	if (e->m_fFlags() & FL_ONGROUND && e->m_vecVelocity().Length() > 0.0f && animlayers[6].m_flWeight <= 0.0f) // проверка на нуливую скорость передвижения противника
		e->m_vecVelocity().Zero();

	e->m_vecAbsVelocity() = e->m_vecVelocity(); // приравнимаем скорость по абс в векторную

	if (is_dormant[e->EntIndex()])
	{
		is_dormant[e->EntIndex()] = false;

		if (e->m_fFlags() & FL_ONGROUND)
		{
			animstate->m_bOnGround = true;
			animstate->m_bInHitGroundAnimation = false;
		}

		animstate->time_since_in_air() = 0.0f;
		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
	}

	c_baseplayeranimationstate state; // тут присваем анимстате
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	if (previous_record) // привёрсим запись
	{
		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));
		memcpy(&e->m_flPoseParameter(), pose_parametrs, 24 * sizeof(float));

		auto ticks_chocked = 1;
		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 32)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			auto land_time = 0.0f;
			auto land_in_cycle = false;
			auto is_landed = false;
			auto on_ground = false;

			if (animlayers[4].m_flCycle < 0.5f && (!(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND)))
			{
				land_time = e->m_flSimulationTime() - animlayers[4].m_flPlaybackRate * animlayers[4].m_flCycle;
				land_in_cycle = land_time >= previous_record->simulation_time;
			}

			auto duck_amount_per_tick = (e->m_flDuckAmount() - previous_record->duck_amount) / ticks_chocked;


			for (auto i = 0; i < ticks_chocked; ++i)
			{
				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				if (duck_amount_per_tick)
					e->m_flDuckAmount() = previous_record->duck_amount + duck_amount_per_tick * (float)i;

				on_ground = e->m_fFlags() & FL_ONGROUND;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				if (on_ground)
					e->m_fFlags() |= FL_ONGROUND;
				else
					e->m_fFlags() &= ~FL_ONGROUND;

				anim_update(e);
			}
		}
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	// проверку на инвалидную физику костей
	e->invalidate_physics_recursive(8);
	e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	pre_anim_update(e, record);
	player_resolver[e->EntIndex()].Initialize(e, record);
	player_resolver[e->EntIndex()].Yaw_Resolver();
	e->m_angEyeAngles().x = player_resolver[e->EntIndex()].Pitch_Resolver();
	anim_update(e);
	post_anim_update(e, record);

	// бекаем наш сетап
	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;

	record->adjust_player(); // то что сетапнули енеми возвращаем на своё место)

	e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
	e->m_flDuckAmount() = backup_duck_amount;
	e->m_fFlags() = backup_flags;
	e->m_iEFlags() = backup_eflags;

	memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(player_resolver[e->EntIndex()].previous_layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	if (e->m_flSimulationTime() < e->m_flOldSimulationTime()) // фикс break_lc
		record->invalid = true;
}

void c_resolver::Initialize(player_t* e, adjust_data* record)
{
	player = e;
	player_record = record;
}

void c_resolver::Reset()
{
	player = nullptr;
	player_record = nullptr;

	fake = false;
}

void c_resolver::Yaw_Resolver()
{
	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(player->EntIndex(), &player_info))
		return;

	if (player_info.fakeplayer || !globals.local()->is_alive() || player->m_iTeamNum() == globals.local()->m_iTeamNum()) //-V807
		return;

	auto animstate = player->get_animation_state();

	if (!animstate)
		return;

	if (fabs(player->m_angEyeAngles().x) > 85.0f)
		fake = true;
	else if (!fake)
		return;

	//detect part
	auto delta = math::normalize_yaw(player->m_angEyeAngles().y - animstate->m_flGoalFeetYaw);

	float delta_en;

	float speed = player->m_vecVelocity().Length();

	auto valid_lby = true;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.f)
		valid_lby = animstate->m_flTimeSinceStartedMoving < 0.22f;

	float delta_move;

	auto delta_eye = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[0][6].m_flPlaybackRate);

	if (delta_eye <= player_record->layers[6].m_flWeight)
		delta_move = delta_eye;
	else
		delta_move = player_record->layers[6].m_flWeight;

	if (delta_move > previous_layers[6].m_flWeight)
		delta_move = previous_layers[6].m_flWeight;

	if (speed < 1.1 && player_record->layers[3].m_flWeight == 0.f && player_record->layers[3].m_flCycle == 0.f)
	{
		if (fabs(delta) > 35.0f && valid_lby)
		{
			if (player->sequence_activity(player_record->layers[3].m_nSequence) == 979)
			{
				if (globals.g.missed_shots[player->EntIndex()])
					delta = -delta;
			}
			else
			{
				delta_en = delta;
			}
		}
	}
	else if (speed > 1.1 && abs(player_record->layers[6].m_flWeight * 1000.0) == abs(previous_layers[6].m_flWeight * 1000.0))
	{

		auto LastDelta = m_flAbsRotationFake;

		if (m_flAbsRotationFake * 1000.f)
			math::normalize_yaw(player->m_angEyeAngles().y);

		if ((!(m_flAbsRotationRight * 1000.f) || !(m_flAbsRotationRightLow * 1000.f)) && (LastDelta >= m_flAbsRotationRight || LastDelta >= m_flAbsRotationRightLow))
		{
			delta_en = math::normalize_yaw(player->m_angEyeAngles().y - 30.0f);
			LastDelta = m_flAbsRotationRight;
		}
		if ((!(m_flAbsRotationLeft * 1000.f) || !(m_flAbsRotationLeftLow * 1000.f)) && (LastDelta >= m_flAbsRotationLeft || LastDelta >= m_flAbsRotationLeftLow))
		{
			delta_en = math::normalize_yaw(player->m_angEyeAngles().y + 30.0f);
			LastDelta = m_flAbsRotationLeft;
		}
		else
		{
			delta_en = math::normalize_yaw(player->m_angEyeAngles().y);
			LastDelta = m_flAbsRotationFake;
		}
	}

	if (player->get_animation_state()->m_flFeetYawRate >= 0.01f && player->get_animation_state()->m_flFeetYawRate <= 0.8f) // slow walk nigga
	{
		if (player->get_animation_state()->m_flGoalFeetYaw != player->get_animation_state()->m_flCurrentFeetYaw)
			player->get_animation_state()->m_flGoalFeetYaw = player->get_animation_state()->m_flCurrentFeetYaw;
	}

	animstate->m_flGoalFeetYaw = delta_en;
}

float c_resolver::Pitch_Resolver()
{
	return player->m_angEyeAngles().x;
}