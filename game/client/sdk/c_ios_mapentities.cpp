#include "cbase.h"

class C_BallShield : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_BallShield, C_BaseEntity ); // generic entity class macro
	DECLARE_CLIENTCLASS(); // this is a client representation of a server class 

	bool ShouldCollide( int collisionGroup, int contentsMask ) const;
};
 
IMPLEMENT_CLIENTCLASS_DT( C_BallShield, DT_BallShield, CBallShield )
END_RECV_TABLE()

bool C_BallShield::ShouldCollide( int collisionGroup, int contentsMask ) const
{

	// Rebel owned projectiles (grenades etc) and players will collide.
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup == COLLISION_GROUP_PROJECTILE )
	{

		if ( ( contentsMask & CONTENTS_TEAM1 ) )
			return true;
		else
			return false;
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}