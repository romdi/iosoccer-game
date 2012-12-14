#include "cbase.h"
#include "iosoptions.h"
#include "ienginevgui.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"

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

#define LABEL_WIDTH 180
#define INPUT_WIDTH 300
#define TEXT_HEIGHT 30

#define SHIRT_NUMBER_COUNT 11

enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = (1024 - 2 * PANEL_MARGIN), PANEL_HEIGHT = (720 - 2 * PANEL_MARGIN) };
enum { PADDING = 15, TOP_PADDING = 15 };
enum { BUTTON_WIDTH = 80, BUTTON_HEIGHT = 30, BUTTON_MARGIN = 5 };
enum { INFOBUTTON_WIDTH = 30, INFOBUTTON_MARGIN = 5 };

#define INTERP_VALUES 5
const int interpValues[INTERP_VALUES] = { 1, 2, 3, 4, 5 };
const char *interpTexts[INTERP_VALUES] = { "Very Short (cl_interp_ratio 1)", "Short (cl_interp_ratio 2)", "Medium (cl_interp_ratio 3)", "Long (cl_interp_ratio 4)", "Very Long (cl_interp_ratio 5)" };
#define SMOOTH_VALUES 5
const int smoothValues[SMOOTH_VALUES] = { 1, 5, 10, 25, 50 };
const char *smoothTexts[SMOOTH_VALUES] = { "Very Short (cl_smoothtime 0.01)", "Short (cl_smoothtime 0.05)", "Medium (cl_smoothtime 0.1)", "Long (cl_smoothtime 0.25)", "Very Long (cl_smoothtime 0.5)" };

CIOSOptionsPanel::CIOSOptionsPanel(VPANEL parent) : BaseClass(NULL, "IOSOptionsPanel")
{
	SetScheme("SourceScheme");

	SetParent(parent);
	m_pContent = new Panel(this, "");
	m_pPlayerNameLabel = new Label(m_pContent, "", "Player Name:");
	m_pPlayerNameText = new TextEntry(m_pContent, "");
	m_pPlayerNameText->SetMaximumCharCount(MAX_PLAYER_NAME_LENGTH - 1);
	m_pClubNameLabel = new Label(m_pContent, "", "IOS Club Initials:");
	m_pClubNameText = new TextEntry(m_pContent, "");
	m_pClubNameText->SetMaximumCharCount(MAX_CLUBNAME_LENGTH - 1);
	m_pCountryNameLabel = new Label(m_pContent, "", "Country Fallback Name:");
	m_pCountryNameList = new ComboBox(m_pContent, "", COUNTRY_NAMES_COUNT, false);

	m_pLegacySideCurl = new CheckButton(m_pContent, "", "Legacy Side Curl");
	m_pLegacyVerticalLook = new CheckButton(m_pContent, "", "Legacy Vertical Look");

	m_pOKButton = new Button(m_pContent, "", "OK");
	m_pCancelButton = new Button(m_pContent, "", "Cancel");
	m_pSaveButton = new Button(m_pContent, "", "Apply");

	m_pChangeInfoText = new Label(m_pContent, "", "Go spectator mode to change");

	m_pCountryNameList->RemoveAll();

	for (int i = 0; i < COUNTRY_NAMES_COUNT; i++)
	{
		KeyValues *kv = new KeyValues("UserData", "index", i);
		m_pCountryNameList->AddItem(g_szCountryNames[i], kv);
		kv->deleteThis();
	}

	m_pPreferredShirtNumberLabel = new Label(m_pContent, "", "Preferred Shirt Number:");
	m_pPreferredShirtNumberList = new ComboBox(m_pContent, "", SHIRT_NUMBER_COUNT, false);
	m_pPreferredShirtNumberList->RemoveAll();

	m_pInterpDurationLabel = new Label(m_pContent, "", "Interpolation Duration:");
	m_pInterpDurationList = new ComboBox(m_pContent, "", 0, false);
	m_pInterpDurationInfoButton = new Button(m_pContent, "", "?");
	m_pInterpDurationInfoButton->GetTooltip()->SetText("The shorter the interpolation duration, the quicker your client will display updated player and ball positions received from the server.\nIf you notice that other players and the ball don't move smoothly, it could mean that too many packets are lost on the way between you and the server.\nTry increasing the interpolation duration until the game is smooth again.");
	m_pInterpDurationInfoButton->GetTooltip()->SetTooltipDelay(0);

	m_pSmoothDurationLabel = new Label(m_pContent, "", "Smoothing Duration:");
	m_pSmoothDurationList = new ComboBox(m_pContent, "", 0, false);
	m_pSmoothDurationInfoButton = new Button(m_pContent, "", "?");
	m_pSmoothDurationInfoButton->GetTooltip()->SetText("The shorter the smoothing duration, the quicker your client will set your local player to the correct position, should your client have incorrectly predicted your own position.\nTo make the game feel more reponsive, your client immediately performs certain actions like moving around and jumping, instead of waiting for the server to give confirmation for them.\nSometimes, when other players or the ball is close to you, the predictions of the client will be wrong and your local player can't actually move to the position he just went to during the prediction.\nThe smoothing duration is the time your client spends moving your own player to the correct position as received from the server.");
	m_pSmoothDurationInfoButton->GetTooltip()->SetTooltipDelay(0);

	KeyValues *kv;

	kv = new KeyValues("UserData", "index", 0);
	m_pPreferredShirtNumberList->AddItem("<None>", kv);
	kv->deleteThis();

	for (int i = 1; i < SHIRT_NUMBER_COUNT; i++)
	{
		kv = new KeyValues("UserData", "index", i);
		m_pPreferredShirtNumberList->AddItem(VarArgs("%d", i + 1), kv);
		kv->deleteThis();
	}

	for (int i = 0; i < INTERP_VALUES; i++)
	{
		kv = new KeyValues("UserData", "value", interpValues[i]);
		m_pInterpDurationList->AddItem(interpTexts[i], kv);
		kv->deleteThis();
	}

	for (int i = 0; i < SMOOTH_VALUES; i++)
	{
		kv = new KeyValues("UserData", "value", smoothValues[i]);
		m_pSmoothDurationList->AddItem(smoothTexts[i], kv);
		kv->deleteThis();
	}

	m_pRateLabel = new Label(m_pContent, "", "Rate (rate):");
	m_pRateText = new TextEntry(m_pContent, "");
	m_pRateInfoButton = new Button(m_pContent, "", "?");
	m_pRateInfoButton->GetTooltip()->SetText("'Rate' sets the maximum bandwidth available for receiving packets from the server.\nIf 'net_graph 3' shows choke, increase the rate until the choke value shows 0.\nIf you can't increase 'Rate' any further due to a slow connection, consider lowering 'Update Rate' and 'Command Rate'.");
	m_pRateInfoButton->GetTooltip()->SetTooltipDelay(0);

	m_pUpdaterateLabel = new Label(m_pContent, "", "Update Rate (cl_updaterate):");
	m_pUpdaterateText = new TextEntry(m_pContent, "");
	m_pUpdaterateInfoButton = new Button(m_pContent, "", "?");
	m_pUpdaterateInfoButton->GetTooltip()->SetText("'Update Rate' sets the number of updates per second you want to receive from the server.\nThe maximum value is the current server tickrate, which is usually 66 or 100.\nThe higher 'Update Rate' the more download bandwidth will be used.");
	m_pUpdaterateInfoButton->GetTooltip()->SetTooltipDelay(0);

	m_pCommandrateLabel = new Label(m_pContent, "", "Command Rate (cl_cmdrate):");
	m_pCommandrateText = new TextEntry(m_pContent, "");
	m_pCommandrateInfoButton = new Button(m_pContent, "", "?");
	m_pCommandrateInfoButton->GetTooltip()->SetText("'Command Rate' sets the number of input updates per second you want to send to the server.\nThe maximum value is the current server tickrate, which is usually 66 or 100.\nThe higher 'Command Rate' the more upload bandwidth will be used.");
	m_pCommandrateInfoButton->GetTooltip()->SetTooltipDelay(0);

	m_pHudMatchEventTick = new CheckButton(m_pContent, "", "Show minor match events (dribblings, passes, etc.)");
	m_pHudMatchNamesTick = new CheckButton(m_pContent, "", "Show player names for minor match events");
}

CIOSOptionsPanel::~CIOSOptionsPanel()
{
}

void CIOSOptionsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;
	BaseClass::ApplySchemeSettings( pScheme );

	SetTitle("PLAYER SETTINGS", false);
	SetProportional(false);
	SetSizeable(false);
	SetBounds(0, 0, 550, 500);
	SetBgColor(Color(0, 0, 0, 255));
	SetPaintBackgroundEnabled(true);
	MoveToCenterOfScreen();

	m_pContent->SetBounds(PADDING, PADDING + TOP_PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING - TOP_PADDING);

	m_pPlayerNameLabel->SetBounds(0, 0, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->SetBounds(LABEL_WIDTH, 0, INPUT_WIDTH, TEXT_HEIGHT);
	m_pPlayerNameText->AddActionSignalTarget( this );
	m_pPlayerNameText->SendNewLine(true); // with the txtEntry Type you need to set it to pass the return key as a message

	m_pClubNameLabel->SetBounds(0, TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pClubNameText->SetBounds(LABEL_WIDTH, TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);

	m_pCountryNameLabel->SetBounds(0, 2 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->SetBounds(LABEL_WIDTH, 2 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pCountryNameList->GetMenu()->MakeReadyForUse();

	m_pPreferredShirtNumberLabel->SetBounds(0, 3 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pPreferredShirtNumberList->SetBounds(LABEL_WIDTH, 3 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pPreferredShirtNumberList->GetMenu()->MakeReadyForUse();

	m_pInterpDurationLabel->SetBounds(0, 4 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationList->SetBounds(LABEL_WIDTH, 4 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationInfoButton->SetBounds(LABEL_WIDTH + INPUT_WIDTH + INFOBUTTON_MARGIN, 4 * TEXT_HEIGHT, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pInterpDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pSmoothDurationLabel->SetBounds(0, 5 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationList->SetBounds(LABEL_WIDTH, 5 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationInfoButton->SetBounds(LABEL_WIDTH + INPUT_WIDTH + INFOBUTTON_MARGIN, 5 * TEXT_HEIGHT, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pSmoothDurationInfoButton->SetContentAlignment(Label::a_center);

	m_pLegacySideCurl->SetBounds(0, 6 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pLegacyVerticalLook->SetBounds(0, 7 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);

	m_pHudMatchEventTick->SetBounds(0, 8 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);
	m_pHudMatchNamesTick->SetBounds(0, 9 * TEXT_HEIGHT, LABEL_WIDTH + INPUT_WIDTH, TEXT_HEIGHT);

	m_pRateLabel->SetBounds(0, 10 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pRateText->SetBounds(LABEL_WIDTH, 10 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pRateInfoButton->SetBounds(LABEL_WIDTH + INPUT_WIDTH + INFOBUTTON_MARGIN, 10 * TEXT_HEIGHT, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pRateInfoButton->SetContentAlignment(Label::a_center);

	m_pUpdaterateLabel->SetBounds(0, 11 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateText->SetBounds(LABEL_WIDTH, 11 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateInfoButton->SetBounds(LABEL_WIDTH + INPUT_WIDTH + INFOBUTTON_MARGIN, 11 * TEXT_HEIGHT, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pUpdaterateInfoButton->SetContentAlignment(Label::a_center);

	m_pCommandrateLabel->SetBounds(0, 12 * TEXT_HEIGHT, LABEL_WIDTH, TEXT_HEIGHT);
	m_pCommandrateText->SetBounds(LABEL_WIDTH, 12 * TEXT_HEIGHT, INPUT_WIDTH, TEXT_HEIGHT);
	m_pCommandrateInfoButton->SetBounds(LABEL_WIDTH + INPUT_WIDTH + INFOBUTTON_MARGIN, 12 * TEXT_HEIGHT, INFOBUTTON_WIDTH, TEXT_HEIGHT);
	m_pCommandrateInfoButton->SetContentAlignment(Label::a_center);

	m_pOKButton->SetBounds(m_pContent->GetWide() - 3 * BUTTON_WIDTH - 2 * BUTTON_MARGIN, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pOKButton->SetCommand("save_and_close");
	m_pOKButton->AddActionSignalTarget(this);

	m_pCancelButton->SetBounds(m_pContent->GetWide() - 2 * BUTTON_WIDTH - BUTTON_MARGIN, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pCancelButton->SetCommand("close");
	m_pCancelButton->AddActionSignalTarget(this);

	m_pSaveButton->SetBounds(m_pContent->GetWide() - BUTTON_WIDTH, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pSaveButton->SetCommand("save_settings");
	m_pSaveButton->AddActionSignalTarget(this);

	m_pChangeInfoText->SetBounds(0, m_pContent->GetTall() - BUTTON_HEIGHT, m_pContent->GetWide() - 3 * BUTTON_WIDTH, BUTTON_HEIGHT); 
	m_pChangeInfoText->SetFgColor(Color(255, 153, 153, 255));
}

void CIOSOptionsPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CIOSOptionsPanel::OnThink()
{
	BaseClass::OnThink();

	//SetTall((int)(gpGlobals->curtime * 100) % 100);
	//m_pSettingsPanel->Update();

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
		else if (GetLocalPlayerTeam() == TEAM_A || GetLocalPlayerTeam() == TEAM_B)
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

void CIOSOptionsPanel::OnCommand(const char *cmd)
{
	if (!stricmp(cmd, "save_settings") || !stricmp(cmd, "save_and_close"))
	{
		char text[64];
		m_pPlayerNameText->GetText(text, sizeof(text));
		g_pCVar->FindVar("playername")->SetValue(text);
		m_pClubNameText->GetText(text, sizeof(text));
		g_pCVar->FindVar("clubname")->SetValue(text);
		g_pCVar->FindVar("fallbackcountryindex")->SetValue(m_pCountryNameList->GetActiveItemUserData()->GetInt("index"));
		m_pPreferredShirtNumberList->GetText(text, sizeof(text));
		g_pCVar->FindVar("preferredshirtnumber")->SetValue(atoi(text));

		g_pCVar->FindVar("cl_interp_ratio")->SetValue(atoi(m_pInterpDurationList->GetActiveItemUserData()->GetString("value")));
		g_pCVar->FindVar("cl_smoothtime")->SetValue(atoi(m_pSmoothDurationList->GetActiveItemUserData()->GetString("value")) / 100.0f);

		g_pCVar->FindVar("legacysidecurl")->SetValue(m_pLegacySideCurl->IsSelected() ? 1 : 0);
		g_pCVar->FindVar("legacyverticallook")->SetValue(m_pLegacyVerticalLook->IsSelected() ? 1 : 0);

		g_pCVar->FindVar("hud_minor_events_visible")->SetValue(m_pHudMatchEventTick->IsSelected() ? 1 : 0);
		g_pCVar->FindVar("hud_minor_eventplayernames_visible")->SetValue(m_pHudMatchNamesTick->IsSelected() ? 1 : 0);

		m_pRateText->GetText(text, sizeof(text));
		g_pCVar->FindVar("rate")->SetValue(atoi(text));
		m_pUpdaterateText->GetText(text, sizeof(text));
		g_pCVar->FindVar("cl_updaterate")->SetValue(atoi(text));
		m_pCommandrateText->GetText(text, sizeof(text));
		g_pCVar->FindVar("cl_cmdrate")->SetValue(atoi(text));

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

	if (Q_strlen(g_pCVar->FindVar("playername")->GetString()) == 0)
		g_pCVar->FindVar("playername")->SetValue(g_pCVar->FindVar("name")->GetString());

	m_pPlayerNameText->SetText(g_pCVar->FindVar("playername")->GetString());
	m_pCountryNameList->SetText(g_szCountryNames[clamp(g_pCVar->FindVar("fallbackcountryindex")->GetInt(), 0, COUNTRY_NAMES_COUNT)]);
	m_pClubNameText->SetText(g_pCVar->FindVar("clubname")->GetString());
	int shirtNum = g_pCVar->FindVar("preferredshirtnumber")->GetInt();
	m_pPreferredShirtNumberList->SetText(shirtNum == 0 ? "<None>" : VarArgs("%d", clamp(shirtNum, 2, 11)));

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

	m_pLegacySideCurl->SetSelected(g_pCVar->FindVar("legacysidecurl")->GetBool());
	m_pLegacyVerticalLook->SetSelected(g_pCVar->FindVar("legacyverticallook")->GetBool());

	m_pHudMatchEventTick->SetSelected(g_pCVar->FindVar("hud_minor_events_visible")->GetBool());
	m_pHudMatchNamesTick->SetSelected(g_pCVar->FindVar("hud_minor_eventplayernames_visible")->GetBool());

	m_pRateText->SetText(g_pCVar->FindVar("rate")->GetString());
	m_pUpdaterateText->SetText(g_pCVar->FindVar("cl_updaterate")->GetString());
	m_pCommandrateText->SetText(g_pCVar->FindVar("cl_cmdrate")->GetString());
}