#pragma once
#include <string>

// Presentation text only. Property tokens and stored object data stay unchanged.
namespace specificationText {
struct Translation {
	const wchar_t *source;
	const wchar_t *target;
};

inline std::wstring translateHeader(const std::wstring &value)
{
	static const Translation headers[] = {
		{L"Поз.", L"Pozicija"},
		{L"Наименование и техническая характеристика", L"Naziv i tehničke karakteristike"},
		{L"Тип, марка, обозначение документа, опросного листа", L"Tip, model i oznaka dokumentacije"},
		{L"Код продукции", L"Šifra proizvoda"},
		{L"Поставщик", L"Dobavljač"},
		{L"Ед. изме - ре - ния", L"Jed. mjere"},
		{L"Кол.", L"Količina"},
		{L"Масса 1 ед., кг", L"Masa/jed., kg"},
		{L"Приме- чание", L"Napomena"},
		{L"Позиция", L"Pozicija"},
		{L"Наименование", L"Naziv"},
		{L"Размер", L"Dimenzije"},
		{L"Типоразмер", L"Tip i dimenzije"},
		{L"Артикул", L"Šifra"},
		{L"Изготовитель", L"Proizvođač"},
		{L"Единица", L"Jed. mjere"},
		{L"Единица2", L"Jed. mjere 2"},
		{L"Кол-во", L"Količina"},
		{L"Масса", L"Masa"},
		{L"Примечание", L"Napomena"}
	};
	for (size_t i = 0; i < sizeof(headers) / sizeof(headers[0]); ++i) {
		if (value == headers[i].source) return headers[i].target;
	}
	return value;
}

// Only the exact header before the first semicolon on each line is translated.
// Keep whitespace, column widths, expressions and line endings byte-for-byte.
inline std::wstring translateTemplate(const std::wstring &source)
{
	std::wstring result;
	std::wstring::size_type start = 0;
	while (start < source.size()) {
		std::wstring::size_type end = source.find(L'\n', start);
		if (end == std::wstring::npos) end = source.size();
		std::wstring row = source.substr(start, end - start);
		std::wstring::size_type separator = row.find(L';');
		if (separator != std::wstring::npos) {
			std::wstring header = row.substr(0, separator);
			std::wstring::size_type first = header.find_first_not_of(L" \t\r");
			if (first != std::wstring::npos) {
				std::wstring::size_type last = header.find_last_not_of(L" \t\r");
				row = header.substr(0, first)
					+ translateHeader(header.substr(first, last - first + 1))
					+ header.substr(last + 1) + row.substr(separator);
			}
		}
		result += row;
		if (end == source.size()) break;
		result += L'\n';
		start = end + 1;
	}
	return result;
}

inline std::wstring translateUnit(const std::wstring &value)
{
	static const Translation units[] = {
		{L"шт", L"kom"}, {L"шт.", L"kom"},
		{L"м", L"m"}, {L"м2", L"m²"}, {L"м²", L"m²"},
		{L"м3", L"m³"}, {L"м³", L"m³"}, {L"кг", L"kg"}
	};
	for (size_t i = 0; i < sizeof(units) / sizeof(units[0]); ++i) {
		if (value == units[i].source) return units[i].target;
	}
	return value;
}
}
