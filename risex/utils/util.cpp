// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "util.hpp"
#include "..\features\visuals\player_esp.h"
#include "..\features\lagcompensation\animfix.h"
#include "..\features\misc\misc.h"
#include <thread>

#define INRANGE(x, a, b) (x >= a && x <= b)  //-V1003
#define GETBITS(x) (INRANGE((x & (~0x20)),'A','F') ? ((x & (~0x20)) - 'A' + 0xA) : (INRANGE(x, '0', '9') ? x - '0' : 0)) //-V1003
#define GETBYTE(x) (GETBITS(x[0]) << 4 | GETBITS(x[1]))

namespace util
{
	int epoch_time()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	uintptr_t find_pattern(const char* module_name, const char* pattern, const char* mask)
	{
		MODULEINFO module_info = {};
		K32GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(module_name), &module_info, sizeof(MODULEINFO));
		const auto address = reinterpret_cast<std::uint8_t*>(module_info.lpBaseOfDll);
		const auto size = module_info.SizeOfImage;
		std::vector < std::pair < std::uint8_t, bool>> signature;
		for (auto i = 0u; mask[i]; i++)
			signature.emplace_back(std::make_pair(pattern[i], mask[i] == 'x'));
		auto ret = std::search(address, address + size, signature.begin(), signature.end(),
			[](std::uint8_t curr, std::pair<std::uint8_t, bool> curr_pattern)
		{
			return (!curr_pattern.second) || curr == curr_pattern.first;
		});
		return ret == address + size ? 0 : std::uintptr_t(ret);
	}

	uint64_t FindSignature(const char* szModule, const char* szSignature)
	{
		MODULEINFO modInfo;
		GetModuleInformation(GetCurrentProcess(), GetModuleHandle(szModule), &modInfo, sizeof(MODULEINFO));

		uintptr_t startAddress = (DWORD)modInfo.lpBaseOfDll; //-V101 //-V220
		uintptr_t endAddress = startAddress + modInfo.SizeOfImage;

		const char* pat = szSignature;
		uintptr_t firstMatch = 0;

		for (auto pCur = startAddress; pCur < endAddress; pCur++)
		{
			if (!*pat)
				return firstMatch;

			if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == GETBYTE(pat))
			{
				if (!firstMatch)
					firstMatch = pCur;

				if (!pat[2])
					return firstMatch;

				if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
					pat += 3;
				else
					pat += 2;
			}
			else
			{
				pat = szSignature;
				firstMatch = 0;
			}
		}

		return 0;
	}

	bool visible(const Vector& start, const Vector& end, entity_t* entity, player_t* from)
	{
		trace_t trace;

		Ray_t ray;
		ray.Init(start, end);

		CTraceFilter filter;
		filter.pSkip = from;

		globals.g.autowalling = true;
		m_trace()->TraceRay(ray, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);
		globals.g.autowalling = false;

		return trace.hit_entity == entity || trace.fraction == 1.0f; //-V550
	}

	bool is_button_down(int code)
	{
		if (code <= KEY_NONE || code >= KEY_MAX)
			return false;

		if (!m_engine()->IsActiveApp())
			return false;

		if (m_engine()->Con_IsVisible())
			return false;

		static auto cl_mouseenable = m_cvar()->FindVar(crypt_str("cl_mouseenable"));

		if (!cl_mouseenable->GetBool())
			return false;

		return m_inputsys()->IsButtonDown((ButtonCode_t)code);
	}

	void FixMove(CUserCmd* cmd, Vector& wish_angles) {
		if (globals.local() == nullptr || cmd == nullptr/*csgo.m_client_state() && ctx.last_autostop_tick == csgo.m_client_state()->m_clockdrift_manager.m_nServerTick*/)
			return;

		Vector  move, dir;
		float   delta, len;
		Vector move_angle;

		// roll nospread fix.
		if (!(globals.local()->m_fFlags() & FL_ONGROUND) && cmd->m_viewangles.z != 0 && cmd->m_buttons & IN_ATTACK)
			cmd->m_sidemove = 0;

		// convert movement to vector.
		move = { cmd->m_forwardmove, cmd->m_sidemove, 0 };

		// get move length and ensure we're using a unit vector ( vector with length of 1 ).
		len = move.Normalize();

		if (!len)
			return;

		// convert move to an angle.
		math::vector_angles(move, move_angle);

		// calculate yaw delta.
		delta = (cmd->m_viewangles.y - wish_angles.y);

		// accumulate yaw delta.
		move_angle.y += delta;

		// calculate our new move direction.
		// dir = move_angle_forward * move_length
		math::angle_vectors(move_angle, dir);

		// scale to og movement.
		dir *= len;

		// fix ladder and noclip.
		if (globals.local()->get_move_type() == MOVETYPE_LADDER) {
			// invert directon for up and down.
			if (cmd->m_viewangles.x >= 45 && wish_angles.x < 45 && std::abs(delta) <= 65)
				dir.x = -dir.x;

			// write to movement.
			cmd->m_forwardmove = dir.x;
			cmd->m_sidemove = dir.y;

			// set new button flags.
			if (cmd->m_forwardmove > 200)
				cmd->m_buttons |= IN_FORWARD;

			else if (cmd->m_forwardmove < -200)
				cmd->m_buttons |= IN_BACK;

			if (cmd->m_sidemove > 200)
				cmd->m_buttons |= IN_MOVERIGHT;

			else if (cmd->m_sidemove < -200)
				cmd->m_buttons |= IN_MOVELEFT;
		}

		// we are moving normally.
		else {
			// we must do this for pitch angles that are out of bounds.
			if (cmd->m_viewangles.x < -90 || cmd->m_viewangles.x > 90)
				dir.x = -dir.x;

			// set move.
			cmd->m_forwardmove = dir.x;
			cmd->m_sidemove = dir.y;

			/*	if (Engine::Movement::Instance()->did_force) {

					cmd->forwardmove = Math::clamp(Engine::Movement::Instance()->forcemovement.x, -450.f, 450.f);
					cmd->sidemove = Math::clamp(Engine::Movement::Instance()->forcemovement.y, -450.f, 450.f);
					Engine::Movement::Instance()->did_force = false;
				}*/
		}

		cmd->m_forwardmove = math::clamp(cmd->m_forwardmove, -450.f, 450.f);
		cmd->m_sidemove = math::clamp(cmd->m_sidemove, -450.f, 450.f);
		cmd->m_upmove = math::clamp(cmd->m_upmove, -320.f, 320.f);
	}

	void movement_fix(Vector& wish_angle, CUserCmd* m_pcmd)
	{
#ifdef VIRTUALIZER
#endif // VIRTUALIZER
		Vector wish_forward, wish_right, wish_up, cmd_forward, cmd_right, cmd_up;

		auto viewangles = m_pcmd->m_viewangles;
		auto movedata = Vector(m_pcmd->m_forwardmove, m_pcmd->m_sidemove, m_pcmd->m_upmove);
		viewangles.Normalize();

		if (!(globals.local()->m_fFlags() & FL_ONGROUND) && viewangles.z != 0.f)
			movedata.y = 0.f;

		math::angle_vectors(wish_angle, &wish_forward, &wish_right, &wish_up);
		math::angle_vectors(viewangles, &cmd_forward, &cmd_right, &cmd_up);

		const auto v8 = sqrtf(wish_forward.x * wish_forward.x + wish_forward.y * wish_forward.y),
			v10 = sqrtf(wish_right.x * wish_right.x + wish_right.y * wish_right.y),
			v12 = sqrtf(wish_up.z * wish_up.z);

		Vector wish_forward_norm(1.0f / v8 * wish_forward.x, 1.0f / v8 * wish_forward.y, 0.f),
			wish_right_norm(1.0f / v10 * wish_right.x, 1.0f / v10 * wish_right.y, 0.f),
			wish_up_norm(0.f, 0.f, 1.0f / v12 * wish_up.z);

		const auto v14 = sqrtf(cmd_forward.x * cmd_forward.x + cmd_forward.y * cmd_forward.y),
			v16 = sqrtf(cmd_right.x * cmd_right.x + cmd_right.y * cmd_right.y),
			v18 = sqrtf(cmd_up.z * cmd_up.z);

		Vector cmd_forward_norm(1.0f / v14 * cmd_forward.x, 1.0f / v14 * cmd_forward.y, 1.0f / v14 * 0.0f),
			cmd_right_norm(1.0f / v16 * cmd_right.x, 1.0f / v16 * cmd_right.y, 1.0f / v16 * 0.0f),
			cmd_up_norm(0.f, 0.f, 1.0f / v18 * cmd_up.z);

		const auto v22 = wish_forward_norm.x * movedata.x,
			v26 = wish_forward_norm.y * movedata.x,
			v28 = wish_forward_norm.z * movedata.x,
			v24 = wish_right_norm.x * movedata.y,
			v23 = wish_right_norm.y * movedata.y,
			v25 = wish_right_norm.z * movedata.y,
			v30 = wish_up_norm.x * movedata.z,
			v27 = wish_up_norm.z * movedata.z,
			v29 = wish_up_norm.y * movedata.z;

		Vector correct_movement = ZERO;

		correct_movement.x = cmd_forward_norm.x * v24 + cmd_forward_norm.y * v23 + cmd_forward_norm.z * v25
			+ (cmd_forward_norm.x * v22 + cmd_forward_norm.y * v26 + cmd_forward_norm.z * v28)
			+ (cmd_forward_norm.y * v30 + cmd_forward_norm.x * v29 + cmd_forward_norm.z * v27);

		correct_movement.y = cmd_right_norm.x * v24 + cmd_right_norm.y * v23 + cmd_right_norm.z * v25
			+ (cmd_right_norm.x * v22 + cmd_right_norm.y * v26 + cmd_right_norm.z * v28)
			+ (cmd_right_norm.x * v29 + cmd_right_norm.y * v30 + cmd_right_norm.z * v27);

		correct_movement.z = cmd_up_norm.x * v23 + cmd_up_norm.y * v24 + cmd_up_norm.z * v25
			+ (cmd_up_norm.x * v26 + cmd_up_norm.y * v22 + cmd_up_norm.z * v28)
			+ (cmd_up_norm.x * v30 + cmd_up_norm.y * v29 + cmd_up_norm.z * v27);

		/*if (did_force)
		{
			correct_movement.x = forcemovement.x;
			correct_movement.y = forcemovement.y;
			did_force = false;
		}*/

		FixMove(m_pcmd, wish_angle);

		correct_movement.x = math::clamp(correct_movement.x, -450.f, 450.f);
		correct_movement.y = math::clamp(correct_movement.y, -450.f, 450.f);
		correct_movement.z = math::clamp(correct_movement.z, -320.f, 320.f);

		m_pcmd->m_forwardmove = correct_movement.x;
		m_pcmd->m_sidemove = correct_movement.y;
		m_pcmd->m_upmove = correct_movement.z;
	}

	unsigned int find_in_datamap(datamap_t* map, const char *name)
	{
		while (map)
		{
			for (auto i = 0; i < map->dataNumFields; ++i)
			{
				if (!map->dataDesc[i].fieldName)
					continue;

				if (!strcmp(name, map->dataDesc[i].fieldName))
					return map->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];

				if (map->dataDesc[i].fieldType == FIELD_EMBEDDED)
				{
					if (map->dataDesc[i].td)
					{
						unsigned int offset;

						if (offset = find_in_datamap(map->dataDesc[i].td, name))
							return offset;
					}
				}
			}

			map = map->baseMap;
		}

		return 0;
	}

	bool get_bbox(entity_t* e, Box& box, bool player_esp)
	{
		auto collideable = e->GetCollideable();
		auto m_rgflCoordinateFrame = e->m_rgflCoordinateFrame();

		auto min = collideable->OBBMins();
		auto max = collideable->OBBMaxs();

		Vector points[8] =
		{
			Vector(min.x, min.y, min.z),
			Vector(min.x, max.y, min.z),
			Vector(max.x, max.y, min.z),
			Vector(max.x, min.y, min.z),
			Vector(max.x, max.y, max.z),
			Vector(min.x, max.y, max.z),
			Vector(min.x, min.y, max.z),
			Vector(max.x, min.y, max.z)
		};

		Vector pointsTransformed[8];

		for (auto i = 0; i < 8; i++)
			math::vector_transform(points[i], m_rgflCoordinateFrame, pointsTransformed[i]);

		Vector pos = e->GetAbsOrigin();
		Vector flb;
		Vector brt;
		Vector blb;
		Vector frt;
		Vector frb;
		Vector brb;
		Vector blt;
		Vector flt;

		auto bFlb = math::world_to_screen(pointsTransformed[3], flb);
		auto bBrt = math::world_to_screen(pointsTransformed[5], brt);
		auto bBlb = math::world_to_screen(pointsTransformed[0], blb);
		auto bFrt = math::world_to_screen(pointsTransformed[4], frt);
		auto bFrb = math::world_to_screen(pointsTransformed[2], frb);
		auto bBrb = math::world_to_screen(pointsTransformed[1], brb);
		auto bBlt = math::world_to_screen(pointsTransformed[6], blt);
		auto bFlt = math::world_to_screen(pointsTransformed[7], flt);

		if (!bFlb && !bBrt && !bBlb && !bFrt && !bFrb && !bBrb && !bBlt && !bFlt)
			return false;

		Vector arr[8] =
		{
			flb,
			brt,
			blb,
			frt,
			frb,
			brb,
			blt,
			flt
		};

		auto left = flb.x;
		auto top = flb.y;
		auto right = flb.x;
		auto bottom = flb.y;

		for (auto i = 1; i < 8; i++)
		{
			if (left > arr[i].x)
				left = arr[i].x;
			if (top < arr[i].y)
				top = arr[i].y;
			if (right < arr[i].x)
				right = arr[i].x;
			if (bottom > arr[i].y)
				bottom = arr[i].y;
		}

		box.x = left;
		box.y = bottom;
		box.w = right - left;
		box.h = top - bottom;

		return true;
	}

	void trace_line(Vector& start, Vector& end, unsigned int mask, CTraceFilter* filter, CGameTrace* tr)
	{
		Ray_t ray;
		ray.Init(start, end);

		m_trace()->TraceRay(ray, mask, filter, tr);
	}

	void clip_trace_to_players(IClientEntity* e, const Vector& start, const Vector& end, unsigned int mask, CTraceFilter* filter, CGameTrace* tr)
	{
		Vector mins = e->GetCollideable()->OBBMins(), maxs = e->GetCollideable()->OBBMaxs();

		Vector dir(end - start);
		dir.Normalize();

		Vector
			center = (maxs + mins) / 2,
			pos(center + e->GetAbsOrigin());

		Vector to = pos - start;
		float range_along = dir.Dot(to);

		float range;
		if (range_along < 0.f)
			range = -to.Length();

		else if (range_along > dir.Length())
			range = -(pos - end).Length();

		else {
			auto ray(pos - (dir * range_along + start));
			range = ray.Length();
		}

		if (range <= 60.f) {
			trace_t trace;

			Ray_t ray;
			ray.Init(start, end);

			m_trace()->ClipRayToEntity(ray, mask, e, &trace);

			if (tr->fraction > trace.fraction)
				*tr = trace;
		}
	}

	void RotateMovement(CUserCmd* cmd, float yaw)
	{
		Vector viewangles;
		m_engine()->GetViewAngles(viewangles);

		float rotation = DEG2RAD(viewangles.y - yaw);

		float cos_rot = cos(rotation);
		float sin_rot = sin(rotation);

		float new_forwardmove = cos_rot * cmd->m_forwardmove - sin_rot * cmd->m_sidemove;
		float new_sidemove = sin_rot * cmd->m_forwardmove + cos_rot * cmd->m_sidemove;

		cmd->m_forwardmove = new_forwardmove;
		cmd->m_sidemove = new_sidemove;
	}

	void color_modulate(float color[3], IMaterial* material)
	{
		auto found = false;
		auto var = material->FindVar(crypt_str("$envmaptint"), &found);

		if (found)
			var->set_vec_value(color[0], color[1], color[2]);

		m_renderview()->SetColorModulation(color[0], color[1], color[2]);
	}

	bool get_backtrack_matrix(player_t* e, matrix3x4_t* matrix)
	{
		if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled)
			return false;

		auto nci = m_engine()->GetNetChannelInfo();

		if (!nci)
			return false;

		auto i = e->EntIndex();

		if (i < 1 || i > 64)
			return false;

		auto records = &player_records[i]; //-V826

		if (records->size() < 2)
			return false;

		for (auto record = records->rbegin(); record != records->rend(); ++record)
		{
			if (!record->valid())
				continue;

			if (record->origin.DistTo(e->GetAbsOrigin()) < 1.0f)
				return false;

			auto curtime = m_globals()->m_curtime;
			auto range = 0.2f;

			if (globals.local()->is_alive())
				curtime = TICKS_TO_TIME(globals.g.fixed_tickbase);
			
			auto next_record = record + 1;
			auto end = next_record == records->rend();

			auto next = end ? e->GetAbsOrigin() : next_record->origin;
			auto time_next = end ? e->m_flSimulationTime() : next_record->simulation_time;

			auto correct = nci->GetLatency(FLOW_OUTGOING) + nci->GetLatency(FLOW_INCOMING) + util::get_interpolation();
			auto time_delta = time_next - record->simulation_time;

			auto add = end ? range : time_delta;
			auto deadtime = record->simulation_time + correct + add;
			auto delta = deadtime - curtime;

			auto mul = 1.0f / add;
			auto lerp = math::lerp(next, record->origin, math::clamp(delta * mul, 0.0f, 1.0f));

			matrix3x4_t result[MAXSTUDIOBONES];
			memcpy(result, record->matrixes_data.main, MAXSTUDIOBONES * sizeof(matrix3x4_t));

			for (auto j = 0; j < MAXSTUDIOBONES; j++)
			{
				auto matrix_delta = math::matrix_get_origin(record->matrixes_data.main[j]) - record->origin;
				math::matrix_set_origin(matrix_delta + lerp, result[j]);
			}

			memcpy(matrix, result, MAXSTUDIOBONES * sizeof(matrix3x4_t));
			return true;
		}

		return false;
	}

	void create_state(c_baseplayeranimationstate* state, player_t* e)
	{
		using Fn = void(__thiscall*)(c_baseplayeranimationstate*, player_t*);
		static auto fn = reinterpret_cast <Fn> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 56 8B F1 B9 ? ? ? ? C7 46")));

		fn(state, e);
	}

	void update_state(c_baseplayeranimationstate* state, const Vector& angles)
	{
		using Fn = void(__vectorcall*)(void*, void*, float, float, float, void*);
		static auto fn = reinterpret_cast <Fn> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24")));

		fn(state, nullptr, 0.0f, angles[1], angles[0], nullptr);
	}

	void reset_state(c_baseplayeranimationstate* state)
	{
		using Fn = void(__thiscall*)(c_baseplayeranimationstate*);
		static auto fn = reinterpret_cast <Fn> (util::FindSignature(crypt_str("client.dll"), crypt_str("56 6A 01 68 ? ? ? ? 8B F1")));

		fn(state);
	}

	void copy_command(CUserCmd* cmd, int tickbase_shift)
	{
		auto commands_to_add = 0;

		static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
		static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

		if (key_binds::get().get_key_bind_state(18))
		{
			if (globals.g.original_forwardmove >= 5.0f)
				cmd->m_forwardmove = -cl_forwardspeed->GetFloat();
			else if (globals.g.original_forwardmove <= -5.0f)
				cmd->m_forwardmove = cl_forwardspeed->GetFloat();

			if (globals.g.original_sidemove >= 5.0f)
				cmd->m_sidemove = -cl_sidespeed->GetFloat();
			else if (globals.g.original_sidemove <= -5.0f)
				cmd->m_sidemove = cl_sidespeed->GetFloat();
		}

		do
		{

			auto sequence_number = commands_to_add + cmd->m_command_number;

			auto command = m_input()->GetUserCmd(sequence_number);
			auto verified_command = m_input()->GetVerifiedUserCmd(sequence_number);

			memcpy(command, cmd, sizeof(CUserCmd)); //-V598

			if (command->m_tickcount != INT_MAX && m_clientstate()->iDeltaTick)
				m_prediction()->Update(m_clientstate()->iDeltaTick, true, m_clientstate()->nLastCommandAck, m_clientstate()->nLastOutgoingCommand + m_clientstate()->iChokedCommands);

			bool v7 = command->m_tickcount != INT_MAX;

			command->m_command_number = sequence_number;
			command->m_predicted = v7;

			++m_clientstate()->iChokedCommands; //-V807

			if (m_clientstate()->pNetChannel)
			{
				++m_clientstate()->pNetChannel->m_nChokedPackets;
				++m_clientstate()->pNetChannel->m_nOutSequenceNr;
			}

			math::normalize_angles(command->m_viewangles);

			memcpy(&verified_command->m_cmd, command, sizeof(CUserCmd)); //-V598
			verified_command->m_crc = command->GetChecksum();

			++commands_to_add;
		} while (commands_to_add != tickbase_shift);

		*(bool*)((uintptr_t)m_prediction() + 0x24) = true;
		*(int*)((uintptr_t)m_prediction() + 0x1C) = 0;
	}

	float get_interpolation()
	{
		static auto cl_interp = m_cvar()->FindVar(crypt_str("cl_interp")); //-V807
		static auto cl_interp_ratio = m_cvar()->FindVar(crypt_str("cl_interp_ratio"));
		static auto sv_client_min_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_min_interp_ratio"));
		static auto sv_client_max_interp_ratio = m_cvar()->FindVar(crypt_str("sv_client_max_interp_ratio"));
		static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
		static auto sv_minupdaterate = m_cvar()->FindVar(crypt_str("sv_minupdaterate"));
		static auto sv_maxupdaterate = m_cvar()->FindVar(crypt_str("sv_maxupdaterate"));

		auto updaterate = math::clamp(cl_updaterate->GetFloat(), sv_minupdaterate->GetFloat(), sv_maxupdaterate->GetFloat());
		auto lerp_ratio = math::clamp(cl_interp_ratio->GetFloat(), sv_client_min_interp_ratio->GetFloat(), sv_client_max_interp_ratio->GetFloat());

		return math::clamp(lerp_ratio / updaterate, cl_interp->GetFloat(), 1.0f);
	}
}