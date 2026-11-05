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
//----- hangeZdg.h : Declaration of the dgChangeZ
//-----------------------------------------------------------------------------
#pragma once
// Promjena kote Z (etaz): existing top / axis / bottom of the duct, new level by reference
// (Vrh / Osa / Dno), angle, and at once the result of the same plan TVS_OFFSET writes
// (difference, sloped piece, length in plan, or why it is not possible) with a side sketch.
//-----------------------------------------------------------------------------
#include "adui.h"
#include "../geometry/OffsetGeometry.h"
#include <functional>

//-----------------------------------------------------------------------------
class dgChangeZ : public CAdUiBaseDialog {
	DECLARE_DYNAMIC(dgChangeZ)

public:
	dgChangeZ(CWnd *pParent = NULL, HINSTANCE hInstance = NULL);

	enum { IDD = IDD_HANGEZDG };
	double startZ, nextZ, Axis;   // axis levels (drawing units), angle in degrees
	double height;                // duct height for top / bottom (drawing units); 0 = axis only
	// Plan for the new axis level and angle: true + geometry (mm), or false + reason.
	std::function<bool(double next, double angle, tvsOffset::Result& g, CString& why)> evaluate;

protected:
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg LRESULT OnAcadKeepFocus(WPARAM, LPARAM);
	afx_msg void OnChanged();
	afx_msg void OnDrawItem(int id, LPDRAWITEMSTRUCT item);
	int reference() const;
	bool read(double& axis, double& angle) const;
	void update();
	bool ready;
	bool feasible;
	tvsOffset::Result geometry;

	DECLARE_MESSAGE_MAP()
};
