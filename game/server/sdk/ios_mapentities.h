#ifndef IOS_MAPENTITIES_H
#define IOS_MAPENTITIES_H

#include "cbase.h"
#include "sdk_player.h"

class CInfoStadium : public CLogicalEntity
{
public:
	DECLARE_CLASS(CInfoStadium, CLogicalEntity);
	DECLARE_DATADESC();

	CInfoStadium()
	{
		m_bHasWalledField = false;
		m_bIsTrainingMap = false;
	}

public:

	bool m_bHasWalledField;
	bool m_bIsTrainingMap;
};

#endif