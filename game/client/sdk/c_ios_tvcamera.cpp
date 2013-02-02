#include "cbase.h"
#include "c_ios_tvcamera.h"
#include "c_ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "c_ios_replaymanager.h"
#include "c_team.h"

enum cam_type_t { CAM_SIDELINE, CAM_BEHIND_GOAL, CAM_TOPDOWN, CAM_GOAL_CORNER, CAM_FLY_FOLLOW, CAM_GOAL_LINE };

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
	m_vOldTargetPos = vec3_invalid;
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
		switch (GetReplayManager()->m_nReplayRunIndex)
		{
		case 0: camType = CAM_SIDELINE; break;
		case 1: camType = CAM_FLY_FOLLOW; break;
		case 2: camType = CAM_GOAL_LINE; break;
		case 3: camType = CAM_GOAL_CORNER; break;
		case 4: camType = CAM_TOPDOWN; break;
		case 5: camType = CAM_BEHIND_GOAL; break;
		default: camType = CAM_SIDELINE; break;
		}
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

	Vector targetPos = pTarget->GetLocalOrigin();

	if (camType == CAM_SIDELINE)
	{
		targetPos.x -= mp_tvcam_offset_north.GetInt();

		if (GetBall() && (GetBall()->m_nCurrentTeam == TEAM_A || GetBall()->m_nCurrentTeam == TEAM_B))
			targetPos.y += GetGlobalTeam(GetBall()->m_nCurrentTeam)->m_nForward * mp_tvcam_offset_forward.GetInt();
	}

	targetPos.x = clamp(targetPos.x, SDKGameRules()->m_vFieldMin.GetX() + mp_tvcam_border_south.GetInt(), SDKGameRules()->m_vFieldMax.GetX() - mp_tvcam_border_north.GetInt());
	targetPos.y = clamp(targetPos.y, SDKGameRules()->m_vFieldMin.GetY() + mp_tvcam_border_west.GetInt(), SDKGameRules()->m_vFieldMax.GetY() - mp_tvcam_border_east.GetInt());
	targetPos.z = SDKGameRules()->m_vKickOff.GetZ();
	Vector targetDir = pTarget->GetLocalVelocity();

	if (m_vOldTargetPos == vec3_invalid)
		m_vOldTargetPos = targetPos;
	else
	{
		Vector changeDir = targetPos - m_vOldTargetPos;
		float length = changeDir.Length();
		changeDir.NormalizeInPlace();
		targetPos = m_vOldTargetPos + changeDir * min(length, pow(length / mp_tvcam_speed_coeff.GetFloat(), mp_tvcam_speed_exponent.GetFloat()));
	}

	switch (camType)
	{
	case CAM_SIDELINE:
		{
			Vector newPos = Vector(SDKGameRules()->m_vFieldMin.GetX() - 800, targetPos.y, SDKGameRules()->m_vKickOff.GetZ() + 1400);
			newPos.y = clamp(newPos.y, SDKGameRules()->m_vFieldMin.GetY() + 1300, SDKGameRules()->m_vFieldMax.GetY() - 1300);
			Vector newDir = targetPos - newPos;
			newDir.NormalizeInPlace();
			newPos = targetPos - newDir * 750;
			pos = newPos;
			VectorAngles(newDir, ang);
		}
		break;
	case CAM_BEHIND_GOAL:
		{
			float yPos = atMinGoalPos ? SDKGameRules()->m_vFieldMin.GetY() : SDKGameRules()->m_vFieldMax.GetY();
			Vector goalCenter = Vector((SDKGameRules()->m_vFieldMin.GetX() + SDKGameRules()->m_vFieldMax.GetX()) / 2.0f, yPos, SDKGameRules()->m_vKickOff.GetZ());
			Vector newPos = goalCenter + Vector(0, 300 * (atMinGoalPos ? -1 : 1), 300);
			Vector newDir = targetPos - newPos;
			newDir.NormalizeInPlace();
			pos = newPos;
			VectorAngles(newDir, ang);
		}
		break;
	case CAM_TOPDOWN:
		{
			Vector newPos = Vector(targetPos.x, targetPos.y, SDKGameRules()->m_vKickOff.GetZ() + 600) + (atMinGoalPos ? 1 : -1) * 600;
			Vector newDir = Vector(0, (atMinGoalPos ? -1 : 1) * 0.0000001, -1);
			newDir.NormalizeInPlace();
			pos = newPos;
			VectorAngles(newDir, ang);
		}
		break;
	case CAM_FLY_FOLLOW:
		{
			Vector newPos = Vector(targetPos.x, targetPos.y + (atMinGoalPos ? 1 : -1) * 500, SDKGameRules()->m_vKickOff.GetZ() + 225);
			Vector newDir = Vector(0, (atMinGoalPos ? -1 : 1) * 1.75f, -1);
			newDir.NormalizeInPlace();
			pos = newPos;
			VectorAngles(newDir, ang);
		}
		break;
	case CAM_GOAL_CORNER:
		{
			// setpos -139.917419 2082.051514 -140.906738;setang 13.860826 -57.723770 0.000000
			// setpos 139.729691 -2086.090820 -149.303741;setang 13.397299 125.494232 0.000000
			Vector newPos = Vector(-139.917419, 2082.051514, -140.906738);
			QAngle newAng = QAngle(13.860826, -57.723770, 0.000000);
			if (atMinGoalPos)
			{
				newPos.x = SDKGameRules()->m_vKickOff.GetX() + (SDKGameRules()->m_vKickOff.GetX() - newPos.x);
				newPos.y = SDKGameRules()->m_vKickOff.GetY() + (SDKGameRules()->m_vKickOff.GetY() - newPos.y);
				newAng[YAW] += 180;
			}
			pos = newPos;
			ang = newAng;
		}
		break;
	case CAM_GOAL_LINE:
		{
			Vector center = Vector(SDKGameRules()->m_vKickOff.GetX(), (atMinGoalPos ? SDKGameRules()->m_vFieldMin.GetY() + 5 : SDKGameRules()->m_vFieldMax.GetY() - 5), SDKGameRules()->m_vKickOff.GetZ());
			Vector newPos = center;
			newPos.x -= 350;
			newPos.z += 200;
			QAngle newAng;
			VectorAngles(center - newPos, newAng);
			pos = newPos;
			ang = newAng;
		}
		break;
	}

	m_vOldTargetPos = targetPos;
}