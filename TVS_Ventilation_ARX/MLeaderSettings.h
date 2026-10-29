#pragma once
#include "arxHeaders.h"
#include "TVS_Entity.h"
#define MLeaderID _T("TVS_MLeaderID")
#define MLeaderFormat _T("TVS_MLeaderFormat")
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
";
		pipeLeader = L"[D]";
		blockLeader = L"[Тип]-[Размер]";

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
};
