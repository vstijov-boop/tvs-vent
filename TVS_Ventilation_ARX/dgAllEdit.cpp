// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- dgAllEdit.cpp : Implementation of dgAllEdit
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "ElevRefSwitch.h"
#include "TransPreview.h"
#include "resource.h"
#include "dgAllEdit.h"

#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "DecimalText.h"
static void DDX_MillimetreString(CDataExchange* dx, int id, CString& value) {
    const double factor = tvsUnits::factor();
    CString display = value;
    double number = 0;
    if (!dx->m_bSaveAndValidate && tvsDecimal::parse(value, number))
        display.Format(L"%.15g", number * factor);
    DDX_Text(dx, id, display);
    if (dx->m_bSaveAndValidate) {
        if (tvsDecimal::parse(display, number)) value.Format(L"%.15g", number / factor);   // decimal comma or point
        else value = display;
    }
}

#include "Calculate.h"

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(dgAllEdit, CAdUiBaseDialog)

BEGIN_MESSAGE_MAP(dgAllEdit, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)

	ON_WM_SHOWWINDOW()
	ON_WM_DRAWITEM()
	ON_CBN_SELCHANGE(IDC_TransVertEdit, &dgAllEdit::RefreshPreview)
	ON_CBN_SELCHANGE(IDC_TransPlanEdit, &dgAllEdit::RefreshPreview)
	ON_EN_CHANGE(IDC_SizeA2, &dgAllEdit::RefreshPreview)
	ON_EN_CHANGE(IDC_SizeB2, &dgAllEdit::RefreshPreview)
	ON_EN_CHANGE(IDC_SizeA, &dgAllEdit::ChangeSizeA)
	ON_EN_CHANGE(IDC_SizeB, &dgAllEdit::ChangeSizeA)
	//ON_EN_CHANGE(IDC_SizeA2, &dgAllEdit::ChangeSizeA2)
	//ON_EN_CHANGE(IDC_SizeB2, &dgAllEdit::ChangeSizeB2)
	ON_EN_CHANGE(IDC_Flow, &dgAllEdit::ChangeSizeA)
	ON_EN_CHANGE(IDC_Speed, &dgAllEdit::ChangeSpeed)
	ON_BN_CLICKED(IDC_ElevUp, &dgAllEdit::SetElevUp)
	ON_BN_CLICKED(IDC_ElevMid, &dgAllEdit::SetElevMid)
	ON_BN_CLICKED(IDC_ElevDown, &dgAllEdit::SetElevDown)
	ON_BN_CLICKED(IDC_Wipeout, &dgAllEdit::ChangeWipe)
	ON_BN_CLICKED(IDC_Grani, &dgAllEdit::ChangeGrani)
	ON_BN_CLICKED(IDC_1D, &dgAllEdit::ChangeD1)
	ON_BN_CLICKED(IDC_TapDown, &dgAllEdit::SetForm_Down)
	ON_BN_CLICKED(IDC_TapMid, &dgAllEdit::SetFForm_Direct)
	ON_BN_CLICKED(IDC_TapUp, &dgAllEdit::SetForm_Up)

	ON_BN_CLICKED(IDC_TapRadiusRoundConst, &dgAllEdit::SetRadiusTypeRound_RadiusConstant)
	ON_BN_CLICKED(IDC_TapRadiusRoundVariable, &dgAllEdit::SetRadiusTypeRound_RadiusVariable)
	ON_BN_CLICKED(IDC_TapRadiusRoundSpecific, &dgAllEdit::SetRadiusTypeRound_RadiusSpecific)

	ON_BN_CLICKED(IDC_TapRadiusRectConst, &dgAllEdit::SetRadiusTypeRect_RadiusConstant)
	ON_BN_CLICKED(IDC_TapRadiusRectVariable, &dgAllEdit::SetRadiusTypeRect_RadiusVariable)

	ON_BN_CLICKED(IDC_DuctFlex, &dgAllEdit::ChangeDuctFlex)

	ON_BN_CLICKED(IDOK, &dgAllEdit::OnBnClickedOk)

END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
dgAllEdit::dgAllEdit(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAdUiBaseDialog(dgAllEdit::IDD, pParent, hInstance)
, SizeA(_T(""))
, SizeB(_T(""))
, Flow(_T(""))
, SizeA2(_T(""))
, SizeB2(_T(""))
, LengthTr(_T(""))
, LengthW(_T(""))
, Wipe(FALSE)
, Grani(FALSE)
, D1(FALSE)
, Elev(_T(""))
, ElevUp(FALSE)
, ElevMid(FALSE)
, ElevDown(FALSE)
, TeeType(0)
, TransVert(-1)
, TransPlan(-1)
, TeeSecond(_T(""))
{
}



//-----------------------------------------------------------------------------
void dgAllEdit::DoDataExchange(CDataExchange *pDX) {
	CAdUiBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SizeA, fSizeA);
	DDX_Control(pDX, IDC_SizeB, fSizeB);
	DDX_Control(pDX, IDC_Flow, fFlow);
	DDX_Control(pDX, IDC_SizeA2, fSizeA2);
	DDX_Control(pDX, IDC_SizeB2, fSizeB2);
	DDX_Control(pDX, IDC_TransLength, fLengthTr);
	DDX_Control(pDX, IDC_LengthWye, fLengthW);
	DDX_Control(pDX, IDC_TeeSecond, fTeeSecond);
	DDX_Control(pDX, IDCANCEL, F);
	DDX_Control(pDX, IDC_Wipeout, fWipeout);
	DDX_Control(pDX, IDC_Grani, fGrani);
	DDX_Control(pDX, IDC_1D, fD1);
	DDX_Control(pDX, IDC_ElevVal, fElevation);
	DDX_Control(pDX, IDC_ElevUp, fElevUp);
	DDX_Control(pDX, IDC_ElevMid, fElevMid);
	DDX_Control(pDX, IDC_ElevDown, fElevDown);
	DDX_Control(pDX, IDC_Speed, fSpeed);
	DDX_Control(pDX, IDC_i, fI);
	DDX_Control(pDX, IDC_dP, fDp);

	DDX_Control(pDX, IDC_TapRadiusRectVariable, fTapRadiusRectVariable);
	DDX_Control(pDX, IDC_TapRadiusRectConst, fTapRadiusRectConst);
	DDX_Control(pDX, IDC_TapRadiusRoundVariable, fTapRadiusRoundVariable);
	DDX_Control(pDX, IDC_TapRadiusRoundConst, fTapRadiusRoundConst);
	DDX_Control(pDX, IDC_TapRadiusRoundSpecific, fTapRadiusRoundSpecific);
	DDX_Control(pDX, IDC_TapUp, fTapUp);
	DDX_Control(pDX, IDC_TapMid, fTapMid);
	DDX_Control(pDX, IDC_TapDown, fTapDown);
	DDX_Control(pDX, IDC_Swectangle, fSwectangle);
	DDX_Control(pDX, IDC_TapRadiusVariableParameter, fTapRadiusVariableParameter);
	DDX_Control(pDX, IDC_TapRadiusConst, fTapRadiusConst);



	DDX_MillimetreString(pDX, IDC_SizeA, SizeA);
	DDX_MillimetreString(pDX, IDC_SizeB, SizeB);
		tvsDecimal::DDX_Number(pDX, IDC_Flow, Flow);
	DDX_MillimetreString(pDX, IDC_SizeA2, SizeA2);
	DDX_MillimetreString(pDX, IDC_SizeB2, SizeB2);
	DDX_MillimetreString(pDX, IDC_TransLength, LengthTr);
	DDX_MillimetreString(pDX, IDC_LengthWye, LengthW);
	DDX_MillimetreString(pDX, IDC_TeeSecond, TeeSecond);
	if (pDX->m_bSaveAndValidate) {   // prelaz po visini (combo filled in OnShowWindow)
		if (CComboBox* vert = static_cast<CComboBox*>(GetDlgItem(IDC_TransVertEdit))) if (vert->GetCount() == 3) TransVert = vert->GetCurSel();
		if (CComboBox* plan = static_cast<CComboBox*>(GetDlgItem(IDC_TransPlanEdit))) if (plan->GetCount() == 3) { const int v = plan->GetCurSel(); TransPlan = v >= 0 ? v + 1 : -1; }
	}
	DDX_MillimetreString(pDX, IDC_ElevVal, Elev);
	DDX_Text(pDX, IDC_Speed, Speed);
	DDX_Text(pDX, IDC_i, I);
	DDX_Text(pDX, IDC_dP, Dp);

		tvsDecimal::DDX_Number(pDX, IDC_Swectangle, Swectangle);
		tvsDecimal::DDX_Number(pDX, IDC_TapRadiusVariableParameter, TapRadiusVariableParameter);
	DDX_MillimetreString(pDX, IDC_TapRadiusConst, TapRadiusConst);





	DDX_Check(pDX, IDC_Wipeout, Wipe);
	DDX_Check(pDX, IDC_Grani, Grani);
	DDX_Check(pDX, IDC_1D, D1);
	DDX_Check(pDX, IDC_ElevUp, ElevUp);
	DDX_Check(pDX, IDC_ElevMid, ElevMid);
	DDX_Check(pDX, IDC_ElevDown, ElevDown);

	DDX_Check(pDX, IDC_Wipeout, Wipe);
	DDX_Check(pDX, IDC_Grani, Grani);
	DDX_Check(pDX, IDC_1D, D1);
	DDX_Check(pDX, IDC_ElevUp, ElevUp);
	DDX_Check(pDX, IDC_ElevMid, ElevMid);
	DDX_Check(pDX, IDC_ElevDown, ElevDown);

	DDX_Check(pDX, IDC_TapRadiusRectVariable, TapRadiusRectVariable);
	DDX_Check(pDX, IDC_TapRadiusRectConst, TapRadiusRectConst);
	DDX_Check(pDX, IDC_TapRadiusRoundVariable, TapRadiusRoundVariable);
	DDX_Check(pDX, IDC_TapRadiusRoundConst, TapRadiusRoundConst);
	DDX_Check(pDX, IDC_TapRadiusRoundSpecific, TapRadiusRoundSpecific);
	DDX_Check(pDX, IDC_TapUp, TapUp);
	DDX_Check(pDX, IDC_TapMid, TapMid);
	DDX_Check(pDX, IDC_TapDown, TapDown);



	DDX_Control(pDX, IDC_WipeoutLength, fWipeoutLength);
	DDX_MillimetreString(pDX, IDC_WipeoutLength, WipeoutLength);

	DDX_Control(pDX, IDC_DuctFlex, fDuctFlex);
	DDX_Check(pDX, IDC_DuctFlex, DuctFlex);
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT dgAllEdit::OnAcadKeepFocus(WPARAM, LPARAM) {
	return (TRUE);
}


void dgAllEdit::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CAdUiBaseDialog::OnShowWindow(bShow, nStatus);

	fSizeA.SetFocus();

	SendDlgItemMessage(IDC_SizeA, EM_SETSEL, (WPARAM)0, (LPARAM)-1);



	if (Tpipe == false)
	{

		fGrani.ShowWindow(false);

	}

	if (Ttrans == false)
	{
		fLengthTr.ShowWindow(false);

	}
	if (CComboBox* vert = static_cast<CComboBox*>(GetDlgItem(IDC_TransVertEdit)))
	{
		if (Ttrans) {
			if (vert->GetCount() == 0) { vert->AddString(L"Osa"); vert->AddString(L"Ravno uz gornju ivicu"); vert->AddString(L"Ravno uz donju ivicu"); }
			vert->SetCurSel(TransVert);   // -1: razno
		} else {
			vert->ShowWindow(false);
			if (CWnd* label = GetDlgItem(IDC_TransVertLabel)) label->ShowWindow(false);
		}
	}
	if (CWnd* preview = GetDlgItem(IDC_TransPreview)) preview->ShowWindow(Ttrans && !Ttap ? SW_SHOW : SW_HIDE);   // sketch only for prelazi
	if (CComboBox* plan = static_cast<CComboBox*>(GetDlgItem(IDC_TransPlanEdit)))
	{
		if (Ttrans) {
			if (plan->GetCount() == 0) { plan->AddString(L"Simetrično"); plan->AddString(L"Ravno uz desnu ivicu"); plan->AddString(L"Ravno uz lijevu ivicu"); }
			plan->SetCurSel(TransPlan >= 1 && TransPlan <= 3 ? TransPlan - 1 : -1);
		} else {
			plan->ShowWindow(false);
			if (CWnd* label = GetDlgItem(IDC_TransPlanLabel)) label->ShowWindow(false);
		}
	}

	if (Twye == false)
	{
		fLengthW.ShowWindow(false);

	}

	// T komad: TK-O asks for the run outlet C, TK-G for the left branch (may differ from the right).
	if (TeeType == 1 || TeeType == 2)
	{
		SetDlgItemTextW(IDC_TeeSecondLabel, TeeType == 1 ? L"Prolaz C" : L"Lijevi ogr.");
		if (TeeType == 1) { SetDlgItemTextW(IDC_LabelSizeA2, L"Ogr. Dotv"); SetDlgItemTextW(IDC_LabelSizeB2, L"Ogr. Hotv"); }
		else { SetDlgItemTextW(IDC_LabelSizeA, L"Desni ogr."); SetDlgItemTextW(IDC_LabelSizeA2, L"Ulaz Dotv"); SetDlgItemTextW(IDC_LabelSizeB2, L"Ulaz Hotv"); }
	}
	else
	{
		if (TeeType == 3) { SetDlgItemTextW(IDC_LabelSizeA2, L"Ogr. Dotv"); SetDlgItemTextW(IDC_LabelSizeB2, L"Ogr. Hotv"); }   // stucna SR5-3
		fTeeSecond.ShowWindow(false);
		if (CWnd* label = GetDlgItem(IDC_TeeSecondLabel)) label->ShowWindow(false);
	}

	if ((Twye == false) && (Ttrans == false))
	{
		fSizeA2.ShowWindow(false);

		fSizeB2.ShowWindow(false);

	}

	if ((Ttap == false) && (Tpipe == false))
	{
		fFlow.ShowWindow(false);

	}

	this->ChangeSizeA();

	// Koljeno (radius, smjer) only for elbows, T-komad only for tees: not shown for a prelaz or kanal.
	if (Ttap == false)
	{
		static const int elbow[] = {10061,10062,10063,10064,1018,1019,1036,1037,1038,1039,1040,1013,10013,1030,1034,1035};
		for (size_t k = 0; k < sizeof(elbow) / sizeof(elbow[0]); ++k) if (CWnd* c = GetDlgItem(elbow[k])) c->ShowWindow(false);
	}
	if (Twye == false)
	{
		static const int tee[] = {10065,123};
		for (size_t k = 0; k < sizeof(tee) / sizeof(tee[0]); ++k) if (CWnd* c = GetDlgItem(tee[k])) c->ShowWindow(false);
	}

	if (Ttap == true)
	{



		iTapForm = _wtoi(TapForm);
		iTypeRoundTap = _wtoi(TypeRoundTap);
		iRadiusTypeRound = _wtoi(RadiusTypeRound);
		iRadiusTypeRect = _wtoi(RadiusTypeRect);
		CString var;
		SwectangleRadians = Swectangle; SwectangleShown.Empty();
		if ((tvsDecimal::value(Swectangle) / M_PI * 180) != 0)
		{
			var.Format(L"%.10g", tvsDecimal::value(Swectangle) / M_PI * 180);
			fSwectangle.SetWindowTextW(var);
			SwectangleShown = var;
		}


		switch (iTapForm)
		{
		case Form_Up:
			TapUp = true;
			fTapUp.SetCheck(1);
			break;
		case Form_Down:
			TapDown = true;
			fTapDown.SetCheck(1);
			break;
		case Form_Direct:
			TapMid = true;
			fTapMid.SetCheck(1);
			break;
		default:
			break;
		}

		switch (iRadiusTypeRound)
		{
		case RadiusTypeRound_RadiusSpecific:
			TapRadiusRoundSpecific = true;
			fTapRadiusRoundSpecific.SetCheck(1);
			break;
		case RadiusTypeRound_RadiusConstant:
			TapRadiusRoundConst = true;
			fTapRadiusRoundConst.SetCheck(1);
			break;
		case RadiusTypeRound_RadiusVariable:
			TapRadiusRoundVariable = true;
			fTapRadiusRoundVariable.SetCheck(1);
			break;
		default:
			break;
		}

		switch (iRadiusTypeRect)
		{

		case RadiusTypeRect_RadiusConstant:
			TapRadiusRectConst = true;
			fTapRadiusRectConst.SetCheck(1);
			break;
		case RadiusTypeRect_RadiusVariable:
			TapRadiusRectVariable = true;
			fTapRadiusRectVariable.SetCheck(1);
			break;
		default:
			break;
		}


	}




	// TODO: добавьте свой код обработчика сообщений
}



void dgAllEdit::RefreshPreview()
{
	if (CWnd* w = GetDlgItem(IDC_TransPreview)) w->Invalidate(FALSE);
}

void dgAllEdit::OnDrawItem(int id, LPDRAWITEMSTRUCT item)
{
	if (id != IDC_TransPreview || !item) { CAdUiBaseDialog::OnDrawItem(id, item); return; }
	CDC* dc = CDC::FromHandle(item->hDC);
	double v[4] = {0, 0, 0, 0};
	const int ids[4] = {IDC_SizeA, IDC_SizeB, IDC_SizeA2, IDC_SizeB2};
	for (int k = 0; k < 4; ++k) tvsElevRef::number(this, ids[k], v[k]);
	int plan = -1, vert = -1;
	if (CComboBox* c = static_cast<CComboBox*>(GetDlgItem(IDC_TransPlanEdit))) { const int s = c->GetCurSel(); plan = s >= 0 ? s + 1 : -1; }
	if (CComboBox* c = static_cast<CComboBox*>(GetDlgItem(IDC_TransVertEdit))) vert = c->GetCurSel();
	tvsTransPreview::sketch(dc, CRect(item->rcItem), plan, vert, v[0], v[1], v[2], v[3]);
}

void dgAllEdit::ChangeSizeA()
{
	RefreshPreview();

	double pSpeed, pFlow, pI, pDp, pSizeA, pSizeB;
	CString zSizeA, zSizeB, zFlow, zSpeed, zI;
	fSizeB.GetWindowTextW(zSizeB);
	fSizeA.GetWindowTextW(zSizeA);
	fFlow.GetWindowTextW(zFlow);

	pSizeA = tvsDecimal::value(zSizeA);
	pSizeB = tvsDecimal::value(zSizeB);
	pFlow = tvsDecimal::value(zFlow);
	pSpeed = TVS_Calculate::getSpeed(pSizeA, pSizeB, pFlow);
	pI = TVS_Calculate::getI(pSizeA, pSizeB, pFlow);
	//pDp=TVS_Calculate::getdP(pSizeA,pSizeB,pFlow);
	zSpeed.Format(L"%g", pSpeed);
	zI.Format(L"%g", pI);
	fSpeed.SetWindowTextW(zSpeed);
	fI.SetWindowTextW(zI);





}

void dgAllEdit::ChangeSpeed()
{

	// 	double pSpeed, pFlow, pI, pDp, pSizeA, pSizeB;
	// 	CString zSizeA,zSizeB,zFlow,zSpeed,zI;
	// 	fSizeB.GetWindowTextW(zSizeB);
	// 	fSizeA.GetWindowTextW(zSizeA);
	// 	fSpeed.GetWindowTextW(zSpeed);
	// 
	// 	pSizeA=tvsDecimal::value(zSizeA);
	// 	pSizeB=tvsDecimal::value(zSizeB);
	// 	pSpeed=tvsDecimal::value(zSpeed);
	// 	pFlow=TVS_Calculate::getFlow(pSizeA,pSizeB,pSpeed);
	// 	pI=TVS_Calculate::getI(pSizeA,pSizeB,pFlow);
	// 
	// 	zFlow.Format(L"%g", pFlow);
	// 	zI.Format(L"%g", pI);
	// 	fFlow.SetWindowTextW(zFlow);
	// 	fI.SetWindowTextW(zI);





}


void dgAllEdit::SetElevUp()
{
	tvsElevRef::convert(this, fElevDown.GetCheck() ? 2 : fElevMid.GetCheck() ? 0 : 1, 1);   // the other box still shows the previous reference

	fElevUp.SetCheck(1);
	fElevMid.SetCheck(0);
	fElevDown.SetCheck(0);




	// TODO: добавьте свой код обработчика уведомлений
}

void dgAllEdit::SetElevMid()
{
	tvsElevRef::convert(this, fElevUp.GetCheck() ? 1 : fElevDown.GetCheck() ? 2 : 0, 0);   // the other box still shows the previous reference


	fElevUp.SetCheck(0);
	fElevMid.SetCheck(1);
	fElevDown.SetCheck(0);
	// TODO: добавьте свой код обработчика уведомлений
}

void dgAllEdit::SetElevDown()
{
	tvsElevRef::convert(this, fElevUp.GetCheck() ? 1 : fElevMid.GetCheck() ? 0 : 2, 2);   // the other box still shows the previous reference
	fElevUp.SetCheck(0);
	fElevMid.SetCheck(0);
	fElevDown.SetCheck(1);
	// TODO: добавьте свой код обработчика уведомлений
}




void dgAllEdit::change_BOOL(BOOL &var, CButton &but)
{
	if (var == 1)
	{
		var = 0;
		but.SetCheck(0);

	}
	else
	{
		var = 1;
		but.SetCheck(1);
	}
}

void dgAllEdit::ChangeWipe()
{
	change_BOOL(Wipe, fWipeout);
}

void dgAllEdit::ChangeGrani()
{
	change_BOOL(Grani, fGrani);
}

void dgAllEdit::ChangeD1()
{
	change_BOOL(D1, fD1);
}




void dgAllEdit::SetForm_Up()
{

	TapUp = true;
	fTapUp.SetCheck(1);


	TapDown = false;
	fTapDown.SetCheck(0);


	TapMid = false;
	fTapMid.SetCheck(0);

}

void dgAllEdit::SetForm_Down()
{
	TapUp = false;
	fTapUp.SetCheck(0);


	TapDown = true;
	fTapDown.SetCheck(1);


	TapMid = false;
	fTapMid.SetCheck(0);
}

void dgAllEdit::SetFForm_Direct()
{
	TapUp = false;
	fTapUp.SetCheck(0);


	TapDown = false;
	fTapDown.SetCheck(0);


	TapMid = true;
	fTapMid.SetCheck(1);

}

void dgAllEdit::SetRadiusTypeRound_RadiusSpecific()
{


	TapRadiusRoundSpecific = true;
	fTapRadiusRoundSpecific.SetCheck(1);


	TapRadiusRoundConst = false;
	fTapRadiusRoundConst.SetCheck(0);


	TapRadiusRoundVariable = false;
	fTapRadiusRoundVariable.SetCheck(0);


}

void dgAllEdit::SetRadiusTypeRound_RadiusConstant()
{
	TapRadiusRoundSpecific = false;
	fTapRadiusRoundSpecific.SetCheck(0);


	TapRadiusRoundConst = true;
	fTapRadiusRoundConst.SetCheck(1);


	TapRadiusRoundVariable = false;
	fTapRadiusRoundVariable.SetCheck(0);
}

void dgAllEdit::SetRadiusTypeRound_RadiusVariable()
{
	TapRadiusRoundSpecific = false;
	fTapRadiusRoundSpecific.SetCheck(0);


	TapRadiusRoundConst = false;
	fTapRadiusRoundConst.SetCheck(0);


	TapRadiusRoundVariable = true;
	fTapRadiusRoundVariable.SetCheck(1);
}

void dgAllEdit::SetRadiusTypeRect_RadiusConstant()
{
	TapRadiusRectConst = true;
	fTapRadiusRectConst.SetCheck(1);


	TapRadiusRectVariable = false;
	fTapRadiusRectVariable.SetCheck(0);


}

void dgAllEdit::SetRadiusTypeRect_RadiusVariable()
{
	TapRadiusRectConst = false;
	fTapRadiusRectConst.SetCheck(0);


	TapRadiusRectVariable = true;
	fTapRadiusRectVariable.SetCheck(1);
}




void dgAllEdit::OnBnClickedOk()
{


	if (TapMid == TRUE)
	{
		TapForm.Format(L"%i", Form_Direct);
	}

	if (TapDown == TRUE)
	{
		TapForm.Format(L"%i", Form_Down);
	}

	if (TapUp == TRUE)
	{
		TapForm.Format(L"%i", Form_Up);
	}

	if (TapRadiusRoundConst == TRUE)
	{
		RadiusTypeRound.Format(L"%i", RadiusTypeRound_RadiusConstant);
	}

	if (TapRadiusRoundVariable == TRUE)
	{
		RadiusTypeRound.Format(L"%i", RadiusTypeRound_RadiusVariable);
	}

	if (TapRadiusRoundSpecific == TRUE)
	{
		RadiusTypeRound.Format(L"%i", RadiusTypeRound_RadiusSpecific);
	}

	if (TapRadiusRectConst == TRUE)
	{
		RadiusTypeRect.Format(L"%i", RadiusTypeRect_RadiusConstant);
	}

	if (TapRadiusRectVariable == TRUE)
	{
		RadiusTypeRect.Format(L"%i", RadiusTypeRect_RadiusVariable);
	}

	CString var;
	fSwectangle.GetWindowTextW(var);

	if (!SwectangleShown.IsEmpty() && tvsDecimal::normalized(var) == tvsDecimal::normalized(SwectangleShown))
		fSwectangle.SetWindowTextW(SwectangleRadians);   // ugao nije diran: ostaje tačno kakav je bio
	else if ((M_PI*tvsDecimal::value(var) / 180) != 0)
	{
		var.Format(L"%.15g", M_PI*tvsDecimal::value(var) / 180);
		fSwectangle.SetWindowTextW(var);
	}






	this->OnOK();
	// TODO: добавьте свой код обработчика уведомлений
}


void dgAllEdit::ChangeDuctFlex()
{
	//change_BOOL(iDuctFlex,fDuctFlex);

	change_BOOL(DuctFlex, fDuctFlex);
}


