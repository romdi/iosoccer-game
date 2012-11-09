//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ImageList.h>
#include "c_baseplayer.h"
#include "voice_status.h"
#include "clientmode_shared.h"
#include "c_playerresource.h"
#include "voice_common.h"
#include <igameresources.h>
#include "c_sdk_player.h"

#include "vgui_avatarimage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar *sv_alltalk = NULL;

using namespace vgui;
//=============================================================================
// Icon for the local player using voice
//=============================================================================
class CHudVoiceSelfStatus : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudVoiceSelfStatus, vgui::Panel );

	CHudVoiceSelfStatus( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void VidInit();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	CHudTexture *m_pVoiceIcon;


	Color	m_clrIcon;
};


DECLARE_HUDELEMENT( CHudVoiceSelfStatus );


CHudVoiceSelfStatus::CHudVoiceSelfStatus( const char *pName ) :
	vgui::Panel( NULL, "HudVoiceSelfStatus" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );

	m_pVoiceIcon = NULL;

	SetHiddenBits( 0 );

	m_clrIcon = Color(255,255,255,255);
}

	
void CHudVoiceSelfStatus::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

#ifdef HL2MP
	SetBgColor( Color( 0, 0, 0, 0 ) );
#endif

	SetPaintBackgroundEnabled( false );
}

void CHudVoiceSelfStatus::VidInit( void )
{
	m_pVoiceIcon = gHUD.GetIcon( "voice_self" );
}

bool CHudVoiceSelfStatus::ShouldDraw()
{
	return GetClientVoiceMgr()->IsLocalPlayerSpeaking();
}

void CHudVoiceSelfStatus::Paint()
{
   if( !m_pVoiceIcon )
		return;
	
	int x, y, w, h;
	GetBounds( x, y, w, h );

	m_pVoiceIcon->DrawSelf( 0, 0, w, h, m_clrIcon );
}


//=============================================================================
// Icons for other players using voice
//=============================================================================
class CHudVoiceStatus : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudVoiceStatus, vgui::Panel );

	CHudVoiceStatus( const char *name );
	~CHudVoiceStatus()
	{
		if ( NULL != m_pImageList )
		{
			delete m_pImageList;
			m_pImageList = NULL;
		}
	};
	virtual void PostApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void VidInit();
	virtual void Init();
	virtual void OnThink();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:

	//Tony; display avatars instead of an icon.
	vgui::ImageList				*m_pImageList;
	int							m_iImageAvatars[MAX_PLAYERS+1];

	int							m_iPlayerAvatar[MAX_PLAYERS+1];
	CUtlMap<int,int>			m_mapAvatarsToImageList;
	int m_iDeadImageID;

	Color	m_clrIcon;

	HFont	m_hFont;

	CUtlLinkedList< int > m_SpeakingList;
	
	CPanelAnimationVar( vgui::HFont, m_NameFont, "Default", "Default" );

	CPanelAnimationVarAliasType( float, item_tall, "item_tall", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, item_wide, "item_wide", "100", "proportional_float" );

	CPanelAnimationVarAliasType( float, item_spacing, "item_spacing", "2", "proportional_float" );

	CPanelAnimationVarAliasType( float, icon_ypos, "icon_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_xpos, "icon_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_tall, "icon_tall", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_wide, "icon_wide", "32", "proportional_float" );

	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "32", "proportional_float" );
};


DECLARE_HUDELEMENT( CHudVoiceStatus );

extern bool AvatarIndexLessFunc( const int &lhs, const int &rhs );
CHudVoiceStatus::CHudVoiceStatus( const char *pName ) :
	vgui::Panel( NULL, "HudVoiceStatus" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );
	m_pImageList = NULL;

	m_mapAvatarsToImageList.SetLessFunc( AvatarIndexLessFunc );
	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	SetHiddenBits( 0 );

	m_clrIcon = Color(255,255,255,255);

	m_iDeadImageID = surface()->DrawGetTextureId( "hud/leaderboard_dead" );
	if ( m_iDeadImageID == -1 ) // we didn't find it, so create a new one
	{
		m_iDeadImageID = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iDeadImageID, "hud/leaderboard_dead", true, false );
}

void CHudVoiceStatus::ApplySchemeSettings(vgui::IScheme *pScheme)
{

	if ( m_pImageList )
		delete m_pImageList;
	m_pImageList = new ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();
	memset( &m_iImageAvatars, 0, sizeof(int) * (MAX_PLAYERS+1) );

	BaseClass::ApplySchemeSettings( pScheme );

#ifdef HL2MP
	SetBgColor( Color( 0, 0, 0, 0 ) );
#endif

	SetPaintBackgroundEnabled( false );

	m_hFont = pScheme->GetFont("IOSPlayerName");

	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
}
void CHudVoiceStatus::PostApplySchemeSettings( vgui::IScheme *pScheme )
{
	// resize the images to our resolution
	for (int i = 0; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide, tall;
		m_pImageList->GetImage(i)->GetSize(wide, tall);
		m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(),wide), scheme()->GetProportionalScaledValueEx( GetScheme(),tall));
	}
}

void CHudVoiceStatus::Init( void )
{
	m_SpeakingList.RemoveAll();
}

void CHudVoiceStatus::VidInit( void )
{
}

void CHudVoiceStatus::OnThink( void )
{
	//Tony; don't update if local player aint here!
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if (!player)
		return;

	for( int i=1;i<=gpGlobals->maxClients;i++ )
	{
		bool bTalking = GetClientVoiceMgr()->IsPlayerSpeaking(i);
		
		//Tony; update avatars.
		if ( steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
		{
			player_info_t pi;
			if ( engine->GetPlayerInfo( i, &pi ) )
			{
				if ( pi.friendsID )
				{
					CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );

					// See if the avatar's changed
					int iAvatar = steamapicontext->SteamFriends()->GetFriendAvatar( steamIDForPlayer );
					if ( m_iImageAvatars[i] != iAvatar )
					{
						m_iImageAvatars[i] = iAvatar;

						// Now see if we already have that avatar in our list
						int iIndex = m_mapAvatarsToImageList.Find( iAvatar );
						if ( iIndex == m_mapAvatarsToImageList.InvalidIndex() )
						{
							CAvatarImage *pImage = new CAvatarImage();
							pImage->SetAvatarSteamID( steamIDForPlayer );
							pImage->SetSize( 32, 32 );	// Deliberately non scaling
							int iImageIndex = m_pImageList->AddImage( pImage );

							m_mapAvatarsToImageList.Insert( iAvatar, iImageIndex );
						}
					}

					int iIndex = m_mapAvatarsToImageList.Find( iAvatar );

					m_iPlayerAvatar[i] = -1; //Default it.
					if ( iIndex != m_mapAvatarsToImageList.InvalidIndex() )
					{
						//Tony; copy the avatar over.
						m_iPlayerAvatar[i] = m_mapAvatarsToImageList[iIndex];
					}
				}
			}
		}

		// if they are in the list and not talking, remove them
		if( !bTalking )
		{
			// remove them if they are in the list
			m_SpeakingList.FindAndRemove(i);
		}
		else
		{
			// if they are talking and not in the list, add them to the end
			if( m_SpeakingList.Find(i) == m_SpeakingList.InvalidIndex() )
			{
				m_SpeakingList.AddToTail(i);
			}
		}
	}
}

bool CHudVoiceStatus::ShouldDraw()
{
	if ( IsInFreezeCam() == true )
		return false;

	return true;
}

void CHudVoiceStatus::Paint()
{
	//draw everyone in the list!
	for(int i = m_SpeakingList.Head(); i != m_SpeakingList.InvalidIndex(); i = m_SpeakingList.Next(i) )
	{
		C_SDKPlayer *pPl = ToSDKPlayer(UTIL_PlayerByIndex(m_SpeakingList.Element(i)));

		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerText[MAX_PLAYER_NAME_LENGTH + 16];

		g_pVGuiLocalize->ConvertANSIToUnicode(pPl->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName));
		_snwprintf( wszPlayerText, ARRAYSIZE(wszPlayerText) - 1, L"Talking: %s", wszPlayerName);
		wszPlayerText[ ARRAYSIZE(wszPlayerText)-1 ] = '\0';	

		int wide, tall;
		int xPos, yPos;
		int zOffset = 120;

		vgui::surface()->GetTextSize(m_hFont, wszPlayerText, wide, tall);

		Color c = GameResources()->GetTeamColor(pPl->GetTeamNumber());
		c = Color(c.r(), c.g(), c.b(), 255);

		surface()->DrawSetTextFont(m_hFont);
		surface()->DrawSetTextColor(c);

		surface()->DrawSetTextPos(20, 20 + i * tall);
		surface()->DrawPrintText(wszPlayerText, wcslen(wszPlayerText));
	}
}
