#pragma once
#include "UnitConversion.h"
#include "dbdict.h"
#include "dbxrecrd.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "acutads.h"

namespace tvsUnits {
// This Xrecord is also read by KGTEZ.lsp. No entry means legacy millimetres.
inline const ACHAR* recordName() { return L"TVS_DRAWING_UNITS"; }
inline double factor(AcDbDatabase* db = NULL) {
    if (!db) db = acdbHostApplicationServices()->workingDatabase();
    if (!db) return 1.0;
    AcDbDictionary* dict = NULL;
    if (db->getNamedObjectsDictionary(dict, AcDb::kForRead) != Acad::eOk) return 1.0;
    AcDbObjectId id;
    Acad::ErrorStatus es = dict->getAt(recordName(), id);
    dict->close();
    if (es != Acad::eOk) return 1.0;
    AcDbXrecord* record = NULL;
    if (acdbOpenObject(record, id, AcDb::kForRead) != Acad::eOk) return 1.0;
    resbuf* data = NULL;
    es = record->rbChain(&data);
    record->close();
    double result = 1.0;
    if (es == Acad::eOk) {
        for (resbuf* p = data; p; p = p->rbnext)
            if (p->restype == 40 && validFactor(p->resval.rreal)) result = p->resval.rreal;
    }
    if (data) acutRelRb(data);
    return result;
}
inline double factor(const AcDbObject* obj) { return factor(obj ? obj->database() : NULL); }
inline double mm(double value, const AcDbObject* obj) { return value * factor(obj); }
inline double drawing(double value, const AcDbObject* obj) { return value / factor(obj); }
inline double metres(double value, const AcDbObject* obj) { return value * factor(obj) / 1000.0; }

inline Acad::ErrorStatus store(double value, AcDbDatabase* db = NULL) {
    if (!validFactor(value)) return Acad::eInvalidInput;
    if (!db) db = acdbHostApplicationServices()->workingDatabase();
    if (!db) return Acad::eNoDatabase;
    AcDbDictionary* dict = NULL;
    Acad::ErrorStatus es = db->getNamedObjectsDictionary(dict, AcDb::kForWrite);
    if (es != Acad::eOk) return es;
    AcDbObjectId id;
    AcDbXrecord* record = NULL;
    if (dict->getAt(recordName(), id) == Acad::eOk) {
        es = acdbOpenObject(record, id, AcDb::kForWrite);
    } else {
        record = new AcDbXrecord;
        es = dict->setAt(recordName(), record, id);
        if (es != Acad::eOk) { delete record; record = NULL; }
    }
    dict->close();
    if (es != Acad::eOk) return es;
    resbuf data = {};
    data.restype = 40;
    data.resval.rreal = value;
    es = record->setFromRbChain(data);
    record->close();
    return es;
}

// Fail closed: changing the interpretation of existing custom geometry is unsafe.
inline bool hasElements(AcDbDatabase* db = NULL) {
    if (!db) db = acdbHostApplicationServices()->workingDatabase();
    AcDbBlockTable* table = NULL;
    if (!db || db->getBlockTable(table, AcDb::kForRead) != Acad::eOk) return true;
    AcDbBlockTableIterator* blocks = NULL;
    if (table->newIterator(blocks) != Acad::eOk) { table->close(); return true; }
    bool found = false;
    for (; !blocks->done() && !found; blocks->step()) {
        AcDbBlockTableRecord* block = NULL;
        if (blocks->getRecord(block, AcDb::kForRead) != Acad::eOk) { found = true; break; }
        if (!block->isFromExternalReference()) {
            AcDbBlockTableRecordIterator* entities = NULL;
            if (block->newIterator(entities) != Acad::eOk) found = true;
            else {
                for (; !entities->done() && !found; entities->step()) {
                    AcDbEntity* entity = NULL;
                    if (entities->getEntity(entity, AcDb::kForRead) != Acad::eOk) { found = true; break; }
                    found = _wcsnicmp(entity->isA()->name(), L"TVS_", 4) == 0;
                    entity->close();
                }
                delete entities;
            }
        }
        block->close();
    }
    delete blocks;
    table->close();
    return found;
}
}
