//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IN_MAIN_H
#define IN_MAIN_H
#ifdef _WIN32
#pragma once
#endif


#include "kbutton.h"


extern kbutton_t in_commandermousemove;
extern kbutton_t in_ducktoggle;

extern float GetPitchup();
extern float GetPitchdown();

#endif // IN_MAIN_H
