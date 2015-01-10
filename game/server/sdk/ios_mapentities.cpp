#include "cbase.h"
#include "triggers.h"
#include "sdk_gamerules.h"
#include "sdk_player.h"
#include "ios_mapentities.h"
#include "team.h"
#include "player_ball.h"
#include "match_ball.h"

class CIOSTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS(CIOSTrigger, CBaseTrigger);
	DECLARE_DATADESC();

	void Spawn()
	{
		BaseClass::Spawn();
		InitTrigger();
	};

	void StartTouch( CBaseEntity *pOther )
	{
	};

	void EndTouch(CBaseEntity *pOther)
	{
	};
};

BEGIN_DATADESC( CIOSTrigger )
END_DATADESC()


class CTriggerField : public CIOSTrigger
{
public:
	DECLARE_CLASS( CTriggerField, CIOSTrigger );
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerField )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_field, CTriggerField );


class CTriggerPenaltyBox : public CIOSTrigger
{
public:
	DECLARE_CLASS( CTriggerPenaltyBox, CIOSTrigger );
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerPenaltyBox )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_penaltybox, CTriggerPenaltyBox );


class CTriggerSixYardBox : public CIOSTrigger
{
public:
	DECLARE_CLASS( CTriggerSixYardBox, CIOSTrigger );
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerSixYardBox )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_sixyardbox, CTriggerSixYardBox );


class CTriggerGoal : public CIOSTrigger
{
public:
	DECLARE_CLASS( CTriggerGoal, CIOSTrigger );
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerGoal )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_goal, CTriggerGoal );


class CAutoTransparentProp : public CDynamicProp
{
	DECLARE_CLASS( CAutoTransparentProp, CDynamicProp );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CAutoTransparentProp )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CAutoTransparentProp, DT_AutoTransparentProp)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(prop_autotransparent, CAutoTransparentProp);	//IOS


LINK_ENTITY_TO_CLASS(info_stadium, CInfoStadium);

BEGIN_DATADESC(CInfoStadium)
	DEFINE_KEYFIELD(m_bHasWalledField, FIELD_BOOLEAN, "HasWalledField"),
	DEFINE_KEYFIELD(m_bIsTrainingMap, FIELD_BOOLEAN, "IsTrainingMap")
END_DATADESC()