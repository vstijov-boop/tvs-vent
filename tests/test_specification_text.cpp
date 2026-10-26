#include "../TVS_Ventilation_ARX/SpecificationText.h"
#include "../TVS_Ventilation_DBX/commonDef.h"
#include <cassert>
#include <iostream>

int main()
{
	using namespace specificationText;
	const std::wstring original =
		L"Поз.; 2000;[Позиция]\r\n"
		L"Наименование и техническая характеристика; 13000; [Имя]\r\n"
		L"Тип, марка, обозначение документа, опросного листа; 6000; [Тип] [Размер]\r\n"
		L"Код продукции; 3500; [Артикул]\r\n"
		L"Поставщик; 4500; [Производитель]\r\n"
		L"Ед. изме - ре - ния; 2000; [Ед.Изм]\r\n"
		L"Кол.; 2000;[Количество]\r\n"
		L"Масса 1 ед., кг; 2500; [Масса]\r\n"
		L"Приме- чание; 4000; [Примечание]\r\n";
	const std::wstring expected =
		L"Pozicija; 2000;[Позиция]\r\n"
		L"Naziv i tehničke karakteristike; 13000; [Имя]\r\n"
		L"Tip, model i oznaka dokumentacije; 6000; [Тип] [Размер]\r\n"
		L"Šifra proizvoda; 3500; [Артикул]\r\n"
		L"Dobavljač; 4500; [Производитель]\r\n"
		L"Jed. mjere; 2000; [Ед.Изм]\r\n"
		L"Količina; 2000;[Количество]\r\n"
		L"Masa/jed., kg; 2500; [Масса]\r\n"
		L"Napomena; 4000; [Примечание]\r\n";
	assert(translateTemplate(original) == expected);
	assert(translateTemplate(expected) == expected);
	assert(translateTemplate(L"").empty());
	assert(translateTemplate(L"\n\r\n") == L"\n\r\n");
	assert(translateTemplate(L"Bez separatora") == L"Bez separatora");
	assert(translateTemplate(L" ; 2000; [Имя]") == L" ; 2000; [Имя]");

	// Custom widths, whitespace, line endings and expressions must survive.
	const std::wstring custom =
		L" \tНаименование \t; 4321.5; [Имя]-Poseban tekst; nastavak\n"
		L"Moj naslov; 9876; [Тип]-[Размер]\r\n"
		L"Наименование opreme; 3333; [Имя]\n"
		L"Napomena; 321; Наименование; [Примечание]\n"
		L"Кол. ;  1234 ; [Количество]";
	const std::wstring customExpected =
		L" \tNaziv \t; 4321.5; [Имя]-Poseban tekst; nastavak\n"
		L"Moj naslov; 9876; [Тип]-[Размер]\r\n"
		L"Наименование opreme; 3333; [Имя]\n"
		L"Napomena; 321; Наименование; [Примечание]\n"
		L"Količina ;  1234 ; [Количество]";
	assert(translateTemplate(custom) == customExpected);
	assert(translateTemplate(customExpected) == customExpected);
	assert(translateHeader(L"Poseban naziv projekta") == L"Poseban naziv projekta");
	assert(translateUnit(L"шт") == L"kom");
	assert(translateUnit(L"шт.") == L"kom");
	assert(translateUnit(L"м") == L"m");
	assert(translateUnit(L"м2") == L"m²");
	assert(translateUnit(L"м²") == L"m²");
	assert(translateUnit(L"м3") == L"m³");
	assert(translateUnit(L"м³") == L"m³");
	assert(translateUnit(L"кг") == L"kg");
	assert(translateUnit(L"kom") == L"kom");
	assert(translateUnit(L"komplet") == L"komplet");
	assert(translateUnit(L"шт posebna oznaka") == L"шт posebna oznaka");
	assert(translateUnit(L"").empty());
	assert(std::wstring(CCommonLength_Unit) == L"m");
	assert(std::wstring(CCommonCount_Unit) == L"kom");
	assert(std::wstring(CCommonPipeName) == L"Kanal");
	assert(std::wstring(CCommonFlexPipeName) == L"Fleksibilni kanal");
	assert(std::wstring(CCommonTapName) == L"Koljeno");
	assert(std::wstring(CCommonWueName) == L"T-komad");
	assert(std::wstring(CCommonTransName) == L"Prelaz");
	std::cout << "Specification translation tests passed.\n";
	return 0;
}
