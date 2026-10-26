#pragma once
#include "vector"
#include "TVSSpecTableColumn.h"
#include "TVSPropertySpec.h"

#define CTVSTablePropertyName L"CTVSTableProperty"
#define CTVSDefaultTableProperty L"\
Pozicija; 2000;[Позиция]\r\n\
Naziv i tehničke karakteristike; 13000; [Имя]\r\n\
Tip, model i oznaka dokumentacije; 6000; [Тип] [Размер]\r\n\
Šifra proizvoda; 3500; [Артикул]\r\n\
Dobavljač; 4500; [Производитель]\r\n\
Jed. mjere; 2000; [Ед.Изм]\r\n\
Količina; 2000;[Количество]\r\n\
Masa/jed., kg; 2500; [Масса]\r\n\
Napomena; 4000; [Примечание]\r\n\
"

#define CTVSInstructionTableProperty L"\
Svaki red šablona određuje jednu kolonu specifikacije:\r\n\
Naziv kolone; Širina; [Oznaka svojstva]\r\n\
Primjer: Tip i dimenzije; 6000; [Тип]-[Размер]\r\n\
Tačka-zarez (;) razdvaja naziv, širinu i sadržaj.\r\n\
Naziv kolone - tekst u zaglavlju tabele.\r\n\
Širina - širina kolone u jedinicama AutoCAD-a.\r\n\
Sadržaj - tekst i jedna ili više oznaka svojstava.\r\n\
Oznake u uglastim zagradama ostaju na ruskom:\r\n\
[Позиция] - Pozicija; [Имя] - Naziv\r\n\
[Тип] - Tip; [Размер] - Dimenzije\r\n\
[Артикул] - Šifra; [Производитель] - Proizvođač\r\n\
[Ед.Изм] - Jedinica mjere; [Количество] - Količina\r\n\
[Масса] - Masa; [Примечание] - Napomena\
"


using namespace std;
class TVSSpecTable
{
public:
	TVSSpecTable();
	int rowsPerList;
	bool paginating;
	double titleHeight;
	double rowHeight;
	double textHeight;
	static bool getRowSettings(CString &rowSettings);
	static bool setRowSettings(const CString &rowSettings);
	bool loadColumnsFromSettings();
	bool buildDataFromSpec(const TVSPropertySpec &spec);
	bool toSimpleTable(const AcGePoint3d &startPoint);
	bool toExcel();
	void showPropsDlg();
	vector<TVSSpecTableColumn> columns;
	vector<vector<CString>> tableData;
	virtual ~TVSSpecTable();
private:
	bool getRowData(const TVSPropertySpecRow &row, const TVSSpecTableColumn &column, CString &data);

};


