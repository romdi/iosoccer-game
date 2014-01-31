#include "cbase.h"
#include "iosoptions.h"
#include "ienginevgui.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"
#include "ios_teamkit_parse.h"
#include "c_ball.h"
#include "vgui/IVgui.h"

extern ConVar
	autohidespecmenu,
	centeredstaminabar,
	rate,
	clubname,
	nationalteamname,
	fallbackcountryindex,
	nationalityindex,
	goalteamcrests,
	legacysidecurl,
	legacyverticallook,
	modelskinindex,
	playerballskinname,
	playername,
	preferredkeepershirtnumber,
	preferredoutfieldshirtnumber,
	quicktacticpanel,
	shirtname;

class CIOSOptionsMenu : public IIOSOptionsMenu
{
private:
	CIOSOptionsPanel *m_pIOSOptionsPanel;
	vgui::VPANEL m_hParent;
 
public:
	CIOSOptionsMenu( void )
	{
		m_pIOSOptionsPanel = NULL;
	}
 
	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		m_pIOSOptionsPanel = new CIOSOptionsPanel(parent);
	}
 
	void Destroy( void )
	{
		if ( m_pIOSOptionsPanel )
		{
			m_pIOSOptionsPanel->SetParent( (vgui::Panel *)NULL );
			delete m_pIOSOptionsPanel;
		}
	}
	CIOSOptionsPanel *GetPanel()
	{
		return m_pIOSOptionsPanel;
	}
};

static CIOSOptionsMenu g_IOSOptionsMenu;
IIOSOptionsMenu *iosOptionsMenu = (IIOSOptionsMenu *)&g_IOSOptionsMenu;

void CC_IOSOptionsMenu(const CCommand &args)
{
	if (!iosOptionsMenu->GetPanel()->IsVisible())
		iosOptionsMenu->GetPanel()->Activate();
	else
		iosOptionsMenu->GetPanel()->Close();
}

ConCommand iosoptionsmenu("iosoptionsmenu", CC_IOSOptionsMenu);

enum { LABEL_WIDTH = 260, INPUT_WIDTH = 260, SHORTINPUT_WIDTH = 200, TEXT_HEIGHT = 26, TEXT_MARGIN = 5 };
enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = (1024 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (720 - 2 * PANEL_MARGIN) };
enum { PADDING = 10, TOP_PADDING = 30 };
enum { BUTTON_WIDTH = 80, BUTTON_HEIGHT = 26, BUTTON_MARGIN = 5 };
enum { SUGGESTED_VALUE_WIDTH = 150, SUGGESTED_VALUE_MARGIN = 5 };
enum { INFOBUTTON_WIDTH = 30, INFOBUTTON_MARGIN = 5 };
enum { APPEARANCE_HOFFSET = 270, APPEARANCE_RADIOBUTTONWIDTH = 70, RENDER_TEXTURE_WIDTH = 256, RENDER_TEXTURE_HEIGHT = 512 };

#define INTERP_VALUES 5
const int interpValues[INTERP_VALUES] = { 1, 2, 3, 4, 5 };
const char *interpTexts[INTERP_VALUES] = { "Very Short (1.0)", "Short (2.0)", "Medium (3.0)", "Long (4.0)", "Very Long (5.0)" };
#define SMOOTH_VALUES 5
const int smoothValues[SMOOTH_VALUES] = { 1, 5, 10, 25, 50 };
const char *smoothTexts[SMOOTH_VALUES] = { "Very Short (0.01s)", "Short (0.05s)", "Medium (0.1s)", "Long (0.25s)", "Very Long (0.5s)" };

#define MAX_VISIBLE_DROPDOWN 20

CIOSOptionsPanel::CIOSOptionsPanel(VPANEL parent) : BaseClass(NULL, "IOSOptionsPanel")
{
	SetScheme("SourceScheme");
	SetParent(parent);
	m_pContent = new PropertySheet(this, "");
	m_pSettingPanels[SETTING_PANEL_NETWORK] = new CNetworkSettingPanel(m_pContent, "");
	m_pSettingPanels[SETTING_PANEL_APPEARANCE] = new CAppearanceSettingPanel(m_pContent, "");
	m_pSettingPanels[SETTING_PANEL_GAMEPLAY] = new CGameplaySettingPanel(m_pContent, "");
	m_pSettingPanels[SETTING_PANEL_VISUAL] = new CVisualSettingPanel(m_pContent, "");
	m_pSettingPanels[SETTING_PANEL_SOUND] = new CSoundSettingPanel(m_pContent, "");
	m_pContent->AddPage(dynamic_cast<CNetworkSettingPanel *>(m_pSettingPanels[SETTING_PANEL_NETWORK]), "Network");
	m_pContent->AddPage(dynamic_cast<CAppearanceSettingPanel *>(m_pSettingPanels[SETTING_PANEL_APPEARANCE]), "Appearance");
	m_pContent->AddPage(dynamic_cast<CGameplaySettingPanel *>(m_pSettingPanels[SETTING_PANEL_GAMEPLAY]), "Gameplay");
	m_pContent->AddPage(dynamic_cast<CVisualSettingPanel *>(m_pSettingPanels[SETTING_PANEL_VISUAL]), "Visuals");
	m_pContent->AddPage(dynamic_cast<CSoundSettingPanel *>(m_pSettingPanels[SETTING_PANEL_SOUND]), "Sound");
	m_pOKButton = new Button(this, "", "OK", this, "save_and_close");
	m_pCancelButton = new Button(this, "", "Cancel", this, "close");
	m_pSaveButton = new Button(this, "", "Apply", this, "save_settings");
	m_pChangeInfoText = new Label(this, "", "Go spectator mode to change");

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CIOSOptionsPanel::~CIOSOptionsPanel()
{
}

void CIOSOptionsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;
	BaseClass::ApplySchemeSettings( pScheme );

	SetTitle("IOS Settings", false);
	SetProportional(false);
	SetSizeable(false);
	SetBounds(0, 0, 600, PANEL_HEIGHT);
	//SetBgColor(Color(0, 0, 0, 255));
	SetPaintBackgroundEnabled(true);
	MoveToCenterOfScreen();

	m_pContent->SetBounds(PADDING, PADDING + TOP_PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING - TOP_PADDING - BUTTON_HEIGHT - PADDING);
	m_pOKButton->SetBounds(this->GetWide() - 3 * BUTTON_WIDTH - 2 * BUTTON_MARGIN - PADDING, this->GetTall() - BUTTON_HEIGHT - PADDING, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pCancelButton->SetBounds(this->GetWide() - 2 * BUTTON_WIDTH - BUTTON_MARGIN - PADDING, this->GetTall() - BUTTON_HEIGHT - PADDING, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pSaveButton->SetBounds(this->GetWide() - BUTTON_WIDTH - PADDING, this->GetTall() - BUTTON_HEIGHT - PADDING, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pChangeInfoText->SetBounds(PADDING, this->GetTall() - BUTTON_HEIGHT - PADDING, this->GetWide() - 3 * BUTTON_WIDTH, BUTTON_HEIGHT); 
	m_pChangeInfoText->SetFgColor(Color(255, 153, 153, 255));

	for (int i = 0; i < SETTING_PANEL_COUNT; i++)
		m_pSettingPanels[i]->PerformLayout();
}

void CIOSOptionsPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CIOSOptionsPanel::OnThink()
{
	BaseClass::OnThink();

	//SetTall((int)(gpGlobals->curtime * 100) % 100);
	//m_pSettingPanel->Update();
	for (int i = 0; i < SETTING_PANEL_COUNT; i++)
		m_pSettingPanels[i]->Update();

	C_SDKPlayer *pLocal = C_SDKPlayer::GetLocalSDKPlayer();

	if (pLocal)
	{
		if (gpGlobals->curtime < pLocal->m_flNextClientSettingsChangeTime)
		{
			//char *text = VarArgs("Wait %d seconds", (int)(pLocal->m_flNextClientSettingsChangeTime - gpGlobals->curtime));
			//m_pOKButton->SetText(text);
			m_pOKButton->SetEnabled(false);
			//m_pSaveButton->SetText(text);
			m_pSaveButton->SetEnabled(false);
			m_pChangeInfoText->SetVisible(true);
			m_pChangeInfoText->SetText(VarArgs("Wait %d seconds to change", (int)(pLocal->m_flNextClientSettingsChangeTime - gpGlobals->curtime)));
			return;
		}
		else if (!SDKGameRules()->IsIntermissionState() && (GetLocalPlayerTeam() == TEAM_A || GetLocalPlayerTeam() == TEAM_B))
		{
			//char *text = VarArgs("Go spec first", (int)(pLocal->m_flNextClientSettingsChangeTime - gpGlobals->curtime));
			//m_pOKButton->SetText(text);
			m_pOKButton->SetEnabled(false);
			//m_pSaveButton->SetText(text);
			m_pSaveButton->SetEnabled(false);
			m_pChangeInfoText->SetVisible(true);
			m_pChangeInfoText->SetText("Go spectator mode to change");
			return;
		}
	}

	//m_pOKButton->SetText("OK");
	m_pOKButton->SetEnabled(true);
	//m_pSaveButton->SetText("Apply");
	m_pSaveButton->SetEnabled(true);
	m_pChangeInfoText->SetVisible(false);
}

void CIOSOptionsPanel::Init()
{
}

void CIOSOptionsPanel::OnTick()
{
	BaseClass::OnTick();

	dynamic_cast<CSoundSettingPanel *>(m_pSettingPanels[SETTING_PANEL_SOUND])->OnTick();
}

void CIOSOptionsPanel::OnCommand(const char *cmd)
{
	if (!stricmp(cmd, "save_settings") || !stricmp(cmd, "save_and_close"))
	{
		for (int i = 0; i < SETTING_PANEL_COUNT; i++)
			m_pSettingPanels[i]->Save();
		
		engine->ClientCmd("host_writeconfig\n");

		if (!stricmp(cmd, "save_and_close"))
			Close();
	}
	else if (!stricmp(cmd, "close"))
	{
		Close();
	}
	else
		BaseClass::OnCommand(cmd);
}

void CIOSOptionsPanel::Activate()
{
	BaseClass::Activate();

	for (int i = 0; i < SETTING_PANEL_COUNT; i++)
		m_pSettingPanels[i]->Load();
}

CNetworkSettingPanel::CNetworkSettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");

	m_pPlayerNameLabel = new Label(m_pContent, "", "Player Name:");
	m_pPlayerNameText = new TextEntry(m_pContent, "");
	m_pPlayerNameText->SetMaximumCharCount(MAX_PLAYER_NAME_LENGTH - 1);
	m_pPlayerNameText->SetAllowNonAsciiCharacters(true);

	m_pClubNameLabel = new Label(m_pContent, "", "IOS Club Initials:");
	m_pClubNameText = new TextEntry(m_pContent, "");
	m_pClubNameText->SetMaximumCharCount(MAX_CLUBNAME_LENGTH - 1);
	m_pClubNameText->SetAllowNonAsciiCharacters(true);

	m_pNationalTeamNameLabel = new Label(m_pContent, "", "IOS National Team Initials:");
	m_pNationalTeamNameText = new TextEntry(m_pContent, "");
	m_pNationalTeamNameText->SetMaximumCharCount(MAX_CLUBNAME_LENGTH - 1);
	m_pNationalTeamNameText->SetAllowNonAsciiCharacters(true);

	m_pCountryNameLabel = new Label(m_pContent, "", "Location (Fallback):");
	m_pCountryNameList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pCountryNameList->RemoveAll();

	KeyValues *kv = NULL;

	kv = new KeyValues("UserData", "index", 0);
	m_pCountryNameList->AddItem("<None>", kv);
	kv->deleteThis();

	for (int i = 1; i < COUNTRY_NAMES_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}

	m_pNationalityNameLabel = new Label(m_pContent, "", "Nationality:");
	m_pNationalityNameList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pNationalityNameList->RemoveAll();

	kv = new KeyValues("UserData", "index", 0);
	m_pNationalityNameList->AddItem("<None>", kv);
	kv->deleteThis();

	for (int i = 1; i < COUNTRY_NAMES_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pNationalityNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}

	const char *suggestedValueText = "Set Recommended Value";

	m_pInterpDurationLabel = new Label(m_pContent, "", "Network Smoothing Ratio (cl_interp_ratio):");
	m_pInterpDurationList = new ComboBox(m_pContent, "", 0, false);
	m_pInterpDurationSuggestedValueButton = new Button(m_pContent, "", suggestedValueText, this, "suggested_interpduration");
	m_pInterpDurationInfoButton = new Button(m_pContent, "", "?");
	m_pInterpDurationInfoButton->GetTooltip()->SetText("The shorter the interpolation duration, the quicker your client will display updated player and ball positions received from the server.\nIf you notice that other players and the ball don't move smoothly, it could mean that too many packets are lost on the way between you and the server.\nTry increasing the interpolation duration until the game is smooth again.");
	m_pInterpDurationInfoButton->GetTooltip()->SetTooltipDelay(0);

	for (int i = 0; i < INTERP_VALUES; i++)
	{
		kv = new KeyValues("UserData", "value", interpValues[i]);
		m_pInterpDurationList->AddItem(interpTexts[i], kv);
		kv->deleteThis();
	}

	m_pSmoothDurationLabel = new Label(m_pContent, "", "Local Smoothing Duration (cl_smoothtime):");
	m_pSmoothDurationList = new ComboBox(m_pContent, "", 0, false);
	m_pSmoothDurationInfoButton = new Button(m_pContent, "", "?");
	m_pSmoothDurationSuggestedValueButton = new Button(m_pContent, "", suggestedValueText, this, "suggested_smoothduration");
	m_pSmoothDurationInfoButton->GetTooltip()->SetText("The shorter the smoothing duration, the quicker your client will set your local player to the correct position, should your client have incorrectly predicted your own position.\nTo make the game feel more reponsive, your client immediately performs certain actions like moving around and jumping, instead of waiting for the server to give confirmation for them.\nSometimes, when other players or the ball is close to you, the predictions of the client will be wrong and your local player can't actually move to the position he just went to during the prediction.\nThe smoothing duration is the time your client spends moving your own player to the correct position as received from the server.");
	m_pSmoothDurationInfoButton->GetTooltip()->SetTooltipDelay(0);

	for (int i = 0; i < SMOOTH_VALUES; i++)
	{
		kv = new KeyValues("UserData", "value", smoothValues[i]);
		m_pSmoothDurationList->AddItem(smoothTexts[i], kv);
		kv->deleteThis();
	}

	m_pRateLabel = new Label(m_pContent, "", "Max bandwidth used (rate) / 1000:");
	m_pRateList = new ComboBox(m_pContent, "", 0, false);
	m_pRateSuggestedValueButton = new Button(m_pContent, "", suggestedValueText, this, "suggested_rate");
	m_pRateInfoButton = new Button(m_pContent, "", "?");
	m_pRateInfoButton->GetTooltip()->SetText("'rate' sets the maximum bandwidth available for receiving packets from the server.\nIf 'net_graph 3' shows choke, increase the rate until the choke value shows 0.\nIf you can't increase 'Rate' any further due to a slow connection, consider lowering 'Update Rate' and 'Command Rate'.");
	m_pRateInfoButton->GetTooltip()->SetTooltipDelay(0);

	for (int i = 1; i <= 20; i++)
	{
		int value = i * 5;
		KeyValues *kv = new KeyValues("UserData", "value", VarArgs("%d", value));
		m_pRateList->AddItem(VarArgs("%d KB/s", value), kv);
		kv->deleteThis();
	}

	m_pUpdaterateLabel = new Label(m_pContent, "", "Max packets received per sec (cl_updaterate):");
	m_pUpdaterateList = new ComboBox(m_pContent, "", 0, false);
	m_pUpdaterateSuggestedValueButton = new Button(m_pContent, "", suggestedValueText, this, "suggested_updaterate");
	m_pUpdaterateInfoButton = new Button(m_pContent, "", "?");
	m_pUpdaterateInfoButton->GetTooltip()->SetText("'cl_updaterate' sets the number of updates per second you want to receive from the server.\nThe maximum value is the current server tickrate, which is usually 66 or 100.\nThe higher 'Update Rate' the more download bandwidth will be used.");
	m_pUpdaterateInfoButton->GetTooltip()->SetTooltipDelay(0);

	for (int i = 1; i <= 10; i++)
	{
		int value = i * 10;
		KeyValues *kv = new KeyValues("UserData", "value", VarArgs("%d", value));
		m_pUpdaterateList->AddItem(VarArgs("%d/s", value), kv);
		kv->deleteThis();
	}

	m_pCommandrateLabel = new Label(m_pContent, "", "Max packets sent per sec (cl_cmdrate):");
	m_pCommandrateList = new ComboBox(m_pContent, "", 0, false);
	m_pCommandrateSuggestedValueButton = new Button(m_pContent, "", suggestedValueText, this, "suggested_commandrate");
	m_pCommandrateInfoButton = new Button(m_pContent, "", "?");
	m_pCommandrateInfoButton->GetTooltip()->SetText("'cl_cmdrate' sets the number of input updates per second you want to send to the server.\nThe maximum value is the current server tickrate, which is usually 66 or 100.\nThe higher 'Command Rate' the more upload bandwidth will be used.");
	m_pCommandrateInfoButton->GetTooltip()->SetTooltipDelay(0);

	for (int i = 1; i <= 10; i++)
	{
		int value = i * 10;
		KeyValues *kv = new KeyValues("UserData", "value", VarArgs("%d", value));
		m_pCommandrateList->AddItem(VarArgs("%d/s", value), kv);
		kv->deleteThis();
	}
}

void CNetworkSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(0, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message

	m_pClubNameLabel->SetBounds(0, 2 * TEXT_HEIGHT + TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(0, 3 * TEXT_HEIGHT + TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);

	m_pNationalTeamNameLabel->SetBounds(0, 4 * TEXT_HEIGHT + TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pNationalTeamNameText->SetBounds(0, 5 * TEXT_HEIGHT + TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);

	m_pCountryNameLabel->SetBounds(0, 6 * TEXT_HEIGHT + 2 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->SetBounds(0, 7 * TEXT_HEIGHT + 2 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);

	m_pNationalityNameLabel->SetBounds(0, 8 * TEXT_HEIGHT + 2 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pNationalityNameList->SetBounds(0, 9 * TEXT_HEIGHT + 2 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);

	m_pInterpDurationLabel->SetBounds(0, 10 * TEXT_HEIGHT + 3 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationList->SetBounds(0, 11 * TEXT_HEIGHT + 3 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationSuggestedValueButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN, 11 * TEXT_HEIGHT + 3 * TEXT_MARGIN, SUGGESTED_VALUE_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pInterpDurationInfoButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 11 * TEXT_HEIGHT + 3 * TEXT_MARGIN, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pSmoothDurationLabel->SetBounds(0, 12 * TEXT_HEIGHT + 4 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationList->SetBounds(0, 13 * TEXT_HEIGHT + 4 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationSuggestedValueButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN, 13 * TEXT_HEIGHT + 4 * TEXT_MARGIN, SUGGESTED_VALUE_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pSmoothDurationInfoButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 13 * TEXT_HEIGHT + 4 * TEXT_MARGIN, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pRateLabel->SetBounds(0, 14 * TEXT_HEIGHT + 5 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pRateList->SetBounds(0, 15 * TEXT_HEIGHT + 5 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);
	m_pRateSuggestedValueButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN, 15 * TEXT_HEIGHT + 5 * TEXT_MARGIN, SUGGESTED_VALUE_WIDTH, TEXT_HEIGHT);
	m_pRateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pRateInfoButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 15 * TEXT_HEIGHT + 5 * TEXT_MARGIN, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pRateInfoButton->SetContentAlignment(Label::a_center);

	m_pUpdaterateLabel->SetBounds(0, 16 * TEXT_HEIGHT + 6 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateList->SetBounds(0, 17 * TEXT_HEIGHT + 6 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateSuggestedValueButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN, 17 * TEXT_HEIGHT + 6 * TEXT_MARGIN, SUGGESTED_VALUE_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pUpdaterateInfoButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 17 * TEXT_HEIGHT + 6 * TEXT_MARGIN, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateInfoButton->SetContentAlignment(Label::a_center);

	m_pCommandrateLabel->SetBounds(0, 18 * TEXT_HEIGHT + 7 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCommandrateList->SetBounds(0, 19 * TEXT_HEIGHT + 7 * TEXT_MARGIN, INPUT_WIDTH, TEXT_HEIGHT);
	m_pCommandrateSuggestedValueButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN, 19 * TEXT_HEIGHT + 7 * TEXT_MARGIN, SUGGESTED_VALUE_WIDTH, TEXT_HEIGHT);
	m_pCommandrateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pCommandrateInfoButton->SetBounds(INPUT_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 19 * TEXT_HEIGHT + 7 * TEXT_MARGIN, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pCommandrateInfoButton->SetContentAlignment(Label::a_center);
}

void CNetworkSettingPanel::OnCommand(const char *cmd)
{
	if (!Q_strcmp(cmd, "suggested_interpduration"))
		m_pInterpDurationList->ActivateItemByRow(0);
	if (!Q_strcmp(cmd, "suggested_smoothduration"))
		m_pSmoothDurationList->ActivateItemByRow(3);
	else if (!Q_strcmp(cmd, "suggested_rate"))
		m_pRateList->ActivateItemByRow(m_pRateList->GetItemCount() - 1);
	else if (!Q_strcmp(cmd, "suggested_updaterate"))
		m_pUpdaterateList->ActivateItemByRow(m_pUpdaterateList->GetItemCount() - 1);
	else if (!Q_strcmp(cmd, "suggested_commandrate"))
		m_pCommandrateList->ActivateItemByRow(m_pCommandrateList->GetItemCount() - 1);
	else
		BaseClass::OnCommand(cmd);
}

void CNetworkSettingPanel::Save()
{
	char text[64];
	m_pPlayerNameText->GetText(text, sizeof(text));
	playername.SetValue(text);

	m_pClubNameText->GetText(text, sizeof(text));
	clubname.SetValue(text);

	m_pNationalTeamNameText->GetText(text, sizeof(text));
	nationalteamname.SetValue(text);

	fallbackcountryindex.SetValue(m_pCountryNameList->GetActiveItemUserData()->GetInt("index"));

	nationalityindex.SetValue(m_pNationalityNameList->GetActiveItemUserData()->GetInt("index"));

	g_pCVar->FindVar("cl_interp_ratio")->SetValue(atoi(m_pInterpDurationList->GetActiveItemUserData()->GetString("value")));
	g_pCVar->FindVar("cl_smoothtime")->SetValue(atoi(m_pSmoothDurationList->GetActiveItemUserData()->GetString("value")) / 100.0f);

	g_pCVar->FindVar("rate")->SetValue(m_pRateList->GetActiveItemUserData()->GetInt("value") * 1000);
	g_pCVar->FindVar("cl_updaterate")->SetValue(m_pUpdaterateList->GetActiveItemUserData()->GetInt("value"));
	g_pCVar->FindVar("cl_cmdrate")->SetValue(m_pCommandrateList->GetActiveItemUserData()->GetInt("value"));
}

void CNetworkSettingPanel::Load()
{
	m_pPlayerNameText->SetText(playername.GetString());
	m_pCountryNameList->ActivateItemByRow(fallbackcountryindex.GetInt());
	m_pNationalityNameList->ActivateItemByRow(nationalityindex.GetInt());
	m_pClubNameText->SetText(clubname.GetString());
	m_pNationalTeamNameText->SetText(nationalteamname.GetString());

	m_pInterpDurationList->ActivateItemByRow(0);

	for (int i = 0; i < INTERP_VALUES; i++)
	{
		if (interpValues[i] == (int)g_pCVar->FindVar("cl_interp_ratio")->GetFloat())
		{
			m_pInterpDurationList->ActivateItemByRow(i);
			break;
		}
	}

	m_pSmoothDurationList->ActivateItemByRow(0);

	for (int i = 0; i < SMOOTH_VALUES; i++)
	{
		if (smoothValues[i] == (int)(g_pCVar->FindVar("cl_smoothtime")->GetFloat() * 100))
		{
			m_pSmoothDurationList->ActivateItemByRow(i);
			break;
		}
	}

	m_pRateList->ActivateItemByRow(g_pCVar->FindVar("rate")->GetInt() / 1000 / 5 - 1);
	m_pUpdaterateList->ActivateItemByRow(g_pCVar->FindVar("cl_updaterate")->GetInt() / 10 - 1);
	m_pCommandrateList->ActivateItemByRow(g_pCVar->FindVar("cl_cmdrate")->GetInt() / 10 - 1);
}

void CNetworkSettingPanel::Update()
{
}

CAppearanceSettingPanel::CAppearanceSettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");

	m_pPlayerPreviewPanel = new ImagePanel(m_pContent, "");

	m_pShirtNameLabel = new Label(m_pContent, "", "Shirt Name:");
	m_pShirtNameText = new TextEntry(m_pContent, "");
	m_pShirtNameText->SetMaximumCharCount(MAX_PLAYER_NAME_LENGTH - 1);
	m_pShirtNameText->SetAllowNonAsciiCharacters(false);

	m_pSkinIndexLabel = new Label(m_pContent, "", "Player Skin:");
	m_pSkinIndexList = new ComboBox(m_pContent, "", 0, false);

	KeyValues *kv = NULL;

	kv = new KeyValues("UserData", "index", 0);
	m_pSkinIndexList->AddItem("Dark skin", kv);
	kv->deleteThis();

	kv = new KeyValues("UserData", "index", 1);
	m_pSkinIndexList->AddItem("Light skin, blond hair", kv);
	kv->deleteThis();

	kv = new KeyValues("UserData", "index", 2);
	m_pSkinIndexList->AddItem("Light skin, brown hair", kv);
	kv->deleteThis();

	kv = new KeyValues("UserData", "index", 3);
	m_pSkinIndexList->AddItem("Light skin, black hair", kv);
	kv->deleteThis();

	kv = new KeyValues("UserData", "index", 4);
	m_pSkinIndexList->AddItem("Light skin, black hair, beard", kv);
	kv->deleteThis();

	kv = new KeyValues("UserData", "index", 5);
	m_pSkinIndexList->AddItem("Darkish skin", kv);
	kv->deleteThis();


	m_pPreferredOutfieldShirtNumberLabel = new Label(m_pContent, "", "Preferred Outfield Shirt Number:");
	m_pPreferredOutfieldShirtNumberList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	for (int i = 2; i <= 99; i++)
	{
		kv = new KeyValues("UserData", "number", i);
		m_pPreferredOutfieldShirtNumberList->AddItem(VarArgs("%d", i), kv);
		kv->deleteThis();
	}


	m_pPreferredKeeperShirtNumberLabel = new Label(m_pContent, "", "Preferred Keeper Shirt Number:");
	m_pPreferredKeeperShirtNumberList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	for (int i = 1; i <= 99; i++)
	{
		kv = new KeyValues("UserData", "number", i);
		m_pPreferredKeeperShirtNumberList->AddItem(VarArgs("%d", i), kv);
		kv->deleteThis();
	}

	m_pPlayerBallSkinLabel = new Label(m_pContent, "", "Player Ball Skin:");
	m_pPlayerBallSkinList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pPreviewTeamLabel = new Label(m_pContent, "", "Preview Team Kit:");
	m_pPreviewTeamList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pBodypartPanel = new Panel(m_pContent, "");
	m_pBodypartRadioButtons[0] = new RadioButton(m_pBodypartPanel, "", "Head");
	m_pBodypartRadioButtons[1] = new RadioButton(m_pBodypartPanel, "", "Body");
	m_pBodypartRadioButtons[2] = new RadioButton(m_pBodypartPanel, "", "Shoes");
	m_pBodypartRadioButtons[1]->SetSelected(true);

	m_pPlayerAngleSlider = new Slider(m_pContent, "");
	m_pPlayerAngleSlider->SetRange(-18000, 18000);
	m_pPlayerAngleSlider->SetValue(0);

	m_pPlayerAngleAutoRotate = new CheckButton(m_pContent, "", "Auto-rotate the player model preview");
	m_pPlayerAngleAutoRotate->SetSelected(true);

	m_pConnectionInfoLabel = new Label(m_pContent, "", "No preview when disconnected");

	m_flLastTeamKitUpdateTime = -1;
	m_flLastBallSkinUpdateTime = -1;
}

void CAppearanceSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	m_pPlayerPreviewPanel->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, 0, RENDER_TEXTURE_WIDTH, RENDER_TEXTURE_HEIGHT);
	m_pPlayerPreviewPanel->SetImage("../_rt_playermodel");
	m_pPlayerPreviewPanel->SetShouldScaleImage(false);

	m_pShirtNameLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 0 * TEXT_HEIGHT + 0 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pShirtNameText->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 1 * TEXT_HEIGHT + 0 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pPreferredKeeperShirtNumberLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 2 * TEXT_HEIGHT + 1 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPreferredKeeperShirtNumberList->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 3 * TEXT_HEIGHT + 1 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pPreferredOutfieldShirtNumberLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 4 * TEXT_HEIGHT + 2 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPreferredOutfieldShirtNumberList->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 5 * TEXT_HEIGHT + 2 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pSkinIndexLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 6 * TEXT_HEIGHT + 3 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pSkinIndexList->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 7 * TEXT_HEIGHT + 3 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pPlayerBallSkinLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 8 * TEXT_HEIGHT + 4 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerBallSkinList->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 9 * TEXT_HEIGHT + 4 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pPreviewTeamLabel->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 10 * TEXT_HEIGHT + 5 * TEXT_MARGIN, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPreviewTeamList->SetBounds(APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH, 11 * TEXT_HEIGHT + 5 * TEXT_MARGIN, SHORTINPUT_WIDTH, TEXT_HEIGHT);

	m_pPlayerAngleSlider->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, RENDER_TEXTURE_HEIGHT, RENDER_TEXTURE_WIDTH + 8, TEXT_HEIGHT);

	m_pPlayerAngleAutoRotate->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, RENDER_TEXTURE_HEIGHT + TEXT_HEIGHT, RENDER_TEXTURE_WIDTH + 8, TEXT_HEIGHT);

	m_pBodypartPanel->SetBounds(0, 0, APPEARANCE_RADIOBUTTONWIDTH, m_pPlayerPreviewPanel->GetTall());
	m_pBodypartRadioButtons[0]->SetBounds(0, 0, APPEARANCE_RADIOBUTTONWIDTH, TEXT_HEIGHT);
	m_pBodypartRadioButtons[1]->SetBounds(0, RENDER_TEXTURE_HEIGHT / 2 - TEXT_HEIGHT / 2, APPEARANCE_RADIOBUTTONWIDTH, TEXT_HEIGHT);
	m_pBodypartRadioButtons[2]->SetBounds(0, RENDER_TEXTURE_HEIGHT - TEXT_HEIGHT, APPEARANCE_RADIOBUTTONWIDTH, TEXT_HEIGHT);

	m_pConnectionInfoLabel->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, 0, RENDER_TEXTURE_WIDTH, RENDER_TEXTURE_HEIGHT);
	m_pConnectionInfoLabel->SetFgColor(Color(255, 153, 153, 255));
	m_pConnectionInfoLabel->SetContentAlignment(Label::a_center);
}

void CAppearanceSettingPanel::Save()
{
	char shirtName[MAX_PLAYER_NAME_LENGTH];
	m_pShirtNameText->GetText(shirtName, sizeof(shirtName));
	shirtname.SetValue(shirtName);
	modelskinindex.SetValue(m_pSkinIndexList->GetActiveItemUserData()->GetInt("index"));
	preferredoutfieldshirtnumber.SetValue(m_pPreferredOutfieldShirtNumberList->GetActiveItemUserData()->GetInt("number"));
	preferredkeepershirtnumber.SetValue(m_pPreferredKeeperShirtNumberList->GetActiveItemUserData()->GetInt("number"));
	playerballskinname.SetValue(m_pPlayerBallSkinList->GetActiveItemUserData()->GetString("ballskinname"));
}

void CAppearanceSettingPanel::Load()
{
	m_pShirtNameText->SetText(shirtname.GetString());

	m_pSkinIndexList->ActivateItemByRow(clamp(modelskinindex.GetInt(), 0, PLAYER_SKIN_COUNT - 1));

	int outfieldNumber = clamp(preferredoutfieldshirtnumber.GetInt(), 2, 99);
	m_pPreferredOutfieldShirtNumberList->ActivateItemByRow(outfieldNumber - 2);

	int keeperNumber = clamp(preferredkeepershirtnumber.GetInt(), 1, 99);
	m_pPreferredKeeperShirtNumberList->ActivateItemByRow(keeperNumber - 1);
}

void CAppearanceSettingPanel::Update()
{
	bool isConnected = CSDKPlayer::GetLocalSDKPlayer();

	m_pConnectionInfoLabel->SetVisible(!isConnected);
	m_pPlayerAngleSlider->SetEnabled(isConnected);
	m_pPlayerAngleAutoRotate->SetEnabled(isConnected);

	for (int i = 0; i < 3; i++)
		m_pBodypartRadioButtons[i]->SetEnabled(isConnected);

	if (m_flLastTeamKitUpdateTime == -1 || m_flLastTeamKitUpdateTime < CTeamInfo::m_flLastUpdateTime)
	{
		m_flLastTeamKitUpdateTime = CTeamInfo::m_flLastUpdateTime;

		m_pPreviewTeamList->RemoveAll();

		int kitCount = 0;

		for (int i = 0; i < CTeamInfo::m_TeamInfo.Count(); i++)
		{
			for (int j = 0; j < CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo.Count(); j++)
			{
				kitCount += 1;
				KeyValues *kv = new KeyValues("UserData", "teamfolder", CTeamInfo::m_TeamInfo[i]->m_szFolderName, "kitfolder", CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szFolderName);
				m_pPreviewTeamList->AddItem(VarArgs("%s/%s", CTeamInfo::m_TeamInfo[i]->m_szFolderName, CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szFolderName), kv);
				kv->deleteThis();
			}
		}

		m_pPreviewTeamList->ActivateItemByRow(0);
	}

	if (m_flLastBallSkinUpdateTime == -1 || m_flLastBallSkinUpdateTime < CBallInfo::m_flLastUpdateTime)
	{
		m_flLastBallSkinUpdateTime = CBallInfo::m_flLastUpdateTime;

		m_pPlayerBallSkinList->RemoveAll();

		int activeItemID = 0;
		int ballCount = 0;

		for (int i = 0; i < CBallInfo::m_BallInfo.Count(); i++)
		{
			ballCount += 1;
			KeyValues *kv = new KeyValues("UserData", "ballskinname", CBallInfo::m_BallInfo[i]->m_szFolderName);
			int itemID = m_pPlayerBallSkinList->AddItem(VarArgs("%s [by %s]", CBallInfo::m_BallInfo[i]->m_szName, CBallInfo::m_BallInfo[i]->m_szAuthor), kv);

			if (!Q_strcmp(CBallInfo::m_BallInfo[i]->m_szFolderName, playerballskinname.GetString()))
				activeItemID = itemID;

			kv->deleteThis();
		}

		m_pPlayerBallSkinList->ActivateItem(activeItemID);
	}

	if (m_pPlayerAngleAutoRotate->IsSelected() && isConnected)
	{
		float value = m_pPlayerAngleSlider->GetValue() / 100.0f + 180;
		value = fmodf(value + 60 * gpGlobals->frametime, 360);
		value = (value - 180) * 100;
		m_pPlayerAngleSlider->SetValue((int)value);
	}
}

const char *CAppearanceSettingPanel::GetPlayerShirtName()
{
	static char shirtName[MAX_PLAYER_NAME_LENGTH];
	m_pShirtNameText->GetText(shirtName, sizeof(shirtName));

	return shirtName;
}

int CAppearanceSettingPanel::GetPlayerSkinIndex()
{
	return m_pSkinIndexList->GetActiveItemUserData()->GetInt("index");
}

int CAppearanceSettingPanel::GetPlayerOutfieldShirtNumber()
{
	return m_pPreferredOutfieldShirtNumberList->GetActiveItemUserData()->GetInt("number");
}

float CAppearanceSettingPanel::GetPlayerPreviewAngle()
{
	return m_pPlayerAngleSlider->GetValue() / 100.0f;
}

int CAppearanceSettingPanel::GetPlayerBodypart()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_pBodypartRadioButtons[i]->IsSelected())
			return i;
	}

	return 0;
}

void CAppearanceSettingPanel::GetPlayerTeamInfo(const char **teamFolder, const char **kitFolder)
{
	*teamFolder = m_pPreviewTeamList->GetActiveItemUserData()->GetString("teamfolder");
	*kitFolder = m_pPreviewTeamList->GetActiveItemUserData()->GetString("kitfolder");
}

CGameplaySettingPanel::CGameplaySettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");
	m_pLegacySideCurl = new CheckButton(m_pContent, "", "Invert the ball curl direction");
	m_pLegacyVerticalLook = new CheckButton(m_pContent, "", "Don't limit the vertical view range");
}

void CGameplaySettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);
	m_pLegacySideCurl->SetBounds(0, 0, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pLegacyVerticalLook->SetBounds(0, TEXT_HEIGHT + TEXT_MARGIN, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
}

void CGameplaySettingPanel::Save()
{
	legacysidecurl.SetValue(m_pLegacySideCurl->IsSelected() ? 1 : 0);
	legacyverticallook.SetValue(m_pLegacyVerticalLook->IsSelected() ? 1 : 0);
}

void CGameplaySettingPanel::Load()
{
	m_pLegacySideCurl->SetSelected(legacysidecurl.GetBool());
	m_pLegacyVerticalLook->SetSelected(legacyverticallook.GetBool());
}

void CGameplaySettingPanel::Update()
{
}

CVisualSettingPanel::CVisualSettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");
	m_pCenteredStaminaBar = new CheckButton(m_pContent, "", "Centered Stamina Bar");
	m_pQuickTactic = new CheckButton(m_pContent, "", "Quick Tactic Panel");
	m_pAutoHideSpecMenu = new CheckButton(m_pContent, "", "Auto-Hide Spectator Menu");
	m_pGoalTeamCrests = new CheckButton(m_pContent, "", "Goal Team Crests");
}

void CVisualSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);
	m_pCenteredStaminaBar->SetBounds(0, 0, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pQuickTactic->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pAutoHideSpecMenu->SetBounds(0, 2 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pGoalTeamCrests->SetBounds(0, 3 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
}

void CVisualSettingPanel::Save()
{
	centeredstaminabar.SetValue(m_pCenteredStaminaBar->IsSelected() ? 1 : 0);
	quicktacticpanel.SetValue(m_pQuickTactic->IsSelected() ? 1 : 0);
	autohidespecmenu.SetValue(m_pAutoHideSpecMenu->IsSelected() ? 1 : 0);
	goalteamcrests.SetValue(m_pGoalTeamCrests->IsSelected() ? 1 : 0);
}

void CVisualSettingPanel::Load()
{
	m_pCenteredStaminaBar->SetSelected(centeredstaminabar.GetBool());
	m_pQuickTactic->SetSelected(quicktacticpanel.GetBool());
	m_pAutoHideSpecMenu->SetSelected(autohidespecmenu.GetBool());
	m_pGoalTeamCrests->SetSelected(goalteamcrests.GetBool());
}

void CVisualSettingPanel::Update()
{
}

#include "engine/IEngineSound.h"

#define MUTED_VOLUME 0.0f

ConVar cl_sound_crowdbg_enabled("cl_sound_crowdbg_enabled", "1", FCVAR_ARCHIVE);
ConVar cl_sound_crowdbg_volume("cl_sound_crowdbg_volume", "100", FCVAR_ARCHIVE);

ConVar cl_sound_crowdevent_enabled("cl_sound_crowdevent_enabled", "1", FCVAR_ARCHIVE);
ConVar cl_sound_crowdevent_volume("cl_sound_crowdevent_volume", "100", FCVAR_ARCHIVE);

CSoundSettingPanel::CSoundSettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");

	m_nCrowdBgGuid = 0;
	m_nCrowdEventGuid = 0;

	m_pCrowdBgVolume = new Slider(m_pContent, "");
	m_pCrowdBgVolume->SetRange((int)(MUTED_VOLUME * 100.0f), 100);
	m_pCrowdBgVolume->SetValue(100);
	m_pCrowdBg = new CheckButton(m_pContent, "", "Crowd Background");
	m_pCrowdBg->SetSelected(true);
	m_pCrowdBg->AddActionSignalTarget(this);

	m_pCrowdEventVolume = new Slider(m_pContent, "");
	m_pCrowdEventVolume->SetRange((int)(MUTED_VOLUME * 100.0f), 100);
	m_pCrowdEventVolume->SetValue(100);
	m_pCrowdEvent = new CheckButton(m_pContent, "", "Crowd Events");
	m_pCrowdEvent->SetSelected(true);
	m_pCrowdEvent->AddActionSignalTarget(this);

	ListenForGameEvent("timeout_pending");
	ListenForGameEvent("start_timeout");
	ListenForGameEvent("end_timeout");
	ListenForGameEvent("match_period");
	ListenForGameEvent("ball_state");
	ListenForGameEvent("set_piece");
	ListenForGameEvent("goal");
	ListenForGameEvent("highlight_goal");
	ListenForGameEvent("highlight_owngoal");
	ListenForGameEvent("highlight_miss");
	ListenForGameEvent("highlight_keepersave");
	ListenForGameEvent("highlight_redcard");
	ListenForGameEvent("own_goal");
	ListenForGameEvent("foul");
	ListenForGameEvent("penalty");
	ListenForGameEvent("wakeupcall");
	ListenForGameEvent("kickoff");

	m_bIsFirstTick = true;
}

void CSoundSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	m_pCrowdBgVolume->SetBounds(0, 0, INPUT_WIDTH, TEXT_HEIGHT);
	m_pCrowdBg->SetBounds(INPUT_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);

	m_pCrowdEventVolume->SetBounds(0, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pCrowdEvent->SetBounds(INPUT_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
}

void CSoundSettingPanel::Save()
{
}

void CSoundSettingPanel::Load()
{
}

void CSoundSettingPanel::Update()
{
}

void CSoundSettingPanel::FireGameEvent(IGameEvent *event)
{
	if (!Q_strcmp(event->GetName(), "match_period"))
	{
		switch (event->GetInt("period"))
		{
		case MATCH_PERIOD_FIRST_HALF:
		case MATCH_PERIOD_SECOND_HALF:
		case MATCH_PERIOD_EXTRATIME_FIRST_HALF:
		case MATCH_PERIOD_EXTRATIME_SECOND_HALF:
		case MATCH_PERIOD_PENALTIES:
			{
			}
			break;
		}
	}
	else if (!Q_strcmp(event->GetName(), "goal") || !Q_strcmp(event->GetName(), "own_goal"))
	{
		if (m_nCrowdEventGuid && enginesound->IsSoundStillPlaying(m_nCrowdEventGuid))
			enginesound->StopSoundByGuid(m_nCrowdEventGuid);

		char *soundnames[3] = { "crowd/goal1.wav", "crowd/goal2.wav", "crowd/goal3.mp3" };
		char drymix[512];
		Q_snprintf(drymix, sizeof(drymix), "#%s", soundnames[g_IOSRand.RandomInt(0, 2)]);
		enginesound->EmitAmbientSound(drymix, MUTED_VOLUME,	PITCH_NORM,	0, 0);
		m_nCrowdEventGuid = enginesound->GetGuidForLastSoundEmitted();
	}
}

void CSoundSettingPanel::OnTick()
{
	if (m_bIsFirstTick)
	{
		m_bIsFirstTick = false;

		m_pCrowdBg->SetSelected(cl_sound_crowdbg_enabled.GetBool());
		m_pCrowdBgVolume->SetValue(cl_sound_crowdbg_volume.GetInt());

		m_pCrowdEvent->SetSelected(cl_sound_crowdevent_enabled.GetBool());
		m_pCrowdEventVolume->SetValue(cl_sound_crowdevent_volume.GetInt());
	}
	else
	{
		if (m_pCrowdBg->IsSelected() != cl_sound_crowdbg_enabled.GetBool())
			cl_sound_crowdbg_enabled.SetValue(m_pCrowdBg->IsSelected() ? 1 : 0);

		if (m_pCrowdBgVolume->GetValue() != cl_sound_crowdbg_volume.GetInt())
			cl_sound_crowdbg_volume.SetValue(m_pCrowdBgVolume->GetValue());

		if (m_pCrowdEvent->IsSelected() != cl_sound_crowdevent_enabled.GetBool())
			cl_sound_crowdevent_enabled.SetValue(m_pCrowdEvent->IsSelected() ? 1 : 0);

		if (m_pCrowdEventVolume->GetValue() != cl_sound_crowdevent_volume.GetInt())
			cl_sound_crowdevent_volume.SetValue(m_pCrowdEventVolume->GetValue());
	}

	C_Ball *pBall = GetBall();

	if (!pBall)
		return;

	float crowdBgVolScale = m_pCrowdBg->IsSelected() ? (float)m_pCrowdBgVolume->GetValue() / 100.0f : MUTED_VOLUME;
	float crowdEventVolScale = m_pCrowdEvent->IsSelected() ? (float)m_pCrowdEventVolume->GetValue() / 100.0f : MUTED_VOLUME;

	if (!m_nCrowdBgGuid || !enginesound->IsSoundStillPlaying(m_nCrowdBgGuid))
	{
		char soundname[512] = "crowd/crowd2.wav";
		char drymix[512];
		Q_snprintf(drymix, sizeof(drymix), "#%s", soundname);
		enginesound->EmitAmbientSound(drymix, MUTED_VOLUME,	PITCH_NORM,	0, 0);
		m_nCrowdBgGuid = enginesound->GetGuidForLastSoundEmitted();
	}

	if (SDKGameRules()->IsIntermissionState())
	{
		float crowdBgVol = clamp(crowdBgVolScale * 0.5f, MUTED_VOLUME, 1.0f);
		enginesound->SetVolumeByGuid(m_nCrowdBgGuid, crowdBgVol);
	}
	else
	{
		float crowdBgfrac = abs(SDKGameRules()->m_nBallZone) / 100.0f;
		float crowdBgVol = clamp(crowdBgVolScale * (0.5f + crowdBgfrac * 0.5f), MUTED_VOLUME, 1.0f);
		enginesound->SetVolumeByGuid(m_nCrowdBgGuid, crowdBgVol);

		if (m_nCrowdEventGuid && enginesound->IsSoundStillPlaying(m_nCrowdEventGuid))
		{
			float crowdEventVol = clamp(crowdEventVolScale * 1.0f, MUTED_VOLUME, 1.0f);
			enginesound->SetVolumeByGuid(m_nCrowdEventGuid, crowdEventVol);
		}
	}
}

void CSoundSettingPanel::OnCheckButtonChecked(Panel *panel)
{
	m_pCrowdBgVolume->SetEnabled(m_pCrowdBg->IsSelected());
}