#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

enum
{
	MAIN,
	NONE,
	FIRST,
	SECOND
};

enum animstate_layer_t
{
	ANIMATION_LAYER_AIMMATRIX = 0, // matrix that be aimed
	ANIMATION_LAYER_WEAPON_ACTION, // defusing bomb / reloading / ducking / planting bomb / throwing grenade
	ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,// ducking && defusing bomb / ducking && reloading / ducking && ducking / ducking && planting bomb / ducking && throwing grenade
	ANIMATION_LAYER_ADJUST, // breaking lowerbody yaw
	ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, // jumping or falling / landing
	ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, // landing / climb
	ANIMATION_LAYER_MOVEMENT_MOVE, // moving
	ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, // strafing
	ANIMATION_LAYER_WHOLE_BODY, // whole body hitbox adjusting
	ANIMATION_LAYER_FLASHED, // player flashed
	ANIMATION_LAYER_FLINCH, // player flinching // flicking lby
	ANIMATION_LAYER_ALIVELOOP, // player alive
	ANIMATION_LAYER_LEAN, // body lean
	ANIMATION_LAYER_COUNT, // layers count
};

enum e_pose_param {
	POSE_PARAM_STRAFE_YAW,
	POSE_PARAM_STAND,
	POSE_PARAM_LEAN_YAW,
	POSE_PARAM_SPEED,
	POSE_PARAM_LADDER_YAW,
	POSE_PARAM_LADDER_SPEED,
	POSE_PARAM_JUMP_FALL,
	POSE_PARAM_MOVE_YAW,
	POSE_PARAM_MOVE_BLEND_CROUCH,
	POSE_PARAM_MOVE_BLEND_WALK,
	POSE_PARAM_MOVE_BLEND_RUN,
	POSE_PARAM_BODY_YAW,
	POSE_PARAM_BODY_PITCH,
	POSE_PARAM_AIM_BLEND_STAND_IDLE,
	POSE_PARAM_AIM_BLEND_STAND_WALK,
	POSE_PARAM_AIM_BLEND_STAND_RUN,
	POSE_PARAM_AIM_BLEND_COURCH_IDLE,
	POSE_PARAM_AIM_BLEND_CROUCH_WALK,
	POSE_PARAM_DEATH_YAW
};

enum animtag
{
	ANIMTAG_UNINITIALIZED,
	ANIMTAG_STARTCYCLE_N,
	ANIMTAG_STARTCYCLE_NE,
	ANIMTAG_STARTCYCLE_E,
	ANIMTAG_STARTCYCLE_SE,
	ANIMTAG_STARTCYCLE_S,
	ANIMTAG_STARTCYCLE_SW,
	ANIMTAG_STARTCYCLE_W,
	ANIMTAG_STARTCYCLE_NW,
	ANIMTAG_AIMLIMIT_YAWMIN_IDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_IDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_WALK,
	ANIMTAG_AIMLIMIT_YAWMAX_WALK,
	ANIMTAG_AIMLIMIT_YAWMIN_RUN,
	ANIMTAG_AIMLIMIT_YAWMAX_RUN,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMIN_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMAX_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMIN_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMAX_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCHWALK,
	ANIMTAG_FLASHBANG_PASSABLE,
	ANIMTAG_WEAPON_POSTLAYER
};

struct matrixes
{
	matrix3x4_t main[MAXSTUDIOBONES];
	matrix3x4_t zero[MAXSTUDIOBONES];
	matrix3x4_t first[MAXSTUDIOBONES];
	matrix3x4_t second[MAXSTUDIOBONES];
};

class adjust_data;

class c_resolver
{
	player_t* player = nullptr;
	adjust_data* player_record = nullptr;
	adjust_data* previous_record = nullptr;
	bool fake = false;

public:
	AnimationLayer resolver_layers[6][13];
	AnimationLayer previous_layers[13];
	float m_flAbsRotationFake; // 0
	float m_flAbsRotationLeft; // 2
	float m_flAbsRotationRight; // 1
	float m_flAbsRotationLeftLow; // 4
	float m_flAbsRotationRightLow; // 3
	void Initialize(player_t* e, adjust_data* record);
	void Reset();
	void Yaw_Resolver();
	float Pitch_Resolver();
};

class adjust_data //-V730
{
public:
	player_t* player;
	int i;

	AnimationLayer layers[13];
	AnimationLayer side_layers[3][13];
	AnimationLayer previous_layers[13];

	matrixes matrixes_data;

	bool invalid;
	bool immune;
	bool dormant;
	bool bot;
	int  choke;
	bool didshot;

	int flags;
	int bone_count;
	int index;

	float last_shot_time;
	float simulation_time;
	float old_simulation_time;
	float duck_amount;
	float lby;

	Vector angles;
	c_baseplayeranimationstate frame_state;
	Vector abs_angles;
	Vector velocity;
	Vector origin;
	Vector mins;
	Vector maxs;

	adjust_data() //-V730
	{
		reset();
	}

	void reset()
	{
		player = nullptr;
		i = -1;

		invalid = false;
		immune = false;
		dormant = false;
		bot = false;

		flags = 0;
		bone_count = 0;
		choke = 0;

		last_shot_time = 0.0f;
		old_simulation_time = 0.0f;
		simulation_time = 0.0f;
		duck_amount = 0.0f;
		lby = 0.0f;

		angles.Zero();
		abs_angles.Zero();
		velocity.Zero();
		origin.Zero();
		mins.Zero();
		maxs.Zero();
	}

	adjust_data(player_t* e, bool store = true)
	{

		invalid = false;
		store_data(e, store);
	}

	void store_data(player_t* e, bool store = true)
	{
		if (!e->is_alive())
			return;

		player = e;
		i = player->EntIndex();

		if (store)
		{
			memcpy(layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
			memcpy(matrixes_data.main, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		}

		immune = player->m_bGunGameImmunity() || player->m_fFlags() & FL_FROZEN;
		dormant = player->IsDormant();

#if RELEASE
		player_info_t player_info;
		m_engine()->GetPlayerInfo(i, &player_info);

		bot = player_info.fakeplayer;
#else
		bot = false;
#endif
		const auto pWeapon = player->m_hActiveWeapon();

		flags = player->m_fFlags();
		bone_count = player->m_CachedBoneData().Count();

		last_shot_time = pWeapon ? pWeapon->m_fLastShotTime() : 0.f;
		old_simulation_time = player->m_flOldSimulationTime();
		simulation_time = player->m_flSimulationTime();
		duck_amount = player->m_flDuckAmount();
		lby = player->m_flLowerBodyYawTarget();
		choke = (int)(0.5f + player->m_flSimulationTime() - player->m_flOldSimulationTime() / m_globals()->m_intervalpertick);

		choke = std::clamp(choke, 0, 16);

		angles = player->m_angEyeAngles();
		abs_angles = player->GetAbsAngles();
		velocity = player->m_vecVelocity();
		origin = player->m_vecOrigin();
		mins = player->GetCollideable()->OBBMins();
		maxs = player->GetCollideable()->OBBMaxs();
	}

	void adjust_player()
	{
		if (!valid(false))
			return;

		memcpy(player->get_animlayers(), layers, player->animlayer_count() * sizeof(AnimationLayer));
		memcpy(player->m_CachedBoneData().Base(), matrixes_data.main, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t)); //-V807

		player->m_fFlags() = flags;
		player->m_CachedBoneData().m_Size = bone_count;

		player->m_flOldSimulationTime() = old_simulation_time;
		player->m_flSimulationTime() = simulation_time;
		player->m_flDuckAmount() = duck_amount;
		player->m_flLowerBodyYawTarget() = lby;

		player->m_angEyeAngles() = angles;
		player->set_abs_angles(abs_angles);
		player->m_vecVelocity() = velocity;
		player->m_vecOrigin() = origin;
		player->set_abs_origin(origin);
		player->GetCollideable()->OBBMins() = mins;
		player->GetCollideable()->OBBMaxs() = maxs;
	}

	void increment_layer_cycle(int layer, bool is_looping)
	{
		AnimationLayer pLayer = layers[layer];
		if (fabs(pLayer.m_flPlaybackRate) <= 0.0f)
			return;

		float newcycle = (pLayer.m_flPlaybackRate * player->get_animation_state()->m_flUpdateTimeDelta) + pLayer.m_flCycle;

		if (!is_looping && newcycle >= 1.0f)
			newcycle = 0.999f;

		newcycle -= (float)(int)newcycle; //round to integer

		if (newcycle < 0.0f)
			newcycle += 1.0f;

		if (newcycle > 1.0f)
			newcycle -= 1.0f;

		pLayer.m_flCycle = newcycle;
	}

	void set_layer_weight(int layer, float weight)
	{
		AnimationLayer pLayer = layers[layer];

		weight = std::clamp(weight, 0.0f, 1.0f);

		pLayer.m_flWeight = weight;
	}

	void set_layers_wight_delta_rate(int layer, float oldweight)
	{
		if (player->get_animation_state()->m_flUpdateTimeDelta != 0.0f)
		{
			AnimationLayer pLayer = layers[layer];
			float weightdeltarate = (pLayer.m_flWeight - oldweight) / player->get_animation_state()->m_flUpdateTimeDelta;
			if (pLayer.m_flWeightDeltaRate != weightdeltarate)
				pLayer.m_flWeightDeltaRate = weightdeltarate;
		}
	}

	bool sequence_unfinished(int layer)
	{
		AnimationLayer pLayer = layers[layer];
		return (player->get_animation_state()->m_flUpdateTimeDelta * pLayer.m_flPlaybackRate) + pLayer.m_flCycle >= 1.0f;
		return false;
	}

	bool valid(bool extra_checks = true)
	{
		if (!this) //-V704
			return false;

		if (i > 0)
			player = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!player)
			return false;

		if (player->m_lifeState() != LIFE_ALIVE)
			return false;

		if (immune)
			return false;

		if (dormant)
			return false;

		if (!extra_checks)
			return true;

		if (invalid)
			return false;

		auto net_channel_info = m_engine()->GetNetChannelInfo();

		if (!net_channel_info)
			return false;

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto outgoing = net_channel_info->GetLatency(FLOW_OUTGOING);
		auto incoming = net_channel_info->GetLatency(FLOW_INCOMING);

		auto correct = math::clamp(outgoing + incoming + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());

		auto curtime = globals.local()->is_alive() ? TICKS_TO_TIME(globals.g.fixed_tickbase) : m_globals()->m_curtime; //-V807
		auto delta_time = correct - (curtime - simulation_time);

		if (fabs(delta_time) > 0.2f)
			return false;

		auto extra_choke = 0;

		if (globals.g.fakeducking)
			extra_choke = 14 - m_clientstate()->iChokedCommands;

		auto server_tickcount = extra_choke + m_globals()->m_tickcount + TIME_TO_TICKS(outgoing + incoming);
		auto dead_time = (int)(TICKS_TO_TIME(server_tickcount) - sv_maxunlag->GetFloat());

		if (simulation_time < (float)dead_time)
			return false;

		return true;
	}
};

class optimized_adjust_data
{
public:
	int i;
	player_t* player;

	float simulation_time;
	float duck_amount;
	float speed;

	Vector angles;
	Vector origin;

	optimized_adjust_data() //-V730
	{
		reset();
	}

	void reset()
	{
		i = 0;
		player = nullptr;

		simulation_time = 0.0f;
		duck_amount = 0.0f;

		angles.Zero();
		origin.Zero();
	}
};

extern std::deque <adjust_data> player_records[65];

class c_anim_fix : public singleton <c_anim_fix>
{
public:
	void fsn(ClientFrameStage_t stage);
	bool valid(int i, player_t* e);
	__forceinline float GetMaxPlayerSpeed(player_t* pl);
	void post_anim_update(player_t* player, adjust_data* record);
	void pre_anim_update(player_t* player, adjust_data* record);
	void anim_update(player_t* player);
	void update_player_animations(player_t* e);

	c_resolver player_resolver[65];
	bool is_dormant[65];
	float previous_goal_feet_yaw[65];
};