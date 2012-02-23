#include "cbase.h"
#include "c_ios_tvcamera.h"
#include "c_ball.h"
#include "sdk_gamerules.h"
#include "convar.h"

ConVar cl_tvcam_angle("cl_tvcam_angle", "10", FCVAR_ARCHIVE);
ConVar cl_tvcam_dist("cl_tvcam_dist", "10", FCVAR_ARCHIVE);
ConVar cl_tvcam_posspeed("cl_tvcam_posspeed", "10", FCVAR_ARCHIVE);
ConVar cl_tvcam_angspeed("cl_tvcam_angspeed", "10", FCVAR_ARCHIVE);

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
	m_vPos = m_vOldPos = m_vNewPos = vec3_invalid;
	m_vDir = m_vOldDir = m_vOldDir = vec3_invalid;
	m_vOldBallPos = vec3_invalid;
	m_flLerpTime = 0;
}

void C_TVCamera::GetPositionAndAngle(Vector &pos, QAngle &ang)
{
	C_Ball *pBall = GetBall();

	Vector newPos = Vector(SDKGameRules()->m_vFieldMin[0] - 400, (SDKGameRules()->m_vFieldMin[1] + SDKGameRules()->m_vFieldMax[1]) / 2, SDKGameRules()->m_vFieldMin[2] + 400);
	Vector newDir = pBall->GetLocalOrigin() - newPos;
	newDir.NormalizeInPlace();
	newPos = pBall->GetLocalOrigin() - newDir * 750;

	if (m_vDir == vec3_invalid)
	{
		m_flLerpTime = 1;
		m_vPos = m_vOldPos = m_vNewPos = newPos;
		m_vDir = m_vOldDir = m_vNewDir = newDir;
	}

	if (RAD2DEG(acos(m_vDir.Dot(newDir))) <= cl_tvcam_angle.GetFloat() && (m_vPos - newPos).Length() <= cl_tvcam_dist.GetFloat())
	{
	}
	else
	{
		Vector camDir = newPos - m_vPos;
		float dist = min(camDir.Length(), camDir.Length() * cl_tvcam_posspeed.GetFloat() * gpGlobals->frametime);
		camDir.NormalizeInPlace();
		m_vPos += camDir * dist;

		camDir = newDir - m_vDir;
		dist = min(camDir.Length(), camDir.Length() * cl_tvcam_angspeed.GetFloat() * gpGlobals->frametime);
		camDir.NormalizeInPlace();
		m_vDir += camDir * dist;
	}

	pos = m_vPos;
	VectorAngles(m_vDir, ang);
}