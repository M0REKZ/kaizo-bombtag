/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

#include <game/server/entity.h>

class CProjectile : public CEntity
{
public:
	CProjectile(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeze,
		bool Explosive,
		int SoundImpact,
		vec2 InitDir,
		int Layer = 0,
		int Number = 0);

	~CProjectile();

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;
	void SwapClients(int Client1, int Client2) override;

private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	//int m_Damage;
	int m_SoundImpact;
	int m_StartTick;
	bool m_Explosive;

	// DDRace

	int m_Bouncing;
	bool m_Freeze;
	int m_TuneZone;
	bool m_BelongsToPracticeTeam;
	int m_DDRaceTeam;
	bool m_IsSolo;
	vec2 m_InitDir;

public:
	void SetBouncing(int Value);
	bool FillExtraInfoLegacy(CNetObj_DDRaceProjectile *pProj);
	void FillExtraInfo(CNetObj_DDNetProjectile *pProj);

	bool CanCollide(int ClientId) override;
	int GetOwnerId() const override { return m_Owner; }

	//+KZ
	int GetStartTick() { return m_StartTick; }
	int GetDDraceTeam() { return m_DDRaceTeam; }
	bool m_GoresTeleportGrenade = false;
	int GetType() { return m_Type; }

	// Kaizo-Insta projectile rollback
	// for rollback players the projectile will be forwarded
	// to match the tick they sent the fire input
	//
	// on DDNet Antiping clients it will send m_OrigStartTick on Snap so it is
	// intended to play with DDNet grenade antiping, but the best would be to find
	// a way to bypass client antiping and show what the server really has
	bool m_FirstTick;
	int m_OrigStartTick;
	bool m_FirstSnap;
	int m_aParticleIds[3]; // particles to let know others a projectile got fired, in case it was forwarded too much
};

#endif
