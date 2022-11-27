// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "local_animations.h"

void local_animations::run(ClientFrameStage_t stage)
{
	if (stage == FRAME_NET_UPDATE_END)
	{
		fake_server_update = false;

		if (globals.local()->m_flSimulationTime() != fake_simulation_time)
		{
			fake_server_update = true;
			fake_simulation_time = globals.local()->m_flSimulationTime();
		}

		globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
		globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
		globals.local()->get_animlayers()[12].m_flWeight = 0.0f;
		globals.local()->m_flPoseParameter()[6] = 1.0f;


		update_fake_animations();
	}
	else if (stage == FRAME_RENDER_START)
	{
		auto animstate = globals.local()->get_animation_state();

		if (!animstate)
			return;

		globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
		globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
		globals.local()->get_animlayers()[12].m_flWeight = 0.0f;
		globals.local()->m_flPoseParameter()[6] = 1.0f;

		update_local_animations(animstate);
	}
	else if (stage == FRAME_RENDER_START)
	{
		auto animstate = globals.local()->get_animation_state();

		if (!animstate)
			return;

		fake_server_update = false;

		if (globals.local()->m_flSimulationTime() != fake_simulation_time)
		{
			fake_server_update = true;
			fake_simulation_time = globals.local()->m_flSimulationTime();
		}

		globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
		globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
		globals.local()->get_animlayers()[12].m_flWeight = 0.0f;
		globals.local()->m_flPoseParameter()[6] = 1.0f;

		update_fake_animations();
		update_local_animations(animstate);
	}
}

void local_animations::update_fake_animations()
{
	auto alloc = !local_data.animstate;
	auto change = !alloc && handle != &globals.local()->GetRefEHandle();
	auto reset = !alloc && !change && globals.local()->m_flSpawnTime() != spawntime;

	if (change)
		m_memalloc()->Free(local_data.animstate);

	if (reset)
	{
		util::reset_state(local_data.animstate);
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (alloc || change)
	{
		local_data.animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

		if (local_data.animstate)
			util::create_state(local_data.animstate, globals.local());

		handle = (CBaseHandle*)&globals.local()->GetRefEHandle();
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (!alloc && !change && !reset && fake_server_update)
	{
		float pose_parameter[24];
		memcpy(pose_parameter, &globals.local()->m_flPoseParameter(), 24 * sizeof(float));

		AnimationLayer layers[13];
		memcpy(layers, globals.local()->get_animlayers(), globals.local()->animlayer_count() * sizeof(AnimationLayer));

		auto backup_frametime = m_globals()->m_frametime;
		auto backup_curtime = m_globals()->m_curtime;

		m_globals()->m_frametime = m_globals()->m_intervalpertick;
		m_globals()->m_curtime = globals.local()->m_flSimulationTime();

		local_data.animstate->m_pBaseEntity = globals.local();
		util::update_state(local_data.animstate, local_animations::get().local_data.fake_angles);

		globals.local()->SetupBones(globals.g.fake_matrix, 256, BONE_USED_BY_ANYTHING, m_globals()->m_curtime);
		local_data.visualize_lag = g_cfg.player.visualize_lag;

		if (!local_data.visualize_lag)
		{
			for (auto& i : globals.g.fake_matrix)
			{
				i[0][3] -= globals.local()->GetRenderOrigin().x;
				i[1][3] -= globals.local()->GetRenderOrigin().y;
				i[2][3] -= globals.local()->GetRenderOrigin().z;
			}
		}

		m_globals()->m_frametime = backup_frametime;
		m_globals()->m_curtime = backup_curtime;

		memcpy(&globals.local()->m_flPoseParameter(), pose_parameter, 24 * sizeof(float));
		memcpy(globals.local()->get_animlayers(), layers, globals.local()->animlayer_count() * sizeof(AnimationLayer));
	}
}

void local_animations::update_local_animations(c_baseplayeranimationstate* animstate)
{
	if (tickcount != m_globals()->m_tickcount)
	{
		memcpy(layers, globals.local()->get_animlayers(), globals.local()->animlayer_count() * sizeof(AnimationLayer));

		animstate->m_iLastClientSideAnimationUpdateFramecount = 0;

		util::update_state(animstate, local_animations::get().local_data.fake_angles);

		if (!m_clientstate()->iChokedCommands)
		{
			abs_angles = animstate->m_flGoalFeetYaw;
			memcpy(pose_parameter, &globals.local()->m_flPoseParameter(), 24 * sizeof(float));
		}

		tickcount = m_globals()->m_tickcount;
	}
	else
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount;

	animstate->m_flGoalFeetYaw = antiaim::get().condition(globals.get_command()) ? abs_angles : local_animations::get().local_data.real_angles.y;
	animstate->m_flFeetYawRate = 0.0f;

	globals.local()->set_abs_angles(Vector(0.0f, abs_angles, 0.0f));
	memcpy(globals.local()->get_animlayers(), layers, globals.local()->animlayer_count() * sizeof(AnimationLayer));
	memcpy(&globals.local()->m_flPoseParameter(), pose_parameter, 24 * sizeof(float));

}