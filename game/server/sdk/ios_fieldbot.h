#ifndef IOS_FIELDBOT_H
#define IOS_FIELDBOT_H
#ifdef _WIN32
#pragma once
#endif

#include "ios_bot.h"

class CFieldBot : public CBot
{
	void BotThink();
	void BotFetchAndPass();
	void BotRunToBall();
	void BotShootBall();
};

#endif