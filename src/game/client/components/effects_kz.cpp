// Copyright (C) Benjamín Gajardo (also known as +KZ)
//
// ColouredSparkleTrailKaizo() has code from effects.cpp

#include <generated/client_data.h>
#include <game/client/gameclient.h>
#include "particles.h"
#include "effects.h"

void CEffects::ColouredSparkleTrailKaizo(vec2 Pos, ColorRGBA Color)
{
    if(!m_Add50hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SPARKLE;
	p.m_Pos = Pos + random_direction() * random_float(40.0f);
	p.m_Vel = vec2(0.0f, 0.0f);
	p.m_LifeSpan = 0.5f;
	p.m_StartSize = 0.0f;
	p.m_EndSize = random_float(20.0f, 30.0f);
	p.m_UseAlphaFading = true;
	p.m_StartAlpha = Color.a;
	p.m_EndAlpha = std::min(0.2f, Color.a);
    p.m_Color = Color;
	p.m_Collides = false;
	GameClient()->m_Particles.Add(CParticles::GROUP_TRAIL_EXTRA, &p);
}