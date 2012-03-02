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
	Vector ballPos = GetBall()->GetLocalOrigin();
	Vector newPos = Vector(SDKGameRules()->m_vFieldMin.GetX() - 500, ballPos.y, SDKGameRules()->m_vKickOff.GetZ() + 700);
	newPos.y = clamp(newPos.y, SDKGameRules()->m_vFieldMin.GetY() + 800, SDKGameRules()->m_vFieldMax.GetY() - 800);
	Vector newDir = ballPos - newPos;
	newDir.NormalizeInPlace();
	newPos = ballPos - newDir * 800;

	pos = newPos;
	VectorAngles(newDir, ang);
	//ang[PITCH] = min(89, ang[PITCH] + 50);
}