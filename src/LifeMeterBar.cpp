#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: LifeMeterBar

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "LifeMeterBar.h"
#include "ThemeManager.h"


const int NUM_SECTIONS = 3;
const float SECTION_WIDTH = 1.0f/NUM_SECTIONS;


LifeMeterBar::LifeMeterBar()
{
	m_fLifePercentage = 0.5f;
	m_fTrailingLifePercentage = 0;
	m_fLifeVelocity = 0;
}

void LifeMeterBar::ChangeLife( TapNoteScore score )
{
	switch( score )
	{
	case TNS_PERFECT:	m_fLifePercentage += m_po.m_fLifeAdjustments[m_po.LIFE_PERFECT];	break;
	case TNS_GREAT:		m_fLifePercentage += m_po.m_fLifeAdjustments[m_po.LIFE_GREAT];	break;
	case TNS_GOOD:		m_fLifePercentage += m_po.m_fLifeAdjustments[m_po.LIFE_GOOD];	break;
	case TNS_BOO:		m_fLifePercentage += m_po.m_fLifeAdjustments[m_po.LIFE_BOO];		break;
	case TNS_MISS:		m_fLifePercentage += m_po.m_fLifeAdjustments[m_po.LIFE_MISS];	break;
	}

	m_fLifePercentage = clamp( m_fLifePercentage, 0, 1 );
}

float LifeMeterBar::GetLifePercentage() 
{ 
	return m_fLifePercentage; 
}

void LifeMeterBar::Update( float fDeltaTime )
{
	const float fDelta = m_fLifePercentage - m_fTrailingLifePercentage;

	m_fLifeVelocity += fDelta * fDeltaTime;

	m_fLifeVelocity = clamp( m_fLifeVelocity, -0.30f, 0.30f );

	m_fLifeVelocity *= 1-fDeltaTime*2;

	if( fDelta != 0 )
	{
		int j=9;
	}

	if( fabsf(m_fLifeVelocity) < 0.01f  &&  fabsf(fDelta) < 0.01f )
	{
		m_fTrailingLifePercentage += fDelta/2;
		m_fLifeVelocity = 0;
	}
	else
	{
		m_fTrailingLifePercentage += m_fLifeVelocity * fDeltaTime*4;
		m_fTrailingLifePercentage = clamp( m_fTrailingLifePercentage, 0, 1 );
	}
}


const D3DXCOLOR COLOR_NORMAL_1	= D3DXCOLOR(1,1,1,1);
const D3DXCOLOR COLOR_NORMAL_2	= D3DXCOLOR(0,1,0,1);
const D3DXCOLOR COLOR_FULL_1	= D3DXCOLOR(1,0,0,1);
const D3DXCOLOR COLOR_FULL_2	= D3DXCOLOR(1,1,0,1);

D3DXCOLOR LifeMeterBar::GetColor( float fPercentIntoSection )
{
	float fPercentColor1 = fabsf( fPercentIntoSection*2 - 1 );
	fPercentColor1 *= fPercentColor1 * fPercentColor1;
	if( m_fLifePercentage == 1 )
		return COLOR_FULL_1 * fPercentColor1 + COLOR_FULL_2 * (1-fPercentColor1);
	else
        return COLOR_NORMAL_1 * fPercentColor1 + COLOR_NORMAL_2 * (1-fPercentColor1);
}

void LifeMeterBar::DrawPrimitives()
{

	// make the object in logical units centered at the origin
	LPDIRECT3DVERTEXBUFFER8 pVB = DISPLAY->GetVertexBuffer();
	RAGEVERTEX* v;
	pVB->Lock( 0, 0, (BYTE**)&v, 0 );

	int iNumV = 0;

	float fPercentIntoSection = (GetTickCount()/300.0f)*SECTION_WIDTH;
	fPercentIntoSection -= (int)fPercentIntoSection;
	fPercentIntoSection = 1-fPercentIntoSection;
	fPercentIntoSection -= (int)fPercentIntoSection;
	ASSERT( fPercentIntoSection >= 0  &&  fPercentIntoSection < 1 );
	float fX = - 0.5f; 


	while( fX < m_fTrailingLifePercentage-0.5f )
	{
		//
		// draw middle
		//
		v[iNumV++].p = D3DXVECTOR3( fX, -0.5f, 0 );
		v[iNumV++].p = D3DXVECTOR3( fX,  0.5f, 0 );
		
		iNumV -= 2;
		const D3DXCOLOR color = GetColor( fPercentIntoSection ); 
		v[iNumV++].color = color;
		v[iNumV++].color = color;


		if( fPercentIntoSection < 0.5f )
		{
			const float fPercentToAdd = 0.5f-fPercentIntoSection;
			fPercentIntoSection += fPercentToAdd;
			fX += fPercentToAdd*SECTION_WIDTH;
		}
		else if( fPercentIntoSection < 1.0f )
		{
			const float fPercentToAdd = 1.0f-fPercentIntoSection;
			fPercentIntoSection = 0;
			fX += fPercentToAdd*SECTION_WIDTH;
		}
		else
			ASSERT( false );
	}

	const float fXToSubtract = fX - (m_fTrailingLifePercentage-0.5f);
	fX -= fXToSubtract;
	fPercentIntoSection -= fXToSubtract/SECTION_WIDTH;
	if( fPercentIntoSection < 0 )
		fPercentIntoSection += 1;

	//
	// draw right edge
	//
	v[iNumV++].p = D3DXVECTOR3( fX, -0.5f, 0 );
	v[iNumV++].p = D3DXVECTOR3( fX,  0.5f, 0 );
	
	iNumV -= 2;
	const D3DXCOLOR color = GetColor( fPercentIntoSection ); 
	v[iNumV++].color = color;
	v[iNumV++].color = color; 

	//
	// draw black filler
	//
	v[iNumV++].p = D3DXVECTOR3( fX, -0.5f, 0 );
	v[iNumV++].p = D3DXVECTOR3( fX,  0.5f, 0 );
	v[iNumV++].p = D3DXVECTOR3( 0.5f, -0.5f, 0 );
	v[iNumV++].p = D3DXVECTOR3( 0.5f,  0.5f, 0 );
	
	iNumV -= 4;
	const D3DXCOLOR colorBlack = D3DXCOLOR(0,0,0,1); 
	v[iNumV++].color = colorBlack;
	v[iNumV++].color = colorBlack; 
	v[iNumV++].color = colorBlack;
	v[iNumV++].color = colorBlack; 


	pVB->Unlock();


	LPDIRECT3DDEVICE8 pd3dDevice = DISPLAY->GetDevice();
	pd3dDevice->SetTexture( 0, NULL );



	// set texture and alpha properties
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );

	pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );


	// finally!  Pump those triangles!	
	pd3dDevice->SetVertexShader( D3DFVF_RAGEVERTEX );
	pd3dDevice->SetStreamSource( 0, pVB, sizeof(RAGEVERTEX) );
	pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, iNumV-2 );

}