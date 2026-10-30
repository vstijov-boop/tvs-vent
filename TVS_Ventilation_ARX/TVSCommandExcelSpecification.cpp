#include "stdafx.h"
#include "TVSCommandExcelSpecification.h"
#include "TVSPropertySpec.h"
#include "TVSSpecTable.h"

TVSCommandExcelSpecification::TVSCommandExcelSpecification()
{
}

TVSCommandExcelSpecification::~TVSCommandExcelSpecification()
{
}

void TVSCommandExcelSpecification::execute(void)
{
	ads_name sset;
	if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
		return;

	Int32 len;
	acedSSLength(sset, &len);
	if (len == 0)
	{
		return;
	}

	TVSPropertySpec spec;
	#ifdef TVS_OFFSET_INTEGRATION
    if(!spec.processSetOfObjects(sset)){acedSSFree(sset);return;}
#else
    spec.processSetOfObjects(sset);
#endif
	spec.sort();
	TVSSpecTable table;
	table.loadColumnsFromSettings();
	table.buildDataFromSpec(spec);
	table.toExcel();
}
