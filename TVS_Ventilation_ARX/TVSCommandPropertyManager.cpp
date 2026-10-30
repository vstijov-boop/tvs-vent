#include "stdafx.h"
#include "TVSCommandPropertyManager.h"
#include "MleaderController.h"
#include "TVSPropertySpec.h"
#include "dgTVSPropertyManager.h"

TVSCommandPropertyManager::TVSCommandPropertyManager()
{
}


TVSCommandPropertyManager::~TVSCommandPropertyManager()
{
}

void TVSCommandPropertyManager::execute(void)
{
	ads_name sset;

	if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
		return;
	TVSPropertySpec spec;
	#ifdef TVS_OFFSET_INTEGRATION
    if(!spec.processSetOfObjects(sset)){acedSSFree(sset);return;}
#else
    spec.processSetOfObjects(sset);
#endif
	spec.sort();
	if (spec.rows.size() > 0) {
		dgTVSPropertyManager dg;
		dg.propertySpec = spec;
		dg.DoModal();
	}
}
