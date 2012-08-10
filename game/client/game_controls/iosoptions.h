#ifndef IRC_IOSOPTIONS_H
#define IRC_IOSOPTIONS_H

#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>

using namespace vgui;
 
class CIOSOptionsPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CIOSOptionsPanel, vgui::Frame);

	CIOSOptionsPanel(vgui::VPANEL parent);
	~CIOSOptionsPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink();
	void OnCommand(const char *cmd);
	void Activate();
	void Reset();
	void Update();
	
protected:

	IScheme *m_pScheme;
	Panel *m_pContent;
	Label *m_pPlayerNameLabel;
	TextEntry *m_pPlayerNameText;
	Label *m_pClubNameLabel;
	TextEntry *m_pClubNameText;
	Label *m_pCountryNameLabel;
	ComboBox *m_pCountryNameList;
	Button *m_pOKButton;
	Button *m_pSaveButton;
	Button *m_pCancelButton;
	Panel *m_pShotButtonPanel;
	Label *m_pShotButtonLabel;
	RadioButton *m_pShotButtonLeft;
	RadioButton *m_pShotButtonRight;
};

class IIOSOptionsMenu
{
public:
	virtual void				Create( vgui::VPANEL parent ) = 0;
	virtual void				Destroy( void ) = 0;
	virtual CIOSOptionsPanel	*GetPanel( void ) = 0;
};

extern IIOSOptionsMenu *iosOptionsMenu;

#endif