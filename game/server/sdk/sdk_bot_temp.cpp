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

#include "keeper.h"

class CSDKBot;
void Bot_Think( CSDKBot *pBot );


ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
static ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );
ConVar bot_frozen( "bot_frozen", "0", 0, "Don't do anything." );

ConVar bot_sendcmd( "bot_sendcmd", "", 0, "Forces bots to send the specified command." );

ConVar bot_crouch( "bot_crouch", "0", 0, "Bot crouches" );

static int g_CurBotNumber = 1;

/*
// This is our bot class.
class CSDKBot : public CSDKPlayer
{
public:
	bool			m_bBackwards;

	float			m_flNextTurnTime;
	bool			m_bLastTurnToRight;

	float			m_flNextStrafeTime;
	float			m_flSideMove;

	QAngle			m_ForwardAngle;
	QAngle			m_LastAngles;
};
*/
LINK_ENTITY_TO_CLASS( sdk_bot, CSDKBot );

class CBotManager
{
public:
	static CBasePlayer* ClientPutInServerOverride_Bot( edict_t *pEdict, const char *playername )
	{
		// This tells it which edict to use rather than creating a new one.
		CBasePlayer::s_PlayerEdict = pEdict;

		CSDKBot *pPlayer = static_cast<CSDKBot *>( CreateEntityByName( "sdk_bot" ) );
		if ( pPlayer )
		{
			//init bot?
			pPlayer->SetPlayerName( playername );
			pPlayer->m_JoinTime = gpGlobals->curtime;
			pPlayer->m_fMissTime = 0.0f;
			pPlayer->m_fNextDive = 0.0f;
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
		Q_snprintf( botname, sizeof( botname ), "KEEPER1");
	}
	else if (keeper==2)
	{
		Q_snprintf( botname, sizeof( botname ), "KEEPER2");
	}
	else
	{
		Q_snprintf( botname, sizeof( botname ), "Arthur");
	}

	// This trick lets us create a CSDKBot for this client instead of the CSDKPlayer
	// that we would normally get when ClientPutInServer is called.
	ClientPutInServerOverride( &CBotManager::ClientPutInServerOverride_Bot );
	edict_t *pEdict = engine->CreateFakeClient( botname );
	ClientPutInServerOverride( NULL );

	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	CSDKBot *pPlayer = ((CSDKBot*)CBaseEntity::Instance( pEdict ));

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	pPlayer->ChangeTeam( TEAM_UNASSIGNED );
	pPlayer->RemoveAllItems( true );
	//pPlayer->Spawn();	//spawning here then moving to goal caused the extremely strange keeper teleport bug. Im not sure why!

	g_CurBotNumber++;

	if (keeper==1)
	{
		pPlayer->ChangeTeam( TEAM_A );
	}
	else if (keeper==2)
	{
		pPlayer->ChangeTeam( TEAM_B );
	}
	else
	{
		if (g_IOSRand.RandomInt(0,2))
			pPlayer->ChangeTeam( TEAM_A );						//TODO autoassign??
		else
			pPlayer->ChangeTeam( TEAM_B );
	}

	if (keeper>0)
	{
		pPlayer->ChooseModel();
		pPlayer->m_TeamPos = 1;
		pPlayer->ChooseKeeperSkin();
		//spawn at correct position
		pPlayer->Spawn();
		pPlayer->RemoveEffects( EF_NODRAW );
		pPlayer->SetSolid( SOLID_BBOX );
		pPlayer->RemoveFlag(FL_FROZEN);
	}
	else
	{
		pPlayer->ChooseModel();									//bot player TODO - autoassign
		pPlayer->m_TeamPos = g_IOSRand.RandomInt(2,11);
		pPlayer->ChoosePlayerSkin();
		pPlayer->Spawn();
		pPlayer->RemoveEffects( EF_NODRAW );
		pPlayer->SetSolid( SOLID_BBOX );
		pPlayer->RemoveFlag(FL_FROZEN);
	}


	return pPlayer;
}

// Handler for the "bot" command.
CON_COMMAND_F( bot_add, "Add a bot.", FCVAR_CHEAT )
{
	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		BotPutInServer( bFrozen, 0 );
	}
}

void BotAdd_Keeper1()
{
	//ffs!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	//extern int FindEngineArgInt( const char *pName, int defaultVal );
	//extern const char* FindEngineArg( const char *pName );
	BotPutInServer( FALSE, 1 );
}
void BotAdd_Keeper2()
{
	//ffs!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
        return;

	//extern int FindEngineArgInt( const char *pName, int defaultVal );
	//extern const char* FindEngineArg( const char *pName );
	BotPutInServer( FALSE, 2 );
}


ConCommand cc_Keeper1( "sv_addkeeper1", BotAdd_Keeper1, "Add keeper1" );
ConCommand cc_Keeper2( "sv_addkeeper2", BotAdd_Keeper2, "Add keeper2" );


//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = ToSDKPlayer( UTIL_PlayerByIndex( i ) );

		// Ignore plugin bots
		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) && !pPlayer->IsEFlagSet( EFL_PLUGIN_BASED_BOT ) )
		{
			CSDKBot *pBot = dynamic_cast< CSDKBot* >( pPlayer );
			if ( pBot )
				Bot_Think( pBot );
		}
	}
}

bool Bot_RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

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
void RunPlayerMove( CSDKPlayer *fakeclient, CUserCmd &cmd, float frametime )
{
	if ( !fakeclient )
		return;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}


/*
void Bot_UpdateStrafing( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( gpGlobals->curtime >= pBot->m_flNextStrafeTime )
	{
		pBot->m_flNextStrafeTime = gpGlobals->curtime + 1.0f;

		if ( random->RandomInt( 0, 5 ) == 0 )
		{
			pBot->m_flSideMove = -600.0f + 1200.0f * random->RandomFloat( 0, 2 );
		}
		else
		{
			pBot->m_flSideMove = 0;
		}
		cmd.sidemove = pBot->m_flSideMove;

		if ( random->RandomInt( 0, 20 ) == 0 )
		{
			pBot->m_bBackwards = true;
		}
		else
		{
			pBot->m_bBackwards = false;
		}
	}
}


void Bot_UpdateDirection( CSDKBot *pBot )
{
	float angledelta = 15.0;
	QAngle angle;

	int maxtries = (int)360.0/angledelta;

	if ( pBot->m_bLastTurnToRight )
	{
		angledelta = -angledelta;
	}

	angle = pBot->GetLocalAngles();

	trace_t trace;
	Vector vecSrc, vecEnd, forward;
	while ( --maxtries >= 0 )
	{
		AngleVectors( angle, &forward );

		vecSrc = pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

		vecEnd = vecSrc + forward * 10;

		UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, 
			MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &trace );

		if ( trace.fraction == 1.0 )
		{
			if ( gpGlobals->curtime < pBot->m_flNextTurnTime )
			{
				break;
			}
		}

		angle.y += angledelta;

		if ( angle.y > 180 )
			angle.y -= 360;
		else if ( angle.y < -180 )
			angle.y += 360;

		pBot->m_flNextTurnTime = gpGlobals->curtime + 2.0;
		pBot->m_bLastTurnToRight = random->RandomInt( 0, 1 ) == 0 ? true : false;

		pBot->m_ForwardAngle = angle;
		pBot->m_LastAngles = angle;
	}
	
	pBot->SetLocalAngles( angle );
}


void Bot_FlipOut( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( bot_flipout.GetInt() > 0 && pBot->IsAlive() )
	{
		if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
		{
			cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
		}

		if ( bot_flipout.GetInt() >= 2 )
		{
			QAngle angOffset = RandomAngle( -1, 1 );

			pBot->m_LastAngles += angOffset;

			for ( int i = 0 ; i < 2; i++ )
			{
				if ( fabs( pBot->m_LastAngles[ i ] - pBot->m_ForwardAngle[ i ] ) > 15.0f )
				{
					if ( pBot->m_LastAngles[ i ] > pBot->m_ForwardAngle[ i ] )
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] + 15;
					}
					else
					{
						pBot->m_LastAngles[ i ] = pBot->m_ForwardAngle[ i ] - 15;
					}
				}
			}

			pBot->m_LastAngles[ 2 ] = 0;

			pBot->SetLocalAngles( pBot->m_LastAngles );
		}
	}
}


void Bot_HandleSendCmd( CSDKBot *pBot )
{
	if ( strlen( bot_sendcmd.GetString() ) > 0 )
	{
		//send the cmd from this bot
//		pBot->ClientCommand( bot_sendcmd.GetString() );

		bot_sendcmd.SetValue("");
	}
}


// If bots are being forced to fire a weapon, see if I have it
void Bot_ForceFireWeapon( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( bot_forcefireweapon.GetString() )
	{
		CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType( bot_forcefireweapon.GetString() );
		if ( pWeapon )
		{
			// Switch to it if we don't have it out
			CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

			// Switch?
			if ( pActiveWeapon != pWeapon )
			{
				pBot->Weapon_Switch( pWeapon );
			}
			else
			{
				// Start firing
				// Some weapons require releases, so randomise firing
				if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
				{
					cmd.buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
				}
			}
		}
	}
}


void Bot_SetForwardMovement( CSDKBot *pBot, CUserCmd &cmd )
{
	if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) )
	{
		if ( pBot->m_iHealth == 100 )
		{
			cmd.forwardmove = 600 * ( pBot->m_bBackwards ? -1 : 1 );
			if ( pBot->m_flSideMove != 0.0f )
			{
				cmd.forwardmove *= random->RandomFloat( 0.1, 1.0f );
			}
		}
		else
		{
			// Stop when shot
			cmd.forwardmove = 0;
		}
	}
}


void Bot_HandleRespawn( CSDKBot *pBot, CUserCmd &cmd )
{
	// Wait for Reinforcement wave
	if ( !pBot->IsAlive() )
	{
		// Try hitting my buttons occasionally
		if ( random->RandomInt( 0, 100 ) > 80 )
		{
			// Respawn the bot
			if ( random->RandomInt( 0, 1 ) == 0 )
			{
				cmd.buttons |= IN_JUMP;
			}
			else
			{
				cmd.buttons = 0;
			}
		}
	}
}
*/

void BotFieldplayerThink( CSDKBot *pBot )
{
	float angledelta = 15.0;
	QAngle angle;

	angle = pBot->GetLocalAngles();

	CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearest("football", pBot->GetLocalOrigin(), 999999);
	if (!pEnt)
		return;

	Vector plballdir;
	Vector pldir;
	float closestDist = FLT_MAX;
	CSDKPlayer *pClosest = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSDKPlayer *pPlayer = ( CSDKPlayer *) UTIL_PlayerByIndex( i );
		if ( pPlayer &&
			pPlayer->GetTeamNumber() > LAST_SHARED_TEAM &&
			pPlayer->IsAlive() &&
			(pPlayer->m_TeamPos > 1))
		{
			float dist = pBot->GetLocalOrigin().DistTo(pPlayer->GetLocalOrigin());
			if (dist < closestDist)
			{
				closestDist = dist;
				pClosest = pPlayer;
				plballdir = pEnt->GetLocalOrigin() - pPlayer->GetLocalOrigin();
				pldir = pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin();
				break;
			}
		}
	}

	if (!pClosest)
		return;

	Vector dir = pEnt->GetLocalOrigin() - pBot->GetLocalOrigin();
	pBot->m_cmd.buttons &= ~IN_ATTACK;
	pBot->m_cmd.buttons &= ~IN_ALT1;
	pBot->m_cmd.forwardmove = 0;
	float pitch = 0;

	if (plballdir.Length2D() > 150)
	{
		pBot->m_cmd.forwardmove = clamp(dir.Length2D() / 2, 50, 150);
		pitch = clamp(plballdir.Length2D() / -50 + 10, -30, 10); //-45;

		if (dir.Length2D() > 50)
		{
			Vector target;
			target = pEnt->GetLocalOrigin() + (plballdir / plballdir.Length()) * 50;
			if (dir.Dot(plballdir) > 0) // < 90°
			{
				VectorAngles(Vector(dir.x, dir.y, 0), angle);
				Vector forward, right, up;
				AngleVectors(angle, &forward, &right, &up);
				target += right * 50;
			}
			dir = target - pBot->GetLocalOrigin();
		}
		else
		{
			dir = plballdir * -1;
			pBot->m_cmd.buttons |= plballdir.Length2D() < 2000 ? IN_ATTACK : IN_ALT1;
		}
	}

	VectorAngles(Vector(dir.x, dir.y, 0), angle);
	angle[PITCH] = pitch;
	pBot->m_LastAngles = angle;
	pBot->SetLocalAngles( angle );
	pBot->m_cmd.viewangles = angle;
}

//-----------------------------------------------------------------------------
// Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CSDKBot *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );


	//CUserCmd cmd;
	//Q_memset( &cmd, 0, sizeof( cmd ) );
	
	Q_memset( &pBot->m_cmd, 0, sizeof( pBot->m_cmd ) );
	
	// Finally, override all this stuff if the bot is being forced to mimic a player.
	/* ios
	if ( !Bot_RunMimicCommand( cmd ) && !bot_frozen.GetBool() )
	{
		cmd.sidemove = pBot->m_flSideMove;

		if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
		{
			Bot_SetForwardMovement( pBot, cmd );

			// Only turn if I haven't been hurt
			if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) && pBot->m_iHealth == 100 )
			{
				Bot_UpdateDirection( pBot );
				Bot_UpdateStrafing( pBot, cmd );
			}

			// Handle console settings.
			Bot_ForceFireWeapon( pBot, cmd );
			Bot_HandleSendCmd( pBot );
		}
		else
		{
			Bot_HandleRespawn( pBot, cmd );
		}

		Bot_FlipOut( pBot, cmd );
		*/

		if (pBot->m_TeamPos==1)
			BotKeeperThink(pBot);
		else
		{
			BotFieldplayerThink(pBot);//return;
		}

		// Fix up the m_fEffects flags
		pBot->PostClientMessagesSent();

		
		
		//cmd.viewangles = pBot->GetLocalAngles();
		//cmd.upmove = 0;
		//cmd.impulse = 0;

		//m_cmd.viewangles = pBot->GetLocalAngles();
		pBot->m_cmd.upmove = 0;
		pBot->m_cmd.impulse = 0;
	//}


	float frametime = gpGlobals->frametime;
	//ios RunPlayerMove( pBot, cmd, frametime );
	RunPlayerMove( pBot, pBot->m_cmd, frametime );
}


