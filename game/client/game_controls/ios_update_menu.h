#ifndef IOS_UPDATE_MENU_H
#define IOS_UPDATE_MENU_H

#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/progressbar.h>
#include "ios_fileupdater.h"

using namespace vgui;

enum UpdateState_t
{
	UPDATE_STATE_NONE,
	UPDATE_STATE_READY_TO_CHECK,
	UPDATE_STATE_CHECK_ONLY_PENDING,
	UPDATE_STATE_CHECK_ONLY_RUNNING,
	UPDATE_STATE_CHECK_ONLY_AND_CLOSE_PENDING,
	UPDATE_STATE_CHECK_ONLY_AND_CLOSE_RUNNING,
	UPDATE_STATE_CHECK_ONLY_FINISHED,
	UPDATE_STATE_CHECK_ONLY_AND_CLOSE_FINISHED,
	UPDATE_STATE_DOWNLOAD_PENDING,
	UPDATE_STATE_DOWNLOAD_RUNNING,
	UPDATE_STATE_DOWNLOAD_FINISHED,
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
	void Activate(UpdateState_t updateState);
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
	UpdateState_t m_eUpdateState;
	IOSUpdateInfo *m_pUpdateInfo;
	ProgressBar *m_pProgressBar;
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