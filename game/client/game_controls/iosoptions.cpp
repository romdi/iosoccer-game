#include "cbase.h"
#include "iosoptions.h"
#include "ienginevgui.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"
#include "ios_teamkit_parse.h"
#include "c_match_ball.h"
#include "vgui/IVgui.h"
#include "clientmode_shared.h"

extern ConVar
	rate,
	clubname,
	nationalteamname,
	countryindex,
	goalteamcrests,
	reversesidecurl,
	modelskinindex,
	modelhairindex,
	modelsleeveindex,
	modelshoename,
	modelkeeperglovename,
	playerballskinname,
	playername,
	preferredkeepershirtnumber,
	preferredoutfieldshirtnumber,
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

enum { PANEL_MARGIN = 5, PANEL_WIDTH = (768 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (768 - 2 * PANEL_MARGIN) };
enum { CONTROL_HMARGIN = 15, CONTROL_VMARGIN = 5, CONTROL_WIDE_WIDTH = 280, CONTROL_SHORT_WIDTH = CONTROL_WIDE_WIDTH / 2 - CONTROL_HMARGIN / 2, CONTROL_HEIGHT = 26 };
enum { PADDING = 15, TOP_PADDING = 30 };
enum { BUTTON_WIDTH = 80, BUTTON_HEIGHT = 26, BUTTON_MARGIN = 5 };
enum { SUGGESTED_VALUE_WIDTH = 150, SUGGESTED_VALUE_MARGIN = 5 };
enum { INFOBUTTON_WIDTH = 30, INFOBUTTON_MARGIN = 5 };
enum { APPEARANCE_HOFFSET = 270, APPEARANCE_RADIOBUTTONWIDTH = 70, RENDER_TEXTURE_WIDTH = 256, RENDER_TEXTURE_HEIGHT = 512 };

#define INTERP_VALUES 5
const int interpValues[INTERP_VALUES] = { 1, 2, 3, 4, 5 };
const char *interpTexts[INTERP_VALUES] = { "Very Short (1)", "Short (2)", "Medium (3)", "Long (4)", "Very Long (5)" };
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
	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
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
			m_pChangeInfoText->SetText(VarArgs("Wait %d seconds to change", 1 + (int)(pLocal->m_flNextClientSettingsChangeTime - gpGlobals->curtime)));
			return;
		}
		else if (!SDKGameRules()->IsIntermissionState() && (GetLocalPlayerTeam() == TEAM_HOME || GetLocalPlayerTeam() == TEAM_AWAY))
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

	m_pCountryNameLabel = new Label(m_pContent, "", "Country:");
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

	m_pPlayerNameLabel->SetBounds(0, 0, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pPlayerNameText->SetBounds(0, CONTROL_HEIGHT, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message

	m_pClubNameLabel->SetBounds(0, 2 * CONTROL_HEIGHT + CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pClubNameText->SetBounds(0, 3 * CONTROL_HEIGHT + CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pNationalTeamNameLabel->SetBounds(0, 4 * CONTROL_HEIGHT + 2 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pNationalTeamNameText->SetBounds(0, 5 * CONTROL_HEIGHT + 2 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pCountryNameLabel->SetBounds(0, 6 * CONTROL_HEIGHT + 3 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCountryNameList->SetBounds(0, 7 * CONTROL_HEIGHT + 3 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pInterpDurationLabel->SetBounds(0, 8 * CONTROL_HEIGHT + 4 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pInterpDurationList->SetBounds(0, 9 * CONTROL_HEIGHT + 4 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pInterpDurationSuggestedValueButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN, 9 * CONTROL_HEIGHT + 4 * CONTROL_VMARGIN, SUGGESTED_VALUE_WIDTH, CONTROL_HEIGHT);
	m_pInterpDurationSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pInterpDurationInfoButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 9 * CONTROL_HEIGHT + 4 * CONTROL_VMARGIN, INFOBUTTON_WIDTH, CONTROL_HEIGHT);
	m_pInterpDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pSmoothDurationLabel->SetBounds(0, 10 * CONTROL_HEIGHT + 5 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pSmoothDurationList->SetBounds(0, 11 * CONTROL_HEIGHT + 5 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pSmoothDurationSuggestedValueButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN, 11 * CONTROL_HEIGHT + 5 * CONTROL_VMARGIN, SUGGESTED_VALUE_WIDTH, CONTROL_HEIGHT);
	m_pSmoothDurationSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pSmoothDurationInfoButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 11 * CONTROL_HEIGHT + 5 * CONTROL_VMARGIN, INFOBUTTON_WIDTH, CONTROL_HEIGHT);
	m_pSmoothDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pRateLabel->SetBounds(0, 12 * CONTROL_HEIGHT + 6 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pRateList->SetBounds(0, 13 * CONTROL_HEIGHT + 6 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pRateSuggestedValueButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN, 13 * CONTROL_HEIGHT + 6 * CONTROL_VMARGIN, SUGGESTED_VALUE_WIDTH, CONTROL_HEIGHT);
	m_pRateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pRateInfoButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 13 * CONTROL_HEIGHT + 6 * CONTROL_VMARGIN, INFOBUTTON_WIDTH, CONTROL_HEIGHT);
	m_pRateInfoButton->SetContentAlignment(Label::a_center);

	m_pUpdaterateLabel->SetBounds(0, 14 * CONTROL_HEIGHT + 7 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pUpdaterateList->SetBounds(0, 15 * CONTROL_HEIGHT + 7 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pUpdaterateSuggestedValueButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN, 15 * CONTROL_HEIGHT + 7 * CONTROL_VMARGIN, SUGGESTED_VALUE_WIDTH, CONTROL_HEIGHT);
	m_pUpdaterateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pUpdaterateInfoButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 15 * CONTROL_HEIGHT + 7 * CONTROL_VMARGIN, INFOBUTTON_WIDTH, CONTROL_HEIGHT);
	m_pUpdaterateInfoButton->SetContentAlignment(Label::a_center);

	m_pCommandrateLabel->SetBounds(0, 16 * CONTROL_HEIGHT + 8 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCommandrateList->SetBounds(0, 17 * CONTROL_HEIGHT + 8 * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCommandrateSuggestedValueButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN, 17 * CONTROL_HEIGHT + 8 * CONTROL_VMARGIN, SUGGESTED_VALUE_WIDTH, CONTROL_HEIGHT);
	m_pCommandrateSuggestedValueButton->SetContentAlignment(Label::a_center);
	m_pCommandrateInfoButton->SetBounds(CONTROL_WIDE_WIDTH + SUGGESTED_VALUE_MARGIN + SUGGESTED_VALUE_WIDTH + INFOBUTTON_MARGIN, 17 * CONTROL_HEIGHT + 8 * CONTROL_VMARGIN, INFOBUTTON_WIDTH, CONTROL_HEIGHT);
	m_pCommandrateInfoButton->SetContentAlignment(Label::a_center);
}

void CNetworkSettingPanel::OnCommand(const char *cmd)
{
	if (!Q_strcmp(cmd, "suggested_interpduration"))
		m_pInterpDurationList->ActivateItemByRow(0);
	if (!Q_strcmp(cmd, "suggested_smoothduration"))
		m_pSmoothDurationList->ActivateItemByRow(2);
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

	countryindex.SetValue(m_pCountryNameList->GetActiveItemUserData()->GetInt("index"));

	g_pCVar->FindVar("cl_interp_ratio")->SetValue(atoi(m_pInterpDurationList->GetActiveItemUserData()->GetString("value")));
	g_pCVar->FindVar("cl_smoothtime")->SetValue(atoi(m_pSmoothDurationList->GetActiveItemUserData()->GetString("value")) / 100.0f);

	g_pCVar->FindVar("rate")->SetValue(m_pRateList->GetActiveItemUserData()->GetInt("value") * 1000);
	g_pCVar->FindVar("cl_updaterate")->SetValue(m_pUpdaterateList->GetActiveItemUserData()->GetInt("value"));
	g_pCVar->FindVar("cl_cmdrate")->SetValue(m_pCommandrateList->GetActiveItemUserData()->GetInt("value"));
}

void CNetworkSettingPanel::Load()
{
	m_pPlayerNameText->SetText(playername.GetString());
	m_pCountryNameList->ActivateItemByRow(countryindex.GetInt());
	m_pClubNameText->SetText(clubname.GetString());
	m_pNationalTeamNameText->SetText(nationalteamname.GetString());

	m_pInterpDurationList->ActivateItemByRow(0);

	for (int i = 0; i < INTERP_VALUES; i++)
	{
		if (interpValues[i] == g_pCVar->FindVar("cl_interp_ratio")->GetInt())
		{
			m_pInterpDurationList->ActivateItemByRow(i);
			break;
		}
	}

	m_pSmoothDurationList->ActivateItemByRow(0);

	for (int i = 0; i < SMOOTH_VALUES; i++)
	{
		if (smoothValues[i] == (int)(g_pCVar->FindVar("cl_smoothtime")->GetFloat() * 100 + 0.5f))
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
	KeyValues *kv = NULL;

	m_pContent = new Panel(this, "");

	m_pPlayerPreviewPanel = new ImagePanel(m_pContent, "");

	m_pShirtNameLabel = new Label(m_pContent, "", "Shirt name:");
	m_pShirtNameText = new TextEntry(m_pContent, "");
	m_pShirtNameText->SetMaximumCharCount(MAX_PLAYER_NAME_LENGTH - 1);
	m_pShirtNameText->SetAllowNonAsciiCharacters(true);


	m_pSkinIndexLabel = new Label(m_pContent, "", "Ethnicity:");
	m_pSkinIndexList = new ComboBox(m_pContent, "", 0, false);

	const char *skins[] = { "White, Black Hair", "White, Bald", "White, Brown Hair", "Asian, Black Hair", "Black, Black Hair" };

	for (int i = 0; i < PLAYER_SKIN_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pSkinIndexList->AddItem(skins[i], kv);
		kv->deleteThis();
	}


	m_pHairIndexLabel = new Label(m_pContent, "", "Hair:");
	m_pHairIndexList = new ComboBox(m_pContent, "", 0, false);

	const char *hair[] = { "Shaved", "Gentleman", "Mohawk", "Short", "Spikey 1", "Spikey 2", "Sweeping" };

	for (int i = 0; i < PLAYER_HAIR_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pHairIndexList->AddItem(hair[i], kv);
		kv->deleteThis();
	}


	m_pSleeveIndexLabel = new Label(m_pContent, "", "Sleeves:");
	m_pSleeveIndexList = new ComboBox(m_pContent, "", 0, false);

	const char *sleeves[] = { "Short", "Long" };

	for (int i = 0; i < PLAYER_SLEEVE_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pSleeveIndexList->AddItem(sleeves[i], kv);
		kv->deleteThis();
	}


	m_pOutfieldShirtNumberLabel = new Label(m_pContent, "", "Outfield number:");
	m_pOutfieldShirtNumberList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	for (int i = 2; i <= 99; i++)
	{
		kv = new KeyValues("UserData", "number", i);
		m_pOutfieldShirtNumberList->AddItem(VarArgs("%d", i), kv);
		kv->deleteThis();
	}


	m_pKeeperShirtNumberLabel = new Label(m_pContent, "", "Keeper number:");
	m_pKeeperShirtNumberList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	for (int i = 1; i <= 99; i++)
	{
		kv = new KeyValues("UserData", "number", i);
		m_pKeeperShirtNumberList->AddItem(VarArgs("%d", i), kv);
		kv->deleteThis();
	}


	m_pShoeLabel = new Label(m_pContent, "", "Shoes:");
	m_pShoeList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pKeeperGloveLabel = new Label(m_pContent, "", "Keeper gloves:");
	m_pKeeperGloveList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pPlayerBallSkinLabel = new Label(m_pContent, "", "Warm up ball:");
	m_pPlayerBallSkinList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pPreviewTeamLabel = new Label(m_pContent, "", "Preview team kit:");
	m_pPreviewTeamList = new ComboBox(m_pContent, "", MAX_VISIBLE_DROPDOWN, false);

	m_pBodypartPanel = new Panel(m_pContent, "");
	m_pBodypartRadioButtons[0] = new RadioButton(m_pBodypartPanel, "", "Head");
	m_pBodypartRadioButtons[1] = new RadioButton(m_pBodypartPanel, "", "Body");
	m_pBodypartRadioButtons[2] = new RadioButton(m_pBodypartPanel, "", "Shoes");
	m_pBodypartRadioButtons[1]->SetSelected(true);

	m_pPositionPreviewType[0] = new RadioButton(m_pContent, "", "Outfield preview");
	m_pPositionPreviewType[1] = new RadioButton(m_pContent, "", "Keeper preview");
	m_pPositionPreviewType[0]->SetSelected(true);

	m_pShowBallPreview = new CheckButton(m_pContent, "", "Ball preview");
	m_pShowBallPreview->SetSelected(true);

	m_pPlayerAngleSlider = new Slider(m_pContent, "");
	m_pPlayerAngleSlider->SetRange(-18000, 18000);
	m_pPlayerAngleSlider->SetValue(0);

	m_pPlayerAngleAutoRotate = new CheckButton(m_pContent, "", "Auto-rotate preview");
	m_pPlayerAngleAutoRotate->SetSelected(true);

	m_pConnectionInfoLabel = new Label(m_pContent, "", "Can't preview when disconnected");

	m_flLastTeamKitUpdateTime = -1;
	m_flLastBallSkinUpdateTime = -1;
	m_flLastShoeUpdateTime = -1;
	m_flLastKeeperGloveUpdateTime = -1;
}

void CAppearanceSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	m_pPlayerPreviewPanel->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, 0, RENDER_TEXTURE_WIDTH, RENDER_TEXTURE_HEIGHT);
	m_pPlayerPreviewPanel->SetImage("../_rt_playermodel");
	m_pPlayerPreviewPanel->SetShouldScaleImage(true);

	int offset = APPEARANCE_HOFFSET + APPEARANCE_RADIOBUTTONWIDTH;
	int row = 0;
	int group = 0;

	m_pShirtNameLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pShirtNameText->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pOutfieldShirtNumberLabel->SetBounds(offset, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pKeeperShirtNumberLabel->SetBounds(offset + CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pOutfieldShirtNumberList->SetBounds(offset, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pKeeperShirtNumberList->SetBounds(offset + CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);

	m_pSkinIndexLabel->SetBounds(offset, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pHairIndexLabel->SetBounds(offset + CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pSkinIndexList->SetBounds(offset, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pHairIndexList->SetBounds(offset + CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);

	m_pSleeveIndexLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pSleeveIndexList->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pShoeLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pShoeList->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pKeeperGloveLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pKeeperGloveList->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pPlayerBallSkinLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pPlayerBallSkinList->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	row++;

	m_pPreviewTeamLabel->SetBounds(offset, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pPreviewTeamList->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pPositionPreviewType[0]->SetBounds(offset, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pPositionPreviewType[1]->SetBounds(offset + CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);

	m_pShowBallPreview->SetBounds(offset, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pPlayerAngleSlider->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, RENDER_TEXTURE_HEIGHT, RENDER_TEXTURE_WIDTH + 8, CONTROL_HEIGHT);

	m_pPlayerAngleAutoRotate->SetBounds(APPEARANCE_RADIOBUTTONWIDTH, RENDER_TEXTURE_HEIGHT + CONTROL_HEIGHT, RENDER_TEXTURE_WIDTH + 8, CONTROL_HEIGHT);

	m_pBodypartPanel->SetBounds(0, 0, APPEARANCE_RADIOBUTTONWIDTH, m_pPlayerPreviewPanel->GetTall());
	m_pBodypartRadioButtons[0]->SetBounds(0, 0, APPEARANCE_RADIOBUTTONWIDTH, CONTROL_HEIGHT);
	m_pBodypartRadioButtons[1]->SetBounds(0, RENDER_TEXTURE_HEIGHT / 2 - CONTROL_HEIGHT / 2, APPEARANCE_RADIOBUTTONWIDTH, CONTROL_HEIGHT);
	m_pBodypartRadioButtons[2]->SetBounds(0, RENDER_TEXTURE_HEIGHT - CONTROL_HEIGHT, APPEARANCE_RADIOBUTTONWIDTH, CONTROL_HEIGHT);

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
	modelhairindex.SetValue(m_pHairIndexList->GetActiveItemUserData()->GetInt("index"));
	modelsleeveindex.SetValue(m_pSleeveIndexList->GetActiveItemUserData()->GetInt("index"));
	preferredoutfieldshirtnumber.SetValue(m_pOutfieldShirtNumberList->GetActiveItemUserData()->GetInt("number"));
	preferredkeepershirtnumber.SetValue(m_pKeeperShirtNumberList->GetActiveItemUserData()->GetInt("number"));
	modelshoename.SetValue(m_pShoeList->GetActiveItemUserData()->GetString("shoe"));
	modelkeeperglovename.SetValue(m_pKeeperGloveList->GetActiveItemUserData()->GetString("keeperglove"));
	playerballskinname.SetValue(m_pPlayerBallSkinList->GetActiveItemUserData()->GetString("ballskinname"));
}

void CAppearanceSettingPanel::Load()
{
	m_pShirtNameText->SetText(shirtname.GetString());

	m_pSkinIndexList->ActivateItemByRow(clamp(modelskinindex.GetInt(), 0, PLAYER_SKIN_COUNT - 1));

	m_pHairIndexList->ActivateItemByRow(clamp(modelhairindex.GetInt(), 0, PLAYER_HAIR_COUNT - 1));

	m_pSleeveIndexList->ActivateItemByRow(clamp(modelsleeveindex.GetInt(), 0, PLAYER_SLEEVE_COUNT - 1));

	int outfieldNumber = clamp(preferredoutfieldshirtnumber.GetInt(), 2, 99);
	m_pOutfieldShirtNumberList->ActivateItemByRow(outfieldNumber - 2);

	int keeperNumber = clamp(preferredkeepershirtnumber.GetInt(), 1, 99);
	m_pKeeperShirtNumberList->ActivateItemByRow(keeperNumber - 1);
}

void CAppearanceSettingPanel::Update()
{
	bool isConnected = CSDKPlayer::GetLocalSDKPlayer();

	m_pConnectionInfoLabel->SetVisible(!isConnected);
	m_pPlayerAngleSlider->SetEnabled(isConnected);
	m_pPlayerAngleAutoRotate->SetEnabled(isConnected);

	for (int i = 0; i < 3; i++)
		m_pBodypartRadioButtons[i]->SetEnabled(isConnected);

	if (m_pPlayerAngleAutoRotate->IsSelected() && isConnected)
	{
		float value = m_pPlayerAngleSlider->GetValue() / 100.0f + 180;
		value = fmodf(value + 60 * gpGlobals->frametime, 360);
		value = (value - 180) * 100;
		m_pPlayerAngleSlider->SetValue((int)value);
	}

	UpdateTeamKits();
	UpdateBalls();
	UpdateShoes();
	UpdateKeeperGloves();
}

void CAppearanceSettingPanel::UpdateTeamKits()
{
	if (m_flLastTeamKitUpdateTime == -1 || m_flLastTeamKitUpdateTime < CTeamInfo::m_flLastUpdateTime)
	{
		m_flLastTeamKitUpdateTime = CTeamInfo::m_flLastUpdateTime;

		m_pPreviewTeamList->RemoveAll();

		for (int i = 0; i < CTeamInfo::m_TeamInfo.Count(); i++)
		{
			for (int j = 0; j < CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo.Count(); j++)
			{
				KeyValues *kv = new KeyValues("UserData", "teamindex", i, "kitindex", j);
				m_pPreviewTeamList->AddItem(VarArgs("%s - %s   [ by %s ]", CTeamInfo::m_TeamInfo[i]->m_szShortName, CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szName, CTeamInfo::m_TeamInfo[i]->m_TeamKitInfo[j]->m_szAuthor), kv);
				kv->deleteThis();
			}
		}

		m_pPreviewTeamList->ActivateItemByRow(0);
	}
}

void CAppearanceSettingPanel::UpdateBalls()
{
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
			int itemID = m_pPlayerBallSkinList->AddItem(VarArgs("%s   [ by %s ]", CBallInfo::m_BallInfo[i]->m_szName, CBallInfo::m_BallInfo[i]->m_szAuthor), kv);

			if (!Q_strcmp(CBallInfo::m_BallInfo[i]->m_szFolderName, playerballskinname.GetString()))
				activeItemID = itemID;

			kv->deleteThis();
		}

		m_pPlayerBallSkinList->ActivateItem(activeItemID);
	}
}

void CAppearanceSettingPanel::UpdateShoes()
{
	if (m_flLastShoeUpdateTime == -1 || m_flLastShoeUpdateTime < CShoeInfo::m_flLastUpdateTime)
	{
		m_flLastShoeUpdateTime = CShoeInfo::m_flLastUpdateTime;

		m_pShoeList->RemoveAll();

		int activeItemID = 0;
		int ballCount = 0;

		for (int i = 0; i < CShoeInfo::m_ShoeInfo.Count(); i++)
		{
			ballCount += 1;
			KeyValues *kv = new KeyValues("UserData", "shoe", CShoeInfo::m_ShoeInfo[i]->m_szFolderName);
			int itemID = m_pShoeList->AddItem(VarArgs("%s   [ by %s ]", CShoeInfo::m_ShoeInfo[i]->m_szName, CShoeInfo::m_ShoeInfo[i]->m_szAuthor), kv);

			if (!Q_strcmp(CShoeInfo::m_ShoeInfo[i]->m_szFolderName, modelshoename.GetString()))
				activeItemID = itemID;

			kv->deleteThis();
		}

		m_pShoeList->ActivateItem(activeItemID);
	}
}

void CAppearanceSettingPanel::UpdateKeeperGloves()
{
	if (m_flLastKeeperGloveUpdateTime == -1 || m_flLastKeeperGloveUpdateTime < CKeeperGloveInfo::m_flLastUpdateTime)
	{
		m_flLastKeeperGloveUpdateTime = CKeeperGloveInfo::m_flLastUpdateTime;

		m_pKeeperGloveList->RemoveAll();

		int activeItemID = 0;
		int ballCount = 0;

		for (int i = 0; i < CKeeperGloveInfo::m_KeeperGloveInfo.Count(); i++)
		{
			ballCount += 1;
			KeyValues *kv = new KeyValues("UserData", "keeperglove", CKeeperGloveInfo::m_KeeperGloveInfo[i]->m_szFolderName);
			int itemID = m_pKeeperGloveList->AddItem(VarArgs("%s   [ by %s ]", CKeeperGloveInfo::m_KeeperGloveInfo[i]->m_szName, CKeeperGloveInfo::m_KeeperGloveInfo[i]->m_szAuthor), kv);

			if (!Q_strcmp(CKeeperGloveInfo::m_KeeperGloveInfo[i]->m_szFolderName, modelkeeperglovename.GetString()))
				activeItemID = itemID;

			kv->deleteThis();
		}

		m_pKeeperGloveList->ActivateItem(activeItemID);
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

int CAppearanceSettingPanel::GetPlayerHairIndex()
{
	return m_pHairIndexList->GetActiveItemUserData()->GetInt("index");
}

int CAppearanceSettingPanel::GetPlayerSleeveIndex()
{
	return m_pSleeveIndexList->GetActiveItemUserData()->GetInt("index");
}

const char *CAppearanceSettingPanel::GetPlayerShoeName()
{
	return m_pShoeList->GetActiveItemUserData()->GetString("shoe");
}

const char *CAppearanceSettingPanel::GetPlayerKeeperGloveName()
{
	return m_pKeeperGloveList->GetActiveItemUserData()->GetString("keeperglove");
}

int CAppearanceSettingPanel::GetShirtNumber(bool keeper)
{
	return keeper ? m_pKeeperShirtNumberList->GetActiveItemUserData()->GetInt("number") : m_pOutfieldShirtNumberList->GetActiveItemUserData()->GetInt("number");
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

bool CAppearanceSettingPanel::IsKeeperPreview()
{
	return m_pPositionPreviewType[1]->IsSelected();
}

bool CAppearanceSettingPanel::ShowBallPreview()
{
	return m_pShowBallPreview->IsSelected();
}

const char *CAppearanceSettingPanel::GetBallName()
{
	return m_pPlayerBallSkinList->GetActiveItemUserData()->GetString("ballskinname");
}

CTeamKitInfo *CAppearanceSettingPanel::GetPlayerTeamKitInfo()
{
	int teamIndex = m_pPreviewTeamList->GetActiveItemUserData()->GetInt("teamindex");
	int kitIndex = m_pPreviewTeamList->GetActiveItemUserData()->GetInt("kitindex");
	return CTeamInfo::m_TeamInfo[teamIndex]->m_TeamKitInfo[kitIndex];
}

CGameplaySettingPanel::CGameplaySettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");
	m_pReverseSideCurl = new CheckButton(m_pContent, "", "Reverse ball side curl direction");
}

void CGameplaySettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);
	m_pReverseSideCurl->SetBounds(0, 0, CONTROL_WIDE_WIDTH + CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
}

void CGameplaySettingPanel::Save()
{
	reversesidecurl.SetValue(m_pReverseSideCurl->IsSelected() ? 1 : 0);
}

void CGameplaySettingPanel::Load()
{
	m_pReverseSideCurl->SetSelected(reversesidecurl.GetBool());
}

void CGameplaySettingPanel::Update()
{
}

extern ConVar
	hud_names_visible,
	hud_names_type,
	cl_cam_dist,
	cl_cam_height,
	cl_cam_firstperson;

CVisualSettingPanel::CVisualSettingPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pContent = new Panel(this, "");

	m_pShowHudPlayerInfo = new CheckButton(m_pContent, "", "Draw info text above players");

	m_pHudPlayerInfoLabel = new Label(m_pContent, "", "Player info text type:");
	m_pHudPlayerInfo[0] = new RadioButton(m_pContent, "", "Name");
	m_pHudPlayerInfo[1] = new RadioButton(m_pContent, "", "Position");
	m_pHudPlayerInfo[2] = new RadioButton(m_pContent, "", "Number");

	m_pCameraDistanceLabel = new Label(m_pContent, "", "Camera distance:");
	m_pCameraDistanceValue = new TextEntry(m_pContent, "");
	m_pCameraDistanceSlider = new Slider(m_pContent, "");

	m_pCameraHeightLabel = new Label(m_pContent, "", "Camera height:");
	m_pCameraHeightValue = new TextEntry(m_pContent, "");
	m_pCameraHeightSlider = new Slider(m_pContent, "");

	m_pFirstPersonCamera = new CheckButton(m_pContent, "", "First person camera");
}

void CVisualSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	int row = 0;
	int group = 0;

	m_pShowHudPlayerInfo->SetBounds(0, row++, CONTROL_WIDE_WIDTH + CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pHudPlayerInfoLabel->SetBounds(0, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pHudPlayerInfo[0]->SetBounds(0, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pHudPlayerInfo[1]->SetBounds(CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pHudPlayerInfo[2]->SetBounds(2 * CONTROL_SHORT_WIDTH + 2* CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);

	m_pCameraDistanceLabel->SetBounds(0, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCameraDistanceValue->SetBounds(0, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pCameraDistanceSlider->SetBounds(CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pCameraHeightLabel->SetBounds(0, row++ * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH + CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCameraHeightValue->SetBounds(0, row * CONTROL_HEIGHT + group * CONTROL_VMARGIN, CONTROL_SHORT_WIDTH, CONTROL_HEIGHT);
	m_pCameraHeightSlider->SetBounds(CONTROL_SHORT_WIDTH + CONTROL_HMARGIN, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pFirstPersonCamera->SetBounds(0, row++ * CONTROL_HEIGHT + group++ * CONTROL_VMARGIN, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	float minDist, maxDist;
	cl_cam_dist.GetMin(minDist);
	cl_cam_dist.GetMax(maxDist);

	m_pCameraDistanceValue->SetAllowNumericInputOnly(true);
	m_pCameraDistanceSlider->SetRange(minDist, maxDist);

	float minHeight, maxHeight;
	cl_cam_height.GetMin(minHeight);
	cl_cam_height.GetMax(maxHeight);

	m_pCameraHeightValue->SetAllowNumericInputOnly(true);
	m_pCameraHeightSlider->SetRange(minHeight, maxHeight);
}

void CVisualSettingPanel::Save()
{
	hud_names_visible.SetValue(m_pShowHudPlayerInfo->IsSelected() ? 1 : 0);

	for (int i = 0; i < 3; i++)
	{
		if (m_pHudPlayerInfo[i]->IsSelected())
		{
			hud_names_type.SetValue(i);
			break;
		}
	}

	cl_cam_dist.SetValue(m_pCameraDistanceValue->GetValueAsInt());
	cl_cam_height.SetValue(m_pCameraHeightValue->GetValueAsInt());
	cl_cam_firstperson.SetValue(m_pFirstPersonCamera->IsSelected());
}

void CVisualSettingPanel::Load()
{
	m_pShowHudPlayerInfo->SetSelected(hud_names_visible.GetBool());
	m_pHudPlayerInfo[clamp(hud_names_type.GetInt(), 0, 2)]->SetSelected(true);

	m_pCameraDistanceValue->SetText(cl_cam_dist.GetString());
	m_pCameraHeightValue->SetText(cl_cam_height.GetString());
	m_pFirstPersonCamera->SetSelected(cl_cam_firstperson.GetBool());
}

void CVisualSettingPanel::Update()
{
	for (int i = 0; i < 3; i++)
	{
		m_pHudPlayerInfo[i]->SetEnabled(m_pShowHudPlayerInfo->IsSelected());
	}

	if (m_pFirstPersonCamera->IsSelected())
	{
		m_pCameraDistanceValue->SetEnabled(false);
		m_pCameraDistanceSlider->SetEnabled(false);
		m_pCameraHeightValue->SetEnabled(false);
		m_pCameraHeightSlider->SetEnabled(false);
	}
	else
	{
		m_pCameraDistanceValue->SetEnabled(true);
		m_pCameraDistanceSlider->SetEnabled(true);
		m_pCameraHeightValue->SetEnabled(true);
		m_pCameraHeightSlider->SetEnabled(true);

		if (m_pCameraDistanceSlider->IsDragged())
			m_pCameraDistanceValue->SetText(VarArgs("%d", m_pCameraDistanceSlider->GetValue()));
		else
			m_pCameraDistanceSlider->SetValue(m_pCameraDistanceValue->GetValueAsInt());

		if (m_pCameraHeightSlider->IsDragged())
			m_pCameraHeightValue->SetText(VarArgs("%d", m_pCameraHeightSlider->GetValue()));
		else
			m_pCameraHeightSlider->SetValue(m_pCameraHeightValue->GetValueAsInt());
	}
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
	ListenForGameEvent("wakeupcall");
	ListenForGameEvent("kickoff");
	ListenForGameEvent("penalty_shootout");

	m_bIsFirstTick = true;
}

void CSoundSettingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pContent->SetBounds(PADDING, PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING);

	m_pCrowdBgVolume->SetBounds(0, 0, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCrowdBg->SetBounds(CONTROL_WIDE_WIDTH, 0, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);

	m_pCrowdEventVolume->SetBounds(0, CONTROL_HEIGHT, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
	m_pCrowdEvent->SetBounds(CONTROL_WIDE_WIDTH, CONTROL_HEIGHT, CONTROL_WIDE_WIDTH, CONTROL_HEIGHT);
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
		char drymix[128];
		Q_snprintf(drymix, sizeof(drymix), "#%s", soundnames[g_IOSRand.RandomInt(0, 2)]);
		enginesound->EmitAmbientSound(drymix, MUTED_VOLUME,	PITCH_NORM,	0, 0);
		m_nCrowdEventGuid = enginesound->GetGuidForLastSoundEmitted();
	}
	else if (!Q_strcmp(event->GetName(), "penalty_shootout"))
	{
		C_SDKPlayer *pTaker = ToSDKPlayer(USERID2PLAYER(event->GetInt("taker_userid")));
		int takingTeam = event->GetInt("taking_team");
		C_SDKPlayer *pKeeper = ToSDKPlayer(USERID2PLAYER(event->GetInt("keeper_userid")));
		penalty_state_t penaltyState = (penalty_state_t)event->GetInt("penalty_state");

		bool cheer = false;

		switch (penaltyState)
		{
		case PENALTY_ASSIGNED:
			{
				cheer = false;
			}
			break;
		case PENALTY_SCORED:
			{
				cheer = true;
			}
			break;
		case PENALTY_SAVED:
			{
				cheer = true;
			}
			break;
		case PENALTY_MISSED:
			{
				cheer = true;
			}
			break;
		default:
			{
				cheer = false;
			}
			break;
		}

		if (cheer)
		{
			if (m_nCrowdEventGuid && enginesound->IsSoundStillPlaying(m_nCrowdEventGuid))
				enginesound->StopSoundByGuid(m_nCrowdEventGuid);

			char *soundnames[3] = { "crowd/goal1.wav", "crowd/goal2.wav", "crowd/goal3.mp3" };
			char drymix[128];
			Q_snprintf(drymix, sizeof(drymix), "#%s", soundnames[g_IOSRand.RandomInt(0, 2)]);
			enginesound->EmitAmbientSound(drymix, MUTED_VOLUME,	PITCH_NORM,	0, 0);
			m_nCrowdEventGuid = enginesound->GetGuidForLastSoundEmitted();
		}
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

	C_MatchBall *pBall = GetMatchBall();

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