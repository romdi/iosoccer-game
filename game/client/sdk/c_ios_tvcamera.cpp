#include "cbase.h"
#include "c_ios_tvcamera.h"
#include "c_ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "c_ios_replaymanager.h"

enum cam_type_t { CAM_SIDELINE, CAM_BEHIND_GOAL };

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

const int posCount = 5;

void C_TVCamera::GetPositionAndAngle(Vector &pos, QAngle &ang)
{
	C_BaseEntity *pTarget;
	cam_type_t camType;
	bool atMinGoalPos;

	if (GetReplayManager() && GetReplayManager()->IsReplaying())
	{
		pTarget = GetReplayBall();
		camType = GetReplayManager()->m_nReplayRunIndex == 0 ? CAM_SIDELINE : CAM_BEHIND_GOAL;
		atMinGoalPos = GetReplayManager()->m_bAtMinGoalPos;
	}
	else
	{
		pTarget = CBasePlayer::GetLocalPlayer()->GetObserverTarget();
		if (!pTarget)
			pTarget = GetBall();

		camType = CAM_SIDELINE;
		atMinGoalPos = true;
	}

	if (!pTarget)
		return;

	m_BallPos.AddToTail(pTarget->GetLocalOrigin());

	while (m_BallPos.Count() > posCount)
		m_BallPos.Remove(0);

	Vector ballPos;
	if (m_BallPos.Count() < posCount)
	{
		ballPos = m_BallPos[0];
	}
	else
	{
		ballPos = vec3_origin;
		for (int i = 0; i < posCount; i++)
		{
			ballPos += m_BallPos[i] / posCount;
		}
	}

	switch (camType)
	{
	case CAM_SIDELINE:
		{
			Vector newPos = Vector(SDKGameRules()->m_vFieldMin.GetX() - 500, ballPos.y, SDKGameRules()->m_vKickOff.GetZ() + 800);
			newPos.y = clamp(newPos.y, SDKGameRules()->m_vFieldMin.GetY() + 900, SDKGameRules()->m_vFieldMax.GetY() - 900);
			Vector newDir = ballPos - newPos;
			newDir.NormalizeInPlace();
			newPos = ballPos - newDir * 900;
			pos = newPos;
			VectorAngles(newDir, ang);
			break;
		}
	case CAM_BEHIND_GOAL:
		{
			float yPos = atMinGoalPos ? SDKGameRules()->m_vFieldMin.GetY() : SDKGameRules()->m_vFieldMax.GetY();
			Vector goalCenter = Vector((SDKGameRules()->m_vFieldMin.GetX() + SDKGameRules()->m_vFieldMax.GetX()) / 2.0f, yPos, SDKGameRules()->m_vKickOff.GetZ());
			Vector newPos = goalCenter + Vector(0, 200 * (atMinGoalPos ? -1 : 1), 250);
			Vector newDir = ballPos - newPos;
			newDir.NormalizeInPlace();
			//newPos = ballPos - newDir * 400;
			pos = newPos;
			VectorAngles(newDir, ang);
			break;
		}
	}
}