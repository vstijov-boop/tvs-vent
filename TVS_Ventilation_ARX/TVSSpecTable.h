#pragma once
#include "vector"
#include "TVSSpecTableColumn.h"
#include "TVSPropertySpec.h"

#define CTVSTablePropertyName L"CTVSTableProperty"
#define CTVSDefaultTableProperty L"\
Поз.; 2000;[Позиция]\r\n\
Наименование и техническая характеристика; 13000; [Имя]\r\n\
Тип, марка, обозначение документа, опросного листа; 6000; [Тип] [Размер]\r\n\
Код продукции; 3500; [Артикул]\r\n\
Поставщик; 4500; [Производитель]\r\n\
Ед. изме - ре - ния; 2000; [Ед.Изм]\r\n\
Кол.; 2000;[Количество]\r\n\
Масса 1 ед., кг; 2500; [Масса]\r\n\
Приме- чание; 4000; [Примечание]\r\n\
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


