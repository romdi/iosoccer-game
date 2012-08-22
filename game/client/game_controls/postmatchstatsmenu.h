#ifndef POST_MATCH_STATS_MENU_H 
#define POST_MATCH_STATS_MENU_H 

#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <igameresources.h>
#include <vgui_controls/RadioButton.h>
#include "GameEventListener.h"

// Non RES-File Control Tutorial
#include <vgui_controls/Button.h>

using namespace vgui;

struct MatchStat
{
	Panel *pContainer;
	Label *pName;
	Label *pPlayers[2];
	Label *pValues[2];

	MatchStat(Panel *pParent, char *name)
	{
		pContainer = new Panel(pParent);
		pName = new Label(pContainer, "", name);
		pPlayers[0] = new Label(pContainer, "", "");
		pValues[0] = new Label(pContainer, "", "");
		pPlayers[1] = new Label(pContainer, "", "");
		pValues[1] = new Label(pContainer, "", "");
	}
};

enum match_stats_t
{
	LONGEST_DISTANCE_COVERED,
	SHORTEST_DISTANCE_COVERED,
	HIGHEST_POSSESSION,
	PLAYERS_CHOICE_MOTM,
	EXPERTS_CHOICE_MOTM,
	MATCH_STATS_COUNT
};

//CMyPanel class: Tutorial example class
class CPostMatchStatsMenu : public Frame, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CPostMatchStatsMenu, Frame); 
	//CMyPanel : This Class / vgui::Frame : BaseClass

	CPostMatchStatsMenu(IViewPort *pViewPort); 	// Constructor
	~CPostMatchStatsMenu(){};				// Destructor

	virtual void ApplySchemeSettings(IScheme *pScheme);

public:
	virtual const char *GetName( void ) { return PANEL_POSTMATCHSTATS; }// return identifer name
	virtual void SetData(KeyValues *data) {}; // set ViewPortPanel data
	virtual void Reset( void );		// clears internal state, deactivates it
	virtual void Update( void );	// updates all (size, position, content, etc)
	virtual bool NeedsUpdate( void ) { return false; } // query panel if content needs to be updated
	virtual bool HasInputElements( void ) { return true; }	// true if panel contains elments which accepts input

	virtual void ShowPanel( bool state ); // activate VGUI Frame
		
	// VGUI functions:
	virtual vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); } // returns VGUI panel handle
	virtual bool IsVisible() { return BaseClass::IsVisible(); }  // true if panel is visible
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent(parent); }
	void OnCommand(const char *cmd);
	void FireGameEvent(IGameEvent *event);

private:
	//Other used VGUI control Elements:

	// Our Code Defined Control
	MatchStat *m_pMatchStats[MATCH_STATS_COUNT];
	Button *m_pClose;
	IViewPort *m_pViewPort;
	Panel *m_pMainPanel;
	int m_nPlayersChoiceMotm[2];
	int m_nPlayersChoiceMotmPercentage[2];
	int m_nExpertsChoiceMotm[2];
	int m_nExpertsChoiceMotmPercentage[2];
};

#endif