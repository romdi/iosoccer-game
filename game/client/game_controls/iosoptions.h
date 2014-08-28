#ifndef IRC_IOSOPTIONS_H
#define IRC_IOSOPTIONS_H

#include "cbase.h"
#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/tooltip.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Slider.h>
#include "GameEventListener.h"

using namespace vgui;

enum SettingPanel_t
{
	SETTING_PANEL_NETWORK, SETTING_PANEL_APPEARANCE, SETTING_PANEL_GAMEPLAY, SETTING_PANEL_VISUAL, SETTING_PANEL_SOUND, SETTING_PANEL_COUNT
};

class ISettingPanel
{
public:

	virtual ~ISettingPanel() {};
	virtual void Save() = 0;
	virtual void Load() = 0;
	virtual void Update() = 0;
	virtual void PerformLayout() = 0;
};

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
	void OnTick();
	void Init();
	ISettingPanel *GetSettingPanel(SettingPanel_t panel) { return m_pSettingPanels[panel]; }
	
protected:

	IScheme *m_pScheme;
	PropertySheet *m_pContent;
	Button *m_pOKButton;
	Button *m_pSaveButton;
	Button *m_pCancelButton;
	Label *m_pChangeInfoText;

	ISettingPanel *m_pSettingPanels[SETTING_PANEL_COUNT];
};

class IIOSOptionsMenu
{
public:
	virtual void				Create( vgui::VPANEL parent ) = 0;
	virtual void				Destroy( void ) = 0;
	virtual CIOSOptionsPanel	*GetPanel( void ) = 0;
};

extern IIOSOptionsMenu *iosOptionsMenu;

class CNetworkSettingPanel : public PropertyPage, public ISettingPanel
{
	DECLARE_CLASS_SIMPLE(CNetworkSettingPanel, PropertyPage);

	Panel *m_pContent;

	Label *m_pPlayerNameLabel;
	TextEntry *m_pPlayerNameText;

	Label *m_pClubNameLabel;
	TextEntry *m_pClubNameText;

	Label *m_pNationalTeamNameLabel;
	TextEntry *m_pNationalTeamNameText;

	Label *m_pCountryNameLabel;
	ComboBox *m_pCountryNameList;

	Label *m_pNationalityNameLabel;
	ComboBox *m_pNationalityNameList;

	Label *m_pInterpDurationLabel;
	ComboBox *m_pInterpDurationList;
	Button *m_pInterpDurationSuggestedValueButton;
	Button *m_pInterpDurationInfoButton;

	Label *m_pSmoothDurationLabel;
	ComboBox *m_pSmoothDurationList;
	Button *m_pSmoothDurationSuggestedValueButton;
	Button *m_pSmoothDurationInfoButton;

	Label *m_pRateLabel;
	ComboBox *m_pRateList;
	Button *m_pRateSuggestedValueButton;
	Button *m_pRateInfoButton;

	Label *m_pUpdaterateLabel;
	ComboBox *m_pUpdaterateList;
	Button *m_pUpdaterateSuggestedValueButton;
	Button *m_pUpdaterateInfoButton;

	Label *m_pCommandrateLabel;
	ComboBox *m_pCommandrateList;
	Button *m_pCommandrateSuggestedValueButton;
	Button *m_pCommandrateInfoButton;

public:

	CNetworkSettingPanel(Panel *parent, const char *panelName);
	void PerformLayout();
	void OnCommand(const char *cmd);
	void Save();
	void Load();
	void Update();
};

class CAppearanceSettingPanel : public PropertyPage, public ISettingPanel
{
	DECLARE_CLASS_SIMPLE(CAppearanceSettingPanel, PropertyPage);

	Panel *m_pContent;

	Label *m_pShirtNameLabel;
	TextEntry *m_pShirtNameText;

	Label *m_pSkinIndexLabel;
	ComboBox *m_pSkinIndexList;
	Label *m_pPreferredOutfieldShirtNumberLabel;
	ComboBox *m_pPreferredOutfieldShirtNumberList;
	Label *m_pPreferredKeeperShirtNumberLabel;
	ComboBox *m_pPreferredKeeperShirtNumberList;
	Label *m_pPlayerBallSkinLabel;
	ComboBox *m_pPlayerBallSkinList;
	ImagePanel *m_pPlayerPreviewPanel;
	Slider *m_pPlayerAngleSlider;
	CheckButton *m_pPlayerAngleAutoRotate;
	Label *m_pPreviewTeamLabel;
	ComboBox *m_pPreviewTeamList;
	Panel *m_pBodypartPanel;
	RadioButton *m_pBodypartRadioButtons[3];
	Label *m_pConnectionInfoLabel;

	float m_flLastTeamKitUpdateTime;
	float m_flLastBallSkinUpdateTime;

public:

	CAppearanceSettingPanel(Panel *parent, const char *panelName);
	void PerformLayout();
	void Save();
	void Load();
	void Update();
	Panel *GetPlayerPreviewPanel() { return m_pPlayerPreviewPanel; }
	const char *GetPlayerShirtName();
	int GetPlayerSkinIndex();
	int GetPlayerOutfieldShirtNumber();
	void GetPlayerTeamInfo(const char **teamFolder, const char **kitFolder);
	float GetPlayerPreviewAngle();
	int GetPlayerBodypart();
};

class CGameplaySettingPanel : public PropertyPage, public ISettingPanel
{
	DECLARE_CLASS_SIMPLE(CGameplaySettingPanel, PropertyPage);

	Panel *m_pContent;

	CheckButton *m_pLegacySideCurl;
	CheckButton *m_pLegacyVerticalLook;

public:

	CGameplaySettingPanel(Panel *parent, const char *panelName);
	void PerformLayout();
	void Save();
	void Load();
	void Update();
};

class CSoundSettingPanel : public PropertyPage, public ISettingPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CSoundSettingPanel, PropertyPage);

	int m_nCrowdBgGuid;
	int m_nCrowdEventGuid;

	Panel *m_pContent;

	Slider *m_pCrowdBgVolume;
	CheckButton *m_pCrowdBg;

	Slider *m_pCrowdEventVolume;
	CheckButton *m_pCrowdEvent;

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

	bool m_bIsFirstTick;

public:

	CSoundSettingPanel(Panel *parent, const char *panelName);
	void PerformLayout();
	void Save();
	void Load();
	void Update();
	void FireGameEvent(IGameEvent *event);
	void OnTick();
};

class CVisualSettingPanel : public PropertyPage, public ISettingPanel
{
	DECLARE_CLASS_SIMPLE(CVisualSettingPanel, PropertyPage);

	Panel *m_pContent;

public:

	CVisualSettingPanel(Panel *parent, const char *panelName);
	void PerformLayout();
	void Save();
	void Load();
	void Update();
};

#endif