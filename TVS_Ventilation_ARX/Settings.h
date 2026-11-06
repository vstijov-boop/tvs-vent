#pragma once
#include "TVS_TAP.h"
#include "MLeaderSettings.h"
#include "TVSPropertySettings.h"

#define globalElevUp 1
#define globalElevmid 2
#define globalElevdown 3
#define M_PI 3.14159265358979323846
#define dCONTINUE 0
#define dCONNECT 1
#define dSIZE 2
#define dParal 1
#define dPerpend 0
#define dPeresec 2
#define dNeperesec 3
#define dComplanar 4
#define dObshaiztochka 5
#define isBlocked 0
#define isNoTVS 1
#define isTVS_Pipe 2
#define isTVS_TAP 3
#define isTVS_TRANS 4
#define isTVS_Wye 5
#define TVSEnt1isGrose 0
#define TVSEnt2isGrose 1
#define TVSEntitiesisSame 2

#define TagPos _T("ПОЗИЦИЯ")
#define TagName _T("ИМЯ")
#define TagType _T("ТИП")
#define TagSize _T("РАЗМЕР")
#define TagArticle _T("АРТИКУЛ")
#define TagManufacture _T("ПРОИЗВОДИТЕЛЬ")
#define TagMass _T("МАССА")
#define TagCommit _T("ПРИМЕЧАНИЕ")



// Drawing defaults (TVS_SETTINGS, last used sizes ...): one shared copy for the whole module,
// defined in Settings.cpp. (They used to be `static` here, so every .cpp had its own copy and a
// value set in one part of the program was not seen in another.)
extern double globSizeA;
extern double globSizeB;
extern bool globRound;
extern bool globalWipeout;
extern bool globalGrani;
extern bool global1D;
extern double ftSizeB;
extern double ftSizeroundA;
extern double ftLengthp;
extern int ftTransType;
extern int globalMlead;
extern int globalRadiusTypeRound;
extern int globalRadiusTypeRect;
extern int globalTypeRoundTap;
extern int globalTapForm;
extern int globalTransType;
extern double globalTapRadiusVariableParameter;
extern double globalTapRadiusConst;
extern double globalFlow;
extern double globalLengthTr;
extern double globalLengthW;
extern double globalElevMid;
extern int globalElevRef; // TVS_SETTINGS level reference: 0 axis (Centar), 1 top (Vrh), 2 bottom (Dno); globalElevMid stays the axis
extern double globalElev;
extern double globalAxis;
extern int globalDuctType;
extern double globalWipeoutLength;   // Maskiranje (mm): default 1

class Settings
{
public:
	Settings(void);
	virtual ~Settings(void);
	static Settings* get();
	TVSPropertySettings tvsProperty;
	MLeaderSettings mLeader;
};



// Geometry defaults follow the active DWG; dialog and COM values remain in mm (Settings.cpp).
extern double tvsGlobalUnitsFactor;
void syncTVSGlobalUnits();
