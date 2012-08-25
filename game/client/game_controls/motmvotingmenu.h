#ifndef MOTM_VOTING_MENU_H 
#define MOTM_VOTING_MENU_H 

#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <igameresources.h>
#include <vgui_controls/RadioButton.h>

// Non RES-File Control Tutorial
#include <vgui_controls/Button.h>

using namespace vgui;

//CMyPanel class: Tutorial example class
class CMotmVotingMenu : public vgui::Frame, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE(CMotmVotingMenu, vgui::Frame); 
	//CMyPanel : This Class / vgui::Frame : BaseClass

	CMotmVotingMenu(IViewPort *pViewPort); 	// Constructor
	~CMotmVotingMenu(){};				// Destructor

	virtual void ApplySchemeSettings(IScheme *pScheme);

public:
	virtual const char *GetName( void ) { return PANEL_MOTMVOTING; }// return identifer name
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
private:
	//Other used VGUI control Elements:

	// Our Code Defined Control
	Panel *m_pTeamPanels[2];
	RadioButton *m_pPlayerNames[2][11];
	Button *m_pVote;
	Panel *m_pMainPanel;
	Label *m_pTitle;
};

#endif