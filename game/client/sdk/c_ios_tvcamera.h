#ifndef C_IOS_TVCAMERA_H
#define C_IOS_TVCAMERA_H

#include "cbase.h"

class C_TVCamera
{
private:
	static C_TVCamera *m_pInstance;
	C_TVCamera();

	float m_flNextUpdate;
	Vector m_vOldPos;
	Vector m_vPos;
	Vector m_vNewPos;
	Vector m_vOldDir;
	Vector m_vDir;
	Vector m_vNewDir;
	Vector m_vOldBallPos;
	float m_flLerpTime;
	CUtlVector<Vector> m_BallPos;

public:
	static C_TVCamera *GetInstance();
	void GetPositionAndAngle(Vector &pos, QAngle &ang);
};

#endif