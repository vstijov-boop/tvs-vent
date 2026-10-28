#include "stdafx.h"
#include "graphicHelper.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"

void graphicHelper::drawText(AcGePoint3d cent, const ACHAR * pAchar, double height)
{
	AcDbDatabase *db = acdbHostApplicationServices()->workingDatabase();

	AcDbObjectId tId = db->textstyle();
	///
	AcDbText *text1 = new AcDbText(cent, pAchar, tId, height, 0);

	AcDbBlockTable *pBlockTable;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable,
		AcDb::kForRead);

	AcDbBlockTableRecord *pBlockTableRecord;
	acdbOpenObject(pBlockTableRecord, db->currentSpaceId(), AcDb::kForWrite);
	pBlockTable->close();

	AcDbObjectId retId = AcDbObjectId::kNull;
	pBlockTableRecord->appendAcDbEntity(retId, text1);
	pBlockTableRecord->close();
	text1->close();
	//
}

void graphicHelper::drawLine(AcGePoint3d start, AcGePoint3d end)
{
	AcDbDatabase *db = acdbHostApplicationServices()->workingDatabase();

	AcDbObjectId tId = db->textstyle();
	///
	AcDbLine *pEnt = new AcDbLine(start, end);

	AcDbBlockTable *pBlockTable;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable,
		AcDb::kForRead);

	AcDbBlockTableRecord *pBlockTableRecord;
	acdbOpenObject(pBlockTableRecord, db->currentSpaceId(), AcDb::kForWrite);
	pBlockTable->close();
	pEnt->setLineWeight(AcDb::kLnWt015);
	AcDbObjectId retId = AcDbObjectId::kNull;
	pBlockTableRecord->appendAcDbEntity(retId, pEnt);
	pBlockTableRecord->close();
	pEnt->close();
	//
}
