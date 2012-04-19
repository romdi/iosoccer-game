#include "cbase.h"
#include "c_ball.h"
#include "c_sdk_player.h"
#include "fx_line.h"
#include "sdk_gamerules.h"
#include "c_team.h"

LINK_ENTITY_TO_CLASS(football, C_Ball);

IMPLEMENT_CLIENTCLASS_DT( C_Ball, DT_Ball, CBall )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	RecvPropFloat(RECVINFO(m_flOffsideLineBallPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineOffsidePlayerPosY)),
	RecvPropFloat(RECVINFO(m_flOffsideLineLastOppPlayerPosY)),
	RecvPropInt(RECVINFO(m_bOffsideLinesEnabled)),
END_RECV_TABLE()

C_Ball *g_pBall = NULL;

C_Ball *GetBall()
{
	return g_pBall;
}

C_Ball::C_Ball()
{
	g_pBall = this;
	m_bOffsideLinesEnabled = false;
	PrecacheMaterial("pitch/offside_line");
	m_pOffsideLineMaterial = materials->FindMaterial( "pitch/offside_line", TEXTURE_GROUP_CLIENT_EFFECTS );
}

void C_Ball::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	return;
}

void SetupVec(Vector& v, int dim1, int dim2, int fixedDim, float dim1Val, float dim2Val, float fixedDimVal)
{
	v[dim1] = dim1Val;
	v[dim2] = dim2Val;
	v[fixedDim] = fixedDimVal;
}

void DrawBoxSide(int dim1, int dim2, int fixedDim, float minX, float minY, float maxX, float maxY, float fixedDimVal, bool bFlip, Vector &color)
{
	Vector v;

	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder builder;
	builder.Begin(pMesh, MATERIAL_TRIANGLE_STRIP, 2);

	SetupVec(v, dim1, dim2, fixedDim, minX, maxY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, bFlip ? maxX : minX, bFlip ? maxY : minY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, bFlip ? minX : maxX, bFlip ? minY : maxY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	SetupVec(v, dim1, dim2, fixedDim, maxX, minY, fixedDimVal);
	builder.Position3fv(v.Base());
	builder.Color3fv(color.Base());
	builder.AdvanceVertex();

	builder.End();
	pMesh->Draw();
}

void DrawOffsideLine(IMaterial *pMaterial, float posY, Vector &color)
{
	CMatRenderContextPtr pRenderContext( materials );
	// Draw it.
	pRenderContext->Bind( pMaterial );

	Vector mins = Vector(SDKGameRules()->m_vFieldMin.GetX() - 500, posY - 2, SDKGameRules()->m_vKickOff.GetZ());
	Vector maxs = Vector(SDKGameRules()->m_vFieldMax.GetX() + 500, posY + 2, SDKGameRules()->m_vKickOff.GetZ() + 4);

	DrawBoxSide(1, 2, 0, mins[1], mins[2], maxs[1], maxs[2], mins[0], false, color);
	DrawBoxSide(1, 2, 0, mins[1], mins[2], maxs[1], maxs[2], maxs[0], true, color);

	DrawBoxSide(0, 2, 1, mins[0], mins[2], maxs[0], maxs[2], mins[1], true, color);
	DrawBoxSide(0, 2, 1, mins[0], mins[2], maxs[0], maxs[2], maxs[1], false, color);

	DrawBoxSide(0, 1, 2, mins[0], mins[1], maxs[0], maxs[1], mins[2], false, color);
	DrawBoxSide(0, 1, 2, mins[0], mins[1], maxs[0], maxs[1], maxs[2], true, color);
}

//void DrawHomeTeamCrest()
//{
//	Vector vFinalRight = Vector(1, 0, 0);
//	Vector vFinalForward = Vector(0, 1, 0);
//	float m_flSpellPreviewRadius = 250;
//	Vector vFinalOrigin = SDKGameRules()->m_vKickOff;
//	vFinalOrigin.y += 3 * m_flSpellPreviewRadius;
//
//	CMatRenderContextPtr pRenderContext( materials );
//	IMaterial *pPreviewMaterial = materials->FindMaterial( "vgui/hometeamcrest", TEXTURE_GROUP_CLIENT_EFFECTS );
//	//IMaterial *pPreviewMaterial = materials->FindMaterial( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );
//	pRenderContext->Bind( pPreviewMaterial );
//	IMesh *pMesh = pRenderContext->GetDynamicMesh();
//	CMeshBuilder meshBuilder;
//	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,0,0 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,1,0 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,1,1 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,0,1 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//	meshBuilder.End();
//	pMesh->Draw();
//}
//
//void DrawAwayTeamCrest()
//{
//	Vector vFinalRight = Vector(1, 0, 0);
//	Vector vFinalForward = Vector(0, 1, 0);
//	float m_flSpellPreviewRadius = 250;
//	Vector vFinalOrigin = SDKGameRules()->m_vKickOff;
//	vFinalOrigin.y -= 3 * m_flSpellPreviewRadius;
//
//	CMatRenderContextPtr pRenderContext( materials );
//	IMaterial *pPreviewMaterial = materials->FindMaterial( "vgui/awayteamcrest", TEXTURE_GROUP_CLIENT_EFFECTS );
//	//IMaterial *pPreviewMaterial = materials->FindMaterial( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );
//	pRenderContext->Bind( pPreviewMaterial );
//	IMesh *pMesh = pRenderContext->GetDynamicMesh();
//	CMeshBuilder meshBuilder;
//	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,0,0 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,1,0 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,1,1 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//
//	meshBuilder.Color3f( 1.0, 1.0, 1.0 );
//	meshBuilder.TexCoord2f( 0,0,1 );
//	meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
//	meshBuilder.AdvanceVertex();
//	meshBuilder.End();
//	pMesh->Draw();
//}

int C_Ball::DrawModel(int flags)
{
	if (m_bOffsideLinesEnabled)
	{
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineBallPosY, Vector(0, 0, 1));
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineLastOppPlayerPosY, Vector(1, 1, 0));
		DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLineOffsidePlayerPosY, Vector(1, 0, 0));
		//DrawOffsideLine(m_pOffsideLineMaterial, m_flOffsideLinePlayerY);
	}

	//DrawHomeTeamCrest();
	//DrawAwayTeamCrest();

	//float m_flSpellPreviewRadius = 200;
	//Vector vFinalOrigin = SDKGameRules()->m_vKickOff;
	//Vector vFinalRight = Vector(1, 0, 0);
	//Vector vFinalForward = Vector(0, 1, 0);

	//CMatRenderContextPtr pRenderContext( materials );
	//IMaterial *pPreviewMaterial = materials->FindMaterial( "pitch/offside_line_ball", TEXTURE_GROUP_CLIENT_EFFECTS );
	////IMaterial *pPreviewMaterial = materials->FindMaterial( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );
	//pRenderContext->Bind( pPreviewMaterial );
	//IMesh *pMesh = pRenderContext->GetDynamicMesh();
	//CMeshBuilder meshBuilder;
	//meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	//meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	//meshBuilder.TexCoord2f( 0,0,0 );
	//meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
	//meshBuilder.AdvanceVertex();

	//meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	//meshBuilder.TexCoord2f( 0,1,0 );
	//meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * m_flSpellPreviewRadius)).Base() );
	//meshBuilder.AdvanceVertex();

	//meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	//meshBuilder.TexCoord2f( 0,1,1 );
	//meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
	//meshBuilder.AdvanceVertex();

	//meshBuilder.Color3f( 1.0, 1.0, 1.0 );
	//meshBuilder.TexCoord2f( 0,0,1 );
	//meshBuilder.Position3fv( (vFinalOrigin + (vFinalRight * -m_flSpellPreviewRadius) + (vFinalForward * -m_flSpellPreviewRadius)).Base() );
	//meshBuilder.AdvanceVertex();
	//meshBuilder.End();
	//pMesh->Draw();

	return BaseClass::DrawModel(flags);
}