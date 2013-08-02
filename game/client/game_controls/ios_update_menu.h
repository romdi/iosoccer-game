#ifndef IOS_UPDATE_MENU_H
#define IOS_UPDATE_MENU_H

#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Button.h>

using namespace vgui;

enum PendingUpdate_t
{
	UPDATE_NONE_PENDING,
	UPDATE_CHECK_ONLY,
	UPDATE_CHECK_ONLY_AND_CLOSE,
	UPDATE_DOWNLOAD,
	UPDATE_DOWNLOAD_AND_CLOSE,
};

class CIOSUpdatePanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CIOSUpdatePanel, vgui::Frame);

	CIOSUpdatePanel(vgui::VPANEL parent);
	~CIOSUpdatePanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink();
	void OnCommand(const char *cmd);
	void Activate(PendingUpdate_t pendingUpdate);
	void Reset();
	void Update();
	void OnTick();
	void Init();
	
protected:

	IScheme *m_pScheme;
	Panel *m_pContent;
	Button *m_pUpdateButton;
	Button *m_pCloseButton;
	Label *m_pInfoText;
	Label *m_pExtraInfoText;
	PendingUpdate_t m_ePendingUpdate;
};

class IIOSUpdateMenu
{
public:
	virtual void				Create( vgui::VPANEL parent ) = 0;
	virtual void				Destroy( void ) = 0;
	virtual CIOSUpdatePanel	*GetPanel( void ) = 0;
};

extern IIOSUpdateMenu *iosUpdateMenu;

#endif