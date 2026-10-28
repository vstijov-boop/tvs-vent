#pragma once
#include "TableScale.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"

namespace tvsTable {
inline Sizing &current() { static Sizing sizing; return sizing; }
inline bool paperSpace() {
    resbuf value = {};
    return acedGetVar(L"CVPORT", &value) == RTNORM && value.resval.rint == 1;
}
inline const ACHAR *scaleRecord() {
    return paperSpace() ? L"TVS_TABLE_SCALE_PAPER" : L"TVS_TABLE_SCALE_MODEL";
}
inline double savedScale() {
    double scale = paperSpace() ? 1.0 : 50.0;
    AcDbDictionary *dict = NULL;
    AcDbDatabase *db = acdbHostApplicationServices()->workingDatabase();
    if (!db || db->getNamedObjectsDictionary(dict,AcDb::kForRead) != Acad::eOk) return scale;
    AcDbObjectId id;
    const Acad::ErrorStatus found = dict->getAt(scaleRecord(),id);
    dict->close();
    if (found != Acad::eOk) return scale;
    AcDbXrecord *record = NULL;
    if (acdbOpenObject(record,id,AcDb::kForRead) != Acad::eOk) return scale;
    resbuf *data = NULL;
    if (record->rbChain(&data) == Acad::eOk)
        for (resbuf *p = data; p; p = p->rbnext)
            if (p->restype == 40 && positive(p->resval.rreal)) scale = p->resval.rreal;
    if (data) acutRelRb(data);
    record->close();
    return scale;
}
inline void saveScale(double scale) {
    AcDbDictionary *dict = NULL;
    AcDbDatabase *db = acdbHostApplicationServices()->workingDatabase();
    if (!db || db->getNamedObjectsDictionary(dict,AcDb::kForWrite) != Acad::eOk) return;
    AcDbXrecord *record = NULL;
    AcDbObjectId id;
    Acad::ErrorStatus status;
    if (dict->getAt(scaleRecord(),id) == Acad::eOk)
        status = acdbOpenObject(record,id,AcDb::kForWrite);
    else {
        record = new AcDbXrecord;
        status = dict->setAt(scaleRecord(),record,id);
        if (status != Acad::eOk) { delete record; record = NULL; }
    }
    dict->close();
    if (status == Acad::eOk) {
        resbuf data = {};
        data.restype = 40; data.resval.rreal = scale;
        record->setFromRbChain(data);
        record->close();
    }
}
inline bool prompt() {
    const double previous = savedScale();
    CString message;
    message.Format(L"\nRazmjera tabele 1:<%g> (50 za 1:50, 1 za 1:1): ",previous);
    double scale = previous;
    acedInitGet(RSG_NOZERO | RSG_NONEG,NULL);
    const int status = acedGetReal(message,&scale);
    if (status != RTNORM && status != RTNONE) return false;
    if (status == RTNONE) scale = previous;
    const Sizing sizing(scale,paperSpace() ? 1.0 : tvsUnits::factor());
    if (!sizing.valid()) {
        acutPrintf(L"\nNeispravna razmjera tabele.");
        return false;
    }
    current() = sizing;
    saveScale(scale);
    acutPrintf(L"\nVisina teksta: 2,5 mm na papiru; %g jedinica crteža.",sizing.textHeight());
    return true;
}
}
