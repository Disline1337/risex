// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ragebot.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include "../visuals/chams.h"

void aim::run(CUserCmd* cmd)
{
	backup.clear();
	targets.clear();
	scanned_targets.clear();
	final_target.reset();
	should_stop = false;

	if (!g_cfg.ragebot.enable)
		return;

	automatic_revolver(cmd);
	prepare_targets();

	if (globals.g.weapon->is_non_aim())
		return;

	if (globals.g.current_weapon == -1)
		return;

	scan_targets();

	if (!should_stop && g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_PREDICTIVE])
	{
		for (auto& target : targets)
		{
			if (!target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data, globals.g.eye_pos);

			if (!last_data.valid())
				continue;

			should_stop = true;
			break;
		}
	}

	if (!automatic_stop(cmd))
		return;

	if (scanned_targets.empty())
		return;

	find_best_target();

	if (!final_target.data.valid())
		return;

	fire(cmd);
}

void aim::automatic_revolver(CUserCmd* cmd)
{
	if (!m_engine()->IsActiveApp())
		return;

	if (globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
		return;

	if (cmd->m_buttons & IN_ATTACK)
		return;

	cmd->m_buttons &= ~IN_ATTACK2;

	static auto r8cock_time = 0.0f;
	auto server_time = TICKS_TO_TIME(globals.g.backup_tickbase);

	if (globals.g.weapon->can_fire(false))
	{
		if (r8cock_time <= server_time)
		{
			if (globals.g.weapon->m_flPostponeFireReadyTime() <= server_time)
				r8cock_time = server_time + TICKS_TO_TIME(14);
			else
				cmd->m_buttons |= IN_ATTACK2;
		}
		else
			cmd->m_buttons |= IN_ATTACK;
	}
	else
	{
		r8cock_time = server_time + TICKS_TO_TIME(14);
		cmd->m_buttons &= ~IN_ATTACK;
	}

	globals.g.revolver_working = true;
}

void aim::prepare_targets()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!e->valid(true, false))
			continue;

		if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
			continue;

		auto records = &player_records[i];

		if (records->empty())
			continue;

		auto record = records->at(0);

		targets.emplace_back(target(e, get_record(records, false), get_record(records, true)));
	}

	for (auto& target : targets)
		backup.emplace_back(adjust_data(target.e));
}

static bool compare_records(const optimized_adjust_data& first, const optimized_adjust_data& second)
{
	auto first_pitch = math::normalize_pitch(first.angles.x);
	auto second_pitch = math::normalize_pitch(second.angles.x);

	if (fabs(first_pitch - second_pitch) > 15.0f)
		return fabs(first_pitch) < fabs(second_pitch);
	else if (first.duck_amount != second.duck_amount)
		return first.duck_amount < second.duck_amount;
	else if (first.origin != second.origin)
		return first.origin.DistTo(globals.local()->GetAbsOrigin()) < second.origin.DistTo(globals.local()->GetAbsOrigin());

	return first.simulation_time > second.simulation_time;
}

adjust_data* aim::get_record(std::deque <adjust_data>* records, bool history)
{
	if (history)
	{
		std::deque <optimized_adjust_data> optimized_records;

		for (auto i = 0; i < records->size(); ++i)
		{
			auto record = &records->at(i);
			optimized_adjust_data optimized_record;

			optimized_record.i = i;
			optimized_record.player = record->player;
			optimized_record.simulation_time = record->simulation_time;
			optimized_record.duck_amount = record->duck_amount;
			optimized_record.angles = record->angles;
			optimized_record.origin = record->origin;

			optimized_records.emplace_back(optimized_record);
		}

		if (optimized_records.size() < 2)
			return nullptr;

		std::sort(optimized_records.begin(), optimized_records.end(), compare_records);

		for (auto& optimized_record : optimized_records)
		{
			auto record = &records->at(optimized_record.i);

			if (!record->valid())
				continue;

			return record;
		}
	}
	else
	{
		for (auto i = 0; i < records->size(); ++i)
		{
			auto record = &records->at(i);

			if (!record->valid())
				continue;

			return record;
		}
	}

	return nullptr;
}

int aim::get_minimum_damage(int health)
{
	auto min_dmg = 1;

	if (key_binds::get().get_key_bind_state(4 + globals.g.current_weapon))
	{
		if (g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage > 100)
			min_dmg = health + g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage - 100;
		else
			min_dmg = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage, 1, health);
	}
	else
	{
		if (g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage > 100)
			min_dmg = health + g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage - 100;
		else
			min_dmg = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage, 1, health);
	}

	return min_dmg;
}

void aim::scan_targets()
{
	if (targets.empty())
		return;

	for (auto& target : targets)
	{
		if (target.history_record->valid())
		{
			scan_data last_data;

			if (target.last_record->valid())
			{
				target.last_record->adjust_player();
				scan(target.last_record, last_data);
			}

			scan_data history_data;

			target.history_record->adjust_player();
			scan(target.history_record, history_data);

			if (last_data.valid() && last_data.damage > history_data.damage)
				scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
			else if (history_data.valid())
				scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
		}
		else
		{
			if (!target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data);

			if (!last_data.valid())
				continue;

			scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
		}
	}
}

bool aim::automatic_stop(CUserCmd* cmd)
{
	if (!should_stop)
		return true;

	if (!g_cfg.ragebot.weapon[globals.g.current_weapon].autostop)
		return true;

	if (globals.g.slowwalking)
		return true;

	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return true;

	if (globals.g.weapon->is_empty())
		return true;

	if (key_binds::get().get_key_bind_state(21))
		return true;

	if (!g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !globals.g.weapon->can_fire(false))
		return true;

	auto animlayer = globals.local()->get_animlayers()[1];

	if (animlayer.m_nSequence)
	{
		auto activity = globals.local()->sequence_activity(animlayer.m_nSequence);

		if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
			return true;
	}

	auto weapon_info = globals.g.weapon->get_csweapon_info();

	if (!weapon_info)
		return true;

	auto max_speed = 0.33f * (globals.g.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

	if (engineprediction::get().backup_data.velocity.Length2D() < max_speed)
		slowwalk::get().create_move(cmd);
	else
	{
		Vector direction;
		Vector real_view;

		math::vector_angles(engineprediction::get().backup_data.velocity, direction);
		m_engine()->GetViewAngles(real_view);

		direction.y = real_view.y - direction.y;

		Vector forward;
		math::angle_vectors(direction, forward);

		static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
		static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

		auto negative_forward_speed = -cl_forwardspeed->GetFloat();
		auto negative_side_speed = -cl_sidespeed->GetFloat();

		auto negative_forward_direction = forward * negative_forward_speed;
		auto negative_side_direction = forward * negative_side_speed;

		cmd->m_forwardmove = negative_forward_direction.x;
		cmd->m_sidemove = negative_side_direction.y;

		if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
			return false;
	}

	return true;
}

static bool compare_points(const scan_point& first, const scan_point& second)
{
	return !first.center && first.hitbox == second.hitbox;
}

void aim::scan(adjust_data* record, scan_data& data, const Vector& shoot_position)
{
	auto weapon = globals.g.weapon;

	if (!weapon)
		return;

	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	auto hitboxes = get_hitboxes(record);

	if (hitboxes.empty())
		return;

	auto best_damage = 0;


	auto get_hitgroup = [](const int& hitbox)
	{
		if (hitbox == HITBOX_HEAD)
			return 0;
		else if (hitbox == HITBOX_PELVIS)
			return 1;
		else if (hitbox == HITBOX_STOMACH)
			return 2;
		else if (hitbox >= HITBOX_LOWER_CHEST && hitbox <= HITBOX_UPPER_CHEST)
			return 3;
		else if (hitbox >= HITBOX_RIGHT_THIGH && hitbox <= HITBOX_LEFT_FOOT)
			return 4;
		else if (hitbox >= HITBOX_RIGHT_HAND && hitbox <= HITBOX_LEFT_FOREARM)
			return 5;

		return -1;
	};

	std::vector <scan_point> points; //-V826

	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox, false);

		for (auto& point : current_points)
		{

			if (point.safe)
				points.emplace_back(point);

		}
	}

	if (points.empty())
		return;

	auto body_hitboxes = true;

	for (auto& point : points)
	{
		if (!point.safe)
		{
			if (g_cfg.ragebot.safe_point_key.holding) {
				continue;
			}
		}

		if (point.hitbox < HITBOX_PELVIS || point.hitbox > HITBOX_UPPER_CHEST)
		{
			if (key_binds::get().get_key_bind_state(22))
				break;

			if (best_damage >= record->player->m_iHealth())
				break;

			if (g_cfg.ragebot.weapon[globals.g.current_weapon].prefer_body_aim && best_damage >= 1)
				break;
		}

		if ((globals.g.eye_pos - final_target.data.point.point).Length() > weapon_info->flRange)
			continue;

		auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player);
		auto minimum_damage = get_minimum_damage(record->player->m_iHealth());

		if (!fire_data.valid)
			continue;

		if (fire_data.damage < 1)
			continue;

		if (!fire_data.visible && !g_cfg.ragebot.autowall)
			continue;

		scan_point best_point;
		scan_data best_data;

		//if (record->player->m_iHealth() < minimum_damage || fire_data.damage < 92)
		//	point.hitbox = HITBOX_STOMACH || HITBOX_CHEST;

		if (g_cfg.ragebot.weapon[globals.g.current_weapon].prefer_safe_points) {
			if (point.safe && point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST && g_cfg.ragebot.weapon[globals.g.current_weapon].prefer_body_aim) {
				if ((fire_data.damage > best_damage && body_hitboxes != point.hitbox) || fire_data.damage > best_damage + 20.f) {
					if (fire_data.damage >= minimum_damage)
					{
						best_damage = best_damage = fire_data.damage;
						best_data.point = point;
						best_data.visible = fire_data.visible;
						best_data.damage = fire_data.damage;
						best_data.hitbox = fire_data.hitbox;
						body_hitboxes = point.hitbox;
					}
				}
			}
		}
		else if (point.hitbox >= HITBOX_PELVIS && point.hitbox <= HITBOX_UPPER_CHEST && g_cfg.ragebot.weapon[globals.g.current_weapon].prefer_body_aim) {
			if ((fire_data.damage > best_damage && body_hitboxes != point.hitbox) || fire_data.damage > best_damage + 20.f) {
				if (fire_data.damage >= minimum_damage)
				{
					best_damage = best_damage = fire_data.damage;
					best_data.point = point;
					best_data.visible = fire_data.visible;
					best_data.damage = fire_data.damage;
					best_data.hitbox = fire_data.hitbox;
					body_hitboxes = point.hitbox;
				}
			}
		}


		if (fire_data.damage >= minimum_damage && fire_data.damage >= best_damage)
		{
			if (!should_stop)
			{
				should_stop = true;

				if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < record->player->m_iHealth())
					should_stop = false;
				else if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_VISIBLE] && !fire_data.visible)
					should_stop = false;
				else if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_CENTER] && !point.center)
					should_stop = false;
			}

			data.point = point;
			data.visible = fire_data.visible;
			data.damage = fire_data.damage;
			data.hitbox = fire_data.hitbox;
		}
	}
}

std::vector <int> aim::get_hitboxes(adjust_data* record)
{
	std::vector <int> hitboxes; //-V827

	auto lethal_if = record->player->m_iHealth() < 92.f;

	bool valid_lethal = false;

	if (lethal_if)
	{
		hitboxes.emplace_back(HITBOX_STOMACH);
		valid_lethal = true;
	}

	if (valid_lethal)
		return hitboxes;

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(1))
		hitboxes.emplace_back(HITBOX_UPPER_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(2))
		hitboxes.emplace_back(HITBOX_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(3))
		hitboxes.emplace_back(HITBOX_LOWER_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(4))
		hitboxes.emplace_back(HITBOX_STOMACH);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(5))
		hitboxes.emplace_back(HITBOX_PELVIS);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(0))
		hitboxes.emplace_back(HITBOX_HEAD);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(6))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
		hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
	}

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(7))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
		hitboxes.emplace_back(HITBOX_LEFT_THIGH);

		hitboxes.emplace_back(HITBOX_RIGHT_CALF);
		hitboxes.emplace_back(HITBOX_LEFT_CALF);
	}

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(8))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
		hitboxes.emplace_back(HITBOX_LEFT_FOOT);
	}

	return hitboxes;
}

std::vector <scan_point> aim::get_points(adjust_data* record, int hitbox, bool from_aim)
{
	std::vector <scan_point> points; //-V827
	auto model = record->player->GetModel();

	if (!model)
		return points;

	auto hdr = m_modelinfo()->GetStudioModel(model);

	if (!hdr)
		return points;

	auto set = hdr->pHitboxSet(record->player->m_nHitboxSet());

	if (!set)
		return points;

	auto bbox = set->pHitbox(hitbox);

	if (!bbox)
		return points;

	auto center = (bbox->bbmin + bbox->bbmax) * 0.5f;

	float flModifier = fmaxf(bbox->radius, 0.f);

	Vector vecMax;
	Vector vecMin;

	math::vector_transform(Vector(bbox->bbmax.x + flModifier, bbox->bbmax.y + flModifier, bbox->bbmax.z + flModifier), record->matrixes_data.main[bbox->bone], vecMax);
	math::vector_transform(Vector(bbox->bbmin.x - flModifier, bbox->bbmin.y - flModifier, bbox->bbmin.z - flModifier), record->matrixes_data.main[bbox->bone], vecMin);

	Vector vecCenter = (vecMin + vecMax) * 0.5f;

	Vector angAngle = math::calculate_angle(globals.g.eye_pos, vecCenter);

	Vector vecForward;
	math::angle_vectors(angAngle, vecForward);

	Vector vecRight = vecForward.Cross(Vector(0, 0, 2.33f));
	Vector vecLeft = Vector(-vecRight.x, -vecRight.y, vecRight.z);

	Vector vecTop = Vector(0, 0, 3.25f);
	Vector vecBottom = Vector(0, 0, -3.25f);

	float iDistanceToPlayer = globals.local()->GetAbsOrigin().DistTo(record->player->m_vecOrigin());

	if (hitbox == HITBOX_HEAD)
	{
		float flScale = g_cfg.ragebot.weapon[globals.g.current_weapon].head_scale;

		points.emplace_back(scan_point(vecCenter, hitbox, false));
		points.emplace_back(scan_point(vecCenter + ((vecTop + vecRight) * (flScale / 75.0f)), hitbox, false));
		points.emplace_back(scan_point(vecCenter + ((vecTop + vecLeft) * (flScale / 75.0f)), hitbox, false));
	}
	else if (hitbox == HITBOX_CHEST)
	{
		float flModifier = 3.05f * (g_cfg.ragebot.weapon[globals.g.current_weapon].body_scale / 80.0f);

		points.emplace_back(scan_point(vecCenter + Vector(0, 0, 3), hitbox, true));
		points.emplace_back(scan_point(vecCenter + vecRight * flModifier + Vector(0, 0, 3), hitbox, false));
		points.emplace_back(scan_point(vecCenter + vecLeft * flModifier + Vector(0, 0, 3), hitbox, false));
	}
	else if (hitbox == HITBOX_STOMACH)
	{
		float flModifier = g_cfg.ragebot.weapon[globals.g.current_weapon].body_scale / 33.0f;

		points.emplace_back(scan_point(vecCenter + Vector(0, 0, 3.0f), hitbox, true));
		points.emplace_back(scan_point(vecCenter + vecRight * flModifier + Vector(0.0f, 0.0f, 3.0f), hitbox, false));
		points.emplace_back(scan_point(vecCenter + vecLeft * flModifier + Vector(0.0f, 0.0f, 3.0f), hitbox, false));
	}
	else if (hitbox == HITBOX_PELVIS)
	{
		float flModifier = g_cfg.ragebot.weapon[globals.g.current_weapon].body_scale / 33.0f;

		points.emplace_back(scan_point(vecCenter - Vector(0.0f, 0.0f, 2.0f), hitbox, true));
		points.emplace_back(scan_point(vecCenter + vecRight * flModifier - Vector(0.0f, 0.0f, 2.0f), hitbox, false));
		points.emplace_back(scan_point(vecCenter + vecLeft * flModifier - Vector(0.0f, 0.0f, 2.0f), hitbox, false));
	}
	else if (hitbox == HITBOX_LEFT_FOOT || hitbox == HITBOX_RIGHT_FOOT || hitbox == HITBOX_LEFT_THIGH || hitbox == HITBOX_RIGHT_THIGH)
	{
		Vector vecAddition = vecLeft;
		if (hitbox == HITBOX_LEFT_FOOT || hitbox == HITBOX_LEFT_THIGH)
			vecAddition = vecRight;
		else if (hitbox == HITBOX_RIGHT_FOOT || hitbox == HITBOX_RIGHT_THIGH)
			vecAddition = vecLeft;

		if (hitbox == HITBOX_LEFT_THIGH || hitbox == HITBOX_RIGHT_THIGH)
			vecCenter -= Vector(0.0f, 0.0f, 2.5f);

		points.emplace_back(scan_point(vecCenter - (vecAddition * 0.90f), hitbox, true));
	}
	else if (hitbox == HITBOX_LEFT_FOREARM || hitbox == HITBOX_RIGHT_FOREARM)
		points.emplace_back(scan_point(vecCenter - (hitbox == HITBOX_LEFT_FOREARM ? vecLeft : -vecLeft), hitbox, false));

	return points;
}

static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
	return first.data.damage > second.data.damage;
}

void aim::OnRoundStart() {
	std::vector <int> hitboxes; //-V827
	std::vector <scan_point> points; //-V827

	hitboxes.clear();
	points.clear();
}

void aim::find_best_target()
{
	std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

	for (auto& target : scanned_targets)
	{
		if (target.fov > (float)g_cfg.ragebot.field_of_view)
			continue;

		final_target = target;
		final_target.record->adjust_player();
		break;
	}
}

bool IsTickValid(float simTime)
{
	static auto cl_interp_ratio = m_cvar()->FindVar(crypt_str("cl_interp_ratio"));
	static auto sv_client_min_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_min_interp_ratio"));
	static auto sv_client_max_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_max_interp_ratio"));
	auto lerp_ratio = math::clamp(cl_interp_ratio->GetFloat(), sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());
	INetChannelInfo* nci = m_engine()->GetNetChannelInfo();

	if (!nci)
		return false;

	auto LerpTicks = TIME_TO_TICKS(lerp_ratio);

	int predCmdArrivTick = m_globals()->m_tickcount + 1 + TIME_TO_TICKS(nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING));

	float flCorrect = math::clamp(lerp_ratio + nci->GetLatency(FLOW_OUTGOING), 0.f, 1.f) - TICKS_TO_TIME(predCmdArrivTick + LerpTicks - (TIME_TO_TICKS(simTime) + TIME_TO_TICKS(lerp_ratio)));

	return abs(flCorrect) < 0.2f;
}

float LagFix()
{
	float updaterate = m_cvar()->FindVar("cl_updaterate")->GetFloat();
	auto minupdate = m_cvar()->FindVar("sv_minupdaterate");
	auto maxupdate = m_cvar()->FindVar("sv_maxupdaterate");

	if (minupdate && maxupdate)
		updaterate = maxupdate->GetFloat();

	float ratio = m_cvar()->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = m_cvar()->FindVar("cl_interp")->GetFloat();
	auto cmin = m_cvar()->FindVar("sv_client_min_interp_ratio");
	auto cmax = m_cvar()->FindVar("sv_client_max_interp_ratio");

	if (cmin && cmax && cmin->GetFloat() != 1)
		ratio = math::clamp(ratio, cmin->GetFloat(), cmax->GetFloat());

	return max(lerp, ratio / updaterate);
}

void aim::fire(CUserCmd* cmd)
{
	if (!globals.g.weapon->can_fire(true))
		return;

	auto aim_angle = math::calculate_angle(globals.g.eye_pos, final_target.data.point.point).Clamp();

	if (!g_cfg.ragebot.silent_aim)
		m_engine()->SetViewAngles(aim_angle);

	if (!g_cfg.ragebot.autoshoot && !(cmd->m_buttons & IN_ATTACK))
		return;

	auto final_hitchance = 0;

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitchance)
	{
		if (!calculate_hitchance(final_hitchance))
		{
			auto is_zoomable_weapon = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AUG || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

			if (g_cfg.ragebot.autoscope && is_zoomable_weapon && !globals.g.weapon->m_zoomLevel())
				cmd->m_buttons |= IN_ZOOM;

			return;
		}
	}

	auto backtrack_ticks = 0;
	auto net_channel_info = m_engine()->GetNetChannelInfo();

	if (net_channel_info)
	{
		auto original_tickbase = globals.g.backup_tickbase;
		auto max_tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 16;

		if (g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && misc::get().double_tap_key)
		{
			if (!globals.local()->m_bGunGameImmunity() && !(globals.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().double_tap_enabled && !globals.g.weapon->is_grenade() && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
			{
				original_tickbase += min(globals.g.weapon->get_max_tickbase_shift(), max_tickbase_shift);
			}
		}

		if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key)
		{
			if (!globals.local()->m_bGunGameImmunity() && !(globals.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().hide_shots_enabled)
			{
				original_tickbase += min(9, max_tickbase_shift);
			}
		}

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + LagFix(), 0.0f, sv_maxunlag->GetFloat());
		auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - final_target.record->simulation_time);

		backtrack_ticks = TIME_TO_TICKS(fabs(delta_time));
	}


	static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
	{
		switch (hitbox)
		{
		case HITBOX_HEAD:
			return shot_info ? crypt_str("Head") : crypt_str("head");
		case HITBOX_LOWER_CHEST:
			return shot_info ? crypt_str("Lower chest") : crypt_str("lower chest");
		case HITBOX_CHEST:
			return shot_info ? crypt_str("Chest") : crypt_str("chest");
		case HITBOX_UPPER_CHEST:
			return shot_info ? crypt_str("Upper chest") : crypt_str("upper chest");
		case HITBOX_STOMACH:
			return shot_info ? crypt_str("Stomach") : crypt_str("stomach");
		case HITBOX_PELVIS:
			return shot_info ? crypt_str("Pelvis") : crypt_str("pelvis");
		case HITBOX_RIGHT_UPPER_ARM:
		case HITBOX_RIGHT_FOREARM:
		case HITBOX_RIGHT_HAND:
			return shot_info ? crypt_str("Left arm") : crypt_str("left arm");
		case HITBOX_LEFT_UPPER_ARM:
		case HITBOX_LEFT_FOREARM:
		case HITBOX_LEFT_HAND:
			return shot_info ? crypt_str("Right arm") : crypt_str("right arm");
		case HITBOX_RIGHT_THIGH:
		case HITBOX_RIGHT_CALF:
			return shot_info ? crypt_str("Left leg") : crypt_str("left leg");
		case HITBOX_LEFT_THIGH:
		case HITBOX_LEFT_CALF:
			return shot_info ? crypt_str("Right leg") : crypt_str("right leg");
		case HITBOX_RIGHT_FOOT:
			return shot_info ? crypt_str("Left foot") : crypt_str("left foot");
		case HITBOX_LEFT_FOOT:
			return shot_info ? crypt_str("Right foot") : crypt_str("right foot");
		}
	};

	player_info_t player_info;
	m_engine()->GetPlayerInfo(final_target.record->i, &player_info);

	cmd->m_viewangles = aim_angle;
	cmd->m_buttons |= IN_ATTACK;
	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + LagFix());

	last_target_index = final_target.record->i;
	last_shoot_position = globals.g.eye_pos;
	last_target[last_target_index] = Last_target
	{
		*final_target.record, final_target.data, final_target.distance
	};

	auto shot = &globals.shots.emplace_back();

	shot->last_target = last_target_index;
	shot->fire_tick = m_globals()->m_tickcount;
	shot->shot_info.target_name = player_info.szName;
	shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
	shot->shot_info.client_damage = final_target.data.damage;
	shot->shot_info.hitchance = math::clamp(final_hitchance, 0, 100);
	shot->shot_info.backtrack_ticks = IsTickValid(final_target.record->simulation_time) ? 22 : TIME_TO_TICKS(fabsf(final_target.record->player->m_flSimulationTime() - final_target.record->simulation_time));
	shot->shot_info.aim_point = final_target.data.point.point;

	chams::get().add_matrix(final_target.record->player, final_target.record->matrixes_data.main);

	globals.g.aimbot_working = true;
	globals.g.revolver_working = false;
	globals.g.last_aimbot_shot = m_globals()->m_tickcount;
}


static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};

__forceinline void aim::build_seed_table()
{
	if (!precomputed_seeds.empty())
		return;

	for (auto i = 0; i < 255; i++) {
		math::random_seed(i + 1);

		const auto pi_seed = math::random_float(0.f, twopi);

		precomputed_seeds.emplace_back(math::random_float(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));
	}
}

bool aim::calculate_hitchance(int& final_hitchance)
{
	// generate look-up-table to enhance performance.
	build_seed_table();

	const auto info = globals.g.weapon->get_csweapon_info();

	if (!info)
	{
		final_hitchance = 0;
		return true;
	}

	const auto hitchance_cfg = g_cfg.ragebot.weapon[globals.g.current_weapon].hitchance_amount;

	// performance optimization.
	if ((globals.g.eye_pos - final_target.data.point.point).Length() > info->flRange)
	{
		final_hitchance = 0;
		return true;
	}

	static auto nospread = m_cvar()->FindVar(crypt_str("weapon_accuracy_nospread"));

	if (nospread->GetBool())
	{
		final_hitchance = INT_MAX;
		return true;
	}

	// setup calculation parameters.
	const auto round_acc = [](const float accuracy) { return roundf(accuracy * 1000.f) / 1000.f; };
	const auto sniper = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1
		|| globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;
	const auto crouched = globals.local()->m_fFlags() & FL_DUCKING;
	const auto on_ground = globals.local()->m_fFlags() & FL_ONGROUND;

	// calculate inaccuracy.
	const auto weapon_inaccuracy = globals.g.weapon->get_inaccuracy();

	// no need for hitchance, if we can't increase it anyway.
	if (crouched)
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyCrouchAlt : info->flInaccuracyCrouch))
		{
			final_hitchance = INT_MAX;
			return true;
		}
	}

	// calculate start and angle.
	static auto weapon_recoil_scale = m_cvar()->FindVar(crypt_str("weapon_recoil_scale"));
	const auto aim_angle = math::calculate_angle(globals.g.eye_pos, final_target.data.point.point).Clamp();
	auto forward = ZERO;
	auto right = ZERO;
	auto up = ZERO;

	math::angle_vectors(aim_angle, &forward, &right, &up);

	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	// keep track of all traces that hit the enemy.
	auto current = 0;

	// setup calculation parameters.
	Vector total_spread, spread_angle, end;
	float inaccuracy, spread_x, spread_y;
	std::tuple<float, float, float>* seed;

	// use look-up-table to find average hit probability.
	for (auto i = 0u; i < 256; i++)  // NOLINT(modernize-loop-convert)
	{
		// get seed.
		seed = &precomputed_seeds[i];

		// calculate spread.
		inaccuracy = std::get<0>(*seed) * weapon_inaccuracy;
		spread_x = std::get<2>(*seed) * inaccuracy;
		spread_y = std::get<1>(*seed) * inaccuracy;
		total_spread = (forward + right * spread_x + up * spread_y);
		total_spread.Normalize();

		// calculate angle with spread applied.
		math::vector_angles(total_spread, spread_angle);

		// calculate end point of trace.
		math::angle_vectors(spread_angle, end);
		end.Normalize();
		end = globals.g.eye_pos + end * info->flRange;

		if (hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, globals.g.eye_pos, end))
			++current;

		// abort if hitchance is already sufficent.
		if ((static_cast<float>(current) / 256.f) * 100.f >= hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current) / 256.f) * 100.f;
			return true;
		}

		// abort if we can no longer reach hitchance.
		if ((static_cast<float>(current + 256.f - i) / 256.f) * 100.f < hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current + 256.f - i) / 256.f) * 100.f;
			return false;
		}
	}

	final_hitchance = (static_cast<float>(current) / 256.f) * 100.f;
	return (static_cast<float>(current) / 256.f) * 100.f >= hitchance_cfg;
}

static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
	static auto fn = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 F3 0F 10 42"));

	trace.fraction = 1.0f;
	trace.startsolid = false;

	return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool aim::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe)
{
	auto model = e->GetModel();

	if (!model)
		return false;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return false;

	auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!studio_set)
		return false;

	auto studio_hitbox = studio_set->pHitbox(hitbox);

	if (!studio_hitbox)
		return false;

	trace_t trace;

	Ray_t ray;
	ray.Init(start, end);

	auto intersected = clip_ray_to_hitbox(ray, studio_hitbox, matrix[studio_hitbox->bone], trace) >= 0;

	if (!safe)
		return intersected;

	Vector min, max;

	math::vector_transform(studio_hitbox->bbmin, matrix[studio_hitbox->bone], min);
	math::vector_transform(studio_hitbox->bbmax, matrix[studio_hitbox->bone], max);

	auto center = (min + max) * 0.5f;
	auto distance = center.DistTo(end);

	if (distance > *safe)
		*safe = distance;

	return intersected;
}