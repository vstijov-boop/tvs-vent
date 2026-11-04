#include "stdafx.h"
#include "TVSSpecTable.h"
#include "stringHelper.h"
#include "graphicHelper.h"
#include "MSExcel.h"	
#include "dgSpecProperty.h"
#include "SpecificationText.h"
#include "TablePresentation.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
TVSSpecTable::TVSSpecTable() :
	rowsPerList(21),
	titleHeight(3200 * tvsTable::current().templateFactor()),
	rowHeight(800 * tvsTable::current().templateFactor()),
	textHeight(tvsTable::current().textHeight()),
	paginating(false)
{
}

TVSSpecTable::~TVSSpecTable()
{
}

bool TVSSpecTable::getRowSettings(CString &rowSettings)
{
	ACHAR stringProperty[2048];
	if (acedGetEnv(CTVSTablePropertyName, stringProperty) == RTNORM) {
		rowSettings = stringProperty;
	}
	else {
		rowSettings = CTVSDefaultTableProperty;
	}
	// Localize the returned view; do not overwrite saved custom settings.
	rowSettings = specificationText::translateTemplate((LPCTSTR)rowSettings).c_str();
	return true;
}

bool TVSSpecTable::setRowSettings(const CString &rowSettings)
{
	if (acedSetEnv(CTVSTablePropertyName, rowSettings) == RTNORM) {
		return true;
	}
	else {
		return false;
	}
}

bool TVSSpecTable::loadColumnsFromSettings()
{
    columns.clear();
	CString tvsTablePropertyString;
	if (!getRowSettings(tvsTablePropertyString)) return false;

	vector <CString> rows;
	stringHelper::splitStringIntoVector(tvsTablePropertyString, L"\n", rows);
	for (CString &row : rows) {
		vector<CString> rowStringVector;
		stringHelper::splitStringIntoVector(row, L";", rowStringVector);
		if (rowStringVector.size() == 3)
		{
			TVSSpecTableColumn col;
			col.name = rowStringVector[0].Trim(L"\n\r\t ;");
			col.width = _tstof(rowStringVector[1].Trim(L"\n\r\t ;")) * tvsTable::current().templateFactor();
			col.value = rowStringVector[2].Trim(L"\n\r\t ;");
			columns.push_back(col);
		}
	}
    // Always include total mass, including drawings with an older saved template.
    bool hasTotalMass = false;
    for (const auto &column : columns)
        if (column.value.Find(L"[UkupnaMasa]") >= 0) hasTotalMass = true;
    if (!hasTotalMass) {
        TVSSpecTableColumn column;
        column.name = L"Masa, kg";
        column.width = 2500 * tvsTable::current().templateFactor();
        column.value = L"[UkupnaMasa]";
        columns.push_back(column);
    }
	return true;
}

bool TVSSpecTable::buildDataFromSpec(const TVSPropertySpec &spec)
{
	int colIdx = 0;
	int rowIdx = 0;
	tableData.resize(spec.rows.size());
	for (auto &row : spec.rows) {
		colIdx = 0;
		tableData[rowIdx].resize(columns.size());
		for (auto &column : columns) {
			CString data;
			if (getRowData(row, column, data)) {
				tableData[rowIdx][colIdx] = data;
			}
			colIdx++;
		}
		rowIdx++;
	}
	return true;
}

bool TVSSpecTable::toSimpleTable(const AcGePoint3d &startPoint)
{
	double width = 0;
	for (auto &col : columns) {
		width += col.width;
	}
	double height = titleHeight + tableData.size()*rowHeight;

	//drawVertical lines
	AcGePoint3d curStartPoint = startPoint;
	AcGePoint3d curEndPoint = curStartPoint;
	curEndPoint.y -= height;
	graphicHelper::drawLine(curStartPoint, curEndPoint);
	for (int i = 0; i < columns.size(); i++)
	{
		curStartPoint.x += columns[i].width;
		curEndPoint.x += columns[i].width;
		graphicHelper::drawLine(curStartPoint, curEndPoint);
	}

	//draw horizontal lines
	curStartPoint = startPoint;
	curEndPoint = curStartPoint;
	curEndPoint.x += width;
	//draw title
	graphicHelper::drawLine(curStartPoint, curEndPoint);
	curStartPoint.y -= titleHeight;
	curEndPoint.y -= titleHeight;
	for (int i = 0; i <= tableData.size(); i++)
	{
		graphicHelper::drawLine(curStartPoint, curEndPoint);
		curStartPoint.y -= rowHeight;
		curEndPoint.y -= rowHeight;

	}

	//draw text;
	int colIdx = 0;
	int rowIdx = 0;
	double otstupX = 100 * tvsTable::current().templateFactor();
	double otstupY = 150 * tvsTable::current().templateFactor();
	double curY = startPoint.y - titleHeight + otstupY;
	double curX = startPoint.x + otstupX;
	//draw title;
	for (int i = 0; i < columns.size(); i++) {
		graphicHelper::drawText(AcGePoint3d(curX, curY, startPoint.z), columns[i].name, textHeight);
		curX += columns[i].width;
	}

	for (auto &row : tableData) {
		colIdx = 0;
		curX = startPoint.x + otstupX;
		curY -= rowHeight;
		for (auto &column : row) {
			graphicHelper::drawText(AcGePoint3d(curX, curY, startPoint.z), column, textHeight);
			curX += columns[colIdx].width;
			colIdx++;
		}
	}
	return true;
}

bool TVSSpecTable::toExcel()
{
	CMSExcel m_msExcel;
	acedSetStatusBarProgressMeter(_T("Kreiranje Excel dokumenta"), 0, 100);
	if (FAILED(m_msExcel.NewExcelBook(false) == E_FAIL))
	{
		acedRestoreStatusBar();
		acutPrintf(_T("\n Greška pri kreiranju Excel dokumenta."));
		return false;
	}

	acedSetStatusBarProgressMeter(_T("Izvoz u Excel"), 0, 100);
	const ACHAR* excelColumns = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	//title;
	int curRowIdx = 1;
	for (int i = 0; i < columns.size(); i++) {
		CString curCol(i < 26 ? excelColumns[i] : excelColumns[25]);
		m_msExcel.printExelText(curCol.AllocSysString(), curRowIdx, columns[i].name);
	}
	int curColIdx = 0;
	curRowIdx++;
	//rows
	for (auto &row : tableData) {
		curColIdx = 0;
		for (auto &column : row) {
			CString curCol(curColIdx < 26 ? excelColumns[curColIdx] : excelColumns[25]);
			column.Replace(L"%%c", L"⌀");
			m_msExcel.printExelText(curCol.AllocSysString(), curRowIdx, column);
			curColIdx++;
		}
		acedSetStatusBarProgressMeter(_T("Izvoz podataka u Excel"), curRowIdx, tableData.size());
		curRowIdx++;
	}

	m_msExcel.SetVisible(true);
	acedRestoreStatusBar();
	return true;
}

void TVSSpecTable::showPropsDlg()
{
	dgSpecProperty dlg;
	dlg.instructionMessage = CTVSInstructionTableProperty;
	getRowSettings(dlg.specProps);
	dlg.DoModal();
	setRowSettings(dlg.specProps);
}

bool TVSSpecTable::getRowData(const TVSPropertySpecRow &row, const TVSSpecTableColumn &column, CString &data)
{
	data = column.value;
	data.Replace(CTVSProperty_position_val, row.tvsPropery.position);
	data.Replace(CTVSProperty_name_val, row.tvsPropery.name);
	data.Replace(CTVSProperty_type_val, row.tvsPropery.type);
	data.Replace(CTVSProperty_size_val, row.tvsPropery.size);
	data.Replace(CTVSProperty_article_val, row.tvsPropery.article);
	data.Replace(CTVSProperty_manufacturer_val, row.tvsPropery.manufacturer);
	CString displayUnits = specificationText::translateUnit((LPCTSTR)row.tvsPropery.units).c_str();
	data.Replace(CTVSProperty_units_val, displayUnits);
	CString temp;
	temp.Format(L"%g", row.tvsPropery.count);
	data.Replace(CTVSProperty_count_val, temp);
	temp.Format(L"%g", row.tvsPropery.mass);
	data.Replace(CTVSProperty_mass_val, temp);
    temp.Format(L"%.3f", row.tvsPropery.hasSheetMetal ? row.tvsPropery.totalMass
        : row.tvsPropery.mass * row.tvsPropery.count);
    data.Replace(L"[UkupnaMasa]", temp);
    temp = L"";
    if (row.tvsPropery.hasSheetMetal) temp.Format(L"%.3f", row.tvsPropery.sheetArea);
    data.Replace(L"[Povrsina]", temp);
    temp = L"";
    if (row.tvsPropery.hasSheetMetal) temp.Format(L"%.1f", row.tvsPropery.sheetThickness);
    data.Replace(L"[Debljina]", temp);
	data.Replace(CTVSProperty_note_val, row.tvsPropery.note);
	data.Trim(L"\n\r\t ;-");
	return true;
}
