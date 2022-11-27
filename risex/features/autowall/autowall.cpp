// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "autowall.h"

bool autowall::is_breakable_entity(IClientEntity* ent) {
	// skip null ents and the world ent.
	if (!ent || !ent->EntIndex())
		return false;

	static auto is_breakable = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 56 8B F1 85 F6 74 ? 83 BE"));

	int32_t take_damage = *(int32_t*)((uintptr_t)(ent)+0x280);

	if (!strcmp(ent->GetClientClass()->m_pNetworkName, crypt_str("CBreakableSurface")))
		*(int32_t*)((uintptr_t)(ent)+0x280) = 2;

	else if (!strcmp(ent->GetClientClass()->m_pNetworkName, crypt_str("CBaseDoor")) || !strcmp(ent->GetClientClass()->m_pNetworkName, crypt_str("CDynamicProp")))
		*(int32_t*)((uintptr_t)(ent)+0x280) = 0;

	*(int32_t*)((uintptr_t)(ent)+0x280) = take_damage;

	return is_breakable;
}

void autowall::scale_damage(player_t* e, CGameTrace& enterTrace, weapon_info_t* weaponData, float& currentDamage)
{

	auto new_damage = currentDamage;

	const int hitgroup = enterTrace.hitgroup;
	const auto is_zeus = globals.local()->m_hActiveWeapon()->m_iItemDefinitionIndex() == WEAPON_TASER;

	static auto is_armored = [](player_t* player, int armor, int hitgroup) {
		if (player && player->m_ArmorValue() > 0)
		{
			if (player->m_bHasHelmet() && hitgroup == HITGROUP_HEAD || (hitgroup >= HITGROUP_CHEST && hitgroup <= HITGROUP_RIGHTARM))
				return true;
		}
		return false;
	};

	auto i = enterTrace.hitgroup;

	bool v4 = e->m_bHasHeavyArmor();

	if (!is_zeus) {
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			new_damage *= 4.f;
			break;
		case HITGROUP_STOMACH:
			new_damage *= 1.25f;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			new_damage *= .75f;
			break;
		default:
			break;
		}
	}
	else
	{
		switch (i)
		{
		case HITGROUP_HEAD:
			currentDamage *= v4 ? 2.0f : 4.0f;
			break;
		case HITGROUP_STOMACH:
			currentDamage *= 1.25f;
			break;
		case HITGROUP_CHEST:
			currentDamage *= 1.0f;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			currentDamage *= 0.75f;
			break;

		}
	}

	if (is_armored(e, e->m_ArmorValue(), hitgroup))
	{
		float flHeavyRatio = 1.0f;
		float flBonusRatio = 0.5f;
		float flRatio = weaponData->flArmorRatio * 0.5f;
		float flNewDamage;

		if (!e->m_bHasHeavyArmor())
		{
			flNewDamage = new_damage * flRatio;
		}
		else
		{
			flBonusRatio = 0.33f;
			flRatio = weaponData->flArmorRatio * 0.5f;
			flHeavyRatio = 0.33f;
			flNewDamage = (new_damage * (flRatio * 0.5)) * 0.85f;
		}

		int iArmor = e->m_ArmorValue();

		if (((new_damage - flNewDamage) * (flBonusRatio * flHeavyRatio)) > iArmor)
			flNewDamage = new_damage - (iArmor / flBonusRatio);

		new_damage = flNewDamage;
	}

	currentDamage = new_damage;

	return;
}

bool autowall::trace_to_exit(CGameTrace& enterTrace, CGameTrace& exitTrace, Vector startPosition, const Vector& direction)
{
	auto enter_point_contents = 0;
	auto point_contents = 0;

	auto is_window = 0;
	auto flag = 0;

	auto fDistance = 0.0f;
	Vector start, end;

	do
	{
		fDistance += 4.0f;

		end = startPosition + direction * fDistance;
		start = end - direction * 4.0f;

		if (!enter_point_contents)
		{
			enter_point_contents = m_trace()->GetPointContents(end, 0x4600400B);
			point_contents = enter_point_contents;
		}
		else
			point_contents = m_trace()->GetPointContents(end, 0x4600400B);

		if (point_contents & MASK_SHOT_HULL && (!(point_contents & CONTENTS_HITBOX) || enter_point_contents == point_contents))
			continue;

		static auto trace_filter_simple = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F0 83 EC 7C 56 52")) + 0x3D;

		uint32_t filter_[4] =
		{
			*(uint32_t*)(trace_filter_simple),
			(uint32_t)globals.local(),
			0,
			0
		};

		util::trace_line(end, start, MASK_SHOT_HULL | CONTENTS_HITBOX, (CTraceFilter*)filter_, &exitTrace); //-V641

		if (exitTrace.startsolid && exitTrace.surface.flags & SURF_HITBOX)
		{
			CTraceFilter filter;
			filter.pSkip = exitTrace.hit_entity;

			filter_[1] = (uint32_t)exitTrace.hit_entity;
			util::trace_line(end, startPosition, MASK_SHOT_HULL, (CTraceFilter*)filter_, &exitTrace); //-V641

			if (exitTrace.DidHit() && !exitTrace.startsolid)
				return true;

			continue;
		}

		auto name = (int*)enterTrace.surface.name; //-V206

		if (name)
		{
			if (*name == 1936744813 && name[1] == 1601397551 && name[2] == 1768318575 && name[3] == 1731159395 && name[4] == 1936941420 && name[5] == 1651668271 && name[6] == 1734307425 && name[7] == 1936941420)
				is_window = 1;
			else
			{
				is_window = 0;

				if (*name != 1936744813)
					goto LABEL_34;
			}

			if (name[1] == 1600480303 && name[2] == 1701536108 && name[3] == 1634494255 && name[4] == 1731162995 && name[5] == 1936941420)
			{
				flag = 1;

			LABEL_35:
				if (is_window || flag)
				{
					exitTrace = enterTrace;
					exitTrace.endpos = end + direction;
					return true;
				}

				goto LABEL_37;
			}
		LABEL_34:
			flag = 0;
			goto LABEL_35;
		}

	LABEL_37:
		if (!exitTrace.DidHit() || exitTrace.startsolid)
		{
			if (enterTrace.hit_entity && enterTrace.hit_entity->EntIndex() && is_breakable_entity(enterTrace.hit_entity))
			{
				exitTrace = enterTrace;
				exitTrace.endpos = startPosition + direction;
				return true;
			}

			continue;
		}

		if (exitTrace.surface.flags & SURF_NODRAW)
		{
			if (is_breakable_entity(exitTrace.hit_entity) && is_breakable_entity(enterTrace.hit_entity))
				return true;

			if (!(enterTrace.surface.flags & SURF_NODRAW))
				continue;
		}

		if (exitTrace.plane.normal.Dot(direction) <= 1.0)
			return true;

	} while (fDistance <= 90.0f);

	return false;
}

void autowall::extrapolate_head_while_jumping(Vector end, Vector start, trace_t* oldtrace, player_t* ent) {
	if (!ent)
		return;

	const auto mins = ent->GetCollideable()->OBBMins();
	const auto maxs = ent->GetCollideable()->OBBMaxs();

	auto dir(end - start);
	auto len = dir.Normalize();

	const auto center = (mins + maxs) / 2;
	const auto pos(ent->m_vecOrigin() + center);

	auto to = pos - start;
	const float range_along = dir.Dot(to);

	float range;
	if (range_along < 0.f) {
		range = -(to).Length();
	}
	else if (range_along > len) {
		range = -(pos - end).Length();
	}
	else {
		auto ray(pos - (start + (dir * range_along)));
		range = ray.Length();
	}

	if (range <= 60.f) {

		Ray_t ray;
		ray.Init(start, end);

		trace_t trace;
		m_trace()->ClipRayToEntity(ray, 0x4600400B, ent, &trace);

		if (oldtrace->fraction > trace.fraction)
			*oldtrace = trace;
	}
}

bool autowall::handle_bullet_penetration(weapon_info_t* weaponData, CGameTrace& enterTrace, Vector& eyePosition, const Vector& direction, int& possibleHitsRemaining, float& currentDamage, float penetrationPower, float ff_damage_reduction_bullets, float ff_damage_bullet_penetration, bool draw_impact)
{
	static ConVar* ff_damage_reduction_bullets_var = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));

	if (possibleHitsRemaining <= 0 || weaponData->flPenetration <= 0 || currentDamage < 1) {
		return false;
	}

	CGameTrace exitTrace;
	auto* pEnemy = (player_t*)enterTrace.hit_entity;
	auto* const enterSurfaceData = m_physsurface()->GetSurfaceData(enterTrace.surface.surfaceProps);
	const int enter_material = enterSurfaceData->game.material;

	if (!enterSurfaceData || enterSurfaceData->game.flPenetrationModifier <= 0)
		return false;

	const auto enter_penetration_modifier = enterSurfaceData->game.flPenetrationModifier;
	const bool isSolidSurf = (enterTrace.contents & CONTENTS_GRATE);
	const bool isLightSurf = (enterTrace.surface.flags & SURF_NODRAW);

	if ((possibleHitsRemaining <= 0 && !isLightSurf && !isSolidSurf && enter_material != CHAR_TEX_GRATE && enter_material != CHAR_TEX_GLASS)
		|| penetrationPower <= 0)
		return false;

	Vector end;

	if (!trace_to_exit(enterTrace, exitTrace, enterTrace.endpos, direction)) {
		if (!(m_trace()->GetPointContents(end, 0x600400B) & 0x600400B))
			return false;
	}

	auto* const exitSurfaceData = m_physsurface()->GetSurfaceData(exitTrace.surface.surfaceProps);
	const int exitMaterial = exitSurfaceData->game.material;
	const float exitSurfPenetrationModifier = exitSurfaceData->game.flPenetrationModifier;

	float combined_damage_modifier = 0.16f;
	float combined_penetration_modifier;

	//Are we using the newer penetration system?
	if (enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE) {
		combined_damage_modifier = 0.05f;
		combined_penetration_modifier = 3;
	}
	else if (isSolidSurf || isLightSurf) {
		combined_damage_modifier = 0.16f;
		combined_penetration_modifier = 1;
	}
	else if (enter_material == CHAR_TEX_FLESH && ff_damage_reduction_bullets_var->GetFloat() == 0 && pEnemy != nullptr && pEnemy->is_player() && pEnemy->m_iTeamNum() == globals.local()->m_iTeamNum())
	{
		if (ff_damage_bullet_penetration == 0)
		{
			// don't allow penetrating players when FF is off
			combined_penetration_modifier = 0;
			return false;
		}

		combined_penetration_modifier = ff_damage_bullet_penetration;
	}
	else {
		combined_penetration_modifier = (enter_penetration_modifier + exitSurfPenetrationModifier) / 2;
	}

	if (enter_material == exitMaterial) {
		if (exitMaterial == CHAR_TEX_WOOD || exitMaterial == CHAR_TEX_CARDBOARD)
			combined_penetration_modifier = 3;
		else if (exitMaterial == CHAR_TEX_PLASTIC)
			combined_penetration_modifier = 2;
	}

	auto penetration_mod = fmaxf(0.f, (3.f / penetrationPower) * 1.25f);

	float modifier = fmaxf(0, 1.0f / combined_penetration_modifier);

	auto thickness = (exitTrace.endpos - enterTrace.endpos).Length();

	const auto lost_damage = ((modifier * 3.f) * penetration_mod + (currentDamage * combined_damage_modifier)) + (((thickness * thickness) * modifier) / 24.f);

	currentDamage -= std::fmaxf(0.f, lost_damage);

	if (currentDamage < 1.f)
		return false;

	eyePosition = exitTrace.endpos;
	--possibleHitsRemaining;

	return true;
}

bool autowall::fire_bullet(weapon_t* pWeapon, Vector& direction, bool& visible, float& currentDamage, int& hitbox, IClientEntity* e, float length, const Vector& pos)
{
	if (!pWeapon)
		return false;

	auto weaponData = pWeapon->get_csweapon_info();

	if (!weaponData)
		return false;

	CGameTrace enterTrace;
	CTraceFilter filter;

	filter.pSkip = globals.local();
	currentDamage = weaponData->iDamage;

	auto eyePosition = pos;
	auto currentDistance = 0.0f;
	auto maxRange = weaponData->flRange;
	auto penetrationDistance = 3000.0f;
	auto penetrationPower = weaponData->flPenetration;
	auto possibleHitsRemaining = 4;

	while (currentDamage >= 1.0f)
	{
		maxRange -= currentDistance;
		auto end = eyePosition + direction * maxRange;

		CTraceFilter filter;
		filter.pSkip = globals.local();

		util::trace_line(eyePosition, end, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace);
		if (e || enterTrace.contents & CONTENTS_HITBOX) {
			extrapolate_head_while_jumping(eyePosition + (direction * 40.f), eyePosition, &enterTrace, (player_t*)enterTrace.hit_entity);
		}
		else
			util::clip_trace_to_players(e, eyePosition, end + direction * 40.0f, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &enterTrace);

		auto enterSurfaceData = m_physsurface()->GetSurfaceData(enterTrace.surface.surfaceProps);
		auto enterSurfPenetrationModifier = enterSurfaceData->game.flPenetrationModifier;
		auto enterMaterial = enterSurfaceData->game.material;

		if (enterTrace.fraction == 1.0f)  //-V550
			break;

		currentDistance += enterTrace.fraction * maxRange;
		currentDamage *= pow(weaponData->flRangeModifier, currentDistance / 500.0f);

		if (currentDistance > penetrationDistance && weaponData->flPenetration || enterSurfPenetrationModifier < 0.1f)  //-V1051
			break;

		auto canDoDamage = enterTrace.hitgroup != HITGROUP_GEAR && enterTrace.hitgroup != HITGROUP_GENERIC;
		auto isPlayer = ((player_t*)enterTrace.hit_entity)->is_player();
		auto isEnemy = ((player_t*)enterTrace.hit_entity)->m_iTeamNum() != globals.local()->m_iTeamNum();

		if (canDoDamage && isPlayer && isEnemy)
		{
			scale_damage((player_t*)enterTrace.hit_entity, enterTrace, weaponData, currentDamage);
			hitbox = enterTrace.hitbox;
			return true;
		}

		if (!possibleHitsRemaining)
			break;

		static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
		static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

		if (!handle_bullet_penetration(weaponData, enterTrace, eyePosition, direction, possibleHitsRemaining, currentDamage, penetrationPower, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat(), !e))
			break;

		visible = false;
	}

	return false;
}

autowall::returninfo_t autowall::wall_penetration(const Vector& eye_pos, Vector& point, IClientEntity* e)
{
	globals.g.autowalling = true;
	auto tmp = point - eye_pos;

	auto angles = ZERO;
	math::vector_angles(tmp, angles);

	auto direction = ZERO;
	math::angle_vectors(angles, direction);

	direction.NormalizeInPlace();

	auto visible = true;
	auto damage = -1.0f;
	auto hitbox = -1;

	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (fire_bullet(weapon, direction, visible, damage, hitbox, e, 0.0f, eye_pos))
	{
		globals.g.autowalling = false;
		return returninfo_t(visible, (int)damage, hitbox); //-V2003
	}
	else
	{
		globals.g.autowalling = false;
		return returninfo_t(false, -1, -1);
	}
}