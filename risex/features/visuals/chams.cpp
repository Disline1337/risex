#include "chams.h"

void chams::add_matrix(player_t* player, matrix3x4_t* bones)
{
    auto& hit = m_Hitmatrix.emplace_back();

    std::memcpy(hit.pBoneToWorld, bones, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
    hit.time = m_globals()->m_realtime + 2.5f;

    static int m_nSkin = util::find_in_datamap(player->GetPredDescMap(), crypt_str("m_nSkin"));
    static int m_nBody = util::find_in_datamap(player->GetPredDescMap(), crypt_str("m_nBody"));

    hit.info.origin = player->GetAbsOrigin();
    hit.info.angles = player->GetAbsAngles();

    auto renderable = player->GetClientRenderable();

    if (!renderable)
        return;

    auto model = player->GetModel();

    if (!model)
        return;

    auto hdr = *(studiohdr_t**)(player->m_pStudioHdr());

    if (!hdr)
        return;

    hit.state.m_pStudioHdr = hdr;
    hit.state.m_pStudioHWData = m_modelcache()->GetHardwareData(model->studio);
    hit.state.m_pRenderable = renderable;
    hit.state.m_drawFlags = 0;

    hit.info.pRenderable = renderable;
    hit.info.pModel = model;
    hit.info.pLightingOffset = nullptr;
    hit.info.pLightingOrigin = nullptr;
    hit.info.hitboxset = player->m_nHitboxSet();
    hit.info.skin = (int)(uintptr_t(player) + m_nSkin);
    hit.info.body = (int)(uintptr_t(player) + m_nBody);
    hit.info.entity_index = player->EntIndex();
    hit.info.instance = call_virtual<ModelInstanceHandle_t(__thiscall*)(void*) >(renderable, 30u)(renderable);
    hit.info.flags = 0x1;

    hit.info.pModelToWorld = &hit.model_to_world;
    hit.state.m_pModelToWorld = &hit.model_to_world;

    hit.model_to_world.AngleMatrix(hit.info.angles, hit.info.origin);
}


IMaterial* chams::create_material(bool lit, const std::string& material_data)
{
	static auto created = 0;
	std::string type = lit ? crypt_str("VertexLitGeneric") : crypt_str("UnlitGeneric");

	auto matname = crypt_str("mat_") + std::to_string(created);
	++created;

	auto keyValues = new KeyValues(matname.c_str());
	static auto key_values_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 33 C0 C7 45"));

	using KeyValuesFn = void(__thiscall*)(void*, const char*);
	reinterpret_cast <KeyValuesFn> (key_values_address)(keyValues, type.c_str());

	static auto load_from_buffer_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89"));
	using LoadFromBufferFn = void(__thiscall*)(void*, const char*, const char*, void*, const char*, void*);

	reinterpret_cast <LoadFromBufferFn> (load_from_buffer_address)(keyValues, matname.c_str(), material_data.c_str(), nullptr, nullptr, nullptr);

	auto material = m_materialsystem()->CreateMaterial(matname.c_str(), keyValues);
	material->IncrementReferenceCount();

	return material;
}

void chams::draw_hit_matrix()
{
	if (!globals.available())
		m_Hitmatrix.clear();

	if (m_Hitmatrix.empty())
		return;

	if (!m_modelrender())
		return;

	auto ctx = m_materialsystem()->GetRenderContext();

	if (!ctx)
		return;

	auto it = m_Hitmatrix.begin();
	while (it != m_Hitmatrix.end()) {
		if (!it->state.m_pModelToWorld || !it->state.m_pRenderable || !it->state.m_pStudioHdr || !it->state.m_pStudioHWData ||
			!it->info.pRenderable || !it->info.pModelToWorld || !it->info.pModel) {
			++it;
			continue;
		}

		auto alpha = 1.0f;
		auto delta = m_globals()->m_realtime - it->time;
		if (delta > 0.0f) {
			alpha -= delta;
			if (delta > 1.0f) {
				it = m_Hitmatrix.erase(it);
				continue;
			}
		}
		static IMaterial* materials[] =
		{
			create_material(true, crypt_str(R"#("VertexLitGeneric"
			{
				"$basetexture"				"vgui/white"
				"$ignorez"					"0"
				"$envmap"					" "
				"$nofog"					"1"
				"$model"					"1"
				"$nocull"					"0"
				"$selfillum"				"1"
				"$halflambert"				"1"
				"$znearer"					"0"
				"$flat"						"0"
				"$wireframe"				"0"
			}
		)#")),
		create_material(false, crypt_str(R"#("UnlitGeneric"
			{
				"$basetexture"				"vgui/white"
				"$ignorez"					"0"
				"$envmap"					" "
				"$nofog"					"1"
				"$model"					"1"
				"$nocull"					"0"
				"$selfillum"				"1"
				"$halflambert"				"1"
				"$znearer"					"0"
				"$flat"						"1"
				"$wireframe"				"0"
			}
		)#")),
		m_materialsystem()->FindMaterial(crypt_str("dev/glow_armsrace.vmt"), nullptr),
		create_material(true, crypt_str(R"#("VertexLitGeneric" 
			{ 
				"$additive"					"1" 
				"$envmap"					"models/effects/cube_white" 
				"$envmaptint"				"[1 1 1]" 
				"$envmapfresnel"			"1" 
				"$envmapfresnelminmaxexp" 	"[0 1 2]" 
				"$alpha" 					"0.8" 
			}
		)#"))
		};
		auto material = materials[g_cfg.player.lag_hitbox_chams];

		auto alpha_c = (float)g_cfg.player.lag_hitbox_color.a() / 255.0f;
		float normal_color[3] =
		{
			g_cfg.player.lag_hitbox_color[0] / 255.0f,
			g_cfg.player.lag_hitbox_color[1] / 255.0f,
			g_cfg.player.lag_hitbox_color[2] / 255.0f
		};

		m_renderview()->SetBlend(alpha_c / alpha);
		util::color_modulate(normal_color, material);

		material->IncrementReferenceCount();
		material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

		m_modelrender()->ForcedMaterialOverride(material);
		m_modelrender()->DrawModelExecute(ctx, it->state, it->info, it->pBoneToWorld);
		m_modelrender()->ForcedMaterialOverride(nullptr);

		++it;
	}
}