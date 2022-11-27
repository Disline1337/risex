// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\features\ragebot\antiaim.h"
#include "..\..\features\visuals\world_esp.h"
#include "..\..\features\misc\fakelag.h"
#include "..\..\features\misc\prediction_system.h"
#include "..\..\features\ragebot\ragebot.h"
#include "..\..\features\misc\airstrafe.h"
#include "..\..\features\fakewalk\slowwalk.h"
#include "..\..\features\misc\misc.h"
#include "..\..\features\misc\logs.h"
#include "..\..\features\visuals\GrenadePrediction.h"
#include "..\..\features\ragebot\knifebot.h"
#include "..\..\features\ragebot\zeusbot.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "..\..\features\lagcompensation\animfix.h"

using CreateMove_t = void(__thiscall*)(IBaseClientDLL*, int, float, bool);

bool SetupPacket(int sequence_number, bool* pbSendPacket)
{
    CUserCmd* m_pcmd = m_input()->GetUserCmd(sequence_number);

    if (!m_pcmd || !m_pcmd->m_command_number)
        return false;

    if (!globals.local()->is_alive())
        return false;

    globals.send_packet = pbSendPacket;
    return true;// ;)))))
}

void __stdcall hooked_createmove(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket)
{
    static auto original_fn = hooks::client_hook->get_func_address <CreateMove_t>(22);
    original_fn(m_client(), sequence_number, input_sample_frametime, active);
    globals.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

    CUserCmd* m_pcmd = m_input()->GetUserCmd(sequence_number);
    CVerifiedUserCmd* verified = m_input()->GetVerifiedUserCmd(sequence_number);

    globals.g.in_createmove = false;

    if (!verified || !SetupPacket(sequence_number, &bSendPacket))
        return original_fn(m_client(), sequence_number, input_sample_frametime, active);

    //if (!m_pcmd->m_command_number)
        //return; //original_fn(m_client(), sequence_number, input_sample_frametime, active);

    //if (!verified)
        //return; //original_fn(m_client(), sequence_number, input_sample_frametime, active);

    //bool original_result = original_fn(m_client(), sequence_number, input_sample_frametime, active);

    if (original_fn)
    {
        m_prediction()->SetLocalViewAngles(m_pcmd->m_viewangles);
        m_engine()->SetViewAngles(m_pcmd->m_viewangles);
    }

    if (!globals.available())
        return; //original_fn(m_client(), sequence_number, input_sample_frametime, active);

    misc::get().rank_reveal();

    if (!globals.local()->is_alive()) //-V807
        return; //original_fn(m_clientmode(), smt, m_pcmd);

    uintptr_t* frame_ptr;
    __asm mov frame_ptr, ebp;

    globals.g.weapon = globals.local()->m_hActiveWeapon().Get();

    if (!globals.g.weapon)
        return; //original_fn(m_clientmode(), smt, m_pcmd);

    globals.g.in_createmove = true;
    globals.set_command(m_pcmd);

    if (hooks::menu_open && globals.g.focused_on_input)
    {
        m_pcmd->m_buttons = 0;
        m_pcmd->m_forwardmove = 0.0f;
        m_pcmd->m_sidemove = 0.0f;
        m_pcmd->m_upmove = 0.0f;
    }

    if (globals.g.isshifting)
    {
        *globals.send_packet = globals.g.shift_ticks == 1;
        m_pcmd->m_buttons &= ~(IN_ATTACK | IN_ATTACK2);
        return;
    }

    //if (globals.g.startcharge)
    //{
    //    ++globals.g.tocharge;

    //    m_pcmd->m_tickcount = INT_MAX;
    //    m_pcmd->m_buttons &= ~IN_ATTACK;
    //    m_pcmd->m_buttons &= ~IN_ATTACK2;

    //    if (globals.g.tocharge >= 16)
    //    {
    //        globals.g.isshifting = false;
    //        *(bool*)(frame_ptr - 0x34 / 0x1c) = true;
    //    }
    //    else
    //        *(bool*)(frame_ptr - 0x34 / 0x1c) = false;
    //}

    if (globals.g.ticks_allowed < 16 && (misc::get().double_tap_enabled && misc::get().double_tap_key || misc::get().hide_shots_enabled && misc::get().hide_shots_key))
        globals.g.startcharge = true;

    globals.g.backup_tickbase = globals.local()->m_nTickBase();
    globals.g.fixed_tickbase = globals.local()->m_nTickBase() - globals.g.shift_ticks;

    if (hooks::menu_open)
    {
        m_pcmd->m_buttons &= ~IN_ATTACK;
        m_pcmd->m_buttons &= ~IN_ATTACK2;
    }

    if (m_pcmd->m_buttons & IN_ATTACK2 && g_cfg.ragebot.enable && globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
        m_pcmd->m_buttons &= ~IN_ATTACK2;

    if (g_cfg.ragebot.enable && !globals.g.weapon->can_fire(true))
    {
        if (m_pcmd->m_buttons & IN_ATTACK && !globals.g.weapon->is_non_aim() && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
            m_pcmd->m_buttons &= ~IN_ATTACK;
        else if ((m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2) && (globals.g.weapon->is_knife() || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER) && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_HEALTHSHOT)
        {
            if (m_pcmd->m_buttons & IN_ATTACK)
                m_pcmd->m_buttons &= ~IN_ATTACK;

            if (m_pcmd->m_buttons & IN_ATTACK2)
                m_pcmd->m_buttons &= ~IN_ATTACK2;
        }
    }

    if (m_pcmd->m_buttons & IN_FORWARD && m_pcmd->m_buttons & IN_BACK)
    {
        m_pcmd->m_buttons &= ~IN_FORWARD;
        m_pcmd->m_buttons &= ~IN_BACK;
    }

    if (m_pcmd->m_buttons & IN_MOVELEFT && m_pcmd->m_buttons & IN_MOVERIGHT)
    {
        m_pcmd->m_buttons &= ~IN_MOVELEFT;
        m_pcmd->m_buttons &= ~IN_MOVERIGHT;
    }

    bSendPacket = true;
    //*globals.send_packet = true;
    globals.g.tickbase_shift = 0;
    globals.g.double_tap_fire = false;
    globals.g.force_send_packet = false;
    globals.g.exploits = misc::get().double_tap_key || misc::get().hide_shots_key;
    globals.g.current_weapon = globals.g.weapon->get_weapon_group(g_cfg.ragebot.enable);
    globals.g.slowwalking = false;
    globals.g.original_forwardmove = m_pcmd->m_forwardmove;
    globals.g.original_sidemove = m_pcmd->m_sidemove;

    antiaim::get().breaking_lby = false;

    auto wish_angle = m_pcmd->m_viewangles;

    misc::get().fast_stop(m_pcmd);

    if (g_cfg.misc.bunnyhop)
        misc::get().bunnyhop();

    misc::get().SlideWalk(m_pcmd);

    misc::get().NoDuck(m_pcmd);

    misc::get().AutoCrouch(m_pcmd);

    GrenadePrediction::get().Tick(m_pcmd->m_buttons);

    if (g_cfg.misc.crouch_in_air && !(globals.local()->m_fFlags() & FL_ONGROUND))
        m_pcmd->m_buttons |= IN_DUCK;

    engine_prediction::m_misc.m_bOverrideModifier = true;
    engine_prediction::update();
    engine_prediction::predict(m_pcmd, globals.local());

    globals.g.eye_pos = globals.local()->get_shoot_position();

    if (g_cfg.misc.airstrafe)
        airstrafe::get().create_move(m_pcmd);

    if (key_binds::get().get_key_bind_state(19) && engineprediction::get().backup_data.flags & FL_ONGROUND && !(globals.local()->m_fFlags() & FL_ONGROUND)) //-V807
        m_pcmd->m_buttons |= IN_JUMP;

    if (key_binds::get().get_key_bind_state(21))
        slowwalk::get().create_move(m_pcmd);

    if (g_cfg.ragebot.enable && !globals.g.weapon->is_non_aim() && engineprediction::get().backup_data.flags & FL_ONGROUND && globals.local()->m_fFlags() & FL_ONGROUND)
        slowwalk::get().create_move(m_pcmd, 0.95f + 0.003125f * (16 - m_clientstate()->iChokedCommands));

    fakelag::get().Createmove();

    globals.g.aimbot_working = false;
    globals.g.revolver_working = false;

    auto backup_velocity = globals.local()->m_vecVelocity();
    auto backup_abs_velocity = globals.local()->m_vecAbsVelocity();

    globals.local()->m_vecVelocity() = engineprediction::get().backup_data.velocity;
    globals.local()->m_vecAbsVelocity() = engineprediction::get().backup_data.velocity;

    globals.g.weapon->update_accuracy_penality();

    globals.local()->m_vecVelocity() = backup_velocity;
    globals.local()->m_vecAbsVelocity() = backup_abs_velocity;

    globals.g.inaccuracy = globals.g.weapon->get_inaccuracy();
    globals.g.spread = globals.g.weapon->get_spread();

    aim::get().run(m_pcmd);

    zeusbot::get().run(m_pcmd);
    knifebot::get().run(m_pcmd);

    misc::get().automatic_peek(m_pcmd, wish_angle.y);

    antiaim::get().desync_angle = 0.0f;
    antiaim::get().create_move(m_pcmd);

    if (!globals.g.weapon->is_non_aim())
    {
        auto double_tap_aim_check = false;

        if (m_pcmd->m_buttons & IN_ATTACK && globals.g.double_tap_aim_check)
        {
            double_tap_aim_check = true;
            globals.g.double_tap_aim_check = false;
        }

        auto revolver_shoot = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !globals.g.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

        if (m_pcmd->m_buttons & IN_ATTACK && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
        {
            static auto weapon_recoil_scale = m_cvar()->FindVar(crypt_str("weapon_recoil_scale"));

            if (g_cfg.ragebot.enable)
                m_pcmd->m_viewangles -= globals.local()->m_aimPunchAngle() * weapon_recoil_scale->GetFloat();

            if (!globals.g.fakeducking)
            {
                globals.g.force_send_packet = true;
                globals.g.should_choke_packet = true;
                bSendPacket = true;
                fakelag::get().started_peeking = false;
            }

            aim::get().last_shoot_position = globals.g.eye_pos;

            if (!double_tap_aim_check)
                globals.g.double_tap_aim = false;
        }
    }
    else if (!globals.g.fakeducking && globals.g.weapon->is_knife() && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2))
    {
        globals.g.force_send_packet = true;
        globals.g.should_choke_packet = true;
        bSendPacket = true;
        fakelag::get().started_peeking = false;
    }

    if (globals.g.fakeducking)
        globals.g.force_send_packet = *globals.send_packet;

    for (auto& backup : aim::get().backup)
        backup.adjust_player();

    auto backup_ticks_allowed = globals.g.ticks_allowed;

    misc::get().double_tap(m_pcmd);

    if (misc::get().double_tap_enabled)
        misc::get().hide_shots(m_pcmd,false);
    else if (misc::get().hide_shots_enabled)
    {
        globals.g.ticks_allowed = backup_ticks_allowed;
        misc::get().hide_shots(m_pcmd, true);
    }

    if (!globals.g.weapon->is_non_aim())
    {
        auto double_tap_aim_check = false;

        if (m_pcmd->m_buttons & IN_ATTACK && globals.g.double_tap_aim_check)
        {
            double_tap_aim_check = true;
            globals.g.double_tap_aim_check = false;
        }

        auto revolver_shoot = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !globals.g.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

        if (!double_tap_aim_check && m_pcmd->m_buttons & IN_ATTACK && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
            globals.g.double_tap_aim = false;
    }

    if (m_globals()->m_tickcount - globals.g.last_aimbot_shot > 16) //-V807
    {
        globals.g.double_tap_aim = false;
        globals.g.double_tap_aim_check = false;
    }

    engineprediction::get().fix_local_commands();

    if (globals.g.loaded_script)
        for (auto current : c_lua::get().hooks.getHooks(crypt_str("on_createmove")))
            current.func();
    auto weapon = globals.local()->m_hActiveWeapon().Get();
    if (weapon->can_fire(false)) {
        if (bSendPacket)
            local_animations::get().local_data.fake_angles = m_input()->m_pCommands[m_pcmd->m_command_number % MULTIPLAYER_BACKUP].m_viewangles;
        else
            local_animations::get().local_data.real_angles = m_input()->m_pCommands[m_pcmd->m_command_number % MULTIPLAYER_BACKUP].m_viewangles;

        local_animations::get().local_data.stored_real_angles = m_input()->m_pCommands[m_pcmd->m_command_number % MULTIPLAYER_BACKUP].m_viewangles;
    }
    else
    {
        local_animations::get().local_data.fake_angles = m_pcmd->m_viewangles; //-V807
        local_animations::get().local_data.real_angles = local_animations::get().local_data.stored_real_angles;
    }

    if (g_cfg.misc.anti_untrusted)
        math::normalize_angles(m_pcmd->m_viewangles);
    else 
    {
        m_pcmd->m_viewangles.y = math::normalize_yaw(m_pcmd->m_viewangles.y);
        m_pcmd->m_viewangles.z = 0.0f;
    }

    util::movement_fix(wish_angle, m_pcmd);

    static auto previous_ticks_allowed = globals.g.ticks_allowed;

    if (globals.g.ticks_allowed > 17)
        globals.g.ticks_allowed = math::clamp(globals.g.ticks_allowed - 1, 0, 17);

    if (previous_ticks_allowed && !globals.g.ticks_allowed)
        globals.g.ticks_choke = 16;

    previous_ticks_allowed = globals.g.ticks_allowed;

    if (globals.g.ticks_choke)
    {
        bSendPacket = globals.g.force_send_packet;
        --globals.g.ticks_choke;
    }

    auto& correct = globals.g.data.emplace_front();

    correct.command_number = m_pcmd->m_command_number;
    correct.choked_commands = m_clientstate()->iChokedCommands + 1;
    correct.tickcount = m_globals()->m_tickcount;

    if (*globals.send_packet)
        globals.g.choked_number.clear();
    else
        globals.g.choked_number.emplace_back(correct.command_number);

    while (globals.g.data.size() > (int)(2.0f / m_globals()->m_intervalpertick))
        globals.g.data.pop_back();

    auto& out = globals.g.commands.emplace_back();

    out.is_outgoing = *globals.send_packet;
    out.is_used = false;
    out.command_number = m_pcmd->m_command_number;
    out.previous_command_number = 0;

    while (globals.g.commands.size() > (int)(1.0f / m_globals()->m_intervalpertick))
        globals.g.commands.pop_front();

    if (!*globals.send_packet && !m_gamerules()->m_bIsValveDS())
    {
        auto net_channel = m_clientstate()->pNetChannel;

        if (net_channel->m_nChokedPackets > 0 && !(net_channel->m_nChokedPackets % 4))
        {
            auto backup_choke = net_channel->m_nChokedPackets;
            net_channel->m_nChokedPackets = 0;

            net_channel->send_datagram();
            --net_channel->m_nOutSequenceNr;

            net_channel->m_nChokedPackets = backup_choke;
        }
    }

    if (!antiaim::get().breaking_lby)
        local_animations::get().local_data.stored_real_angles = m_pcmd->m_viewangles;

    if (*globals.send_packet && globals.g.should_send_packet)
        globals.g.should_send_packet = false;

    if (g_cfg.misc.buybot_enable && globals.g.should_buy)
    {
        --globals.g.should_buy;

        if (!globals.g.should_buy)
        {
            std::string buy;

            switch (g_cfg.misc.buybot1)
            {
            case 1:
                buy += crypt_str("buy g3sg1; ");
                break;
            case 2:
                buy += crypt_str("buy awp; ");
                break;
            case 3:
                buy += crypt_str("buy ssg08; ");
                break;
            }

            switch (g_cfg.misc.buybot2)
            {
            case 1:
                buy += crypt_str("buy elite; ");
                break;
            case 2:
                buy += crypt_str("buy deagle; buy revolver; ");
                break;
            }

            if (g_cfg.misc.buybot3[BUY_ARMOR])
                buy += crypt_str("buy vesthelm; buy vest; ");

            if (g_cfg.misc.buybot3[BUY_TASER])
                buy += crypt_str("buy taser; ");

            if (g_cfg.misc.buybot3[BUY_GRENADES])
                buy += crypt_str("buy molotov; buy hegrenade; buy smokegrenade; buy flashbang; buy flashbang; buy decoy; ");

            if (g_cfg.misc.buybot3[BUY_DEFUSER])
                buy += crypt_str("buy defuser; ");

            m_engine()->ExecuteClientCmd(buy.c_str());
        }
    }

    globals.g.in_createmove = false;

    *(bool*)(frame_ptr - 0x34 / 0x1c) = *globals.send_packet;
    //return false;

    verified->m_cmd = *m_pcmd;
    verified->m_crc = m_pcmd->GetChecksum();
}

//pasted не помню откуда.
__declspec(naked) void __stdcall hooks::hooked_createmove_naked(int sequence_number, float input_sample_frametime, bool active)
{
    __asm
    {
        push ebx
        push esp
        push dword ptr[esp + 20]
        push dword ptr[esp + 0Ch + 8]
        push dword ptr[esp + 10h + 4]
        call hooked_createmove
        pop ebx
        retn 0Ch
    }
}