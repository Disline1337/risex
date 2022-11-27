#include "..\..\includes.hpp"
#include "hitmarker.h"

player_t* sget_entity(const int index) { return reinterpret_cast<player_t*>(m_entitylist()->GetClientEntity(index)); }

void hitmarker::listener( IGameEvent * game_event )
{
	if ( !g_cfg.esp.hitmarker)
		return;

	const auto attacker = m_engine()->GetPlayerForUserID( game_event->GetInt( "attacker" ) );

	const auto victim = m_engine()->GetPlayerForUserID( game_event->GetInt( "userid" ) );

	if ( attacker != m_engine()->GetLocalPlayer() )
		return;

	if ( victim == m_engine()->GetLocalPlayer() )
		return;

	const auto player = sget_entity( victim );
	if ( !player || !(player->m_iTeamNum() != globals.local()->m_iTeamNum()) )
		return;

}

void hitmarker::draw_hits()
{
	for ( auto i = 0; i < hits.size(); i++ )
	{
		auto& hit = hits[ i ];

		if ( hit.time + 2.1f < m_globals()->m_curtime )
		{
			hits.erase( hits.begin() + i );
			i--;
		}

		Vector screen_pos;

		if ( math::world_to_screen( hit.pos, screen_pos ) )
		{
			if (g_cfg.esp.hitmarker)
			    render_hitmarker( hit, screen_pos );
			if (g_cfg.esp.damage_marker)
			    render_damage( hit, screen_pos, Color(255, 0, 0) );
		}
	}
}

void hitmarker::add_hit( const hitmarker_t hit )
{
	hits.push_back( hit );
}

void hitmarker::render_hitmarker( hitmarker_t& hit, const Vector& screen_pos )
{
	static auto line_size = 6;

	const auto step = 255.f / 1.0f * m_globals()->m_frametime;
	const auto step_move = 30.f / 1.5f * m_globals()->m_frametime;
	const auto multiplicator = 0.3f;

	hit.moved -= step_move;
	
	if ( hit.time + 1.8f <= m_globals()->m_curtime )
		hit.alpha -= step;

	const auto int_alpha = static_cast< int >( hit.alpha );

	if ( int_alpha > 0 )
	{
		auto col = Color( 255, 255, 255, int_alpha );
		render::get().line(screen_pos.x + 2, screen_pos.y + 2, screen_pos.x + 6, screen_pos.y + 6, Color(255,255,255));
		render::get().line(screen_pos.x - 2, screen_pos.y - 2, screen_pos.x - 6, screen_pos.y - 6, Color(255, 255, 255));
		render::get().line(screen_pos.x + 2, screen_pos.y - 2, screen_pos.x + 6, screen_pos.y - 6, Color(255, 255, 255));
		render::get().line(screen_pos.x - 2, screen_pos.y + 2, screen_pos.x - 6, screen_pos.y + 6, Color(255, 255, 255));
		std::stringstream dmg;
		dmg << "-" << std::to_string(hit.damage);
		col = Color( 255, 0, 0, int_alpha );
		
		//render::get().text(fonts[ESP] , screen_pos.x + 8 -hit.moved, screen_pos.y - 12 + hit.moved , otheresp::get().hitmarker.hurt_color, false, dmg.str().c_str());
	}
}

void hitmarker::render_damage(hitmarker_t& hit, const Vector& screen_pos, Color col)
{
	static auto line_size = 6;

	const auto step = 255.f / 1.0f * m_globals()->m_frametime;
	const auto step_move = 30.f / 1.5f * m_globals()->m_frametime;
	const auto multiplicator = 0.3f;

	hit.moved -= step_move;

	if (hit.time + 1.2f <= m_globals()->m_curtime)
		hit.alpha -= step;

	const auto int_alpha = static_cast<int>(hit.alpha);

	if (int_alpha > 0)
	{
		//auto col = Color(255, 255, 255, int_alpha);
	    std::stringstream dmg;
		dmg << "-" << std::to_string(hit.damage);
		//col = Color(255, 0, 0, int_alpha);

		render::get().text(fonts[INDICATORFONT], screen_pos.x + 8 - hit.moved, screen_pos.y - 12 + hit.moved, Color(col.r(), col.g(), col.b(), int_alpha), false, dmg.str().c_str());
	}
}