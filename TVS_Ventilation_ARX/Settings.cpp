#include "stdafx.h"
#include "Settings.h"
Settings settings;

// The one definition of the drawing defaults declared in Settings.h.
double globSizeA = 150;
double globSizeB = 150;
bool globRound = false;
bool globalWipeout = false;
bool globalGrani = false;
bool global1D = false;
double ftSizeB = 100;
double ftSizeroundA = 160;
double ftLengthp = 150;
int ftTransType = 1;
int globalMlead = 1;
int globalRadiusTypeRound = RadiusTypeRound_RadiusVariable;
int globalRadiusTypeRect = RadiusTypeRect_RadiusVariable;
int globalTypeRoundTap = 1;
int globalTapForm = Form_Direct;
int globalTransType = 0;
double globalTapRadiusVariableParameter = 0.5;
double globalTapRadiusConst = 150;
double globalFlow = 0;
double globalLengthTr = 300;
double globalLengthW = 50;
double globalElevMid = 0;
int globalElevRef = 0; // TVS_SETTINGS level reference: 0 axis (Centar), 1 top (Vrh), 2 bottom (Dno); globalElevMid stays the axis
double globalElev = globalElevdown;
double globalAxis = 90;
int globalDuctType = DuctTypeStill;
double globalWipeoutLength = 1;   // Maskiranje (mm): default 1

// Geometry defaults follow the active DWG; dialog and COM values remain in mm.
double tvsGlobalUnitsFactor = 1.0;
void syncTVSGlobalUnits() {
    const double next = tvsUnits::factor();
    if (next == tvsGlobalUnitsFactor) return;
    double* values[] = { &globSizeA, &globSizeB, &ftSizeB, &ftSizeroundA, &ftLengthp,
        &globalTapRadiusConst, &globalLengthTr, &globalLengthW, &globalElevMid,
        &globalWipeoutLength };
    for (size_t i = 0; i < sizeof(values)/sizeof(values[0]); ++i)
        *values[i] *= tvsGlobalUnitsFactor / next;
    tvsGlobalUnitsFactor = next;
}


Settings::Settings(void)
{
}


Settings::~Settings(void)
{
}

Settings* Settings::get()
{
	return &settings;
}
