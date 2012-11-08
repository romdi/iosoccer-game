//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLIENTSCOREBOARDDIALOG_H
#define CLIENTSCOREBOARDDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "gameeventlistener.h"
#include "vgui_bitmapbutton.h"
#include "statsmenu.h"
#include "formationmenu.h"

using namespace vgui;

enum stats_t
{
	CORNERS,
	DISTANCECOVERED,
	FOULS,
	FOULSSUFFERED,
	FREEKICKS,
	INTERCEPTIONS,
	OFFSIDES,
	OWNGOALS,
	PASSES,
	PASSESCOMPLETED,
	POSSESSION,
	REDCARDS,
	KEEPERSAVES,
	SHOTS,
	SHOTSONGOAL,
	YELLOWCARDS,
	STAT_COUNT,
};

static const char g_szStatIdentifiers[STAT_COUNT][32] =
{
	"corners",
	"distancecovered",
	"fouls",
	"foulssuffered",
	"freekicks",
	"interceptions",
	"offsides",
	"owngoals",
	"passes",
	"passescompleted",
	"possession",
	"redcards",
	"keepersaves",
	"shots",
	"shotsongoal",
	"yellowcards",
};

static const char g_szStatNames[STAT_COUNT][32] =
{
	"Corners",
	"Distance",
	"Fouls",
	"Fouls suffered",
	"Free kicks",
	"Interceptions",
	"Offsides",
	"Own goals",
	"Passes",
	"Passes completed",
	"Possession",
	"Reds",
	"Saves",
	"Shots",
	"Shots on goal",
	"Yellows",
};

enum stat_categories_t
{
	DEFAULT_STATS = -1,
	GENERAL = 0,
	TACKLES,
	SET_PIECES,
	KEEPER,
	OFFENSE,
	STAT_CATEGORY_COUNT,
};

static const char g_szStatCategoryNames[STAT_CATEGORY_COUNT][32] =
{
	"General",
	"Tackles",
	"Set pieces",
	"Keeper",
	"Offense",
};

struct SpecInfo
{
	int playerIndex;
	char playerName[MAX_PLAYER_NAME_LENGTH];
};

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CClientScoreBoardDialog : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CClientScoreBoardDialog, vgui::EditablePanel );

protected:
// column widths at 640
	//ios enum { NAME_WIDTH = 160, SCORE_WIDTH = 60, DEATH_WIDTH = 60, PING_WIDTH = 80, VOICE_WIDTH = 0, FRIENDS_WIDTH = 0 };
	enum { NAME_WIDTH = 100, SCORE_WIDTH = 60, DEATH_WIDTH = 60, PING_WIDTH = 80, VOICE_WIDTH = 0, FRIENDS_WIDTH = 0, SMALL_WIDTH = 35, VSMALL_WIDTH = 20};
	// total = 340
	enum { SPEC_FONT_COUNT = 4 };

public:
	CClientScoreBoardDialog( IViewPort *pViewPort );
	~CClientScoreBoardDialog();

	virtual const char *GetName( void ) { return PANEL_SCOREBOARD; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	virtual bool ShowAvatars() { return false; }

	void ToggleMenu();

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
 	
	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);

	virtual void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );

	void SetHighlightedPlayer(int playerIndex);
			
protected:
	MESSAGE_FUNC_INT( OnPollHideCode, "PollHideCode", code );
	MESSAGE_FUNC_PTR( OnCursorEntered, "OnCursorEntered", panel );
	MESSAGE_FUNC_PTR( OnCursorExited, "OnCursorExited", panel );

	// functions to override
	virtual bool GetPlayerInfo(int playerIndex, KeyValues *kv);
	virtual bool GetTeamInfo(int team, KeyValues *kv);
	virtual void UpdateTeamInfo();
	virtual void UpdatePlayerInfo();
	virtual void OnThink();
	virtual void AddHeader(); // add the start header of the scoreboard
	//virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team
	virtual int GetAdditionalHeight() { return 0; } 

	void Paint();

	// sorts players within a section
	static bool StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// finds the player in the scoreboard
	int FindItemIDForPlayerIndex(int playerIndex, int &side);

	MESSAGE_FUNC_PARAMS(OnItemSelected, "ItemSelected", data);

	int m_iNumTeams;

	vgui::SectionedListPanel *m_pPlayerList[2];
	int				m_iSectionId; // the current section we are entering into

	int s_VoiceImage[5];
	int TrackerImage;
	int	m_HLTVSpectators;
	float m_fNextUpdateTime;

	void MoveLabelToFront(const char *textEntryName);
	void MoveToCenterOfScreen();

	void OnCommand(char const *cmd);
	
	vgui::ImagePanel *m_pImagePanel; //ios

	vgui::ImageList				*m_pImageList;
	int							m_iImageAvatars[MAX_PLAYERS+1];
	CUtlMap<int,int>			m_mapAvatarsToImageList;

	//CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "34" );		// Avatar width doesn't scale with resolution
	//CPanelAnimationVarAliasType( int, m_iAvatarWidth, "avatar_width", "34", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "136", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassWidth, "class_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nGoalsWidth, "score_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iDeathWidth, "death_width", "35", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPingWidth, "ping_width", "23", "proportional_int" );

	int m_iAvatarWidth;

private:
	int			m_iPlayerIndexSymbol;
	int			m_iDesiredHeight;
	IViewPort	*m_pViewPort;
	ButtonCode_t m_nCloseKey;
	Panel		*m_pMainPanel;
	Panel		*m_pExtraInfoPanel;
	Label		*m_pSpectatorText;
	Panel		*m_pSpectatorNames;
	Button		*m_pSpectateButton;
	Panel		*m_pSpectatorContainer;
	HFont		m_pSpectatorFontList[SPEC_FONT_COUNT];
	ImagePanel	*m_pTeamCrests[2];
	Label		*m_pPlayerCount[2];
	Label		*m_pPossession[2];
	Panel		*m_pStatButtonOuterContainer;
	Panel		*m_pStatButtonInnerContainer;
	Label		*m_pStatText;
	Button		*m_pStatButtons[STAT_COUNT];
	Panel		*m_pPlayerListDivider;
	Button		*m_pJoinRandom;
	Button		*m_pBecomeCaptain;
	Button		*m_pToggleMenu;
	ComboBox	*m_pFormationList;
	Button		*m_pRequestTimeout;

	CStatsMenu	*m_pStatsMenu;
	CFormationMenu	*m_pFormationMenu;

	Label		*m_pSpecInfo;

	CUtlVector<SpecInfo> m_SpecList;

	int			m_nCurStat;
	int			m_nCurSpecIndex;
	Button		*m_pCurSpecButton;

	int			m_nSelectedPlayerIndex;

	bool		m_bIsStatsMenuEnabled;

	bool		m_bShowCaptainMenu;

	IScheme *m_pScheme;
};


#endif // CLIENTSCOREBOARDDIALOG_H
