#include "cbase.h"
#include "triggers.h"
#include "sdk_gamerules.h"
#include "sdk_player.h"
#include "ball.h"
#include "ios_mapentities.h"

class CBallTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS( CBallTrigger, CBaseTrigger );
	DECLARE_DATADESC();
	COutputEvent m_OnTrigger;

	void Spawn()
	{
		BaseClass::Spawn();
		InitTrigger();
	};
	void StartTouch( CBaseEntity *pOther )
	{
		CBall *pBall = dynamic_cast<CBall *>(pOther);
		if (pBall && !pBall->IgnoreTriggers())
		{
			m_OnTrigger.FireOutput(pOther, this);
			BallStartTouch(pBall);
		}
	};
	virtual void BallStartTouch(CBall *pBall) = 0;
};

BEGIN_DATADESC( CBallTrigger )
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
END_DATADESC()

const static float CELEBRATION_TIME = 10.0f;

class CTriggerGoal : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerGoal, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nTeam;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerGoal(m_nTeam == 1 ? TEAM_A : TEAM_B);
	};
};

BEGIN_DATADESC( CTriggerGoal )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_goal, CTriggerGoal );


class CTriggerGoalLine : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerGoalLine, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nTeam;
	int	m_nSide;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerGoalline(m_nTeam == 1 ? TEAM_A : TEAM_B, m_nSide);
	};
};

BEGIN_DATADESC( CTriggerGoalLine )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_nSide, FIELD_INTEGER, "Side" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_GoalLine, CTriggerGoalLine );


class CTriggerSideLine : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerSideLine, CBallTrigger );
	DECLARE_DATADESC();
	int	m_nSide;

	void BallStartTouch(CBall *pBall)
	{
		pBall->TriggerSideline(m_nSide);
	};
};

BEGIN_DATADESC( CTriggerSideLine )
	DEFINE_KEYFIELD( m_nSide, FIELD_INTEGER, "Side" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_SideLine, CTriggerSideLine );


class CTriggerPenaltyBox : public CBallTrigger
{
public:
	DECLARE_CLASS( CTriggerPenaltyBox, CBallTrigger );

	int	m_nTeam;
	int	m_nSide;
	DECLARE_DATADESC();

	void BallStartTouch(CBall *pBall) {};
};

BEGIN_DATADESC( CTriggerPenaltyBox )
	DEFINE_KEYFIELD( m_nTeam, FIELD_INTEGER, "Team" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_PenaltyBox, CTriggerPenaltyBox );

extern CBaseEntity *CreateBall (const Vector &pos);

class CBallStart : public CPointEntity //IOS
{
public:
	DECLARE_CLASS( CBallStart, CPointEntity );
	void Spawn(void)
	{
		CreateBall (GetLocalOrigin());	
	};
};

//ios entities
LINK_ENTITY_TO_CLASS(info_ball_start, CBallStart);	//IOS

LINK_ENTITY_TO_CLASS(info_team1_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player2, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player3, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player4, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player5, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player6, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player7, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player8, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player9, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player10, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_player11, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team2_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player2, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player3, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player4, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player5, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player6, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player7, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player8, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player9, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player10, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_player11, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_goalkick0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_goalkick1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_goalkick0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_goalkick1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_corner0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_corner1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_corner_player0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team1_corner_player1, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner_player0, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_corner_player1, CPointEntity);

LINK_ENTITY_TO_CLASS(info_throw_in, CPointEntity);

LINK_ENTITY_TO_CLASS(info_team1_penalty_spot, CPointEntity);
LINK_ENTITY_TO_CLASS(info_team2_penalty_spot, CPointEntity);

LINK_ENTITY_TO_CLASS(info_stadium, CPointEntity);

Vector GetSpotPos(const char *name)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname(NULL, name);
	if (pEnt)
		return pEnt->GetLocalOrigin();
	else
		return vec3_origin;
}

void InitMapSpots()
{
	g_vBallSpot = GetSpotPos("info_ball_start");

	for (int i = 0; i < 2; i++)
	{
		g_pTeamSpots[i] = new CTeamSpots();
		g_pTeamSpots[i]->m_vCornerLeft = GetSpotPos(UTIL_VarArgs("info_team%d_corner1", i + 1));
		g_pTeamSpots[i]->m_vCornerRight = GetSpotPos(UTIL_VarArgs("info_team%d_corner0", i + 1));
		g_pTeamSpots[i]->m_vGoalkickLeft = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick1", i + 1));
		g_pTeamSpots[i]->m_vGoalkickRight = GetSpotPos(UTIL_VarArgs("info_team%d_goalkick0", i + 1));
		g_pTeamSpots[i]->m_vPenalty = GetSpotPos(UTIL_VarArgs("info_team%d_penalty_spot", i + 1));

		for (int j = 0; j < 11; j++)
		{
			g_pTeamSpots[i]->m_vPlayers[j] = GetSpotPos(UTIL_VarArgs("info_team%d_player%d", i + 1, j + 1));
		}
	}
}

CTeamSpots *GetOwnTeamSpots(CSDKPlayer *pPl)
{
	return g_pTeamSpots[pPl->GetTeamNumber() - TEAM_A];
}

CTeamSpots *GetOpponentTeamSpots(CSDKPlayer *pPl)
{
	return g_pTeamSpots[TEAM_B - pPl->GetTeamNumber()];
}

//class CBallShield : public CBaseEntity
//{
//public:
//	DECLARE_CLASS( CTriggerPenaltyBox, CBaseEntity );
//	DECLARE_SERVERCLASS();
//
//	void Spawn( void );
//	bool ForceVPhysicsCollide( CBaseEntity *pEntity );
//	bool CreateVPhysics();
//	bool ShouldCollide( int collisionGroup, int contentsMask ) const;
//
//	int UpdateTransmitState()	// always send to all clients
//	{
//		return SetTransmitState( FL_EDICT_ALWAYS );
//	}
//};
//
//LINK_ENTITY_TO_CLASS(ball_shield, CBallShield);
//
//IMPLEMENT_SERVERCLASS_ST( CBallShield, DT_BallShield )
//END_SEND_TABLE()
//
////-----------------------------------------------------------------------------
//// Purpose: Called when spawning, after keyvalues have been handled.
////-----------------------------------------------------------------------------
//void CBallShield::Spawn( )
//{
//	BaseClass::Spawn();
//
//	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
//	SetSolid( SOLID_VPHYSICS );
//	//AddSolidFlags( FSOLID_NOT_SOLID );
//	RemoveSolidFlags(FSOLID_NOT_SOLID);
//	SetSolid(SOLID_VPHYSICS);
//	//AddFlag(FL_WORLDBRUSH);
//
//	SetCollisionGroup(COLLISION_GROUP_BALL_SHIELD);
//	//SetCollisionBounds(WorldAlignMins(), WorldAlignMaxs());
//	SetCollisionBounds(Vector(0, 0, 0), Vector(500, 500, 500));
//
//	CBaseEntity *ent = gEntList.FindEntityByClassname(NULL, "trigger_PenaltyBox");
//
//	SetModel( STRING( ent->GetModelName() ) );
//	//SetModel( STRING( GetModelName() ) );
//	//SetModel( "models/player/barcelona/barcelona.mdl" );
//	//AddEffects( EF_NODRAW );
//	RemoveEffects(EF_NODRAW);
//	CreateVPhysics();
//	VPhysicsGetObject()->EnableCollisions( true );
//}
//
//#include "vcollide_parse.h"
//
//bool CBallShield::CreateVPhysics( void )
//{
//	//VPhysicsInitStatic();
//
//	objectparams_t params =
//	{
//		NULL,
//		1.0, //mass
//		1.0, // inertia
//		0.1f, // damping
//		0.1f, // rotdamping
//		0.05f, // rotIntertiaLimit
//		"DEFAULT",
//		NULL,// game data
//		1.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
//		1.0f, // drag coefficient
//		true,// enable collisions?
//	};
//
//	params.pGameData = static_cast<void	*>(this);
//	//objectparams_t params = g_PhysDefaultObjectParams;
//	//solid_t solid;
//	//solid.params = params;	// copy world's params
//	//solid.params.enableCollisions = true;
//	//solid.params.pName = "fluid";
//	//solid.params.pGameData = static_cast<void *>(this);
//	int	nMaterialIndex = physprops->GetSurfaceIndex("ios");
//	IPhysicsObject *obj = physenv->CreatePolyObjectStatic(PhysCreateBbox(Vector(0, 0, 0), Vector(500, 500, 500)), nMaterialIndex, GetLocalOrigin(), GetLocalAngles(), &params);
//	//IPhysicsObject *obj = physenv->CreateSphereObject(200.0f, nMaterialIndex, GetLocalOrigin(), GetLocalAngles(), &params, true);
//	VPhysicsSetObject(obj);
//	return true;
//}
//
//bool CBallShield::ForceVPhysicsCollide( CBaseEntity *pEntity )
//{
//	return true;
//}
//
//bool CBallShield::ShouldCollide( int collisionGroup, int contentsMask ) const
//{
//	// Rebel owned projectiles (grenades etc) and players will collide.
//	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup == COLLISION_GROUP_PROJECTILE )
//	{
//
//		if ( ( contentsMask & CONTENTS_TEAM1 ) )
//			return true;
//		else
//			return false;
//	}
//	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
//}