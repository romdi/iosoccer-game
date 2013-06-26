//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "sdk_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "gameinterface.h"
#include "game.h"
#include "ios_bot.h"
#include "ios_keeperbot.h"
#include "ios_fieldbot.h"


ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
static ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );
ConVar bot_frozen( "bot_frozen", "0", 0, "Don't do anything." );
//ConVar bot_reaction_time( "bot_reaction_time", "500", 0, "Reaction time in milliseconds" );

ConVar bot_sendcmd( "bot_sendcmd", "", 0, "Forces bots to send the specified command." );

ConVar bot_crouch( "bot_crouch", "0", 0, "Bot crouches" );

static int g_CurBotNumber = 1;

LINK_ENTITY_TO_CLASS( ios_bot, CBot );

class CBotManager
{
public:
	static CBasePlayer* ClientPutInServerOverride_KeeperBot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CKeeperBot *pPlayer = static_cast<CKeeperBot *>( CreateEntityByName( "ios_keeperbot" ) );
		if ( pPlayer )
		{
			//init bot?
			pPlayer->SetPlayerName( playername );
			Q_memset( &pPlayer->m_cmd, 0, sizeof( pPlayer->m_cmd ) );
		}

		return pPlayer;
	}
	static CBasePlayer* ClientPutInServerOverride_FieldBot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CFieldBot *pPlayer = static_cast<CFieldBot *>( CreateEntityByName( "ios_fieldbot" ) );
		if ( pPlayer )
		{
			//init bot?
			pPlayer->SetPlayerName( playername );
			//pPlayer->m_fMissTime = 0.0f;
			//pPlayer->m_fNextDive = 0.0f;
			Q_memset( &pPlayer->m_cmd, 0, sizeof( pPlayer->m_cmd ) );
		}

		return pPlayer;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen, int keeper )
{
	char botname[ 64 ];
//	Q_snprintf( botname, sizeof( botname ), "Bot%02i", g_CurBotNumber );

	//pick name
	if (keeper==1)
	{
		Q_snprintf( botname, sizeof( botname ), "KeeperBotHome");
	}
	else if (keeper==2)
	{
		Q_snprintf( botname, sizeof( botname ), "KeeperBotAway");
	}
	else
	{
		Q_snprintf( botname, sizeof( botname ), "FieldBot");
	}

	// This trick lets us create a CBot for this client instead of the CSDKPlayer
	// that we would normally get when ClientPutInServer is called.
	ClientPutInServerOverrideFn overrideFn;
	if (keeper > 0)
		overrideFn = &CBotManager::ClientPutInServerOverride_KeeperBot;
	else
		overrideFn = &CBotManager::ClientPutInServerOverride_FieldBot;

	ClientPutInServerOverride( overrideFn );
	edict_t *pEdict = engine->CreateFakeClient( botname );
	ClientPutInServerOverride( NULL );

	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	CBot *pPlayer = ((CBot*)CBaseEntity::Instance( pEdict ));

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
	pPlayer->m_nPlayerType = keeper;

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	return pPlayer;
}

void CBot::FieldBotJoinTeam()
{
	int playerCount[2] = {};

	for (int i = 1; i <= gpGlobals->maxClients; i++) 
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));
		if (!pPl)
			continue;

		if (!CSDKPlayer::IsOnField(pPl) && pPl->GetTeamToJoin() != TEAM_A && pPl->GetTeamToJoin() != TEAM_B)
			continue;

		int team = CSDKPlayer::IsOnField(pPl) ? pPl->GetTeamNumber() : pPl->GetTeamToJoin();

		playerCount[team - TEAM_A] += 1;
	}

	int team = playerCount[0] < playerCount[1] ? TEAM_A : TEAM_B;

	if (playerCount[team] == mp_maxplayers.GetInt())
		team = team == TEAM_A ? TEAM_B : TEAM_A;

	if (playerCount[team] == mp_maxplayers.GetInt())
		SetDesiredTeam(TEAM_SPECTATOR, GetTeamNumber(), 0, true, true, false);
	else
	{
		int startPosIndex = g_IOSRand.RandomInt(0, 10);
		int posIndex = startPosIndex;
		int keeperPosIndex = GetKeeperPosIndex();
		while (true)
		{
			CSDKPlayer *pPl = NULL;

			if (posIndex != keeperPosIndex && IsTeamPosFree(team, posIndex, false, &pPl))
			{
				SetDesiredTeam(team, team, posIndex, true, false, false);
				break;
			}
			posIndex += 1;
			if (posIndex == 12)
				posIndex = 0;
			if (posIndex == startPosIndex)
				break;
		}
	}
}

// Handler for the "bot" command.
CON_COMMAND_F( bot_add, "Add a bot.", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	int playerCount = 0;

	// TODO: Check if there's a variable for that without iterating through it manually
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (UTIL_PlayerByIndex(i))
			playerCount++;
	}

	if (playerCount == gpGlobals->maxClients)
		return;

	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		CBot *pBot = (CBot *)BotPutInServer(bFrozen, 0);
		pBot->FieldBotJoinTeam();
	}
}

ConVar sv_bots_fill_field("sv_bots_fill_field", "0", 0);

//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	bool keeperSpotTaken[2] = {};

	int keeperPosIndex = GetKeeperPosIndex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

		if (!pPl)
			continue;

		if (pPl->IsBot())
		{
			if (((CBot *)pPl)->m_nPlayerType != 0)
			{
				int team = ((CBot *)pPl)->m_nPlayerType == 1 ? TEAM_A : TEAM_B;

				if (!botkeepers.GetBool())
				{
					char kickcmd[512];
					Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Bot keepers not allowed\n", pPl->GetUserID());
					engine->ServerCommand(kickcmd);
				}
				else if (!CSDKPlayer::IsOnField(pPl) && pPl->GetTeamToJoin() != TEAM_A && pPl->GetTeamToJoin() != TEAM_B)
				{
					CSDKPlayer *pSpotTaker = NULL;

					if (!pPl->IsTeamPosFree(team, keeperPosIndex, false, &pSpotTaker))
					{
						char kickcmd[512];
						Q_snprintf(kickcmd, sizeof(kickcmd), "kickid %i Position already taken\n", pPl->GetUserID());
						engine->ServerCommand(kickcmd);
					}
					else
					{
						pPl->SetDesiredTeam(team, team, keeperPosIndex, true, false, false);
						keeperSpotTaken[team - TEAM_A] = true;
					}
				}
				else
				{
					keeperSpotTaken[team - TEAM_A] = true;
				}
			}
			else
			{
				if (!CSDKPlayer::IsOnField(pPl) && pPl->GetTeamToJoin() != TEAM_A && pPl->GetTeamToJoin() != TEAM_B)
					((CBot *)pPl)->FieldBotJoinTeam();
			}
		}
		else
		{
			int team = CSDKPlayer::IsOnField(pPl) ? pPl->GetTeamNumber() : pPl->GetTeamToJoin();
			if ((team == TEAM_A || team == TEAM_B) && pPl->GetTeamPosType() == POS_GK)
				keeperSpotTaken[team - TEAM_A] = true;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (!keeperSpotTaken[i] && botkeepers.GetBool())
		{
			CSDKPlayer *pPl = ToSDKPlayer(BotPutInServer(false, i + 1));
			int team = (i == 0 ? TEAM_A : TEAM_B);
			pPl->SetDesiredTeam(team, team, keeperPosIndex, true, false, false);
		}
	}

	if (sv_bots_fill_field.GetBool())
	{
		int playerCount = 0;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CSDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(i));

			if (!pPl/* || pPl->GetTeamNumber() != TEAM_A && pPl->GetTeamNumber() != TEAM_B && pPl->GetTeamToJoin() != TEAM_A && pPl->GetTeamToJoin() != TEAM_B*/)
				continue;

			playerCount += 1;
		}

		for (int i = (playerCount + 1); i <= 2 * mp_maxplayers.GetInt(); i++)
		{
			CBot *pBot = (CBot *)BotPutInServer(false, 0);
			pBot->FieldBotJoinTeam();
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CSDKPlayer *pPl = ToSDKPlayer( UTIL_PlayerByIndex(i));
		// Ignore plugin bots
		if ( pPl && (pPl->GetFlags() & FL_FAKECLIENT) && !pPl->IsEFlagSet( EFL_PLUGIN_BASED_BOT ) )
		{
			CBot *pBot = dynamic_cast< CBot* >( pPl );
			if ( pBot )
				pBot->BotFrame();
		}
	}
}

bool CBot::RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;
	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	if( bot_crouch.GetInt() )
		cmd.buttons |= IN_DUCK;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			m_flSideMove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
void CBot::RunPlayerMove( CUserCmd &cmd, float frametime )
{
	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( this );
	PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}



//-----------------------------------------------------------------------------
// Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void CBot::BotFrame()
{
	// Make sure we stay being a bot
	AddFlag( FL_FAKECLIENT );

	Q_memcpy(&m_oldcmd, &m_cmd, sizeof(m_oldcmd));
	Q_memset(&m_cmd, 0, sizeof(m_cmd));

	if (CSDKPlayer::IsOnField(this))
	{
		if (bot_frozen.GetBool())
		{
		}
		else if (bot_mimic.GetInt() > 0)
			RunMimicCommand(m_cmd);
		else
		{
			m_pBall = GetNearestBall(GetLocalOrigin());
			m_vBallPos = m_pBall->GetPos();
			m_vBallVel = m_pBall->GetVel();

			m_vBallDir2D = m_vBallVel;
			m_vBallDir2D.z = 0;
			m_vBallDir2D.NormalizeInPlace();
			m_vDirToBall = m_vBallPos - GetLocalOrigin();
			VectorIRotate(m_vDirToBall, EntityToWorldTransform(), m_vLocalDirToBall);

			if (m_vBallVel.Length2D() == 0)
			{
				m_flAngToBallVel = 0;
			}
			else
			{
				Vector vel = m_vBallVel;
				vel.z = 0;
				vel.NormalizeInPlace();
				Vector dir = -m_vDirToBall;
				dir.z = 0;
				dir.NormalizeInPlace();
				m_flAngToBallVel = RAD2DEG(acos(dir.Dot(vel)));
			}

			BotThink();

			if (m_cmd.viewangles[YAW] > 180.0f )
				m_cmd.viewangles[YAW] -= 360.0f;
			else if ( m_cmd.viewangles[YAW] < -180.0f )
				m_cmd.viewangles[YAW] += 360.0f;

			if (m_cmd.viewangles[PITCH] > 180.0f )
				m_cmd.viewangles[PITCH] -= 360.0f;
			else if ( m_cmd.viewangles[PITCH] < -180.0f )
				m_cmd.viewangles[PITCH] += 360.0f;

			m_LastAngles = m_cmd.viewangles;
			SetLocalAngles(m_cmd.viewangles);
			SnapEyeAngles(m_cmd.viewangles);
		}
	}

	//BotHistory history = { m_cmd, m_oldcmd };
	//m_BotHistory.AddToTail(history);
	//
	//m_cmd = m_BotHistory[0].m_cmd;
	//m_oldcmd = m_BotHistory[0].m_oldcmd;

	// Fix up the m_fEffects flags
	PostClientMessagesSent();

	float frametime = gpGlobals->frametime;
	//ios RunPlayerMove( pBot, cmd, frametime );
	RunPlayerMove( m_cmd, frametime );
}

void CBot::BotThink()
{
}

void CBot::PhysicsSimulate()
{
	BaseClass::PhysicsSimulate();

	// Since this isn't called for bots.. call it here
	UpdateVPhysicsPosition(m_vNewVPhysicsPosition, m_vNewVPhysicsVelocity, gpGlobals->frametime);
}

bool CBot::ShotButtonsReleased()
{
	return (gpGlobals->curtime >= m_flBotNextShot);
}

void CBot::SetShotButtonsReleased(bool released)
{
	if (!released)
		m_flBotNextShot = gpGlobals->curtime + 1;
}