#include "cbase.h"
#include "ios_update_menu.h"
#include "ienginevgui.h"
#include "c_sdk_player.h"
#include "c_playerresource.h"
#include "sdk_gamerules.h"
#include "ios_teamkit_parse.h"
#include "c_match_ball.h"
#include "vgui/IVgui.h"
#include "ios_fileupdater.h"
#include "Windows.h"
#include "Filesystem.h"

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
		iosUpdateMenu->GetPanel()->Activate(UPDATE_STATE_READY_TO_CHECK);
	else
		iosUpdateMenu->GetPanel()->Close();
}

ConCommand iosupdatemenu("iosupdatemenu", CC_IOSUpdateMenu);

enum { LABEL_WIDTH = 260, INPUT_WIDTH = 260, SHORTINPUT_WIDTH = 200, TEXT_HEIGHT = 26, TEXT_MARGIN = 5 };
enum { PANEL_TOPMARGIN = 70, PANEL_MARGIN = 5, PANEL_WIDTH = 850, PANEL_HEIGHT = 750 };
enum { PADDING = 10, TOP_PADDING = 30 };
enum { UPDATE_BUTTON_WIDTH = 160, UPDATE_BUTTON_HEIGHT = 52, UPDATE_BUTTON_MARGIN = 5 };
enum { BUTTON_WIDTH = 80, BUTTON_HEIGHT = 26, BUTTON_MARGIN = 40 };
enum { INFO_WIDTH = 800, INFO_HEIGHT = 20 };
enum { PROGRESSBAR_WIDTH = 800, PROGRESSBAR_HEIGHT = 25 };
enum { CHANGELOG_WIDTH = 800, CHANGELOG_HEIGHT = 400 };

CIOSUpdatePanel::CIOSUpdatePanel(VPANEL parent) : BaseClass(NULL, "IOSUpdatePanel")
{
	SetScheme("SourceScheme");
	SetParent(parent);
	m_pContent = new Panel(this, "");
	m_pUpdateButton = new Button(m_pContent, "", "", this, "");
	m_pCloseButton = new Button(m_pContent, "", "", this, "");
	m_pInfoText = new Label(m_pContent, "", "");
	m_pExtraInfoText = new Label(m_pContent, "", "");
	m_pExtraInfoText2 = new Label(m_pContent, "", "");
	m_pProgressBar = new ProgressBar(m_pContent, "");
	m_pChangelog = new RichText(m_pContent, "");

	m_eUpdateState = UPDATE_STATE_NONE;
	m_pUpdateInfo = new IOSUpdateInfo();
	m_bGoToTextStart = false;

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

	m_pExtraInfoText2->SetBounds(PADDING, 3 * PADDING + 2 * (INFO_HEIGHT + PADDING), m_pContent->GetWide() - 2 * PADDING, INFO_HEIGHT); 
	m_pExtraInfoText2->SetFont(pScheme->GetFont("DefaultBig"));
	m_pExtraInfoText2->SetContentAlignment(Label::a_center);
	m_pExtraInfoText2->SetFgColor(Color(255, 255, 255, 255));

	m_pProgressBar->SetBounds(m_pContent->GetWide() / 2 - PROGRESSBAR_WIDTH / 2, 3 * PADDING + 3 * (INFO_HEIGHT + PADDING), PROGRESSBAR_WIDTH, PROGRESSBAR_HEIGHT);

	m_pChangelog->SetBounds(m_pContent->GetWide() / 2 - CHANGELOG_WIDTH / 2, 4 * PADDING + 3 * (INFO_HEIGHT + PADDING) + PROGRESSBAR_HEIGHT, CHANGELOG_WIDTH, CHANGELOG_HEIGHT);
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

	if (m_bGoToTextStart)
	{
		m_pChangelog->GotoTextStart();
		m_bGoToTextStart = false;
	}

	if (m_eUpdateState == UPDATE_STATE_READY_TO_CHECK)
	{
		m_pInfoText->SetText("");
		m_pExtraInfoText->SetText("");
		m_pExtraInfoText2->SetText("");
		m_pProgressBar->SetVisible(false);
		m_pUpdateButton->SetVisible(true);
		m_pUpdateButton->SetText("Check for updates");
		m_pUpdateButton->SetCommand("check");
		m_pCloseButton->SetText("Close");
		m_pCloseButton->SetCommand("close");

		m_eUpdateState = UPDATE_STATE_NONE;
	}
	else if (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_PENDING || m_eUpdateState == UPDATE_STATE_CHECK_ONLY_AND_CLOSE_PENDING)
	{
		m_pInfoText->SetText("Checking for updates...");
		m_pExtraInfoText->SetText("");
		m_pExtraInfoText2->SetText("");
		m_pUpdateButton->SetVisible(false);
		m_pCloseButton->SetText("Cancel");
		m_pCloseButton->SetCommand("cancel");

		m_eUpdateState = (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_PENDING ? UPDATE_STATE_CHECK_ONLY_RUNNING : UPDATE_STATE_CHECK_ONLY_AND_CLOSE_RUNNING);

		m_pUpdateInfo->Reset();
		m_pUpdateInfo->checkOnly = true;
		m_pUpdateInfo->async = true;

		CFileUpdater::UpdateFiles(m_pUpdateInfo);
	}
	else if (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_RUNNING || m_eUpdateState == UPDATE_STATE_CHECK_ONLY_AND_CLOSE_RUNNING)
	{
		if (m_pUpdateInfo->finished)
		{
			m_eUpdateState = (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_RUNNING ? UPDATE_STATE_CHECK_ONLY_FINISHED : UPDATE_STATE_CHECK_ONLY_AND_CLOSE_FINISHED);
		}
	}
	else if (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_FINISHED || m_eUpdateState == UPDATE_STATE_CHECK_ONLY_AND_CLOSE_FINISHED)
	{
		if (m_pUpdateInfo->connectionError)
		{
			m_pInfoText->SetText("Can't connect to the IOS update server");
			m_pExtraInfoText->SetText("Try to update again in a few minutes");
			m_pExtraInfoText2->SetText("");
			m_pCloseButton->SetText("Close");
			m_pCloseButton->SetCommand("close");
		}
		else
		{
			if (m_pUpdateInfo->changelogDownloaded)
			{
				// We have to use InsertString or the \r won't be filtered out and there will be extra newlines
				m_pChangelog->SetText("");
				m_pChangelog->InsertString(m_pUpdateInfo->changelogText);
				m_bGoToTextStart = true;
			}

			if (m_pUpdateInfo->filesToUpdateCount > 0)
			{
				m_pInfoText->SetText(VarArgs("Updates available for %d %s", m_pUpdateInfo->filesToUpdateCount, m_pUpdateInfo->filesToUpdateCount == 1 ? "file" : "files"));
				m_pExtraInfoText->SetText("");
				m_pExtraInfoText2->SetText("");
				m_pUpdateButton->SetVisible(true);
				m_pUpdateButton->SetText("Download the updates");
				m_pUpdateButton->SetCommand("update");
				m_pCloseButton->SetText("Close");
				m_pCloseButton->SetCommand("close");
			}
			else
			{
				m_pInfoText->SetText("No updates found");
				m_pUpdateButton->SetVisible(false);
				m_pCloseButton->SetText("Close");
				m_pCloseButton->SetCommand("close");

				if (m_eUpdateState == UPDATE_STATE_CHECK_ONLY_AND_CLOSE_FINISHED)
				{
					m_eUpdateState = UPDATE_STATE_NONE;
					Close();
				}
			}
		}

		m_eUpdateState = UPDATE_STATE_NONE;
	}
	else if (m_eUpdateState == UPDATE_STATE_DOWNLOAD_PENDING)
	{
		m_pInfoText->SetText("Downloading updates. Please wait...");
		m_pExtraInfoText->SetText("");
		m_pExtraInfoText2->SetText("");
		m_pProgressBar->SetVisible(true);
		m_pProgressBar->SetProgress(0.0f);
		m_pUpdateButton->SetVisible(false);
		m_pCloseButton->SetText("Cancel");
		m_pCloseButton->SetCommand("cancel");

		m_eUpdateState = UPDATE_STATE_DOWNLOAD_RUNNING;

		int filesToUpdateCount = m_pUpdateInfo->filesToUpdateCount;
		m_pUpdateInfo->Reset();

		// Just make sure the file doesn't jump to 0 for a short duration. The thread will set the value eventually.
		m_pUpdateInfo->filesToUpdateCount = filesToUpdateCount;

		m_pUpdateInfo->checkOnly = false;
		m_pUpdateInfo->async = true;

		CFileUpdater::UpdateFiles(m_pUpdateInfo);
	}
	else if (m_eUpdateState == UPDATE_STATE_DOWNLOAD_RUNNING)
	{
		int filesUpdatedCount = m_pUpdateInfo->filesUpdatedCount;
		int filesToUpdateCount = m_pUpdateInfo->filesToUpdateCount;
		char filePath[256];
		Q_strncpy(filePath, m_pUpdateInfo->filePath, sizeof(filePath));
		float receivedMegaBytes = m_pUpdateInfo->receivedBytes / 1024.0 / 1024.0;
		float totalMegaBytes = m_pUpdateInfo->totalBytes / 1024.0 / 1024.0;

		m_pExtraInfoText->SetText(VarArgs("%.2f/%.2f MB (%d/%d files) downloaded", receivedMegaBytes, totalMegaBytes, filesUpdatedCount, filesToUpdateCount));
		m_pExtraInfoText2->SetText(filePath);
		m_pProgressBar->SetProgress(clamp(receivedMegaBytes / totalMegaBytes, 0.0f, 1.0f));

		if (m_pUpdateInfo->finished)
		{
			m_eUpdateState = UPDATE_STATE_DOWNLOAD_FINISHED;
		}
	}
	else if (m_eUpdateState == UPDATE_STATE_DOWNLOAD_FINISHED)
	{
		if (m_pUpdateInfo->connectionError)
		{
			m_pInfoText->SetText("Can't connect to the IOS update server");
			m_pExtraInfoText->SetText("Try to update again in a few minutes");
			m_pExtraInfoText2->SetText("");
			m_pCloseButton->SetText("Close");
			m_pCloseButton->SetCommand("close");
		}
		else
		{
			m_pCloseButton->SetText("Close");
			m_pCloseButton->SetCommand("close");
			m_pInfoText->SetText("All files successfully updated");
			m_pExtraInfoText->SetText("RESTART IOSOCCER TO APPLY THE UPDATES");
			m_pExtraInfoText2->SetText("");
			m_pUpdateButton->SetVisible(true);
			m_pUpdateButton->SetText("Quit IOSoccer");
			m_pUpdateButton->SetCommand("quit");
		}

		m_eUpdateState = UPDATE_STATE_NONE;
	}
}

void CIOSUpdatePanel::OnCommand(const char *cmd)
{
	if (!stricmp(cmd, "update"))
	{
		m_eUpdateState = UPDATE_STATE_DOWNLOAD_PENDING;
	}
	else if (!stricmp(cmd, "check"))
	{
		m_eUpdateState = UPDATE_STATE_CHECK_ONLY_PENDING;
	}
	else if (!stricmp(cmd, "cancel"))
	{
		m_pUpdateInfo->cancelled = true;
		m_eUpdateState = UPDATE_STATE_READY_TO_CHECK;
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

void CIOSUpdatePanel::Activate(UpdateState_t updateState)
{
	BaseClass::Activate();

	m_eUpdateState = updateState;

	OnTick();

	m_bGoToTextStart = true;
}