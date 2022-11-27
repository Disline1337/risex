// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "world_esp.h"
#include "grenade_warning.h"

void worldesp::paint_traverse()
{
	skybox_changer();

	for (int i = 1; i <= m_entitylist()->GetHighestEntityIndex(); i++)  //-V807
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_player())
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		switch (client_class->m_ClassID)
		{
		case CEnvTonemapController:
			world_modulation(e);
			break;
		case CInferno:
			molotov_timer(e);
			break;
		case CSmokeGrenadeProjectile:
			smoke_timer(e);
			break;
		case CPlantedC4:
			bomb_timer(e);
			break;
		case CC4:
			if (g_cfg.player.type[ENEMY].flags[FLAGS_C4] || g_cfg.player.type[TEAM].flags[FLAGS_C4] || g_cfg.player.type[LOCAL].flags[FLAGS_C4] || g_cfg.esp.bomb_timer)
			{
				auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(e->m_hOwnerEntity());

				if ((g_cfg.player.type[ENEMY].flags[FLAGS_C4] || g_cfg.player.type[TEAM].flags[FLAGS_C4] || g_cfg.player.type[LOCAL].flags[FLAGS_C4]) && owner->valid(false, false))
					globals.g.bomb_carrier = owner->EntIndex();
				else if (g_cfg.esp.bomb_timer && !owner->is_player())
				{
					auto screen = ZERO;

					if (math::world_to_screen(e->GetAbsOrigin(), screen))
						render::get().text(fonts[ESP], screen.x, screen.y, Color(215, 20, 20), HFONT_CENTERED_X | HFONT_CENTERED_Y, "BOMB");
				}
			}

			break;
		default:
			grenade_projectiles(e);

			if (client_class->m_ClassID == CAK47 || client_class->m_ClassID == CDEagle || client_class->m_ClassID >= CWeaponAug && client_class->m_ClassID <= CWeaponZoneRepulsor) //-V648
				dropped_weapons(e);

			break;
		}
	}
}

void worldesp::skybox_changer()
{
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(util::FindSignature(crypt_str("engine.dll"), crypt_str("55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45")));
	auto skybox_name = backup_skybox;

	switch (g_cfg.esp.skybox)
	{
	case 1:
		skybox_name = "cs_tibet";
		break;
	case 2:
		skybox_name = "cs_baggage_skybox_";
		break;
	case 3:
		skybox_name = "italy";
		break;
	case 4:
		skybox_name = "jungle";
		break;
	case 5:
		skybox_name = "office";
		break;
	case 6:
		skybox_name = "sky_cs15_daylight01_hdr";
		break;
	case 7:
		skybox_name = "sky_cs15_daylight02_hdr";
		break;
	case 8:
		skybox_name = "vertigoblue_hdr";
		break;
	case 9:
		skybox_name = "vertigo";
		break;
	case 10:
		skybox_name = "sky_day02_05_hdr";
		break;
	case 11:
		skybox_name = "nukeblank";
		break;
	case 12:
		skybox_name = "sky_venice";
		break;
	case 13:
		skybox_name = "sky_cs15_daylight03_hdr";
		break;
	case 14:
		skybox_name = "sky_cs15_daylight04_hdr";
		break;
	case 15:
		skybox_name = "sky_csgo_cloudy01";
		break;
	case 16:
		skybox_name = "sky_csgo_night02";
		break;
	case 17:
		skybox_name = "sky_csgo_night02b";
		break;
	case 18:
		skybox_name = "sky_csgo_night_flat";
		break;
	case 19:
		skybox_name = "sky_dust";
		break;
	case 20:
		skybox_name = "vietnam";
		break;
	case 21:
		skybox_name = g_cfg.esp.custom_skybox;
		break;
	}

	static auto skybox_number = 0;
	static auto old_skybox_name = skybox_name;

	static auto color_r = (unsigned char)255;
	static auto color_g = (unsigned char)255;
	static auto color_b = (unsigned char)255;

	if (skybox_number != g_cfg.esp.skybox)
	{
		changed = true;
		skybox_number = g_cfg.esp.skybox;
	}
	else if (old_skybox_name != skybox_name)
	{
		changed = true;
		old_skybox_name = skybox_name;
	}
	else if (color_r != g_cfg.esp.skybox_color[0])
	{
		changed = true;
		color_r = g_cfg.esp.skybox_color[0];
	}
	else if (color_g != g_cfg.esp.skybox_color[1])
	{
		changed = true;
		color_g = g_cfg.esp.skybox_color[1];
	}
	else if (color_b != g_cfg.esp.skybox_color[2])
	{
		changed = true;
		color_b = g_cfg.esp.skybox_color[2];
	}

	if (changed)
	{
		changed = false;
		load_skybox(skybox_name.c_str());

		auto materialsystem = m_materialsystem();

		for (auto i = materialsystem->FirstMaterial(); i != materialsystem->InvalidMaterial(); i = materialsystem->NextMaterial(i))
		{
			auto material = materialsystem->GetMaterial(i);

			if (!material)
				continue;

			if (strstr(material->GetTextureGroupName(), crypt_str("SkyBox")))
				material->ColorModulate(g_cfg.esp.skybox_color[0] / 255.0f, g_cfg.esp.skybox_color[1] / 255.0f, g_cfg.esp.skybox_color[2] / 255.0f);
		}
	}
}

void worldesp::fog_changer()
{
	static auto fog_override = m_cvar()->FindVar(crypt_str("fog_override")); //-V807

	if (!g_cfg.esp.fog)
	{
		if (fog_override->GetBool())
			fog_override->SetValue(FALSE);

		return;
	}

	if (!fog_override->GetBool())
		fog_override->SetValue(TRUE);

	static auto fog_start = m_cvar()->FindVar(crypt_str("fog_start"));

	if (fog_start->GetInt())
		fog_start->SetValue(0);

	static auto fog_end = m_cvar()->FindVar(crypt_str("fog_end"));

	if (fog_end->GetInt() != g_cfg.esp.fog_distance)
		fog_end->SetValue(g_cfg.esp.fog_distance);

	static auto fog_maxdensity = m_cvar()->FindVar(crypt_str("fog_maxdensity"));

	if (fog_maxdensity->GetFloat() != (float)g_cfg.esp.fog_density * 0.01f) //-V550
		fog_maxdensity->SetValue((float)g_cfg.esp.fog_density * 0.01f);

	char buffer_color[12];
	sprintf_s(buffer_color, 12, "%i %i %i", g_cfg.esp.fog_color.r(), g_cfg.esp.fog_color.g(), g_cfg.esp.fog_color.b());

	static auto fog_color = m_cvar()->FindVar(crypt_str("fog_color"));

	if (strcmp(fog_color->GetString(), buffer_color)) //-V526
		fog_color->SetValue(buffer_color);
}

void worldesp::world_modulation(entity_t* entity)
{
	if (g_cfg.esp.world_modulation)
	{

		entity->set_m_bUseCustomBloomScale(TRUE);
		entity->set_m_flCustomBloomScale(g_cfg.esp.bloom * 0.01f);

	}
	if (g_cfg.esp.nightmode) {
		entity->set_m_bUseCustomAutoExposureMin(TRUE);
		entity->set_m_flCustomAutoExposureMin(g_cfg.esp.exposure * 0.001f);

		entity->set_m_bUseCustomAutoExposureMax(TRUE);
		entity->set_m_flCustomAutoExposureMax(g_cfg.esp.exposure * 0.001f);
	}
}

void worldesp::molotov_timer(entity_t* entity)
{
	if (!g_cfg.esp.molotov_timer)
		return;

	auto inferno = reinterpret_cast<inferno_t*>(entity);
	auto origin = inferno->GetAbsOrigin();
	Vector mins, maxs;
	inferno->GetClientRenderable()->GetRenderBounds(mins, maxs);
	Vector screen_origin;
	auto color_main = Color(g_cfg.esp.molotov_timer_color.r(), g_cfg.esp.molotov_timer_color.g(), g_cfg.esp.molotov_timer_color.b(), (int)((g_cfg.esp.molotov_timer_color.a()) * 0.1f));
	//render::get().Draw3DFilledCircle(origin, Vector(maxs - mins).Length2D() * 0.5, color_main);
	render::get().Circle3D(origin, Vector(maxs - mins).Length2D() * 0.5, g_cfg.esp.molotov_timer_color);
	if (!math::world_to_screen(origin, screen_origin))
		return;

	auto spawn_time = inferno->get_spawn_time();
	auto factor = (spawn_time + inferno_t::get_expiry_time() - m_globals()->m_curtime) / inferno_t::get_expiry_time();
	auto timer = inferno_t::get_expiry_time();
	static auto size = Vector2D(35.0f, 5.0f);
	static bool yy = false;
	static bool yys = false;
	static bool yyz = false;
	static float size_c = 0.f;
	if (size_c == 8.f || size_c > 8.f)
		yy = true;
	if (size_c < 1.0f)
		yy = false;
	if (!yy)
		size_c += 0.1f;
	else if (yy)
		size_c -= 0.1f;
	/*render::get().circle_filled(screen_origin.x, screen_origin.y - size.y * 0.5f, 60, 20, Color(15, 15, 15, 187));


	*/
	//render::get().text(fonts[ESP], screen_origin.x, screen_origin.y - size.y * 0.5f + 12.0f, vars.esp.molotov_timer_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "FIRE");
	//render::get().text(fonts[GRENADES], screen_origin.x + 1.0f, screen_origin.y - size.y * 0.5f - 9.0f - 20, g_cfg.esp.molotov_timer_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "l");
	//render::get().triangle(Vector2D(screen_origin.x + 1.0f - 6, screen_origin.y - size.y * 0.5f - 9.0f - size_c), Vector2D(screen_origin.x + 1.0f + 6, screen_origin.y - size.y * 0.5f - 9.0f - size_c), Vector2D(screen_origin.x + 1.0f, screen_origin.y - size.y * 0.5f - 9.0f + 10 - size_c), Color(255, 255, 255, 200));
	render::get().circle_filled(screen_origin.x, screen_origin.y - size.y * 0.5f, 60, 20, g_cfg.esp.grenade_proximity_warning_inner_color);
	render::get().circle_filled(screen_origin.x, screen_origin.y - size.y * 0.5f, 60, 20, g_cfg.esp.grenade_proximity_warning_inner_color);
	render::get().draw_arc(screen_origin.x, screen_origin.y - size.y * 0.5f, 20, 0, 360 * factor, 2, g_cfg.esp.molotov_timer_color);
	render::get().text(fonts[GRENADES], screen_origin.x, screen_origin.y - size.y * 0.5f, g_cfg.esp.smoke_timer_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "l");

	//std::stringstream text;
	//text << "FIRE: " << std::to_string(factor * 10);
	//render::get().text(fonts[ESP], screen_origin.x, screen_origin.y - size.y * 0.5f - 1.0f, Color(220,50,0,255), HFONT_CENTERED_X | HFONT_CENTERED_Y, text.str().c_str());


}

void worldesp::smoke_timer(entity_t* entity)
{
	if (!g_cfg.esp.smoke_timer)
		return;

	auto smoke = reinterpret_cast<smoke_t*>(entity);

	if (!smoke->m_nSmokeEffectTickBegin() || !smoke->m_bDidSmokeEffect())
		return;

	static auto alpha = 0.0f;


	auto color_main = Color(g_cfg.esp.smoke_timer_color.r(), g_cfg.esp.smoke_timer_color.g(), g_cfg.esp.smoke_timer_color.b(), (int)((g_cfg.esp.smoke_timer_color.a()) * 0.1f));
	auto origin = smoke->GetAbsOrigin();
	Vector screen_origin;
	render::get().Circle3D(origin, 160.5f, g_cfg.esp.smoke_timer_color);

	if (!math::world_to_screen(origin, screen_origin))
		return;

	auto spawn_time = TICKS_TO_TIME(smoke->m_nSmokeEffectTickBegin());
	auto factor = (spawn_time + smoke_t::get_expiry_time() - m_globals()->m_curtime) / smoke_t::get_expiry_time();

	static auto size = Vector2D(35.0f, 5.0f);
	static float sizing = 4.f;
	static float size_c = 0.f;

	static bool yy = false;
	static bool yys = false;
	static bool yyz = false;
	if (size_c == 8.f || size_c > 8.f)
		yy = true;
	if (size_c < 1.0f)
		yy = false;
	if (!yy)
		size_c += 0.1f;
	else if (yy)
		size_c -= 0.1f;

	render::get().circle_filled(screen_origin.x, screen_origin.y - size.y * 0.5f, 60, 20, g_cfg.esp.grenade_proximity_warning_inner_color);
	render::get().circle_filled(screen_origin.x, screen_origin.y - size.y * 0.5f, 60, 20, g_cfg.esp.grenade_proximity_warning_inner_color);
	render::get().draw_arc(screen_origin.x, screen_origin.y - size.y * 0.5f, 20, 0, 360 * factor, 2, g_cfg.esp.smoke_timer_color);
	render::get().text(fonts[GRENADES], screen_origin.x, screen_origin.y - size.y * 0.5f, g_cfg.esp.smoke_timer_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "k");

}

void worldesp::grenade_projectiles(entity_t* entity)
{
	if (!g_cfg.esp.grenade_proximity_warning)
		return;

	c_grenade_prediction::get().grenade_warning((projectile_t*)entity);
}

void worldesp::bomb_timer(entity_t* entity)
{
	if (!g_cfg.esp.bomb_timer)
		return;

	if (!globals.g.bomb_timer_enable)
		return;
	auto bomb = (CCSBomb*)entity;

	auto local_player = reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()));
	Vector bomb_origin, bomb_position;
	bomb_origin = entity->GetAbsOrigin();
	auto distance_bomb = globals.local()->GetAbsOrigin().DistTo(entity->GetAbsOrigin());
	static auto mp_c4timer = m_cvar()->FindVar(crypt_str("mp_c4timer"));
	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::world_to_screen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;


	auto c4timer = mp_c4timer->GetFloat();
	auto bomb_timer = bomb->m_flC4Blow() - m_globals()->m_curtime;

	auto explode_time = bomb->m_flC4Blow();
	explode_time -= m_globals()->m_intervalpertick * local_player->m_nTickBase();
	if (explode_time <= 0)
		explode_time = 0;
	auto c4_timer = mp_c4timer->GetInt();
	char buffer[64];
	sprintf_s(buffer, "bomb: %.1f", bomb_timer);

	char asd[64];
	sprintf_s(asd, "%.1f", bomb_timer);


	if (!math::world_to_screen(bomb_origin, bomb_position))
		return;
	if (isOnScreen(bomb->GetAbsOrigin(), screenPos))
	{
		render::get().text(fonts[GRENADES], bomb_position.x, bomb_position.y, Color(255, 255, 255), true, "o");

		if (distance_bomb <= 915.0f)
			render::get().text(fonts[ESP], bomb_position.x, bomb_position.y + 20, Color(255, 255, 255), true, "dead");

		//render::get().rect_filled(bomb_position.x - c4_timer / 2, bomb_position.y + 37, c4_timer, 3 + 7 + 9, Color(15, 15, 15, 155));
		render::get().rect_filled(bomb_position.x - 35 / 2, bomb_position.y + 37, explode_time, 2, Color(22, 92, 245));
		render::get().text(fonts[ESP], bomb_position.x, bomb_position.y + 40, Color(255, 255, 255), true, asd);

		if (m_inputsys()->IsButtonDown(KEY_E))
		{
			if (entity->m_iTeamNum() != 2)
			{
				if (distance_bomb <= 85.0f)
				{
					render::get().rect_filled(bomb_position.x - 35 / 2, bomb_position.y + 37 + 3 + 7 + 7, c4_timer, 2, Color(245, 92, 108));
				}
			}
		}
	}
	else {
		Vector viewAngles;
		m_engine()->GetViewAngles(viewAngles);

		static int width, height;
		m_engine()->GetScreenSize(width, height);

		auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
		auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(globals.g.eye_pos, bomb->GetAbsOrigin()).y - 90.0f);

		auto radius = 30;
		auto size = 30;

		auto newPointX = screenCenter.x + ((((width - (size * 3)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
		auto newPointY = screenCenter.y + ((((height - (size * 3)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));

		std::array <Vector2D, 3> points
		{
			Vector2D(newPointX - size, newPointY - size),
			Vector2D(newPointX + size, newPointY),
			Vector2D(newPointX - size, newPointY + size)
		};

		//math::rotate_triangle(points, viewAngles.y - math::calculate_angle(csgo.globals.eye_pos, bomb->GetAbsOrigin()).y - 90.0f);
		render::get().text(fonts[GRENADES], newPointX, newPointY, Color(255, 255, 255), true, "o");
		render::get().text(fonts[ESP], newPointX, newPointY + 20, Color(255, 255, 255), true, asd);
		//render::get().circle_filled(newPointX, newPointY, 1, 1, Color(255, 255, 255));
	}

}

void worldesp::dropped_weapons(entity_t* entity)
{
	auto weapon = (weapon_t*)entity; //-V1027
	auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(weapon->m_hOwnerEntity());

	if (!owner)
		return;


	if (owner->is_player())
		return;

	Box box;

	if (util::get_bbox(weapon, box, false))
	{
		auto offset = 0;

		if (g_cfg.esp.weapon[WEAPON_BOX])
		{
			render::get().rect(box.x, box.y, box.w, box.h, g_cfg.esp.box_color);

			if (g_cfg.esp.weapon[WEAPON_ICON])
			{
				render::get().text(fonts[SUBTABWEAPONS], box.x + box.w / 2, box.y - 14, g_cfg.esp.weapon_color, HFONT_CENTERED_X, weapon->get_icon());
				offset = 14;
			}

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				render::get().text(fonts[ESP], box.x + box.w / 2, box.y + box.h + 2, g_cfg.esp.weapon_color, HFONT_CENTERED_X, weapon->get_name().c_str());

			if (weapon->get_csweapon_info() && g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				render::get().rect_filled(box.x - 1, box.y + box.h + 14, box.w + 2, 4, inner_back_color);
				render::get().rect_filled(box.x, box.y + box.h + 15, weapon->m_iClip1() * box.w / weapon->get_csweapon_info()->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

			if (g_cfg.esp.weapon[WEAPON_DISTANCE])
			{
				auto distance = globals.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;
				render::get().text(fonts[ESP], box.x + box.w / 2, box.y - 13 - offset, g_cfg.esp.weapon_color, HFONT_CENTERED_X, "%i FT", (int)distance);
			}
		}
		else
		{
			if (g_cfg.esp.weapon[WEAPON_ICON])
				render::get().text(fonts[SUBTABWEAPONS], box.x + box.w / 2, box.y + box.h / 2 - 7, g_cfg.esp.weapon_color, HFONT_CENTERED_X, weapon->get_icon());

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				render::get().text(fonts[ESP], box.x + box.w / 2, box.y + box.h / 2 + 6, g_cfg.esp.weapon_color, HFONT_CENTERED_X, weapon->get_name().c_str());

			if (g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				static auto pos = 0;

				if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;
				else if (g_cfg.esp.weapon[WEAPON_ICON])
					pos = 8;
				else if (g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;

				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				render::get().rect_filled(box.x - 1, box.y + box.h / 2 + pos - 1, box.w + 2, 4, inner_back_color);
				render::get().rect_filled(box.x, box.y + box.h / 2 + pos, weapon->m_iClip1() * box.w / weapon->get_csweapon_info()->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

			if (g_cfg.esp.weapon[WEAPON_DISTANCE])
			{
				auto distance = globals.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;

				if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
					offset = 21;
				else if (g_cfg.esp.weapon[WEAPON_ICON])
					offset = 21;
				else if (g_cfg.esp.weapon[WEAPON_TEXT])
					offset = 8;

				render::get().text(fonts[ESP], box.x + box.w / 2, box.y + box.h / 2 - offset, g_cfg.esp.weapon_color, HFONT_CENTERED_X, "%i FT", (int)distance);
			}
		}
	}
}

bool can_penetrate(weapon_t* weapon)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = globals.local();

	trace_t trace;
	util::trace_line(globals.g.eye_pos, globals.g.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f) //-V550
		return false;

	auto eye_pos = globals.g.eye_pos;
	auto hits = 1;
	auto damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return autowall::get().handle_bullet_penetration(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat());
}

void worldesp::penetration_reticle()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.penetration_reticle)
		return;

	if (!globals.local()->is_alive())
		return;

	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto color = Color::Red;

	if (!weapon->is_non_aim() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && can_penetrate(weapon))
		color = Color::Green;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	render::get().rect_filled(width / 2, height / 2 - 1, 1, 3, color);
	render::get().rect_filled(width / 2 - 1, height / 2, 3, 1, color);
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device);

void worldesp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.show_spread)
		return;

	if (!globals.local()->is_alive())
		return;

	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return;

	int w, h;
	m_engine()->GetScreenSize(w, h);

	draw_circe((float)w * 0.5f, (float)h * 0.5f, globals.g.inaccuracy * 500.0f, 50, D3DCOLOR_RGBA(g_cfg.esp.show_spread_color.r(), g_cfg.esp.show_spread_color.g(), g_cfg.esp.show_spread_color.b(), g_cfg.esp.show_spread_color.a()), D3DCOLOR_RGBA(0, 0, 0, 0), device);
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr); //-V107

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0); //-V107
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}

void worldesp::automatic_peek_indicator()
{
	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!globals.g.start_position.IsZero())
		position = globals.g.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 0.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 2.5f * m_globals()->m_frametime; //-V807
		else
			alpha -= 2.5f * m_globals()->m_frametime;
		auto color_main = Color(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)((alpha * g_cfg.menu.menu_theme.a()) / 3));
		auto color_outline = Color(g_cfg.menu.menu_theme.r(), g_cfg.menu.menu_theme.g(), g_cfg.menu.menu_theme.b(), (int)(255));
		alpha = math::clamp(alpha, 0.0f, 1.0f);
		render::get().Draw3DFilledCircle(position, alpha * 15.f, Color(color_main));
		//render::get().Circle3D(position, alpha * 15.f, Color(color_outline));

		Vector screen;

	}
}