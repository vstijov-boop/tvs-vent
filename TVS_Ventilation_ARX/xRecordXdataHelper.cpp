#include "stdafx.h"
#include "xRecordXdataHelper.h"


XRecordXdataHelper::XRecordXdataHelper()
{
}

bool XRecordXdataHelper::setXdataBuf(const AcDbObjectId &objectId, const  resbuf *rb)
{
	//open object

	AcDbObjectPointer<AcDbObject> pobj(objectId, AcDb::kForWrite);
	if (pobj.openStatus() != Acad::eOk) return false;

	//set object format
	pobj->setXData(rb);
	return true;
}


bool XRecordXdataHelper::setXrecordBuf(const AcDbObjectId &objectId, const ACHAR * paramName, const resbuf *rb)
{
	//create or get extension dictionary
	AcDbObjectId dictId;
	{
		AcDbObjectPointer<AcDbObject> pobj(objectId, AcDb::kForWrite);
		if (pobj.openStatus() != Acad::eOk) return false;
		pobj->createExtensionDictionary();
		dictId = pobj->extensionDictionary();
	}

	//Add xrecord to extension dictionary
	AcDbObjectId xrecid;
	AcDbXrecord * pXrec;
	{
		AcDbObjectPointer<AcDbDictionary> pDict(dictId, AcDb::kForWrite);
		if (pDict.openStatus() != Acad::eOk) return false;
		pXrec = new AcDbXrecord;
		pDict->setAt(paramName, pXrec, xrecid);
	}

	//add resbuf to xrecord
	pXrec->setFromRbChain(*rb);
	pXrec->close();

	return true;
}

bool XRecordXdataHelper::getStringXdata(const AcDbObjectId &objectId, const ACHAR * paramName, CString &value)
{
	resbuf *rb;

	if (!getXDataResbuf(objectId, paramName, rb)) return false;
	for (resbuf *rbnext = rb; rbnext != NULL; rbnext = rbnext->rbnext)
	{

		if (rbnext->restype == AcDb::kDxfXdAsciiString)
		{
			value = rbnext->resval.rstring;
			acutRelRb(rb);
			return true;
		}
	}
	acutRelRb(rb);
	return false;
}

bool XRecordXdataHelper::checkXdataApp(const AcDbObjectId &objectId, const ACHAR * paramName)
{
	// Memory (2026-11-28): every resbuf chain from xData() is released - before, the chain
	// leaked at every check / read / write (labels, properties palette, TVS_DIM_PRATI ...).
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForRead);
	if (pObject.openStatus() != Acad::eOk) return false;
	resbuf *xdata = pObject->xData(paramName);
	if (xdata == NULL) return false;
	acutRelRb(xdata);
	return true;
}

bool XRecordXdataHelper::addXdataApp(const AcDbObjectId &objectId, const ACHAR * paramName)
{
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForWrite);
	if (pObject.openStatus() != Acad::eOk) return false;
	resbuf *rb = acutBuildList(AcDb::kDxfRegAppName, paramName, AcDb::kDwgNull);
	if (rb == NULL) return false;
	const Acad::ErrorStatus es = pObject->setXData(rb);
	acutRelRb(rb);
	return es == Acad::eOk;
}

bool XRecordXdataHelper::removeXdata(const AcDbObjectId &objectId, const ACHAR * paramName)
{
	// for write: setXData on an object opened for read did nothing
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForWrite);
	if (pObject.openStatus() != Acad::eOk) return false;
	resbuf *xdata = pObject->xData(paramName);
	if (xdata)
	{
		// only the application name: removes this application's xdata
		resbuf *rbnext = xdata->rbnext;
		xdata->rbnext = NULL;
		pObject->setXData(xdata);
		xdata->rbnext = rbnext;
		acutRelRb(xdata);
	}
	return true;
}

bool XRecordXdataHelper::removeXrecord(const AcDbObjectId &objectId, const ACHAR * paramName)
{
	//get extansion dictionary
	AcDbObjectId dictId;
	{
		AcDbObjectPointer<AcDbObject> pobj(objectId, AcDb::kForWrite);
		if (pobj.openStatus() != Acad::eOk) return false;
		dictId = pobj->extensionDictionary();
	}

	//Remove xrecord from extension dictionary
	AcDbObjectId xrecid;
	{
		AcDbObjectPointer<AcDbDictionary> pDict(dictId, AcDb::kForWrite);
		if (pDict.openStatus() != Acad::eOk) return false;
		if ((pDict->getAt(paramName, xrecid)) != Acad::eOk) return false;
	}

	//erase xrecord
	if (!xrecid.isNull())
	{
		AcDbObjectPointer<AcDbObject> pObj(xrecid, AcDb::kForWrite);
		if ((pObj.openStatus()) != Acad::eOk) return false;
		return pObj->erase() == Acad::eOk;
	}
	return false;
}

bool XRecordXdataHelper::getObjectIdXrecord(const AcDbObjectId &objectId, const ACHAR * paramName, AcDbObjectId &objId)
{
	resbuf *rb;

	if (!getXRecordResbuf(objectId, paramName, rb) || !rb) return false;

	if (rb->restype == AcDb::kDxfHardPointerId) {
		if (acdbGetObjectId(objId, rb->resval.rlname) == Acad::eOk) {

			acutRelRb(rb);
			return true;
		}
		else {
			acutRelRb(rb);
			return false;
		}
	}
	acutRelRb(rb);
	return false;
}

bool XRecordXdataHelper::getXDataResbuf(const AcDbObjectId &objectId, const ACHAR * paramName, resbuf *&rb)
{
	//open object
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForRead);
	if (pObject.openStatus() != Acad::eOk) return false;

	return getXDataResbuf(pObject, paramName, rb);
}

bool XRecordXdataHelper::getXDataResbuf(const AcDbObjectPointer<AcDbObject> &pObject, const ACHAR * paramName, resbuf *&rb)
{
	//parse parameter
	rb = pObject->xData(paramName);
	return true;
}

bool XRecordXdataHelper::getXRecordResbuf(const AcDbObjectId &objectId, const ACHAR * paramName, resbuf *&rb)
{
	// Read only (2026-11-24): the old version opened every object for WRITE and, for an object whose
	// extension dictionary has no such entry (annotative MLeaders, bound xrefs, other add-ons), called
	// rbChain on an uninitialised pointer -> AutoCAD crash in TVS_DIM_PRATI (label refresh).
	rb = NULL;
	AcDbObjectId dictId;
	{
		AcDbObjectPointer<AcDbObject> pobj(objectId, AcDb::kForRead);
		if (pobj.openStatus() != Acad::eOk) return false;
		dictId = pobj->extensionDictionary();
	}
	if (dictId.isNull()) return false;
	AcDbObjectId xrecId;
	{
		AcDbObjectPointer<AcDbDictionary> pDict(dictId, AcDb::kForRead);
		if (pDict.openStatus() != Acad::eOk) return false;
		if (pDict->getAt(paramName, xrecId) != Acad::eOk || xrecId.isNull()) return false;
	}
	AcDbObjectPointer<AcDbXrecord> pXrec(xrecId, AcDb::kForRead);
	if (pXrec.openStatus() != Acad::eOk) return false;   // entry is not an xrecord
	if (pXrec->rbChain(&rb) != Acad::eOk) { rb = NULL; return false; }
	return rb != NULL;
}

resbuf* XRecordXdataHelper::findSubXdata(int tag, resbuf* xdata)
{
	resbuf* tmp = xdata;
	while (tmp) {
		if (tmp->restype == 1070) {
			if (tmp->resval.rint == tag)
				return tmp->rbnext;

			// skip around value of the tag
			if (tmp->rbnext) {
				tmp = tmp->rbnext;
				if (tmp->restype == 1002)
					;    // start of a list, do nothing
				else {        // skip next rb which is tag value
					if (tmp->rbnext == NULL)
						return NULL;    // never found it
					tmp = tmp->rbnext;
				}
			}
			else
				return NULL;        // never found it
		}
		else
			tmp = tmp->rbnext;
	}
	return NULL;    // never found it
}

resbuf* tailOfResbufChain(resbuf* const rb)
{
	resbuf* tmp = rb;
	if (tmp != NULL) {
		while (tmp->rbnext != NULL)
			tmp = tmp->rbnext;
	}
	return tmp;
}
resbuf *lastPointer;

bool XRecordXdataHelper::getSubXReal(const AcDbObjectId &objectId, const ACHAR * appName, int tag, double &parameter)
{
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForRead);   // reading only
	if (pObject.openStatus() != Acad::eOk) return false;
	return getSubXReal(pObject, appName, tag, parameter);
}

bool XRecordXdataHelper::getSubXReal(AcDbObjectPointer<AcDbObject> &pObject, const ACHAR * appName, int tag, double &parameter)
{
	resbuf *rb = NULL;
	getXDataResbuf(pObject, appName, rb);
	parameter = 0;
	if (rb == NULL) return false;
	resbuf* tagBuff = findSubXdata(tag, rb);
	const bool found = tagBuff != NULL;
	if (found) parameter = tagBuff->resval.rreal;
	acutRelRb(rb);
	return found;
}

bool XRecordXdataHelper::setSubXReal(AcDbObjectPointer<AcDbObject> &pObject, const ACHAR * appName, int tag, const double &parameter)
{
	resbuf *rb = NULL;
	getXDataResbuf(pObject, appName, rb);
	if (rb == NULL) {
		rb = acutBuildList(AcDb::kDxfRegAppName, appName,
			AcDb::kDxfXdInteger16, tag,
			AcDb::kDxfXdReal, parameter,
			AcDb::kDwgNull);
		if (rb == NULL) return false;
	}
	else {
		resbuf* tagBuff = findSubXdata(tag, rb);
		if (tagBuff != NULL) tagBuff->resval.rreal = parameter;
		else {
			tagBuff = acutNewRb(AcDb::kDxfXdInteger16);
			tagBuff->resval.rint = tag;
			resbuf * valBuff = acutNewRb(AcDb::kDxfXdReal);
			valBuff->resval.rreal = parameter;
			tagBuff->rbnext = valBuff;
			tailOfResbufChain(rb)->rbnext = tagBuff;
		}
	}
	const Acad::ErrorStatus es = pObject->setXData(rb);
	acutRelRb(rb);
	return es == Acad::eOk;
}
bool XRecordXdataHelper::setSubXReal(const AcDbObjectId &objectId, const ACHAR * appName, int tag, const double &parameter)
{
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForWrite);
	if (pObject.openStatus() != Acad::eOk) return false;
	return setSubXReal(pObject, appName, tag, parameter);
}


bool XRecordXdataHelper::setSubXString(const AcDbObjectId &objectId, const ACHAR * appName, int tag, const CString &parameter)
{
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForWrite);
	if (pObject.openStatus() != Acad::eOk) return false;
	return setSubXString(pObject, appName, tag, parameter);
}
bool XRecordXdataHelper::setSubXString(AcDbObjectPointer<AcDbObject> &pObject, const ACHAR * appName, int tag, const CString &parameter)
{
	// strings through acutNewString / acutBuildList (copies): AllocSysString leaked a BSTR at every
	// write and gave acutRelRb a string it did not allocate
	resbuf *rb = NULL;
	getXDataResbuf(pObject, appName, rb);
	if (rb == NULL) {
		rb = acutBuildList(AcDb::kDxfRegAppName, appName,
			AcDb::kDxfXdInteger16, tag,
			AcDb::kDxfXdAsciiString, parameter.GetString(),
			AcDb::kDwgNull);
		if (rb == NULL) return false;
	}
	else {
		resbuf* tagBuff = findSubXdata(tag, rb);
		if (tagBuff != NULL)
		{
			if (tagBuff->resval.rstring) acutDelString(tagBuff->resval.rstring);
			tagBuff->resval.rstring = NULL;
			acutNewString(parameter.GetString(), tagBuff->resval.rstring);
		}
		else {
			tagBuff = acutNewRb(AcDb::kDxfXdInteger16);
			tagBuff->resval.rint = tag;
			resbuf * valBuff = acutNewRb(AcDb::kDxfXdAsciiString);
			valBuff->resval.rstring = NULL;
			acutNewString(parameter.GetString(), valBuff->resval.rstring);
			tagBuff->rbnext = valBuff;
			tailOfResbufChain(rb)->rbnext = tagBuff;
		}
	}
	const Acad::ErrorStatus es = pObject->setXData(rb);
	acutRelRb(rb);
	return es == Acad::eOk;
}

bool XRecordXdataHelper::getSubXString(const AcDbObjectId &objectId, const ACHAR * appName, int tag, CString &parameter)
{
	AcDbObjectPointer<AcDbObject> pObject(objectId, AcDb::kForRead);   // reading only
	if (pObject.openStatus() != Acad::eOk) return false;
	return getSubXString(pObject, appName, tag, parameter);
}

bool XRecordXdataHelper::getSubXString(AcDbObjectPointer<AcDbObject> &pObject, const ACHAR * appName, int tag, CString &parameter)
{
	resbuf *rb = NULL;
	getXDataResbuf(pObject, appName, rb);
	if (rb == NULL) return false;
	resbuf* tagBuff = findSubXdata(tag, rb);
	const bool found = tagBuff != NULL && tagBuff->restype == AcDb::kDxfXdAsciiString && tagBuff->resval.rstring != NULL;
	if (found) parameter = tagBuff->resval.rstring;
	acutRelRb(rb);
	return found;
}

XRecordXdataHelper::~XRecordXdataHelper()
{
}
