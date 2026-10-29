#include "stdafx.h"
#include "TVSCommandSpecification.h"
#include "TVSPropertySpec.h"
#include "TVSSpecTable.h"
#include "TablePresentation.h"
#include "CoordinateInput.h"
TVSCommandSpecification::TVSCommandSpecification()
{
}

TVSCommandSpecification::~TVSCommandSpecification()
{
}

void TVSCommandSpecification::execute(void)
{
	ads_name sset;
	if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
		return;

	Int32 len;
	acedSSLength(sset, &len);
	if (len == 0)
	{
		acedSSFree(sset);
		return;
	}
	
	ads_point pt1;
	if (!tvsTable::prompt()) { acedSSFree(sset); return; }
	TVSSpecTable table;
	int status = 0;

	while (status != RTNORM) {
		acedInitGet(RSG_NONULL, _T("Podešavanja P"));
		status = tvsCoordinates::getPoint(NULL, _T("\nZadajte tačku umetanja specifikacije ili [Podešavanja]: "), pt1);
		switch (status)
		{
		case RTCAN:
			acedSSFree(sset);
			return;
			break;
		case RTKWORD:
			table.showPropsDlg();
			break;
		}
	}

	TVSPropertySpec spec;
	spec.processSetOfObjects(sset);
	acedSSFree(sset);
	spec.sort();
	
	table.loadColumnsFromSettings();
	table.buildDataFromSpec(spec);
	table.toSimpleTable(asPnt3d(pt1));
}

