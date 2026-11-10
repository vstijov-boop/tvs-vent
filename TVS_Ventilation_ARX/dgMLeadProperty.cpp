#include "StdAfx.h"
#include "dgMLeadProperty.h"
#include "DecimalText.h"
#include "Settings.h"
#include "MLeaderStyleTools.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"

//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC(dgMLeadProperty, CAdUiBaseDialog)

BEGIN_MESSAGE_MAP(dgMLeadProperty, CAdUiBaseDialog)
	ON_MESSAGE(WM_ACAD_KEEPFOCUS, OnAcadKeepFocus)
	ON_BN_CLICKED(IDC_MLIMPORT, OnImport)
	ON_CBN_SELCHANGE(IDC_MLSTYLE, OnStyleOrScaleChanged)
	ON_EN_CHANGE(IDC_MLSCALE, OnStyleOrScaleChanged)
END_MESSAGE_MAP()

static const wchar_t* currentStyleText() { return L"<trenutni stil crteža>"; }
static const wchar_t* filePrefix() { return L"[iz crteža] "; }

//-----------------------------------------------------------------------------
dgMLeadProperty::dgMLeadProperty(CWnd *pParent /*=NULL*/, HINSTANCE hInstance /*=NULL*/) : CAdUiBaseDialog(dgMLeadProperty::IDD, pParent, hInstance)
{

}


//-----------------------------------------------------------------------------
void dgMLeadProperty::DoDataExchange(CDataExchange *pDX) {
	CAdUiBaseDialog::DoDataExchange(pDX);
	Settings *settings = Settings::get();

	DDX_Text(pDX, IDC_EDIT3, settings->mLeader.instructionMessage);
	DDX_Text(pDX, IDC_EDIT1, settings->mLeader.pipeLeader);
	DDX_Text(pDX, IDC_EDIT2, settings->mLeader.blockLeader);
	DDX_Control(pDX, IDC_MLSTYLE, styles);
}

BOOL dgMLeadProperty::OnInitDialog() {
	const BOOL result = CAdUiBaseDialog::OnInitDialog();
	MLeaderSettings& m = Settings::get()->mLeader;
	CString scale; scale.Format(L"%.6g", m.scale); SetDlgItemText(IDC_MLSCALE, scale);
	fileSource = m.styleSource;
	tvsMLeaderStyle::ensureCurrentDocument(m.style, m.styleSource); // TVS_MleaderStyle always in the list
	fillStyles(m.style);
	updateInfo();
	return result;
}

void dgMLeadProperty::fillStyles(const CString& select) {
	styles.ResetContent();
	styles.AddString(currentStyleText());
	std::vector<CString> names; tvsMLeaderStyle::names(acdbHostApplicationServices()->workingDatabase(), names);
	int chosen = 0;
	for (size_t i = 0; i < names.size(); ++i) { const int at = styles.AddString(names[i]); if (names[i] == select) chosen = at; }
	for (size_t i = 0; i < fileStyles.size(); ++i) {
		bool present = false; for (size_t j = 0; j < names.size(); ++j) if (names[j] == fileStyles[i]) present = true;
		if (present) continue;
		const int at = styles.AddString(CString(filePrefix()) + fileStyles[i]); if (fileStyles[i] == select) chosen = at;
	}
	styles.SetCurSel(chosen);
}

// Text height shown as: style height (paper mm) x scale = model height.
void dgMLeadProperty::updateInfo() {
	CString scaleText; GetDlgItemText(IDC_MLSCALE, scaleText); const double scale = tvsDecimal::value(scaleText);
	AcDbDatabase* db = acdbHostApplicationServices()->workingDatabase();
	CString name; const int sel = styles.GetCurSel(); if (sel > 0) styles.GetLBText(sel, name);
	AcDbObjectId id = sel <= 0 ? (db ? db->mleaderstyle() : AcDbObjectId()) : tvsMLeaderStyle::find(db, name);
	double height = 0; CString textStyle; CString info;
	if (!(scale > 0)) info = L"Razmjera mora biti veća od 0.";
	else if (tvsMLeaderStyle::info(id, height, textStyle)) {
		const double f = tvsUnits::factor(db);
		info.Format(L"Tekst %.2f mm × %.6g = %.1f mm (%.4g jed. crteža), stil teksta: %s", height, scale, height * scale, height * scale / f, textStyle.GetString());
	}
	else info = L"Stil se preuzima iz izabranog crteža pri potvrdi (U redu).";
	SetDlgItemText(IDC_MLINFO, info);
}

void dgMLeadProperty::OnStyleOrScaleChanged() { updateInfo(); }

void dgMLeadProperty::OnImport() {
	CFileDialog dlg(TRUE, L"dwg", NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, L"AutoCAD crtež (*.dwg)|*.dwg||", this);
	if (dlg.DoModal() != IDOK) return;
	std::vector<CString> names;
	if (!tvsMLeaderStyle::namesInFile(dlg.GetPathName(), names) || names.empty()) {
		AfxMessageBox(L"Izabrani crtež nema stilova multioznake ili se ne može pročitati."); return; }
	fileSource = dlg.GetPathName(); fileStyles = names;
	CString current; const int sel = styles.GetCurSel(); if (sel > 0) styles.GetLBText(sel, current);
	fillStyles(current);
	// Offer the first style from the file when nothing was chosen before.
	if (styles.GetCurSel() <= 0) for (int i = 0; i < styles.GetCount(); ++i) {
		CString t; styles.GetLBText(i, t); if (t.Find(filePrefix()) == 0) { styles.SetCurSel(i); break; } }
	updateInfo();
}

void dgMLeadProperty::OnOK() {
	if (!UpdateData(TRUE)) return;
	CString scaleText; GetDlgItemText(IDC_MLSCALE, scaleText); const double scale = tvsDecimal::value(scaleText);
	if (!(scale > 0) || scale > 100000) { AfxMessageBox(L"Razmjera mora biti broj veći od 0 (npr. 50 za 1:50)."); return; }
	MLeaderSettings& m = Settings::get()->mLeader;
	CString name; const int sel = styles.GetCurSel(); if (sel > 0) styles.GetLBText(sel, name);
	const bool fromFile = name.Find(filePrefix()) == 0;
	if (fromFile) {
		name = name.Mid(static_cast<int>(wcslen(filePrefix())));
		AcApDocument* doc = acDocManager->curDocument();
		const bool locked = doc && acDocManager->lockDocument(doc, AcAp::kWrite) == Acad::eOk;
		AcDbObjectId id; const Acad::ErrorStatus es = tvsMLeaderStyle::import(acdbHostApplicationServices()->workingDatabase(), fileSource, name, id);
		if (locked) acDocManager->unlockDocument(doc);
		if (es != Acad::eOk) { CString t; t.Format(L"Stil \"%s\" nije preuzet (%s).", name.GetString(), acadErrorStatusText(es)); AfxMessageBox(t); return; }
	}
	m.style = name;
	if (fromFile) m.styleSource = fileSource;
	m.scale = scale;
	m.save();
	CAdUiBaseDialog::OnOK();
}

//-----------------------------------------------------------------------------
//----- Needed for modeless dialogs to keep focus.
//----- Return FALSE to not keep the focus, return TRUE to keep the focus
LRESULT dgMLeadProperty::OnAcadKeepFocus(WPARAM, LPARAM) {
	return (TRUE);
}
