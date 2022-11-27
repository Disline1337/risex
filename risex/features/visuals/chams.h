#pragma once
#include "..\..\includes.hpp"

struct hit_matrix_story {
	int ent_index;
	ModelRenderInfo_t info;
	DrawModelState_t state;
	matrix3x4_t pBoneToWorld[128] = {};
	float time;
	matrix3x4_t model_to_world;
};

class chams : public singleton<chams>
{
public:
	void on_dme();
	void on_sceneend();
	void add_matrix(player_t* player, matrix3x4_t* bones);
	void draw_hit_matrix();

	std::vector< hit_matrix_story > m_Hitmatrix;
private:
	IMaterial* create_material(bool lit, const std::string& material_data);

};

