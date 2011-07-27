//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "sdk_hudsprint.h"
#include "hud_macros.h"
#include "c_baseplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include <igameresources.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudSuitPower );

#define SUITPOWER_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudSuitPower::CHudSuitPower( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudStaminaBar" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

   m_nImport = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImport, "vgui/sprintbutton" , true, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Init( void )
{
	m_flSuitPower = SUITPOWER_INIT;
	m_nSuitPowerLow = -1;
	m_iActiveSuitDevices = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudSuitPower::ShouldDraw()
{

	//bool bNeedsDraw = false;

	C_BasePlayer *pPlayer = (C_BasePlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if (pPlayer->GetTeamNumber() < TEAM_A)
		return FALSE;

	return TRUE;

	//// needs draw if suit power changed or animation in progress
	//bNeedsDraw = ( ( pPlayer->m_HL2Local.m_flSuitPower != m_flSuitPower ) || ( m_AuxPowerColor[3] > 0 ) );

	//return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::OnThink( void )
{
	//float flCurrentPower = 0;
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	IGameResources *gr = GameResources();
	float fSprint = gr->GetSprint(pPlayer->index);


	//NB - THESE ANIMS GIVE THE BACKGOUND IT'S COLOUR! - see scripts/HudAnimations.txt
	//if ( fSprint >= 60.0f && m_fPrevSprint < 60.0f )
	//{
	//	// we've reached max power
	//	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	//}
	//else if ( fSprint < 60.0f && (m_fPrevSprint >= 60.0f || m_fPrevSprint == SUITPOWER_INIT) )
	//{
	//	// we've lost power
	//	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	//}

	m_fPrevSprint = fSprint;

	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerOneItemActive");

	/*
	flCurrentPower = pPlayer->m_HL2Local.m_flSuitPower;

	// Only update if we've changed suit power
	if ( flCurrentPower == m_flSuitPower )
		return;

	if ( flCurrentPower >= 100.0f && m_flSuitPower < 100.0f )
	{
		// we've reached max power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	}
	else if ( flCurrentPower < 100.0f && (m_flSuitPower >= 100.0f || m_flSuitPower == SUITPOWER_INIT) )
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	}

	bool flashlightActive = pPlayer->IsFlashlightActive();
	bool sprintActive = pPlayer->IsSprinting();
	bool breatherActive = pPlayer->IsBreatherActive();
	int activeDevices = (int)flashlightActive + (int)sprintActive + (int)breatherActive;

	if (activeDevices != m_iActiveSuitDevices)
	{
		m_iActiveSuitDevices = activeDevices;

		switch ( m_iActiveSuitDevices )
		{
		default:
		case 3:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerThreeItemsActive");
			break;
		case 2:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerTwoItemsActive");
			break;
		case 1:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerOneItemActive");
			break;
		case 0:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNoItemsActive");
			break;
		}
	}

	m_flSuitPower = flCurrentPower;
	*/
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudSuitPower::Paint()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;


	IGameResources *gr = GameResources();
	float fSprint = gr->GetSprint(pPlayer->index);
	float fSprintScaled = fSprint * 1.66666667f;		//scale 60 sprint to 100.
	if (fSprintScaled > 100.0f) fSprintScaled = 100.0f;

	//go red when can't powershot - value to be determined...
	bool bLowPower = false;
	if (fSprint < 31.0f)
		bLowPower = true;

	//draw red if not enough for a powershot
	if (bLowPower)
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
	else
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");

	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (fSprintScaled * 1.0f/100.0f) + 0.5f );

	//doing this stops the orange rounded rect from drawing and lets the glass image come through
	//need to sort out hudanimation.txt for red glow at full.
	SetBgColor( Color( 0, 0, 0, 0 ) );	

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );			//colour from HudAnimations.txt
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_AuxPowerColor[0], m_AuxPowerColor[1], m_AuxPowerColor[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	wchar_t *tempString;
	tempString = vgui::localize()->Find("#IOS_Stamina");
	if (tempString)
		surface()->DrawPrintText(tempString, wcslen(tempString));
	else
		surface()->DrawPrintText(L"STAMINA", wcslen(L"STAMINA"));


	//draw back image
	SetPaintBorderEnabled(false);
	//surface()->DrawSetColor( Color(255, 255, 255, 128) );		//let col get set by text - that why it goes red
	surface()->DrawSetTexture( m_nImport );
	int wide, tall;
	GetSize(wide, tall);
	surface()->DrawTexturedRect( 0, 0, wide, tall );

	/*
	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flSuitPower * 1.0f/100.0f) + 0.5f );

	// see if we've changed power state
	int lowPower = 0;
	if (enabledChunks <= (chunkCount / 4))
	{
		lowPower = 1;
	}
	if (m_nSuitPowerLow != lowPower)
	{
		if (m_iActiveSuitDevices || m_flSuitPower < 100.0f)
		{
			if (lowPower)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");
			}
			m_nSuitPowerLow = lowPower;
		}
	}

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_AuxPowerColor[0], m_AuxPowerColor[1], m_AuxPowerColor[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);

	wchar_t *tempString = vgui::localize()->Find("#Valve_Hud_AUX_POWER");

	if (tempString)
	{
		surface()->DrawPrintText(tempString, wcslen(tempString));
	}
	else
	{
		surface()->DrawPrintText(L"AUX POWER", wcslen(L"AUX POWER"));
	}

	if ( m_iActiveSuitDevices )
	{
		// draw the additional text
		int ypos = text2_ypos;

		if (pPlayer->IsBreatherActive())
		{
			tempString = vgui::localize()->Find("#Valve_Hud_OXYGEN");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"OXYGEN", wcslen(L"OXYGEN"));
			}
			ypos += text2_gap;
		}

		if (pPlayer->IsFlashlightActive())
		{
			tempString = vgui::localize()->Find("#Valve_Hud_FLASHLIGHT");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"FLASHLIGHT", wcslen(L"FLASHLIGHT"));
			}
			ypos += text2_gap;
		}

		if (pPlayer->IsSprinting())
		{
			tempString = vgui::localize()->Find("#Valve_Hud_SPRINT");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"SPRINT", wcslen(L"SPRINT"));
			}
			ypos += text2_gap;
		}
	}
	*/
}


