#ifndef EMOTEMENU_H
#define EMOTEMENU_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_bitmapbutton.h>
#include <vgui_controls/ImagePanel.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include <UtlVector.h>

#include <vgui_controls/EditablePanel.h>

using namespace vgui;

#define EMOTE_COUNT 25

static const char *g_szEmotes[EMOTE_COUNT] = {
	"Pass!",
	"Shoot!",
	"Cross!",
	"Pass back!",
	"Clear it!",
	"Come out!",
	"Go up!",
	"Go back!",
	"Offside trap!",
	"Cut in!",
	"Stay wide!",
	"Pressure!",
	"Left!",
	"Right!",
	"Behind!",
	"Swap wings!",
	"Switch the play!",
	"I'm free!",
	"1-2 pass!",
	"Through pass!",
	"Short pass!",
	"Need help!",
	"Man on!",
	"Cover posts!",
	"Make a wall!",
};

class CEmoteMenu : public Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CEmoteMenu, Frame );

	Button *m_pEmoteButtons[EMOTE_COUNT];

	int m_nEmote;

	IScheme *m_pScheme;

	MESSAGE_FUNC_PTR( OnCursorEntered, "OnCursorEntered", panel );
	MESSAGE_FUNC_PTR( OnCursorExited, "OnCursorExited", panel );

public:
	CEmoteMenu(IViewPort *pViewPort);
	virtual ~CEmoteMenu();

	virtual const char *GetName( void ) { return PANEL_EMOTEMENU; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink() {};
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void PerformLayout();
	virtual void OnCommand( char const *cmd );
};

#endif