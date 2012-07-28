#include "cbase.h"
#include "ircclient.h"

void CC_IRCMenu(const CCommand &args)
{
	//IViewPortPanel *pPanel = gViewPortInterface->FindPanelByName("TestPanel");
	//pPanel->ShowPanel(!pPanel->IsVisible());
	g_pTestPanel->SetVisible(!g_pTestPanel->IsVisible());
}

ConCommand ircmenu("ircmenu", CC_IRCMenu);

CTestPanel::CTestPanel(vgui::VPANEL parent) : BaseClass(NULL, "TestPanel")
{
	SetVisible(false);
	pConnect = new vgui::Button(this, "", "Connect");
	pConnect->SetCommand("connect");

	pJoin = new vgui::Button(this, "", "Join");
	pJoin->SetCommand("join");
	pJoin->SetEnabled(false);

	pStatus = new vgui::Label(this, "", "");

	SetParent( parent );
	SetTitleBarVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );
	SetProportional( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( true );
}

CTestPanel::~CTestPanel()
{
	g_IRCThread.quit = true;
}

void CTestPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBounds(0, 0, 700, 200);
	SetBgColor(Color(0, 0, 0, 255));
	SetPaintBackgroundEnabled(true);

	pConnect->SetBounds(GetWide() - 100, GetTall() - 40, 100, 40);
	pJoin->SetBounds(GetWide() - 200, GetTall() - 40, 100, 40);
	pStatus->SetBounds(0, 0, GetWide(), 150);
}

void CTestPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CTestPanel::OnThink()
{
	BaseClass::OnThink();

	//SetTall((int)(gpGlobals->curtime * 100) % 100);
}

void CTestPanel::SetStatus(char *msg)
{
	pStatus->SetText(msg);
}

void CTestPanel::OnCommand(const char *cmd)
{
	if (!Q_strcmp(cmd, "connect"))
	{
		if (!g_IRCThread.IsAlive())
			g_IRCThread.Start();

		g_IRCThread.ConnectToIRC();

		pConnect->SetText("Connecting...");
		pConnect->SetEnabled(false);
	}
	else if (!Q_strcmp(cmd, "disconnect"))
	{
		if (g_IRCThread.IsAlive())
			g_IRCThread.Stop();

		pConnect->SetText("Connect");
		pConnect->SetCommand("connect");
		pJoin->SetEnabled(false);
	}
	else if (!Q_strcmp(cmd, "join"))
	{
		g_IRCThread.SendJoinMessage();
	}
	else
		BaseClass::OnCommand(cmd);
}

#include "steam/steam_api.h"

CIRCThread::CIRCThread() :
m_pKV( NULL )
{
	SetName( "IRCThread" );
	quit = false;
	m_Chan = "#ios.dev";
	m_Nick = "IOS_dirom";//steamapicontext->SteamFriends()->GetPersonaName();
}

CIRCThread::~CIRCThread()
{
}

void CIRCThread::ConnectToIRC()
{
	//Assert( !m_pKV );
	//m_pKV = pKV;
	CallWorker( CALL_CONNECT );
	//Assert( !m_pKV );
}

void CIRCThread::SendJoinMessage()
{
	Send("PRIVMSG " + m_Chan + " !join");
}

void CIRCThread::SendMessage(char *msg)
{

}

int CIRCThread::Run()
{
	unsigned nCall;
	while ( WaitForCall( &nCall ) )
	{
		if ( nCall == EXIT )
		{
			Reply( 1 );
			break;
		}

		KeyValues *pKV = m_pKV;
		m_pKV = NULL;
		Reply( 1 );
		Connect();
	}
	return 0;
}

void CIRCThread::Message(std::string msg)
{
	if (msg.find(":iosmixbot") == 0 && msg.find("PRIVMSG") != std::string::npos)
	{
		size_t pos = msg.find_first_of(':', 1);
		g_pTestPanel->SetStatus(const_cast<char *>(msg.substr(pos + 1).c_str()));
	}
}

int CIRCThread::ConnectToServer()
{
	char serverip[20];
	//sockaddr_in addr;
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo("irc.quakenet.org", "6667", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	m_Socket = INVALID_SOCKET;
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	// Create a SOCKET for connecting to server
	m_Socket = socket(ptr->ai_family, ptr->ai_socktype,	ptr->ai_protocol);

	if (m_Socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Connect to server.
	iResult = connect(m_Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (m_Socket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	std::string nickmsg = "NICK " + m_Nick;
	std::string usermsg = "USER " + m_Nick + " localhost irc.quakenet.org " + m_Nick;
	Send(nickmsg);
	Send(usermsg);
}

void CIRCThread::Send(std::string msg)
{
	msg += "\r\n";
	send(m_Socket, msg.c_str(), msg.length(), 0);
}

void CIRCThread::ReceiveMessages()
{
	char ausgabe[1000];
	int x = 0;

	while(1)
	{
		if (quit)
			break;

		x = recv((SOCKET)m_Socket, ausgabe, 1000, 0);

		if(x > 0)
		{
			ausgabe[x] = 0;
			Message(ausgabe);
			char *line;
			char *tok = ausgabe;
			while ((line = strtok(tok, "\r\n")) != NULL)
			{
				tok = NULL;
				if (strncmp(line, "PING ", 5) == 0)
				{
					line[1] = 'O';
					Send(line);
				}
				else if (strcmp(line, "NOTICE AUTH :*** No ident response") == 0)
				{
					Send("JOIN " + m_Chan);
					g_pTestPanel->pJoin->SetEnabled(true);
					g_pTestPanel->pConnect->SetText("Disconnect");
					g_pTestPanel->pConnect->SetCommand("disconnect");
				}
			}
		}
		Sleep(10);
	}
}


void CIRCThread::Connect()
{
	ConnectToServer();
	ReceiveMessages();
	WSACleanup();
}
