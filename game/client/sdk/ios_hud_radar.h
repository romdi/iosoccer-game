#include "cbase.h"
#include "hud.h"
#include "sdk_hud_stamina.h"
#include "hud_macros.h"
#include "c_sdk_player.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <igameresources.h>
#include "c_baseplayer.h"
#include "in_buttons.h"
#include "sdk_gamerules.h"
#include "c_ios_replaymanager.h"
#include "c_match_ball.h"
#include "view.h"
#include "ios_camera.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudRadar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudRadar, vgui::Panel);

public:
	CHudRadar(const char *pElementName);
	virtual void	Init(void);
	virtual void	Reset(void);
	virtual void	OnThink(void);
	bool			ShouldDraw(void);

protected:
	virtual void	ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void	Paint();

	float m_flNextUpdate;
};

DECLARE_HUDELEMENT(CHudRadar);

using namespace vgui;
