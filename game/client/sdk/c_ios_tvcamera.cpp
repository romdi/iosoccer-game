#include "cbase.h"
#include "c_ios_tvcamera.h"
#include "c_ball.h"
#include "sdk_gamerules.h"

C_TVCamera *C_TVCamera::m_pInstance = NULL;

C_TVCamera *C_TVCamera::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new C_TVCamera();

	return m_pInstance;
}

C_TVCamera::C_TVCamera()
{
	m_flNextUpdate = gpGlobals->curtime;
	m_vPos = vec3_invalid;
	m_vDir = vec3_invalid;
	m_vOldBallPos = vec3_invalid;
	m_flLerpTime = 0;
}

void C_TVCamera::GetPositionAndAngle(Vector &pos, QAngle &ang)
{
	C_Ball *pBall = GetBall();

	if (pBall->GetLocalOrigin().DistTo(m_vOldBallPos) > 200)
	{
		m_vNewPos = Vector(SDKGameRules()->m_vFieldMin[0] - 400, (SDKGameRules()->m_vFieldMin[1] + SDKGameRules()->m_vFieldMax[1]) / 2, SDKGameRules()->m_vFieldMin[2] + 400);
		m_vNewDir = pBall->GetLocalOrigin() - m_vNewPos;
		m_vNewDir.NormalizeInPlace();
		m_vNewPos = pBall->GetLocalOrigin() - m_vNewDir * 750;
		m_vOldBallPos = pBall->GetLocalOrigin();
		m_flLerpTime = 0;

		if (m_vDir == vec3_invalid)
		{
			m_flLerpTime = 1;
			m_vPos = m_vNewPos;
			m_vDir = m_vNewDir;
		}

		m_vOldPos = m_vPos;
		m_vOldDir = m_vDir;
	}

	m_flLerpTime += gpGlobals->frametime;
	m_vPos = m_vOldPos + (m_vNewPos - m_vOldPos) * min(1, m_flLerpTime);
	m_vDir = m_vOldDir + (m_vNewDir - m_vOldDir) * min(1, m_flLerpTime);

	pos = m_vPos;
	VectorAngles(m_vDir, ang);
}