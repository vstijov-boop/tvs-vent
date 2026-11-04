#pragma once
#include "adui.h"
#include "afxwin.h"
#include "Resource.h"
#include <vector>
#define IDC_MLSTYLE  1201
#define IDC_MLIMPORT 1202
#define IDC_MLSCALE  1203
#define IDC_MLINFO   1204

class dgMLeadProperty : public CAdUiBaseDialog {
	DECLARE_DYNAMIC(dgMLeadProperty)

public:
	dgMLeadProperty(CWnd *pParent = NULL, HINSTANCE hInstance = NULL);

	enum { IDD = IDD_MLEADERPROP };

protected:
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
	afx_msg void OnImport();
	afx_msg void OnStyleOrScaleChanged();
	void fillStyles(const CString& select);
	void updateInfo();

	DECLARE_MESSAGE_MAP()
	CComboBox styles;
	CString fileSource;                 // DWG chosen with "Iz crteža..."
	std::vector<CString> fileStyles;    // styles offered from that DWG
public:

};
