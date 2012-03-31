//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STATSMENU_H
#define STATSMENU_H
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
#include <vgui_controls/ComboBox.h>

using namespace vgui;

enum Stats
{
	STAT_PLAYERS = 0,
	STAT_GOALS,
	STAT_POSSESSION,
	STAT_PING,
	STAT_COUNT
};

char g_szStatNames[STAT_COUNT][32] =
{
	"PLAYERS", "GOALS", "POSSESSION", "PING"
};

struct StatRow_t
{
	Panel *pPanel;
	Label *pTeams[2];
	Label *pName;
};

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CStatsMenu : public Panel
{
private:
	//DECLARE_CLASS_SIMPLE( CTeamMenu, vgui::Frame );
	DECLARE_CLASS_SIMPLE( CStatsMenu, Panel );
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

public:
	CStatsMenu(Panel *parent, const char *name);
	virtual ~CStatsMenu();

	virtual const char *GetName( void ) { return PANEL_TEAM; }
	virtual void OnThink();
	virtual bool HasInputElements( void ) { return true; }
	
	virtual void OnCommand( char const *cmd );
	
protected:
	
	// VGUI2 overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	float m_flNextUpdateTime;

	ComboBox *m_pStatTargets[2];
	StatRow_t *m_pStatRows[STAT_COUNT];
};


#endif // TEAMMENU_H
