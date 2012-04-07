//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMMENU_H
#define TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_bitmapbutton.h>
#include <vgui_controls/ImagePanel.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

#include <vgui_controls/EditablePanel.h>

namespace vgui
{
	class RichText;
	class HTML;
}
class TeamFortressViewport;

using namespace vgui;

struct StatPanel_t
{
	Panel *pPanel;
	Label *pGoals;
	Label *pGoalText;
	Label *pAssists;
	Label *pAssistText;
	Label *pFouls;
	Label *pFoulsText;
	Label *pYellowCards;
	Label *pYellowCardText;
	Label *pRedCards;
	Label *pRedCardText;
	Label *pPossession;
	Label *pPossessionText;
	Label *pPing;
	Label *pPingText;
};

struct PosPanel_t
{
	Panel *pPosPanel;
	Button *pPlayerName;
	Label *pClubName;
	Label *pPosName;
	StatPanel_t *pStatPanel;
	Button *pKickButton;
	ImagePanel *pCountryFlag;
};

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CTeamMenu : public Panel
{
private:
	//DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );
	DECLARE_CLASS_SIMPLE( CTeamMenu, Panel );

public:
	CTeamMenu(Panel *parent, const char *name);
	virtual ~CTeamMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	
	virtual void OnCommand( char const *cmd );
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	IScheme *m_pScheme;

	PosPanel_t *m_pPosPanels[2][11];

	Button *m_pSpectateButton;
	Button *m_pTeamButtons[2];

	ImagePanel *m_pTeamCrests[2];

	int m_nActiveTeam;

	Label *m_pTeamNames[2];
	Label *m_pTeamPossession[2];
	Label *m_pTeamPlayerCount[2];

	Panel *m_pTeamPanels[2];

	Label *m_pSpectatorNames;

	float m_flNextUpdateTime;
	int m_nOldMaxPlayers;
};


#endif // TEAMMENU_H
