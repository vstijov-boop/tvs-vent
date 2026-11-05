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
//----- hangeZdg.cpp : Implementation of dgChangeZ
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "dgChangeZ.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"

#include <cmath>
#include <vector>

namespace { int& lastReference(){static int r=0;return r;} }   // 0 Osa, 1 Vrh, 2 Dno (remembered)

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(dgChangeZ, CAdUiBaseDialog)

BEGIN_MESSAGE_MAP(dgChangeZ, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_EN_CHANGE(IDC_NextZ, OnChanged)
	ON_CBN_EDITCHANGE(IDC_Axis, OnChanged)
	ON_CBN_SELCHANGE(IDC_Axis, OnChanged)
	ON_BN_CLICKED(IDC_ZRefTop, OnChanged)
	ON_BN_CLICKED(IDC_ZRefAxis, OnChanged)
	ON_BN_CLICKED(IDC_ZRefBottom, OnChanged)
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
dgChangeZ::dgChangeZ(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAdUiBaseDialog(dgChangeZ::IDD, pParent, hInstance),
	startZ(0), nextZ(0), Axis(45), height(0), ready(false), feasible(false) {
}

void dgChangeZ::DoDataExchange(CDataExchange *pDX) {
	CAdUiBaseDialog::DoDataExchange(pDX);
}

static CString mm(double v){CString s;s.Format(L"%.0f",v);return s;}

BOOL dgChangeZ::OnInitDialog() {
	CAdUiBaseDialog::OnInitDialog();
	const double f = tvsUnits::factor(), h = height / 2;
	SetDlgItemText(IDC_ZOldTop, height > 0 ? mm((startZ + h) * f) : CString(L"-"));
	SetDlgItemText(IDC_StartZ, mm(startZ * f));
	SetDlgItemText(IDC_ZOldBottom, height > 0 ? mm((startZ - h) * f) : CString(L"-"));
	int ref = height > 0 ? lastReference() : 0;
	CheckRadioButton(IDC_ZRefTop, IDC_ZRefBottom, ref == 1 ? IDC_ZRefTop : ref == 2 ? IDC_ZRefBottom : IDC_ZRefAxis);
	if (height <= 0) { GetDlgItem(IDC_ZRefTop)->EnableWindow(FALSE); GetDlgItem(IDC_ZRefBottom)->EnableWindow(FALSE); }
	const double shown = nextZ + (ref == 1 ? h : ref == 2 ? -h : 0);
	SetDlgItemText(IDC_NextZ, mm(shown * f));
	CComboBox* angle = (CComboBox*)GetDlgItem(IDC_Axis);
	angle->AddString(L"30"); angle->AddString(L"45"); angle->AddString(L"60");
	CString a; a.Format(L"%g", Axis > 0 && Axis < 90 ? Axis : 45.0); angle->SetWindowText(a);
	ready = true; update();
	GetDlgItem(IDC_NextZ)->SetFocus(); ((CEdit*)GetDlgItem(IDC_NextZ))->SetSel(0, -1);
	return FALSE;
}

int dgChangeZ::reference() const {
	if (IsDlgButtonChecked(IDC_ZRefTop)) return 1;
	if (IsDlgButtonChecked(IDC_ZRefBottom)) return 2;
	return 0;
}

// New axis level (drawing units) and angle from the fields; false = not a number.
bool dgChangeZ::read(double& axis, double& angle) const {
	CString t; GetDlgItemText(IDC_NextZ, t); t.Replace(L',', L'.');
	wchar_t* end = NULL; const double v = wcstod(t, &end); if (t.IsEmpty() || !end || *end) return false;
	CString a; GetDlgItemText(IDC_Axis, a); a.Replace(L',', L'.');
	const double g = wcstod(a, &end); if (a.IsEmpty() || !end || *end) return false;
	const int ref = reference(); const double h = height / 2;
	axis = v / tvsUnits::factor() - (ref == 1 ? h : ref == 2 ? -h : 0); angle = g;
	return true;
}

void dgChangeZ::OnChanged() { if (ready) update(); }

void dgChangeZ::update() {
	const double f = tvsUnits::factor(), h = height / 2;
	double axis = 0, angle = 0; feasible = false;
	if (!read(axis, angle)) { SetDlgItemText(IDC_ZNewInfo, L""); SetDlgItemText(IDC_ZResult, L"Upišite kotu i ugao (broj)."); }
	else {
		CString info;
		if (height > 0) info.Format(L"vrh %.0f\nosa %.0f, dno %.0f", (axis + h) * f, axis * f, (axis - h) * f);
		else info.Format(L"osa %.0f", axis * f);
		SetDlgItemText(IDC_ZNewInfo, info);
		CString text;
		if (!(angle > 0 && angle < 90)) text = L"Ugao mora biti veći od 0 i manji od 90°.";
		else if (evaluate) {
			CString why;
			if (evaluate(axis, angle, geometry, why)) {
				feasible = true;
				text.Format(L"Razlika kota %+.0f mm  ·  kosi dio %.0f mm  ·  dužina etaža u osnovi %.0f mm  ·  ugao %g°",
					(axis - startZ) * f, geometry.straightLength, geometry.totalRun, angle);
			}
			else text = L"Nije izvodljivo: " + why;
		}
		else { feasible = std::fabs(axis - startZ) * f > 0.5; text.Format(L"Razlika kota %+.0f mm", (axis - startZ) * f); }
		SetDlgItemText(IDC_ZResult, text);
	}
	if (CWnd* w = GetDlgItem(IDC_ZSketch)) w->Invalidate(FALSE);
}

// Side view of the etaz: level duct, elbow, sloped piece, elbow, level duct (axis + outline).
void dgChangeZ::OnDrawItem(int id, LPDRAWITEMSTRUCT item) {
	if (id != IDC_ZSketch || !item) { CAdUiBaseDialog::OnDrawItem(id, item); return; }
	CDC* dc = CDC::FromHandle(item->hDC); CRect r(item->rcItem);
	dc->FillSolidRect(r, RGB(255, 255, 255));
	if (!feasible || !evaluate) {
		dc->SetTextColor(RGB(120, 120, 120)); dc->SetBkMode(TRANSPARENT);
		dc->DrawText(L"skica etaža", r, DT_CENTER | DT_VCENTER | DT_SINGLELINE); return;
	}
	const tvsOffset::Result& g = geometry;
	const double B = height * tvsUnits::factor() > 0 ? height * tvsUnits::factor() : 0;
	// centre line (s, z) in mm with the elbows as quadratic curves
	std::vector<std::pair<double, double> > c;
	const double run = g.totalRun, lead = run * 0.35 + 50;
	c.push_back(std::make_pair(-lead, 0.0));
	auto curve = [&](double s0, double z0, double cs, double cz, double s1, double z1) {
		for (int k = 0; k <= 12; ++k) { const double t = k / 12.0, u = 1 - t;
			c.push_back(std::make_pair(u*u*s0 + 2*u*t*cs + t*t*s1, u*u*z0 + 2*u*t*cz + t*t*z1)); } };
	const double ds = g.axis[2].s - g.axis[1].s, dz = g.axis[2].z - g.axis[1].z;
	const double t1 = std::fabs(dz) > 1e-9 ? (g.axis[1].z - g.axis[0].z) / dz : 0, t2 = std::fabs(dz) > 1e-9 ? (g.axis[3].z - g.axis[2].z) / dz : 0;
	curve(g.axis[0].s, g.axis[0].z, g.axis[1].s - ds * t1, g.axis[0].z, g.axis[1].s, g.axis[1].z);
	curve(g.axis[2].s, g.axis[2].z, g.axis[2].s + ds * t2, g.axis[3].z, g.axis[3].s, g.axis[3].z);
	c.push_back(std::make_pair(run + lead, g.axis[3].z));
	// outline at +-B/2 normal to the centre line
	std::vector<std::pair<double, double> > up, dn;
	for (size_t k = 0; k < c.size(); ++k) {
		const size_t a = k ? k - 1 : k, b = k + 1 < c.size() ? k + 1 : k;
		double tx = c[b].first - c[a].first, tz = c[b].second - c[a].second; const double l = std::sqrt(tx*tx + tz*tz); if (l < 1e-12) { tx = 1; tz = 0; } else { tx /= l; tz /= l; }
		up.push_back(std::make_pair(c[k].first - tz * B / 2, c[k].second + tx * B / 2));
		dn.push_back(std::make_pair(c[k].first + tz * B / 2, c[k].second - tx * B / 2));
	}
	double s0 = 1e300, s1 = -1e300, z0 = 1e300, z1 = -1e300;
	for (auto* v : { &up, &dn }) for (auto& p : *v) { s0 = (std::min)(s0, p.first); s1 = (std::max)(s1, p.first); z0 = (std::min)(z0, p.second); z1 = (std::max)(z1, p.second); }
	const double m = 8, k = (std::min)((r.Width() - 2 * m) / (s1 - s0 > 1e-9 ? s1 - s0 : 1), (r.Height() - 2 * m) / (z1 - z0 > 1e-9 ? z1 - z0 : 1));
	const double ox = r.left + (r.Width() - (s1 - s0) * k) / 2, oy = r.top + (r.Height() + (z1 - z0) * k) / 2;
	auto P = [&](const std::pair<double, double>& p) { return CPoint(int(ox + (p.first - s0) * k), int(oy - (p.second - z0) * k)); };
	CPen outline(PS_SOLID, 1, RGB(0, 0, 0)), axisPen(PS_DOT, 1, RGB(0, 120, 200));
	CPen* old = dc->SelectObject(&outline);
	for (auto* v : { &up, &dn }) { dc->MoveTo(P((*v)[0])); for (size_t j = 1; j < v->size(); ++j) dc->LineTo(P((*v)[j])); }
	dc->SelectObject(&axisPen); dc->MoveTo(P(c[0])); for (size_t j = 1; j < c.size(); ++j) dc->LineTo(P(c[j]));
	dc->SelectObject(old);
}

void dgChangeZ::OnOK() {
	double axis = 0, angle = 0;
	if (!read(axis, angle) || !(angle > 0 && angle < 90)) { AfxMessageBox(L"Upišite ispravnu kotu i ugao (manji od 90°)."); return; }
	if (evaluate && !feasible) { CString t; GetDlgItemText(IDC_ZResult, t); AfxMessageBox(t); return; }
	nextZ = axis; Axis = angle; lastReference() = reference();
	CAdUiBaseDialog::OnOK();
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT dgChangeZ::OnAcadKeepFocus(WPARAM, LPARAM) {
	return (TRUE);
}
