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
		m_bAllowGrassFieldMaterial = true;
		m_bAllowArtificialFieldMaterial = false;
		m_bAllowStreetFieldMaterial = false;
		m_bAllowSandFieldMaterial = false;
		m_bAllowMudFieldMaterial = false;
	}

public:

	bool m_bHasWalledField;
	bool m_bIsTrainingMap;
	bool m_bAllowGrassFieldMaterial;
	bool m_bAllowArtificialFieldMaterial;
	bool m_bAllowStreetFieldMaterial;
	bool m_bAllowSandFieldMaterial;
	bool m_bAllowMudFieldMaterial;
};

#endif