//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_baseplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_sdk_team.h"
#include "sdk_gamerules.h"

#include <igameresources.h>

#include "c_playerresource.h"	//ios for g_PR

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SUITPOWER_INIT -1
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>

static ConVar cl_shownames( "cl_shownames", "0" );

//-----------------------------------------------------------------------------
// Purpose: Shows the logo top left, time and score.
//-----------------------------------------------------------------------------
class CHudShowNames : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudShowNames, vgui::Panel );

public:
	CHudShowNames( const char *pElementName );
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnThink( void );
	bool			ShouldDraw( void );

protected:
	virtual void	Paint();

private:
	CPanelAnimationVar( Color, m_ShowNamesColourA, "ShowNamesColourA", "0 0 0 255" );		//team a col
	CPanelAnimationVar( Color, m_ShowNamesColourB, "ShowNamesColourB", "0 0 0 255" );		//team b col
	CPanelAnimationVar( Color, m_ShowNamesColour, "ShowNamesColour", "0 0 0 255" );			//other

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "0", "proportional_float" );
};	

DECLARE_HUDELEMENT( CHudShowNames );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudShowNames::CHudShowNames( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudShowNames" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

   //m_nImport = surface()->CreateNewTextureID();
   //surface()->DrawSetTextureFile( m_nImport, "vgui/logobutton" , true, true);

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudShowNames::Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudShowNames::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudShowNames::ShouldDraw()
{
	C_BasePlayer *pPlayer = (C_BasePlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if ( cl_shownames.GetInt() )
		return TRUE;
	else
		return FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudShowNames::OnThink( void )
{
}

//-----------------------------------------------------------------------------
//	Draw top left logo
//
//	scripts/hudlayout.red/hudlogo
//-----------------------------------------------------------------------------
void CHudShowNames::Paint()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !g_PR )
		return;

	//IGameResources *gr = GameResources();



	SetBgColor(Color(0,0,0,0));


	char	name[256];
	wchar	wname[256];
	int		team;

	//draw player names
	for ( int i = 1; i<= gpGlobals->maxClients; i++)
	{
		// update from global player resources
		if ( g_PR && g_PR->IsConnected(i) )
		{
			strcpy (name, g_PR->GetPlayerName(i));

			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			if (pPlayer == pLocalPlayer)
				continue;

			team = g_PR->GetTeam( i );

			Vector pos = pPlayer->GetAbsOrigin();
			int iX, iY;
			Vector offset = Vector(0.0f, 0.0f, 0.0f);
			bool onScreen = GetVectorInScreenSpace( pos, iX, iY, &offset);
			if (onScreen)
			{
				vgui::localize()->ConvertANSIToUnicode( name, wname, sizeof(wname)  );
				surface()->DrawSetTextFont(m_hTextFont);
				if (team == TEAM_A)
					surface()->DrawSetTextColor(m_ShowNamesColourA);
				else if (team == TEAM_B)
					surface()->DrawSetTextColor(m_ShowNamesColourB);
				else
					surface()->DrawSetTextColor(m_ShowNamesColour);
				surface()->DrawSetTextPos(iX + text_xpos, iY + text_ypos);
				//helps if we actually draw the name
				surface()->DrawPrintText(wname, wcslen(wname));
			}
		}
	}

/*
	///////////////draw background coloured rectangles first

	//logo rect
	surface()->DrawSetColor( Color(255,255,255,200) );
	surface()->DrawFilledRect( 0, 0, m_flBarWidth, m_flBarHeight );
	//draw clock rect
	surface()->DrawSetColor( Color(0,0,0,200) );
	surface()->DrawFilledRect( m_flBarWidth, 0, m_flBarWidth*2, m_flBarHeight );	//x,y, xEND!!!, YEND!!
	//score rect
	surface()->DrawSetColor( Color(182, 83, 10, 200) );
	surface()->DrawFilledRect( m_flBarWidth*2, 0, m_flBarWidth*4, m_flBarHeight );

	//////////////overlay the glass button texture
	surface()->DrawSetColor( Color(255, 255, 255, 128) );
	SetPaintBorderEnabled(false);
	surface()->DrawSetTexture( m_nImport );
	surface()->DrawTexturedRect( 0, 0, m_flBarWidth*4, m_flBarHeight );



	/////////////LOGO/////////////////////////////////////////
	// draw logo text
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(Color(0,0,0,200));
	surface()->DrawSetTextPos(text_xpos-10, text_ypos);		//fine tune xpos

	wchar_t wtag[7];
	C_SDKTeam *pTeam = (C_SDKTeam*)GetGlobalTeam( 0 );		//why have it in all teams - doesnt seem right.
	vgui::localize()->ConvertANSIToUnicode( pTeam->GetScoreTag(), wtag, sizeof(wtag)  );
	surface()->DrawPrintText(wtag, wcslen(wtag));

	/////////////CLOCK/////////////////////////////////////////

	surface()->DrawSetTextColor(Color(255,255,255,200));
	surface()->DrawSetTextPos(text_xpos+m_flBarWidth-2, text_ypos);
	int t = GetScaledTime();
	int mins = (int)(t / 60.0f);
	int secs = t % 60;
	char matchtime[32];
	Q_snprintf (matchtime, sizeof(matchtime), " %02i:%02i", mins, secs);
	wchar wmatchtime[32];
	vgui::localize()->ConvertANSIToUnicode( matchtime, wmatchtime, sizeof(wmatchtime)  );
	surface()->DrawPrintText(wmatchtime, wcslen(wmatchtime));


	/////////////SCORE/////////////////////////////////////////
	//draw the team initials and score background twice as wide - orange bg / black text 

	surface()->DrawSetTextColor(Color(0,0,0,200));
	surface()->DrawSetTextPos(text_xpos+2*m_flBarWidth, text_ypos);
	char teama[64], teamb[64];
	wchar_t wteama[64], wteamb[64];
	Q_strncpy(teama, gr->GetTeamName(TEAM_A), 4);
	Q_strncpy(teamb, gr->GetTeamName(TEAM_B), 4);
	vgui::localize()->ConvertANSIToUnicode( teama, wteama, sizeof(wteama)  );
	vgui::localize()->ConvertANSIToUnicode( teamb, wteamb, sizeof(wteamb)  );
	wchar_t scoretext[64];
	swprintf(scoretext, L"%s %d - %d %s", wteama, gr->GetTeamScore(TEAM_A), gr->GetTeamScore(TEAM_B), wteamb);
	surface()->DrawPrintText(scoretext, wcslen(scoretext));
	*/
}
