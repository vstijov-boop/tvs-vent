#include "stdafx.h"
#include "TVSCommandLeadRecalculate.h"
#include "MleaderController.h"

TVSCommandLeadRecalculate::TVSCommandLeadRecalculate()
{
}


TVSCommandLeadRecalculate::~TVSCommandLeadRecalculate()
{
}

// TVS_LEAD_Recalculate: the TVS labels (MLeader with the element link in its xdata) get their
// text again from their element. Pre-selected objects are used as they are; otherwise
// [Sve/Izbor]: Sve = every MLeader in the drawing, Izbor = ordinary selection (pick, window,
// crossing; other objects are skipped). Prints how many were updated and how many not
// (element erased, label on a locked layer).
void TVSCommandLeadRecalculate::execute(void)
{
	ads_name sset;
	bool have = acedSSGet(_T("_I"), NULL, NULL, NULL, sset) == RTNORM;   // pickfirst
	if (!have)
	{
		acedInitGet(0, _T("Sve Izbor"));
		ACHAR kw[32] = _T("");
		const int rc = acedGetKword(_T("\nTVS oznake za ažuriranje [Sve/Izbor] <Izbor>: "), kw, 32);
		if (rc != RTNORM && rc != RTNONE) return;
		if (rc == RTNORM && _wcsicmp(kw, _T("Sve")) == 0)
		{
			resbuf filter = {};
			filter.restype = 0;
			filter.resval.rstring = const_cast<ACHAR*>(_T("MULTILEADER"));
			if (acedSSGet(_T("_X"), NULL, NULL, &filter, sset) != RTNORM)
			{
				acutPrintf(_T("\nU crtežu nema MLeader oznaka."));
				return;
			}
		}
		else if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;
	}

	Int32 len = 0;
	acedSSLength(sset, &len);
	MleaderController mlctrl;
	int updated = 0, failed = 0;
	for (Int32 i = 0; i < len; i++)
	{
		ads_name eName;
		AcDbObjectId mLeaderId, objectId;
		if (acedSSName(sset, i, eName) != RTNORM || acdbGetObjectId(mLeaderId, eName) != Acad::eOk)
			continue;
		if (!mlctrl.checkIfMLead(mLeaderId) || !mlctrl.getObjectId(mLeaderId, objectId))
			continue;   // not a TVS label
		if (mlctrl.recalculateMleader(mLeaderId)) ++updated; else ++failed;
	}
	acedSSFree(sset);
	if (failed)
		acutPrintf(_T("\nAžurirano TVS oznaka: %d; nije ažurirano: %d (element obrisan ili oznaka na zaključanom sloju)."), updated, failed);
	else
		acutPrintf(_T("\nAžurirano TVS oznaka: %d."), updated);
}
