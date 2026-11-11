#include "stdafx.h"
#include "TVSCommandAttributesToProperties.h"
#include "TVSAttrtibutesController.h"

TVSCommandAttributesToProperties::TVSCommandAttributesToProperties()
{
}


TVSCommandAttributesToProperties::~TVSCommandAttributesToProperties()
{
}

// Only the selected block references (pickfirst or a selection), after a confirmation: the
// attributes become TVS properties and are then removed from the blocks.
void TVSCommandAttributesToProperties::execute(void)
{
	ads_name sset;
	acutPrintf(L"\nIzaberite blokove čije atribute treba pretvoriti u TVS svojstva: ");
	if (acedSSGet(NULL, NULL, NULL, NULL, sset) != RTNORM)
		return;
	Int32 len = 0;
	ads_name  eName;
	AcDbObjectId objectId;
	acedSSLength(sset, &len);
	AcDbObjectIdArray blocks;
	for (Int32 i = 0; i < len; i++)
	{
		if (acedSSName(sset, i, eName) == RTNORM && acdbGetObjectId(objectId, eName) == Acad::eOk
			&& objectId.objectClass() && objectId.objectClass()->isDerivedFrom(AcDbBlockReference::desc()))
			blocks.append(objectId);
	}
	acedSSFree(sset);
	if (blocks.isEmpty()) { acutPrintf(L"\nU izboru nema blokova."); return; }
	wchar_t prompt[200];
	swprintf_s(prompt, L"\nAtributi %d blokova postaju TVS svojstva i brišu se iz blokova. Nastaviti? [Da/Ne] <Ne>: ", blocks.length());
	acedInitGet(0, L"Da Ne");
	ACHAR kw[16] = L"";
	if (acedGetKword(prompt, kw, 16) != RTNORM || _wcsicmp(kw, L"Da") != 0) { acutPrintf(L"\nNišta nije promijenjeno."); return; }
	for (int i = 0; i < blocks.length(); i++)
	{
		TVSAttrtibutesController ctrl;
		ctrl.convertAttributesToProperties(blocks[i]);
	}
	for (int i = 0; i < blocks.length(); i++)
	{
		TVSAttrtibutesController ctrl;
		ctrl.removeAttributes(blocks[i]);
	}
	acutPrintf(L"\nObrađeno blokova: %d.", blocks.length());
}
