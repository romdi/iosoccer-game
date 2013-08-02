#include "cbase.h"
#include "ios_update_menu.h"
#include "ienginevgui.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"
#include "ios_teamkit_parse.h"
#include "c_ball.h"
#include "vgui/IVgui.h"
#include "ios_fileupdater.h"

class CIOSUpdateMenu : public IIOSUpdateMenu
{
private:
	CIOSUpdatePanel *m_pIOSUpdatePanel;
	vgui::VPANEL m_hParent;
 
public:
	CIOSUpdateMenu( void )
	{
		m_pIOSUpdatePanel = NULL;
	}
 
	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		m_pIOSUpdatePanel = new CIOSUpdatePanel(parent);
	}
 
	void Destroy( void )
	{
		if ( m_pIOSUpdatePanel )
		{
			m_pIOSUpdatePanel->SetParent( (vgui::Panel *)NULL );
			delete m_pIOSUpdatePanel;
		}
	}
	CIOSUpdatePanel *GetPanel()
	{
		return m_pIOSUpdatePanel;
	}
};

static CIOSUpdateMenu g_IOSUpdateMenu;
IIOSUpdateMenu *iosUpdateMenu = (IIOSUpdateMenu *)&g_IOSUpdateMenu;

void CC_IOSUpdateMenu(const CCommand &args)
{
	if (!iosUpdateMenu->GetPanel()->IsVisible())
		iosUpdateMenu->GetPanel()->Activate(UPDATE_NONE_PENDING);
	else
		iosUpdateMenu->GetPanel()->Close();
}

ConCommand iosupdatemenu("iosupdatemenu", CC_IOSUpdateMenu);

enum { LABEL_WIDTH = 260, INPUT_WIDTH = 260, SHORTINPUT_WIDTH = 200, TEXT_HEIGHT = 26, TEXT_MARGIN = 5 };
enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = 500, PANEL_HEIGHT = 300 };
enum { PADDING = 10, TOP_PADDING = 30 };
enum { UPDATE_BUTTON_WIDTH = 160, UPDATE_BUTTON_HEIGHT = 52, UPDATE_BUTTON_MARGIN = 5 };
enum { BUTTON_WIDTH = 80, BUTTON_HEIGHT = 26, BUTTON_MARGIN = 50 };
enum { SUGGESTED_VALUE_WIDTH = 100, SUGGESTED_VALUE_MARGIN = 5 };
enum { INFO_WIDTH = 400, INFO_HEIGHT = 20 };
enum { APPEARANCE_HOFFSET = 270, APPEARANCE_RADIOBUTTONWIDTH = 70 };

CIOSUpdatePanel::CIOSUpdatePanel(VPANEL parent) : BaseClass(NULL, "IOSUpdatePanel")
{
	SetScheme("SourceScheme");
	SetParent(parent);
	m_pContent = new Panel(this, "");
	m_pUpdateButton = new Button(m_pContent, "", "", this, "");
	m_pCloseButton = new Button(m_pContent, "", "", this, "");
	m_pInfoText = new Label(m_pContent, "", "");
	m_pExtraInfoText = new Label(m_pContent, "", "");

	m_ePendingUpdate = UPDATE_NONE_PENDING;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

CIOSUpdatePanel::~CIOSUpdatePanel()
{
}

void CIOSUpdatePanel::ApplySchemeSettings( IScheme *pScheme )
{
	m_pScheme = pScheme;
	BaseClass::ApplySchemeSettings( pScheme );

	SetTitle("IOS Updater", false);
	SetProportional(false);
	SetSizeable(false);
	SetBounds(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
	//SetBgColor(Color(0, 0, 0, 255));
	SetPaintBackgroundEnabled(true);
	SetCloseButtonVisible(false);
	MoveToCenterOfScreen();

	m_pContent->SetBounds(PADDING, PADDING + TOP_PADDING, GetWide() - 2 * PADDING, GetTall() - 2 * PADDING - TOP_PADDING);
	
	m_pUpdateButton->SetBounds(m_pContent->GetWide() / 2 - UPDATE_BUTTON_WIDTH / 2, m_pContent->GetTall() - UPDATE_BUTTON_HEIGHT - BUTTON_MARGIN - BUTTON_HEIGHT, UPDATE_BUTTON_WIDTH, UPDATE_BUTTON_HEIGHT);
	m_pUpdateButton->SetContentAlignment(Label::a_center);
	
	m_pCloseButton->SetBounds(m_pContent->GetWide() / 2 - BUTTON_WIDTH / 2, m_pContent->GetTall() - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT);
	m_pCloseButton->SetContentAlignment(Label::a_center);
	
	m_pInfoText->SetBounds(PADDING, 3 * PADDING, m_pContent->GetWide() - 2 * PADDING, INFO_HEIGHT); 
	m_pInfoText->SetFont(pScheme->GetFont("DefaultBig"));
	m_pInfoText->SetContentAlignment(Label::a_center);
	m_pInfoText->SetFgColor(Color(255, 255, 255, 255));

	m_pExtraInfoText->SetBounds(PADDING, 3 * PADDING + INFO_HEIGHT + PADDING, m_pContent->GetWide() - 2 * PADDING, INFO_HEIGHT); 
	m_pExtraInfoText->SetFont(pScheme->GetFont("DefaultBig"));
	m_pExtraInfoText->SetContentAlignment(Label::a_center);
	m_pExtraInfoText->SetFgColor(Color(255, 255, 255, 255));
}

void CIOSUpdatePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();
}

void CIOSUpdatePanel::OnThink()
{
	BaseClass::OnThink();
}

void CIOSUpdatePanel::Init()
{
}

void CIOSUpdatePanel::OnTick()
{
	BaseClass::OnTick();

	if (m_ePendingUpdate != UPDATE_NONE_PENDING)
	{
		IOSUpdateInfo updateInfo = IOSUpdateInfo();
		updateInfo.checkOnly = (m_ePendingUpdate == UPDATE_CHECK_ONLY || m_ePendingUpdate == UPDATE_CHECK_ONLY_AND_CLOSE);

		CFileUpdater::UpdateFiles(&updateInfo);

		if (updateInfo.connectionError)
		{
			m_pInfoText->SetText("Can't connect to the IOS update server.");
			m_pExtraInfoText->SetText("Try to update again in a few minutes.");
			m_pCloseButton->SetText("Close");
			m_pCloseButton->SetCommand("close");
		}
		else if (updateInfo.checkOnly)
		{
			if (updateInfo.filesToUpdateCount > 0)
			{
				m_pInfoText->SetText(VarArgs("Updates available for %d %s.", updateInfo.filesToUpdateCount, updateInfo.filesToUpdateCount == 1 ? "file" : "files"));
				m_pExtraInfoText->SetText("");
				m_pUpdateButton->SetVisible(true);
				m_pUpdateButton->SetText("Download the updates");
				m_pUpdateButton->SetCommand("update");
				m_pCloseButton->SetText("Close");
				m_pCloseButton->SetCommand("close");
			}
			else
			{
				m_pInfoText->SetText("Your files are up to date.");
				m_pUpdateButton->SetVisible(false);
				m_pCloseButton->SetText("Close");
				m_pCloseButton->SetCommand("close");

				if (m_ePendingUpdate == UPDATE_CHECK_ONLY_AND_CLOSE)
				{
					m_ePendingUpdate = UPDATE_NONE_PENDING;
					Close();
				}
			}
		}
		else
		{
			CTeamInfo::ParseTeamKits();

			m_pCloseButton->SetText("Close");
			m_pCloseButton->SetCommand("close");
			m_pInfoText->SetText("Files successfully updated.");

			if (updateInfo.restartRequired)
			{
				m_pExtraInfoText->SetText("RESTART THE GAME TO APPLY THE UPDATES.");
				m_pUpdateButton->SetVisible(true);
				m_pUpdateButton->SetText("Quit the game");
				m_pUpdateButton->SetCommand("quit");
			}
		}

		m_ePendingUpdate = UPDATE_NONE_PENDING;
	}
}

void CIOSUpdatePanel::OnCommand(const char *cmd)
{
	if (!stricmp(cmd, "update"))
	{
		m_pInfoText->SetText("Downloading updates. This may take a while. Please wait...");
		m_pExtraInfoText->SetText("");
		m_pUpdateButton->SetVisible(false);
		m_pCloseButton->SetText("Cancel");
		m_pCloseButton->SetCommand("cancel");

		m_ePendingUpdate = UPDATE_DOWNLOAD;
	}
	else if (!stricmp(cmd, "check"))
	{
		m_ePendingUpdate = UPDATE_CHECK_ONLY;
	}
	else if (!stricmp(cmd, "cancel"))
	{
		Close();
	}
	else if (!stricmp(cmd, "close"))
	{
		Close();
	}
	else if (!stricmp(cmd, "quit"))
	{
		engine->ClientCmd("quit");
	}
	else
		BaseClass::OnCommand(cmd);
}

void CIOSUpdatePanel::Activate(PendingUpdate_t pendingUpdate)
{
	BaseClass::Activate();

	m_ePendingUpdate = pendingUpdate;

	if (m_ePendingUpdate == UPDATE_CHECK_ONLY_AND_CLOSE)
	{
		m_pInfoText->SetText("Checking for updates...");
		m_pExtraInfoText->SetText("");
		m_pUpdateButton->SetVisible(false);
		m_pCloseButton->SetText("Cancel");
		m_pCloseButton->SetCommand("cancel");
	}
	else
	{
		m_pInfoText->SetText("");
		m_pExtraInfoText->SetText("");
		m_pUpdateButton->SetVisible(true);
		m_pUpdateButton->SetText("Check for updates");
		m_pUpdateButton->SetCommand("check");
		m_pCloseButton->SetText("Close");
		m_pCloseButton->SetCommand("close");
	}
}