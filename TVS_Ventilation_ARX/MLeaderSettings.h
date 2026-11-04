#pragma once
#include "arxHeaders.h"
#include "TVS_Entity.h"
#include <windows.h>
#define MLeaderID _T("TVS_MLeaderID")
#define MLeaderFormat _T("TVS_MLeaderFormat")
// Settings of TVS multileader labels. Kept for the user (HKCU\Software\TVS_Vent\MLeader),
// so the format, style and scale survive AutoCAD restarts. Style heights are paper mm; the
// label's overall scale = drawing scale / drawing units (mm per unit).
class MLeaderSettings
{
public:
	MLeaderSettings(void) {
		instructionMessage = L"\
[L] - Protok, m³/h\r\n\
[V] - Brzina, m/s\r\n\
[D] - Dimenzije, mm\r\n\
[M] - Masa elementa, kg\r\n\
[A] - Površina lima, m²\r\n\
[T] - Debljina lima, mm\r\n\
[P] - Jedinični pad, Pa/m (kanal)\r\n\
[BN] - Naziv bloka\r\n\
[N] - Naziv sloja\r\n\
[Naziv atributa] - Vrijednost\r\n\
atributa bloka. Koristite stvarni\r\n\
naziv postojećeg atributa.\r\n\
\\P - novi red, \\U+00B3 - ³\r\n\
";
		pipeLeader = L"[D]mm\\P[L]m\\U+00B3/h";
		blockLeader = L"[Тип]-[Размер]";
		scale = 50;
		style = L"TVS_MleaderStyle";   // shipped in TVS_Oznake.dwg (text style TVS_TextStyle)
		load();
	};

	CString & getFormatForEntity(const AcDbObjectId &id) {

		AcDbObjectPointer<AcDbEntity>pEnt(id, AcDb::kForRead);
		TVS_Entity * ent;
		if ((ent = TVS_Entity::cast(pEnt)) != NULL)
			return pipeLeader;

		return blockLeader;
	}
	virtual ~MLeaderSettings(void) {};
	CString  instructionMessage;
	CString  pipeLeader;
	CString  blockLeader;
	CString  style;        // multileader style name ("" = drawing's current style)
	CString  styleSource;  // DWG the style was taken from ("" = shipped TVS_Oznake.dwg)
	double   scale;        // drawing scale 1:scale

	static const wchar_t* key() { return L"Software\\TVS_Vent\\MLeader"; }
	void load() {
		HKEY k = NULL; if (RegOpenKeyExW(HKEY_CURRENT_USER, key(), 0, KEY_READ, &k) != ERROR_SUCCESS) return;
		readString(k, L"PipeFormat", pipeLeader); readString(k, L"BlockFormat", blockLeader);
		readString(k, L"Style", style); readString(k, L"StyleSource", styleSource);
		CString s; if (readString(k, L"Scale", s)) { const double v = _wtof(s); if (v > 0 && v <= 100000) scale = v; }
		RegCloseKey(k);
	}
	void save() const {
		HKEY k = NULL; if (RegCreateKeyExW(HKEY_CURRENT_USER, key(), 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) return;
		writeString(k, L"PipeFormat", pipeLeader); writeString(k, L"BlockFormat", blockLeader);
		writeString(k, L"Style", style); writeString(k, L"StyleSource", styleSource);
		CString s; s.Format(L"%.6g", scale); writeString(k, L"Scale", s);
		RegCloseKey(k);
	}
private:
	static bool readString(HKEY k, const wchar_t* name, CString& out) {
		DWORD type = 0, size = 0; if (RegQueryValueExW(k, name, NULL, &type, NULL, &size) != ERROR_SUCCESS || type != REG_SZ || size > 65536) return false;
		CString v; wchar_t* buf = v.GetBuffer(size / sizeof(wchar_t) + 1);
		const LONG rc = RegQueryValueExW(k, name, NULL, NULL, reinterpret_cast<LPBYTE>(buf), &size);
		buf[size / sizeof(wchar_t)] = 0; v.ReleaseBuffer(); if (rc != ERROR_SUCCESS) return false; out = v; return true;
	}
	static void writeString(HKEY k, const wchar_t* name, const CString& v) {
		RegSetValueExW(k, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(v.GetString()), static_cast<DWORD>((v.GetLength() + 1) * sizeof(wchar_t)));
	}
};
