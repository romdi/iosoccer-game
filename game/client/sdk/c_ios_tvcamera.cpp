#include "cbase.h"
#include "c_ios_tvcamera.h"
#include "c_ball.h"
#include "sdk_gamerules.h"
#include "convar.h"
#include "c_ios_replaymanager.h"
#include "c_team.h"

enum cam_type_t { CAM_SIDELINE, CAM_FIXED_SIDELINE, CAM_BEHIND_GOAL, CAM_TOPDOWN, CAM_GOAL_CORNER, CAM_FLY_FOLLOW, CAM_GOAL_LINE };

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
	m_flLastPossessionChange = 0;
	m_nLastPossessingTeam = TEAM_UNASSIGNED;
}

const int posCount = 5;

void C_TVCamera::GetPositionAndAngle(Vector &pos, QAngle &ang)
{
	if (!GetBall())
		return;

	C_BaseEntity *pTarget;
	cam_type_t camType;
	bool atMinGoalPos;
	Vector targetPos;

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
		case 6: camType = CAM_FIXED_SIDELINE; break;
		default: camType = CAM_SIDELINE; break;
		}

		atMinGoalPos = GetReplayManager()->m_bAtMinGoalPos;
		targetPos = pTarget->GetLocalOrigin();
	}
	else
	{
		pTarget = CBasePlayer::GetLocalPlayer()->GetObserverTarget();
		if (!pTarget)
			pTarget = GetBall();

		camType = CAM_SIDELINE;
		atMinGoalPos = pTarget->GetLocalOrigin().y < SDKGameRules()->m_vKickOff.GetY();
		targetPos = pTarget->GetLocalOrigin();

		if (GetBall()->m_nCurrentTeam != TEAM_UNASSIGNED)
		{
			if (m_nLastPossessingTeam == TEAM_UNASSIGNED)
			{
				m_nLastPossessingTeam = GetBall()->m_nCurrentTeam;
				m_flLastPossessionChange = gpGlobals->curtime;
				m_flPossCoeff = 0;
				m_flOldPossCoeff = 0;
			}
			else
			{
				if (GetBall()->m_nCurrentTeam != m_nLastPossessingTeam)
				{
					m_nLastPossessingTeam = GetBall()->m_nCurrentTeam;
					m_flLastPossessionChange = gpGlobals->curtime;
					m_flOldPossCoeff = m_flPossCoeff;
				}

				float timeFrac = min(1.0f, (gpGlobals->curtime - m_flLastPossessionChange) / mp_tvcam_offset_forward_time.GetFloat());
				float frac = pow(timeFrac, 2) * (3 - 2 * timeFrac); 
				m_flPossCoeff = Lerp(frac, m_flOldPossCoeff, (float)GetGlobalTeam(GetBall()->m_nCurrentTeam)->m_nForward);
			}

			if (camType == CAM_SIDELINE)
			{
				targetPos.y += m_flPossCoeff * mp_tvcam_offset_forward.GetInt();
			}
		}
	}

	targetPos.x = clamp(targetPos.x, SDKGameRules()->m_vFieldMin.GetX() + mp_tvcam_border_south.GetInt(), SDKGameRules()->m_vFieldMax.GetX() - mp_tvcam_border_north.GetInt());
	targetPos.y = clamp(targetPos.y, SDKGameRules()->m_vFieldMin.GetY() + mp_tvcam_border_west.GetInt(), SDKGameRules()->m_vFieldMax.GetY() - mp_tvcam_border_east.GetInt());
	targetPos.z = SDKGameRules()->m_vKickOff.GetZ();

	if (m_vOldTargetPos == vec3_invalid)
		m_vOldTargetPos = targetPos;
	else
	{
		Vector changeDir = targetPos - m_vOldTargetPos;
		float changeDist = changeDir.Length();
		changeDir.NormalizeInPlace();
		targetPos = m_vOldTargetPos + changeDir * min(changeDist, pow(changeDist * mp_tvcam_speed_coeff.GetFloat(), mp_tvcam_speed_exponent.GetFloat()) * gpGlobals->frametime);
	}

	switch (camType)
	{
	case CAM_SIDELINE:
		{
			if (SDKGameRules() && SDKGameRules()->IsIntermissionState() && GetReplayManager() && !GetReplayManager()->IsReplaying())
			{
				//float xLength = SDKGameRules()->m_vFieldMax.GetX() - SDKGameRules()->m_vFieldMin.GetX();
				//float yLength = SDKGameRules()->m_vFieldMax.GetY() - SDKGameRules()->m_vFieldMin.GetY();

				float zPos = 450;
				Vector points[4];
				points[0] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vFieldMin.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[1] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vFieldMax.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[2] = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vFieldMax.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				points[3] = Vector(SDKGameRules()->m_vFieldMax.GetX(), SDKGameRules()->m_vFieldMin.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPos);
				float totalLength = (points[1] - points[0]).Length() + (points[2] - points[1]).Length() + (points[3] - points[2]).Length() + (points[0] - points[3]).Length();

				Vector newPoints[4];

				const float maxDuration = 180.0f;
				float timePassed = fmodf(gpGlobals->curtime, maxDuration);
				
				float lengthSum = 0;
				int offset = 0;
				float length = 0;

				do
				{
					for (int i = 0; i < 4; i++)
						newPoints[i] = points[(i + offset) % 4];

					length = (newPoints[2] - newPoints[1]).Length();
					lengthSum += length;
					offset += 1;
				} while (timePassed > (lengthSum / totalLength * maxDuration));

				float maxStepTime = length / totalLength * maxDuration;

				float frac = 1 - ((lengthSum / totalLength * maxDuration) - timePassed) / maxStepTime;

				Vector output;
				Catmull_Rom_Spline(newPoints[0], newPoints[1], newPoints[2], newPoints[3], frac, output);

/*				float targetDist = (newPoints[2] - newPoints[1]).Length() * frac;
				float epsilon = 10.0f;
				float oldDiff = 0;
				float diff = 0;
				float change = 0.001f;
				bool add = true;

				do
				{
					frac = clamp(frac += (add ? change : -change), 0.0f, 1.0f);

					Catmull_Rom_Spline(newPoints[0], newPoints[1], newPoints[2], newPoints[3], frac, output);
					oldDiff = diff;
					diff = abs((output - newPoints[1]).Length() - targetDist);
					if (diff >= oldDiff)
						add = !add;
				} while (diff > epsilon);*/ 

				pos = output;
				VectorAngles(SDKGameRules()->m_vKickOff - output, ang);
			}
			else if (SDKGameRules() && !SDKGameRules()->IsIntermissionState() && gpGlobals->curtime <= SDKGameRules()->m_flStateEnterTime + 6)
			{
				Vector points[4];
				float zPosStart = 450;
				float zPosEnd = 50;
				points[0] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY() + 10000, SDKGameRules()->m_vKickOff.GetZ() + zPosStart);
				points[1] = Vector(SDKGameRules()->m_vFieldMin.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + zPosStart);
				points[2] = Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY() - 300, SDKGameRules()->m_vKickOff.GetZ() + zPosEnd);
				points[3] = Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY() + 10000, SDKGameRules()->m_vKickOff.GetZ() + zPosEnd);
				float frac = min(1.0f, (gpGlobals->curtime - SDKGameRules()->m_flStateEnterTime) / 3.0f);
				Vector output;
				Catmull_Rom_Spline(points[0], points[1], points[2], points[3], frac, output);
				pos = output;
				VectorAngles(Vector(SDKGameRules()->m_vKickOff.GetX(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + 100) - output, ang);
			}
			else
			{		
				Vector newPos = Vector(targetPos.x - mp_tvcam_sideline_offset_north.GetInt(), targetPos.y, targetPos.z + mp_tvcam_sideline_offset_height.GetInt());
				Vector newDir = targetPos - newPos;
				newDir.NormalizeInPlace();
				newPos.x -= mp_tvcam_offset_north.GetInt();
				pos = newPos;
				VectorAngles(newDir, ang);
			}
		}
		break;
	case CAM_FIXED_SIDELINE:
		{
			Vector newPos = Vector(SDKGameRules()->m_vKickOff.GetX() - mp_tvcam_sideline_offset_north.GetInt(), SDKGameRules()->m_vKickOff.GetY(), SDKGameRules()->m_vKickOff.GetZ() + mp_tvcam_sideline_offset_height.GetInt());
			pos = Lerp(mp_tvcam_sideline_zoomcoeff.GetFloat(), newPos, targetPos);
			VectorAngles(Vector(targetPos.x -  mp_tvcam_offset_north.GetInt(), targetPos.y, targetPos.z) - newPos, ang);
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