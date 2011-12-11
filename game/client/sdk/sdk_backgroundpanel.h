//======== Copyright © 1996-2008, Valve Corporation, All rights reserved. =========//
//
// Purpose: 
//
// $NoKeywords: $
//=================================================================================//

#ifndef SDK_BACKHROUNDPANEL_H
#define SDK_BACKHROUNDPANEL_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawRoundedBackground( Color bgColor, int wide, int tall, int xOrigin = 0, int yOrigin = 0 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawRoundedBorder( Color borderColor, int wide, int tall, int xOrigin = 0, int yOrigin = 0 );

//-----------------------------------------------------------------------------
#endif // SDK_BACKHROUNDPANEL_H
