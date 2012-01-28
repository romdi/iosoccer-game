/********************************************************************
   KEEPER
 
	Date:			07 July 2002 (HL1). Jan 2008 (HL2)
	Author:			Mark Gornall
	Description:	bot goal keeper

********************************************************************/
 
#ifndef IOS_KEEPERBOT_H
#define IOS_KEEPERBOT_H
#ifdef _WIN32
#pragma once
#endif

#include "ios_bot.h"

// This is our bot class.
class CKeeperBot : public CBot
{
public:
	void BotThink();
	void BotCenter();
	void BotAdjustPos();
};

#endif