#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "cbase.h"

#include <vgui_controls/Frame.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Button.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace vgui;

class CTestPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CTestPanel, vgui::Frame);

	CTestPanel(vgui::VPANEL parent);
	~CTestPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink();
	void SetStatus(char *text);
	void OnCommand(const char *cmd);

	//private:
	Button *pConnect;
	Button *pJoin;
	Label *pStatus;
	TextEntry *pMessage;

};

class CIRCThread : public CWorkerThread 
{
public:
	CIRCThread();
	~CIRCThread();

	enum
	{
		CALL_CONNECT,
		CALL_SEND,
		EXIT,
	};

	void ConnectToIRC();
	void SendJoinMessage();
	void SendMessage(char *msg);
	void Send(std::string msg);
	int Run();
	bool quit;

private:
	KeyValues *m_pKV;
	SOCKET m_Socket;

	void Message(std::string msg);
	int ConnectToServer();
	void ReceiveMessages();
	void Connect();

	std::string m_Chan;
	std::string m_Nick;
};

static CIRCThread g_IRCThread;

CTestPanel *g_pTestPanel;

#endif