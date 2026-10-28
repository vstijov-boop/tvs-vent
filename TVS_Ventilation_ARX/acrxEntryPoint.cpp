// (C) Copyright 2002-2012 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "resource.h"
#include "Func.h"
#include "tdbl.h"

#include "dgPipeSize.h"
#include "dgChangeZ.h"
#include "MSExcel.h"	
#include "HeatFloor.h"
#include "dgBlockManager.h"
//#import "acax21ENU.tlb" 
#include <rxmfcapi.h>
#include <axpnt3d.h>

//#include <diskid.h>
//#include <protection.h>
//#include "Regedit.h"
#include <TVS_Connector.h>

#include "TVSController.h"
#include "MleaderController.h"
#include "PropertyAdmin.h"

#include "TVSLeadCommand.h"
#include "TVSCommandLeadRecalculate.h"
#include "TVSCommandPropertyManager.h"
#include "TVSCommandAttributesToProperties.h"
#include "TVSCommandAddProperties.h"
#include "TVSCommandExcelSpecification.h"
#include "TVSCommandSpecification.h"
#include "TablePresentation.h"
//-----------------------------------------------------------------------------
#define szRDS _RXST("TVS")
#define dCONTINUE 0
#define dCONNECT 1
#define dSIZE 2
#define dZ 2
#define dCONNECTT 3
#define dCONNECTW 4
#define dUP 5
#define dDOWN 6

#define TVSisLinear 1
#define TVSisPerpendicular 2
#define TVSisParallel 3
#define TVSisIntersection 4
#define TVSisNoSameSize 0
#define TVSisSameSize 1
#define TVSFirstPoint 0
#define TVSLastpoint 1

#define pError_Ok 0
//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint

class CTVS_Ventilation_ARXApp : public AcRxArxApp {

public:
	CTVS_Ventilation_ARXApp() : AcRxArxApp() {}

	virtual AcRx::AppRetCode On_kInitAppMsg(void *pkt) {
		// TODO: Load dependencies here

		tdbl *pdbl = new tdbl;
		pdbl->InitApplication();
		delete pdbl;
		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode = AcRxArxApp::On_kInitAppMsg(pkt);
		PropertyAdmin::initialize();
		// TODO: Add your initialization code here

		return (retCode);
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg(void *pkt) {
        PropertyAdmin::uninitialize();
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);

		// TODO: Unload dependencies here

		return (retCode);
	}

	virtual AcRx::AppRetCode On_kLoadDwgMsg(void * pkt)
	{
		AcRx::AppRetCode retCode = AcRxArxApp::On_kLoadDwgMsg(pkt);

		// register apps
		MleaderController mctrl;
		mctrl.registerApp();
		TVSController::get()->tvsPropertyController.registerApp();
        PropertyAdmin::initialize();

		return (retCode);
	}

	virtual AcRx::AppRetCode On_(void *pkt) {
		// TODO: Add your code here

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);

		// TODO: Unload dependencies here

		return (retCode);
	}

	virtual void RegisterServerComponents() {
		//----- Self-register COM server upon loading.
		if (FAILED(::DllRegisterServer()))
			acutPrintf(_RXST("Failed to register COM server.\n"));
	}















	//new commands

	static void Ventilation_ARXTVS_LEAD(void)
	{
		TVSCommandLead cmnd;
		cmnd.execute();
	}

	static void Ventilation_ARXTVS_LEAD_Recalculate(void)
	{
		TVSCommandLeadRecalculate cmnd;
		cmnd.execute();
	}


	static void Ventilation_ARXTVS_PropertyManager(void)
	{
		TVSCommandPropertyManager cmnd;
		cmnd.execute();
	}

	static void Ventilation_ARXTVS_AttributesToProps(void)
	{
		TVSCommandAttributesToProperties cmnd;
		cmnd.execute();
	}

	static void Ventilation_ARXTVS_AddProperty(void)
	{
		TVSCommandAddProperties cmnd;
		cmnd.execute();
	}

	static void Ventilation_ARXTVS_ExcelSp(void)
	{
		TVSCommandExcelSpecification cmnd;
		cmnd.execute();
	}

	static void Ventilation_ARXTVS_Sp(void)
	{
		TVSCommandSpecification cmnd;
		cmnd.execute();
	}









	// The ACED_ARXCOMMAND_ENTRY_AUTO macro can be applied to any static member 
	// function of the CTVS_Ventilation_ARXApp class.
	// The function should take no arguments and return nothing.
	//
	// NOTE: ACED_ARXCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid and
	// have arguments to define context and command mechanism.

	// ACED_ARXCOMMAND_ENTRY_AUTO(classname, group, globCmd, locCmd, cmdFlags, UIContext)
	// ACED_ARXCOMMAND_ENTRYBYID_AUTO(classname, group, globCmd, locCmdId, cmdFlags, UIContext)
	// only differs that it creates a localized name using a string in the resource file
	//   locCmdId - resource ID for localized command

	// Modal Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL)
	static void TVSMyGroupMyCommand() {
		// Put your command code here

	}




	// Modal Command with pickfirst selection
	// ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET)
	static void TVSMyGroupMyPickFirst() {
		ads_name result;
		int iRet = acedSSGet(ACRX_T("_I"), NULL, NULL, NULL, result);
		if (iRet == RTNORM)
		{


			// There are selected entities
			// Put your command using pickfirst set code here
		}
		else
		{
			// There are no selected entities
			// Put your command code here
		}
	}

	// Application Session Command with localized name
	// ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION)
	static void TVSMyGroupMySessionCmd() {
		// Put your command code here
	}

	// The ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO macros can be applied to any static member 
	// function of the CTVS_Ventilation_ARXApp class.
	// The function may or may not take arguments and have to return RTNORM, RTERROR, RTCAN, RTFAIL, RTREJ to AutoCAD, but use
	// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal to return
	// a value to the Lisp interpreter.
	//
	// NOTE: ACED_ADSFUNCTION_ENTRY_AUTO / ACED_ADSCOMMAND_ENTRY_AUTO has overloads where you can provide resourceid.

	//- ACED_ADSFUNCTION_ENTRY_AUTO(classname, name, regFunc) - this example
	//- ACED_ADSSYMBOL_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file
	//- ACED_ADSCOMMAND_ENTRY_AUTO(classname, name, regFunc) - a Lisp command (prefix C:)
	//- ACED_ADSCOMMAND_ENTRYBYID_AUTO(classname, name, nameId, regFunc) - only differs that it creates a localized name using a string in the resource file

	// Lisp Function is similar to ARX Command but it creates a lisp 
	// callable function. Many return types are supported not just string
	// or integer.
	// ACED_ADSFUNCTION_ENTRY_AUTO(CTVS_Ventilation_ARXApp, MyLispFunction, false)
	static int ads_MyLispFunction() {
		//struct resbuf *args =acedGetArgs () ;

		// Put your command code here

		//acutRelRb (args) ;

		// Return a value to the AutoCAD Lisp Interpreter
		// acedRetNil, acedRetT, acedRetVoid, acedRetInt, acedRetReal, acedRetStr, acedRetPoint, acedRetName, acedRetList, acedRetVal

		return (RTNORM);
	}




	static bool addTrans(TVS_Pipe *pPipe1,
		TVS_Pipe  *pPipe2, AcGePoint3d &point1, AcGePoint3d &midp, AcGePoint3d &point2)
	{
		if (length2p(pPipe1->FirstPoint, point1) > length2p(pPipe1->LastPoint, point1))
		{
			AcDbEntity* pEnt;
			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_Lastpoint(pPipe1->FirstPoint);
				pPipe1->put_FirstPoint(point1);
				pPipe1->close();

			}
		}

		if (length2p(pPipe2->FirstPoint, point2) < length2p(pPipe2->LastPoint, point2))
		{
			AcDbEntity* pEnt;
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {
				pPipe2->put_FirstPoint(pPipe2->LastPoint);
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();
			}
		}


		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcDbEntity* pEnt;
		if (GroseStatus == TVSEnt1isGrose)
		{

			TVS_TRANS * pTrans;
			TVS_Pipe* GPipe = pPipe1;
			TVS_Pipe* SPipe = pPipe2;


			pTrans = pTrans->add_new(GPipe->SizeA,
				GPipe->SizeB,
				SPipe->SizeA,
				SPipe->SizeB,
				globalLengthTr,
				AcGeVector3d(-GPipe->FirstPoint.x + GPipe->LastPoint.x, -GPipe->FirstPoint.y + GPipe->LastPoint.y, -GPipe->FirstPoint.z + GPipe->LastPoint.z).normalize(),
				GPipe->FirstPoint,
				GPipe->ThisRound,
				SPipe->ThisRound,
				globalLengthTr,
				GPipe->This1D
			);
			if (acdbOpenAcDbEntity(pEnt, pTrans->id(), AcDb::kForWrite) == eOk) {
				pTrans->put_Length(globalLengthTr);
				pTrans->close();
			}
			SetGlobalProperty(pTrans);
			SetPropertyLikePipe(SPipe, pTrans);
			SetPropertyLikePipe(SPipe, GPipe);
			AcGePoint3d lastPoint = GPipe->LastPoint;

			point1 = shortlength(midp, point1, globalLengthTr);
			return true;
		}
		if (GroseStatus == TVSEnt2isGrose)
		{

			TVS_TRANS * pTrans;
			TVS_Pipe* GPipe = pPipe2;
			TVS_Pipe* SPipe = pPipe1;


			pTrans = pTrans->add_new(GPipe->SizeA,
				GPipe->SizeB,
				SPipe->SizeA,
				SPipe->SizeB,
				globalLengthTr,
				AcGeVector3d(GPipe->FirstPoint.x - GPipe->LastPoint.x, GPipe->FirstPoint.y - GPipe->LastPoint.y, GPipe->FirstPoint.z - GPipe->LastPoint.z).normalize(),
				GPipe->LastPoint,
				GPipe->ThisRound,
				SPipe->ThisRound,
				globalLengthTr,
				GPipe->This1D
			);
			if (acdbOpenAcDbEntity(pEnt, pTrans->id(), AcDb::kForWrite) == eOk) {
				pTrans->put_Length(globalLengthTr);
				pTrans->close();
			}
			SetGlobalProperty(pTrans);
			SetPropertyLikePipe(SPipe, pTrans);
			SetPropertyLikePipe(SPipe, GPipe);

			point2 = shortlength(midp, point2, globalLengthTr);
			return true;

		}

		return false;
	}



	static bool ConnectWithTap(AcDbEntity *pEnt1,
		AcDbEntity  *pEnt2, ads_point pnt1, ads_point pnt2)

	{
		//Проверка
		if (TVSClassCheck(pEnt1) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}

		if (TVSClassCheck(pEnt2) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}


		TVS_Pipe* pPipe1 = TVS_Pipe::cast(pEnt1);
		TVS_Pipe* pPipe2 = TVS_Pipe::cast(pEnt2);
		AcGePoint3d p1 = pPipe1->get_FirstPoint();
		AcGePoint3d p2 = pPipe1->get_Lastpoint();
		AcGePoint3d p3 = pPipe2->get_FirstPoint();
		AcGePoint3d p4 = pPipe2->get_Lastpoint();

		AcGeLine3d line1 = AcGeLine3d(p1, p2);
		AcGeLine3d  line2 = AcGeLine3d(p3, p4);
		AcGeTol dop;
		dop.setEqualPoint(0.01 / tvsUnits::factor());
		int LineStatus = TVSisIntersection;
		int SizeStatus;
		if (line1.isParallelTo(line2, dop)) { LineStatus = TVSisParallel; acutPrintf(_T("\n Paralelni")); }
		if (line1.isColinearTo(line2, dop)) { LineStatus = TVSisLinear; acutPrintf(_T("\n Kolinearni")); }
		if (line1.isPerpendicularTo(line2, dop)) { LineStatus = TVSisPerpendicular; acutPrintf(_T("\n Upravni")); }
		if ((pPipe1->SizeA == pPipe2->SizeA) && (pPipe1->SizeA == pPipe2->SizeA)) SizeStatus = TVSisSameSize;
		else SizeStatus = TVSisNoSameSize;
		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcGePoint3d pointSelect1 = line1.closestPointTo(asPnt3d(pnt1));
		AcGePoint3d pointSelect2 = line2.closestPointTo(asPnt3d(pnt2));

#pragma region  line intersection
		if (LineStatus != TVSisParallel&&LineStatus != TVSisLinear)
		{
			AcGePoint3d midp, point1, point2;

			line1.intersectWith(line2, midp);
			//find point1
			if (midp.distanceTo(p1) <= p1.distanceTo(p2) && midp.distanceTo(p2) <= p1.distanceTo(p2))// midp check between p3 p4
			{
				if (midp.distanceTo(p1) <= pointSelect1.distanceTo(p1)) point1 = p2;
				else point1 = p1;
			}
			else
			{
				if (midp.distanceTo(p1) >= midp.distanceTo(p2)) point1 = p1;
				else point1 = p2;
			}//end findpoint1

			//find point2
			if (midp.distanceTo(p3) <= p3.distanceTo(p4) && midp.distanceTo(p4) <= p3.distanceTo(p4)) // midp check between p3 p4
			{
				if (midp.distanceTo(p3) <= pointSelect2.distanceTo(p3)) point2 = p4;
				else point2 = p3;
			}
			else
			{
				if (midp.distanceTo(p3) >= midp.distanceTo(p4)) point2 = p3;
				else point2 = p4;
			}//end findpoint2
			// Validate before changing either duct: zero-length or collinear legs cannot form an elbow.
            double elbowAngle = 0.0;
            AcGeVector3d elbowStart;
            if (!GiveStartvectorAndAngle(point1, midp, point2, elbowAngle, elbowStart)) {
                acutPrintf(L"\nNije moguće napraviti koljeno: provjerite smjerove i krajnje tačke kanala.");
                return false;
            }
			///Tap with point1 midp point2;

			AcDbEntity * pEnt;

			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_FirstPoint(point1);
				pPipe1->put_Lastpoint(midp);
				pPipe1->close();

			}
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {


				pPipe2->put_FirstPoint(midp);
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();
			}
#pragma region		//insert trans
			if (GroseStatus != TVSEntitiesisSame)
			{

				addTrans(pPipe1, pPipe2, point1, midp, point2);


			}
#pragma endregion
			TVS_TAP* pTap = drawTapDirect(point1, midp, point2);
			if (!pTap) return false;
			SetGlobalProperty(pTap);
			SetPropertyLikePipe(pPipe1, pTap);

			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_FirstPoint(point1);
				pPipe1->put_Lastpoint(shortlength(point1, midp, length2p(pTap->MA, pTap->MiddlePoint)));
				pPipe1->close();

			}
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {


				pPipe2->put_FirstPoint(shortlength(point2, midp, length2p(pTap->MA, pTap->MiddlePoint)));
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();

			}




		} //end if line intersection
#pragma endregion 

#pragma region linear
		if (LineStatus == TVSisLinear)
		{
			AcGePoint3d kr1;
			AcGePoint3d kr2;
			AcGePoint3d midp;
			if (max(length2p(p1, p3), length2p(p1, p4)) > max(length2p(p2, p3), length2p(p2, p4)))
			{
				kr1 = p1; midp = p2;
			}
			else { kr1 = p2; midp = p1; }

			if (length2p(p1, p4) > length2p(p1, p3)) kr2 = p4;
			else kr2 = p3;

			AcDbEntity * pEnt;

			//put trans

			addTrans(pPipe1, pPipe2, kr1, midp, kr2);

			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();
			pPipe1->put_FirstPoint(kr1);
			pPipe1->put_Lastpoint(kr2);
			pPipe1->close();

			ads_name eName;
			acdbGetAdsName(eName, pPipe2->id());
			acdbEntDel(eName);

		}
#pragma endregion 
		if (LineStatus == TVSisParallel) return false;
		return true;
	}


	static bool ConnectWithWye(AcDbEntity *pEnt1,
		AcDbEntity  *pEnt2, ads_point pnt1, ads_point pnt2)

	{
		//Проверка
		if (TVSClassCheck(pEnt1) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}

		if (TVSClassCheck(pEnt2) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}


		TVS_Pipe* pPipe1 = TVS_Pipe::cast(pEnt1);
		TVS_Pipe* pPipe2 = TVS_Pipe::cast(pEnt2);
		AcGePoint3d p1 = pPipe1->get_FirstPoint();
		AcGePoint3d p2 = pPipe1->get_Lastpoint();
		AcGePoint3d p3 = pPipe2->get_FirstPoint();
		AcGePoint3d p4 = pPipe2->get_Lastpoint();

		AcGeLine3d line1 = AcGeLine3d(p1, p2);
		AcGeLine3d  line2 = AcGeLine3d(p3, p4);
		AcGeTol dop;
		dop.setEqualPoint(0.01 / tvsUnits::factor());
		int LineStatus = TVSisIntersection;
		int SizeStatus;
		if (line1.isParallelTo(line2, dop)) { LineStatus = TVSisParallel; acutPrintf(_T("\n Paralelni")); }
		if (line1.isColinearTo(line2, dop)) { LineStatus = TVSisLinear; acutPrintf(_T("\n Kolinearni")); }
		if (line1.isPerpendicularTo(line2, dop)) { LineStatus = TVSisPerpendicular; acutPrintf(_T("\n Upravni")); }
		if ((pPipe1->SizeA == pPipe2->SizeA) && (pPipe1->SizeA == pPipe2->SizeA)) SizeStatus = TVSisSameSize;
		else SizeStatus = TVSisNoSameSize;
		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcGePoint3d pointSelect1 = line1.closestPointTo(asPnt3d(pnt1));
		AcGePoint3d pointSelect2 = line2.closestPointTo(asPnt3d(pnt2));
		AcDbEntity * pEnt;
#pragma region  line intersection
		if (LineStatus == TVSisPerpendicular || LineStatus == TVSisIntersection)
		{

			AcGePoint3d midp, point1, point2, intersectpoint;

			line1.intersectWith(line2, midp);
			//find point1
			if (max(midp.distanceTo(p1), midp.distanceTo(p2)) < p1.distanceTo(p2))
			{
				if (p2.distanceTo(pointSelect1) < p2.distanceTo(midp))
				{
					AcGePoint3d temp;
					temp = p2;
					p2 = p1;
					p1 = temp;
					acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
					pPipe1->assertWriteEnabled();
					pPipe1->put_FirstPoint(p1);
					pPipe1->put_Lastpoint(p2);
					pPipe1->close();
				}

			}
			else
			{
				if (p2.distanceTo(midp) > p1.distanceTo(midp))
				{
					AcGePoint3d temp;
					temp = p2;
					p2 = p1;
					p1 = temp;
					acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
					pPipe1->assertWriteEnabled();
					pPipe1->put_FirstPoint(p1);
					pPipe1->put_Lastpoint(p2);
					pPipe1->close();
				}
			}

			intersectpoint = line2.closestPointTo(p2);

			//made perpendicular

			if (LineStatus == TVSisIntersection)
			{
				TVS_Pipe* tempPipe;
				tempPipe = tempPipe->add_new(p2, intersectpoint, globSizeA, globSizeB);
				SetPropertyLikePipe(pPipe1, tempPipe);
				ads_point pt1;
				ads_point pt2;





				ConnectWithTap(pPipe1, tempPipe, asDblArray(p1), asDblArray(intersectpoint));
				pPipe1 = tempPipe;
				p1 = p2;
				p2 = intersectpoint;

			}



			if (length2p(p3, intersectpoint) < length2p(p4, intersectpoint))
			{
				AcGePoint3d temp;
				temp = p4;
				p4 = p3;
				p3 = temp;
			}




			acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite);
			pPipe2->assertWriteEnabled();
			pPipe2->put_FirstPoint(p3);
			pPipe2->put_Lastpoint(intersectpoint);
			pPipe2->close();

			TVS_WYE* pWye;
			pWye = pWye->add_new(pPipe2->SizeA,
				pPipe2->SizeB,
				pPipe1->SizeA,
				pPipe1->SizeB,
				globalLengthW,
				AcGeVector3d(-pPipe2->FirstPoint.x + pPipe2->LastPoint.x, -pPipe2->FirstPoint.y + pPipe2->LastPoint.y, -pPipe2->FirstPoint.z + pPipe2->LastPoint.z).normalize(),
				AcGeVector3d(p1.x - p2.x, p1.y - p2.y, p1.z - p2.z).normalize(),
				intersectpoint,
				pPipe2->ThisRound,
				pPipe1->ThisRound,
				pPipe2->This1D);
			SetGlobalProperty(pWye);
			SetPropertyLikePipe(pPipe2, pWye);


			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();

			pPipe1->put_Lastpoint(shortlength(p1, intersectpoint, pPipe2->SizeA / 2 + pWye->LengthPl));
			pPipe1->close();
			acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite);
			pPipe2->assertWriteEnabled();
			pPipe2->put_Lastpoint(shortlength(p3, intersectpoint, pPipe1->SizeA / 2 + pWye->LengthPl));
			pPipe2->close();
			//if intersection between p3 and p4
			if (max(length2p(p3, intersectpoint), length2p(p4, intersectpoint)) < length2p(p3, p4))
			{
				TVS_Pipe* tempPipe;
				tempPipe = tempPipe->add_new(intersectpoint, p4, globSizeA, globSizeB);
				SetPropertyLikePipe(pPipe2, tempPipe);
				acdbOpenAcDbEntity(pEnt, tempPipe->id(), AcDb::kForWrite);
				tempPipe->assertWriteEnabled();
				tempPipe->put_FirstPoint(shortlength(p4, intersectpoint, pPipe1->SizeA / 2 + pWye->LengthPl));
				tempPipe->close();
			}



		} //end if line intersection
#pragma endregion 

#pragma region linear
		if (LineStatus == TVSisLinear)
		{
			AcGePoint3d kr1;
			AcGePoint3d kr2;
			AcGePoint3d midp;
			if (max(length2p(p1, p3), length2p(p1, p4)) > max(length2p(p2, p3), length2p(p2, p4)))
			{
				kr1 = p1; midp = p2;
			}
			else { kr1 = p2; midp = p1; }

			if (length2p(p1, p4) > length2p(p1, p3)) kr2 = p4;
			else kr2 = p3;

			AcDbEntity * pEnt;

			//put trans

			addTrans(pPipe1, pPipe2, kr1, midp, kr2);

			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();
			pPipe1->put_FirstPoint(kr1);
			pPipe1->put_Lastpoint(kr2);
			pPipe1->close();

			ads_name eName;
			acdbGetAdsName(eName, pPipe2->id());
			acdbEntDel(eName);

		}
#pragma endregion 
		if (LineStatus == TVSisParallel) return false;
		return true;
	}


	// 
	// 	static bool ConnectPutTrans(AcDbEntity *pEnt1, AcDbEntity *pEnt2, int pointStatus)
	// 
	// 	{
	// 		//Проверка
	// 		if (TVSClassCheck(pEnt1)!=isTVS_Pipe)
	// 		{
	// 			acutPrintf(_T("\n Выбран не тот обьект"));
	// 			return false;
	// 		}
	// 
	// 		if (TVSClassCheck(pEnt2)!=isTVS_Pipe)
	// 		{
	// 			acutPrintf(_T("\n Выбран не тот обьект"));
	// 			return false;
	// 		}
	// 
	// 
	// 		TVS_Pipe* pPipe1=TVS_Pipe::cast(pEnt1);
	// 		TVS_Pipe* pPipe2=TVS_Pipe::cast(pEnt2);
	// 		AcGePoint3d p1 = pPipe1->get_FirstPoint();
	// 		AcGePoint3d p2 = pPipe1->get_Lastpoint();
	// 		AcGePoint3d p3 = pPipe2->get_FirstPoint();
	// 		AcGePoint3d p4 = pPipe2->get_Lastpoint();
	// 
	// 		AcGeLine3d line1=AcGeLine3d (p1,p2);
	// 		AcGeLine3d  line2=AcGeLine3d (p3,p4);
	// 		AcGeTol dop;
	// 		dop.setEqualPoint(0.01);
	// 		int LineStatus=TVSisIntersection;
	// 		int SizeStatus;
	// 		if(line1.isParallelTo(line2,dop)) {LineStatus=TVSisParallel; acutPrintf(_T("\n Параллельны"));} 
	// 		if(line1.isColinearTo(line2,dop)) {LineStatus=TVSisLinear; acutPrintf(_T("\n Линейны"));} 
	// 		if(line1.isPerpendicularTo(line2,dop)) {LineStatus=TVSisPerpendicular; acutPrintf(_T("\n Перпендикулярны"));} 
	// 		if((pPipe1->SizeA==pPipe2->SizeA)&&(pPipe1->SizeA==pPipe2->SizeA)) SizeStatus=TVSisSameSize;
	// 		else SizeStatus=TVSisNoSameSize;
	// 		int GroseStatus=whyIsGrose(pPipe1,pPipe2);
	// 
	// 
	// #pragma region //if pipes various
	// 		
	// 			if (GroseStatus=TVSisNoSameSize)
	// 			{
	// 
	// 
	// 				if (pointStatus==TVSFirstPoint)
	// 				{
	// 
	// 					p1=
	// 				}
	// 
	// 
	// 
	// 				TVS_TAP* pTap=drawTapDirect(point1,midp,point2);
	// 				SetGlobalProperty(pTap);
	// 				SetPropertyLikePipe(pPipe1,pTap);
	// 				AcDbEntity * pEnt;
	// 				if (acdbOpenAcDbEntity(pEnt,pPipe1->id(),AcDb::kForWrite)==eOk){
	// 					pPipe1->put_FirstPoint(point1);
	// 					pPipe1->put_Lastpoint(shortlength(point1, midp,length2p(pTap->MA,pTap->MiddlePoint)));
	// 					pPipe1->close();
	// 
	// 				}
	// 				if (acdbOpenAcDbEntity(pEnt,pPipe2->id(),AcDb::kForWrite)==eOk){
	// 
	// 
	// 					pPipe2->put_FirstPoint(shortlength(point2, midp,length2p(pTap->MA,pTap->MiddlePoint)));
	// 					pPipe2->put_Lastpoint(point2);
	// 					pPipe2->close();
	// 				
	// 
	// 			}
	// 		} //end if line intersection
	// #pragma endregion
	// 
	// 	}
	// 

	static void SetPropertyLikePipe(TVS_Pipe *pPipe, TVS_Entity* pEnt)
	{
		AcDbEntity * pAEnt;
		if (acdbOpenAcDbEntity(pAEnt, pEnt->id(), AcDb::kForWrite) == eOk) {
			pEnt->put_SizeA(pPipe->get_SizeA());
			pEnt->put_SizeB(pPipe->get_SizeB());
			pEnt->put_Wipeout(pPipe->get_Wipeout());
			pEnt->put_Grani(pPipe->get_Grani());
			pEnt->put_This1D(pPipe->get_This1D());
			pEnt->put_Flow(pPipe->get_Flow());
			pEnt->put_Elevation(pPipe->get_Elevation());
			pEnt->setDuctType(pPipe->getDuctType());
			pEnt->setColor(pPipe->color());
			pEnt->setLineWeight(pPipe->lineWeight());
			pEnt->setLayer(pPipe->layer());
			pEnt->setLinetype(pPipe->linetype());
			pEnt->setLinetypeScale(pPipe->linetypeScale());
			pEnt->draw();
			pEnt->close();
		}
		else
		{
			acutPrintf(_T("\n Objekat je zaključan: svojstva se ne mogu kopirati"));
		}
	}
	static void InstallGlobalProperty(TVS_Entity *pEnt)
	{
		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;
		AcDbObjectId	id = pEnt->id();
		ads_name eName;


		acdbGetAdsName(eName, id);
		acdbGetObjectId(id, eName);
		AcDbEntity* pnt;


		//acdbOpenAcDbEntity(pnt,id,AcDb::kForRead);




		globSizeA = pEnt->get_SizeA();
		globSizeB = pEnt->get_SizeB();
		globalWipeout = pEnt->get_Wipeout();
		globalGrani = pEnt->get_Grani();
		global1D = pEnt->get_This1D();
		globalFlow = pEnt->get_Flow();
		globalDuctType = pEnt->getDuctType();
		globalElevMid = pEnt->get_Elevation();


		//	pEnt->close();
	}
	static void SetGlobalProperty(TVS_Entity *pEnt)
	{

		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;
		AcDbObjectId	id = pEnt->id();
		ads_name eName;


		// 	acdbGetAdsName(eName,id);
		// 	acdbGetObjectId(id,eName);
		AcDbEntity* pnt;


		if (acdbOpenAcDbEntity(pnt, pEnt->id(), AcDb::kForWrite) == eOk)


		{



			pEnt->put_SizeA(globSizeA);
			pEnt->put_SizeB(globSizeB);
			pEnt->put_Wipeout(globalWipeout);
			pEnt->put_WipeoutLength(globalWipeoutLength);
			pEnt->put_Grani(globalGrani);
			pEnt->put_This1D(global1D);
			pEnt->put_Flow(globalFlow);
			pEnt->put_Elevation(globalElevMid);
			pEnt->setDuctType(globalDuctType);


			if ((Pipi = TVS_Pipe::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Pipi->put_SizeA(SizeA);
				// 			if (cSizeB==false) Pipi->put_SizeB(SizeB);
				// 			if (cFlow==false) Pipi->put_Flow(Flow);
				// 			if (cGrani==false) Pipi->put_Grani(Grani);
				// 			if (cD1==false) Pipi->put_This1D(D1);
				// 			if (cWipe==false) Pipi->put_Wipeout(Wipe);
				// 			if (cElev==false) Pipi->put_Elevation(getElev(Elev,Emode,Pipi->SizeA,Pipi->SizeB));
				//


			}

			if ((Tapie = TVS_TAP::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Tapie->put_SizeA(SizeA);
				// 			if (cSizeB==false) Tapie->put_SizeB(SizeB);
				// 			if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Tapie->put_This1D(D1);
				// 			if (cWipe==false) Tapie->put_Wipeout(Wipe);
				// 			if (cElev==false) Tapie->put_Elevation(getElev(Elev,Emode,Tapie->SizeA,Tapie->SizeB));
				// 			//
				// 
				// 
				// 			if (cTapForm==false) Tapie->put_Form(TapForm);
				Tapie->put_TypeRoundTap(globalTypeRoundTap);
				Tapie->put_RadiusTypeRound(globalRadiusTypeRound);
				Tapie->put_RadiusTypeRect(globalRadiusTypeRect);
				Tapie->put_RadiusVariableParameter(globalTapRadiusVariableParameter);
				Tapie->put_RadiusConst(globalTapRadiusConst);
				// 			if (cSwectangle==false) Tapie->put_Swectangle(Swectangle);

			}

			if ((Wyeie = TVS_WYE::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Wyeie->put_SizeApr(SizeA);
				// 			if (cSizeB==false) Wyeie->put_SizeBpr(SizeB);
				// 			if (cSizeA2==false) Wyeie->put_SizeAotv(SizeA2);
				// 			if (cSizeB2==false) Wyeie->put_SizeBotv(SizeB2);
				// 			if (cLengthW==false) Wyeie->put_Length(LengthW);
				// 			//if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Wyeie->put_This1D(D1);
				// 			if (cWipe==false) Wyeie->put_Wipeout(Wipe);
				// 			if (cElev==false) Wyeie->put_Elevation(getElev(Elev,Emode,Wyeie->SizeApr,Wyeie->SizeBpr));
				// 			//


			}

			if ((Transie = TVS_TRANS::cast(pEnt)) != NULL)
			{
				// 			///
				// 			if (cSizeA==false) Transie->put_SizeAp1(SizeA);
				// 			if (cSizeB==false) Transie->put_SizeBp1(SizeB);
				// 			if (cSizeA2==false) Transie->put_SizeAp2(SizeA2);
				// 			if (cSizeB2==false) Transie->put_SizeBp2(SizeB2);
				// 			if (cLengthTr==false) Transie->put_Length(LengthTr);
				// 			//if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Transie->put_This1D(D1);
				// 			if (cWipe==false) Transie->put_Wipeout(Wipe);
				// 			if (cElev==false) Transie->put_Elevation(getElev(Elev,Emode,Transie->SizeAp1,Transie->SizeBp1));
				// 			//



			}

			pEnt->draw();
			pEnt->close();
		}



	}





	static bool changesize(bool allowUnits = false)
	{
		syncTVSGlobalUnits();

		//	ads_real sa,sb;
		double a, b;
		// 			a=globSizeA;
		// 			b=globSizeB;
		// 		if (acedGetReal(_T("\nВведите Ширину:"),&sa) != RTNORM)
		// 		{
		// 			return false;}
		// 
		// 		if (acedGetReal(_T("\nВведите Высоту:"),&sb) != RTNORM)
		// 		{
		// 			return false;}
		// 		globSizeA=sa;
		// 		globSizeB=sb;
		// 		if (globSizeB==0) globRound=true;
		// 		else globRound=false;

		dgPipeSize dg;
		dg.allowDrawingUnitsChange = allowUnits;
		a = dg.SizeA = globSizeA;
		b = dg.SizeB = globSizeB;
		dg.Flow = globalFlow;
		dg.LengthTr = globalLengthTr;
		dg.LengthW = globalLengthW;
		//dg.Tpipe=firstPipe;
		//dg.Twye=firstWye;
		//dg.Ttrans=firstTrans;
		//dg.Ttap=firstTap;
		dg.Wipe = globalWipeout;
		dg.Grani = globalGrani;
		dg.D1 = global1D;
		dg.ElevMid = true;
		dg.Elev = globalElevMid;
		dg.TapForm = globalTapForm;
		dg.TypeRoundTap = globalTypeRoundTap;
		dg.RadiusTypeRound = globalRadiusTypeRound;
		dg.RadiusTypeRect = globalRadiusTypeRect;
		dg.TapRadiusVariableParameter = globalTapRadiusVariableParameter;
		dg.TapRadiusConst = globalTapRadiusConst;

		dg.WipeoutLength = globalWipeoutLength;
		if (globalDuctType == DuctTypeFlex) dg.DuctFlex = TRUE; else dg.DuctFlex = FALSE;

		//AcApDocument *pDoc=acDocManager->curDocument();
		//acDocManager->lockDocument(pDoc,AcAp::kWrite);





		if (dg.DoModal() != IDOK) return false;
		syncTVSGlobalUnits();
		//acDocManager->unlockDocument(pDoc);
		globSizeA = dg.SizeA;
		globSizeB = dg.SizeB;
		double fl = globalFlow = dg.Flow;
		globalLengthTr = dg.LengthTr;
		globalLengthW = dg.LengthW;

		globalWipeout = dg.Wipe;
		globalGrani = dg.Grani;
		global1D = dg.D1;
		globalElevMid = dg.Elev;
		globalElev = dg.Elev;
		globalTapForm = dg.TapForm;
		globalTypeRoundTap = dg.TypeRoundTap;
		globalRadiusTypeRound = dg.RadiusTypeRound;
		globalRadiusTypeRect = dg.RadiusTypeRect;
		globalTapRadiusVariableParameter = dg.TapRadiusVariableParameter;
		globalTapRadiusConst = dg.TapRadiusConst;

		globalWipeoutLength = dg.WipeoutLength;

		if (dg.DuctFlex == TRUE) { globalDuctType = DuctTypeFlex; globSizeB = 0; }
		else globalDuctType = DuctTypeStill;
		if (globSizeB == 0) globRound = true;
		else globRound = false;
		if (a == globSizeA && b == globSizeB)
			return false;
		else
			return true;
	}


	static TVS_TAP* drawTapDirect(AcGePoint3d t1, AcGePoint3d t2, AcGePoint3d t3)
	{



		double pSizeA = globSizeA;
		double pSizeB = globSizeB;
		double pRadius = globalTapRadiusConst;
		AcGeVector3d pNormVect;
		AcGeVector3d pStartVect;
		AcGePoint3d pMidPoint;
		double pSwectangle;
		bool pThisRound = globRound;

		pMidPoint = t2;
		if (!GiveStartvectorAndAngle(t1, t2, t3, pSwectangle, pStartVect)) return NULL;
		pStartVect.normalize();
		pNormVect = AcGeVector3d(0, 0, 1);
		TVS_TAP* pEnt = TVS_TAP::add_new(pSizeA, pSizeB, pRadius, pNormVect, pStartVect, pMidPoint, pSwectangle, false, pThisRound);
		AcDbObjectId	id = pEnt->id();
		ads_name eName;


		acdbGetAdsName(eName, id);
		acdbGetObjectId(id, eName);
		AcDbEntity* pnt;
		acdbOpenAcDbEntity(pnt, id, AcDb::kForWrite);

		pEnt->assertWriteEnabled();
		pEnt->RadiusConst = (globalTapRadiusConst);
		pEnt->RadiusTypeRound = (globalRadiusTypeRound);
		pEnt->RadiusTypeRect = (globalRadiusTypeRect);
		pEnt->TypeRoundTap = (globalTypeRoundTap);
		pEnt->Form = (globalTapForm);
		pEnt->RadiusVariableParameter = (globalTapRadiusVariableParameter);
		pEnt->RadiusConst = (globalTapRadiusConst);
		pEnt->draw();
		pEnt->close();

		return pEnt;
	}



	static void nextpipe(AcGePoint3d &A1,
		AcGePoint3d &A2,
		AcGePoint3d &A3,
		TVS_Pipe *&pipi,
		ads_point &pt1
	)
	{


		double tapradius, radius, pSwectangle;
		AcGePoint3d pCenterpoint;
		AcGePoint3d lastpipi;
		TVS_Pipe pipie;
		TVS_TAP * pTap;
		AcGeVector3d pnormvect = AcGeVector3d(0, 0, 1);
		AcGeVector3d pstartvect;
		AcDbObjectId id;
		ads_name eName;
		AcDbEntity *pEnt;
		AcGeVector3d vect1, vect2, vect22;
		vect1 = AcGeVector3d(A2.x - A1.x,
			A2.y - A1.y,
			A2.z - A1.z);
		vect2 = AcGeVector3d(A3.x - A2.x,
			A3.y - A2.y,
			A3.z - A2.z);
		vect22 = AcGeVector3d(-A3.x + A2.x,
			-A3.y + A2.y,
			-A3.z + A2.z);
		if (vect1.length() == 0.0 || vect2.length() == 0.0) return;
		vect1.normalize();
		vect2.normalize();
		vect22.normalize();
		if ((vect1 != vect2) && (vect1 != vect22))
		{



			pTap = drawTapDirect(A1, A2, A3);
			if (!pTap) return;
			SetGlobalProperty(pTap);
			double delta = length2p(pTap->MA, pTap->MiddlePoint);
			AcGePoint3d A2a = shortlength(A1, A2, delta);
			AcGePoint3d A2b = shortlength(A3, A2, delta);
			if ((length2p(A1, A2) >= delta) && (length2p(A2, A3) >= delta))
			{

				id = pipi->id();
				acdbGetAdsName(eName, id);
				acdbGetObjectId(id, eName);
				acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite);
				pipi->assertWriteEnabled();
				pipi->LastPoint = A2a;
				pipi->close();
				pipi = pipie.add_new(A2b, A3, globSizeA, globSizeB, false, globRound);
				SetGlobalProperty(pipi);
				A1 = A2;
				A2 = A3;

				pt1[0] = A2.x;
				pt1[1] = A2.y;
				pt1[2] = A2.z;

			}
			else
			{

				AcDbEntity* pnt;
				acdbOpenAcDbEntity(pnt, pTap->id(), AcDb::kForWrite);
				pTap->erase();
				pTap->close();

			}
		}
		else
		{


			id = pipi->id();
			acdbGetAdsName(eName, id);
			acdbGetObjectId(id, eName);
			acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite);
			pipi->assertWriteEnabled();
			pipi->setLastpoint(A3);
			pipi->close();
			SetGlobalProperty(pipi);
			A2 = A3;

			pt1[0] = A2.x;
			pt1[1] = A2.y;
			pt1[2] = A2.z;


		}

	}


	static void connect2(
		TVS_Pipe *&pipi1
		, int connectstatus
	)

	{

		TVS_Pipe *pipi2;
		bool ft = false;
		bool lt = false;
		int stat;
		ads_point pt1;
		AcDbObjectId id;
		ads_name eName, vozd;
		AcDbEntity *pEnt, *pEnt2;
		while (ft == false)
		{
			while (lt == false)
			{
				stat = acedEntSel(_T("\nIzaberite kanal:"), vozd, pt1);

				switch (stat)
				{
				case RTNORM:
					lt = true;
					break;
				case RTCAN:
					return;
					break;
				}
			}

			if (acdbGetObjectId(id, vozd) == eOk)
			{

				acdbGetObjectId(id, vozd);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite) == eOk)
					{
						if ((pipi2 = TVS_Pipe::cast(pEnt)) != NULL)
						{

							ft = true;

						}
					}

					else {
						consoleprint(0, _T("\nObjekat je zaključan"));

						return;
					}
					pEnt->close();
				}
				}


			}
		}
		pEnt2 = AcDbEntity::cast(pipi1);
		//pCon(pEnt2,pEnt);
		if (connectstatus == dCONNECTT) ConnectWithTap(pipi1, pipi2, asDblArray(pipi1->LastPoint), pt1);
		if (connectstatus == dCONNECTW) ConnectWithWye(pipi1, pipi2, asDblArray(pipi1->LastPoint), pt1);

	}


	static bool changeZ(TVS_Pipe* &pPipe)
	{
		AcDbEntity * pEnt;
		double startZ, nextZ, Axis;
		dgChangeZ dg;
		startZ = globalElevMid;
		nextZ = globalElevMid;
		Axis = globalAxis;
		dg.startZ = startZ;
		dg.nextZ = nextZ;
		dg.Axis = Axis;
		dg.DoModal();
		startZ = dg.startZ;
		nextZ = dg.nextZ;
		Axis = dg.Axis;
		globalAxis = Axis;
		globalElevMid = nextZ;


		if (dg.nextZ != startZ)
		{
			double x = pPipe->LastPoint.x - pPipe->FirstPoint.x;
			double y = pPipe->LastPoint.y - pPipe->FirstPoint.y;
			double z = pPipe->LastPoint.z - pPipe->FirstPoint.z;
			Axis = M_PI*Axis / 180;
			double Axis2 = Axis; //correct axis while correct view tap
			if (abs(M_PI / 2 - Axis2) < 0.0001) Axis2 = M_PI*89.4 / 180;

			TVS_TAP* pTap = TVS_TAP::add_new(pPipe->SizeA, pPipe->SizeB, Axis, AcGeVector3d(0, 0, 1), AcGeVector3d(y, -x, 0), pPipe->LastPoint, Axis2, pPipe->This1D, pPipe->ThisRound);
			SetGlobalProperty(pTap);
			SetPropertyLikePipe(pPipe, pTap);

			acdbOpenAcDbEntity(pEnt, pTap->id(), AcDb::kForWrite);
			pTap->put_Form(Form_Up);
			pTap->close();
			pTap->draw();
			TVS_TAP* pTap2 = TVS_TAP::add_new(pPipe->SizeA, pPipe->SizeB, Axis, AcGeVector3d(0, 0, 1), AcGeVector3d(-y, x, 0), pPipe->LastPoint, Axis2, pPipe->This1D, pPipe->ThisRound);
			SetGlobalProperty(pTap2);
			SetPropertyLikePipe(pPipe, pTap2);

			double plecho1 = abs(length2p(pTap->MA, pTap->MiddlePoint));
			double plecho2 = abs(length2p(pTap->MC, pTap->MiddlePoint));
			double dLx;
			if (abs(M_PI / 2 - Axis) >= 0.0001)  dLx = abs(tan(pTap->Swectangle)*(nextZ - startZ));
			else dLx = 0;
			AcGePoint3d p1, p2, p3, p4, p5, p6, p7;
			p2 = pPipe->LastPoint;
			p1 = shortlength(pPipe->FirstPoint, p2, plecho1);
			p3 = shortlength(pPipe->FirstPoint, p2, -plecho2);
			p4 = shortlength(pPipe->FirstPoint, p2, plecho2 - dLx);
			p5 = shortlength(pPipe->FirstPoint, p2, -dLx);
			p6 = shortlength(pPipe->FirstPoint, p2, -dLx - plecho1);
			p7 = shortlength(pPipe->FirstPoint, p2, -dLx - plecho1 - 300 / tvsUnits::factor());


			if (startZ > nextZ)
			{

				acdbOpenAcDbEntity(pEnt, pTap2->id(), AcDb::kForWrite);
				pTap2->put_Form(Form_Up);
				pTap2->put_Centerpoint(p5);
				pTap2->close();
				acdbOpenAcDbEntity(pEnt, pTap->id(), AcDb::kForWrite);
				pTap->put_Form(Form_Down);
				pTap->close();

			}
			else
			{
				acdbOpenAcDbEntity(pEnt, pTap->id(), AcDb::kForWrite);
				pTap->put_Form(Form_Up);
				pTap->put_Elevation(startZ);
				pTap->close();

				acdbOpenAcDbEntity(pEnt, pTap2->id(), AcDb::kForWrite);
				pTap2->put_Form(Form_Down);
				pTap2->put_Centerpoint(p5);
				pTap2->put_Elevation(nextZ);
				pTap2->close();

			}

			if (abs(M_PI / 2 - Axis) >= 0.0001) //draw pPipe1
			{
				TVS_Pipe*pPipe1 = pPipe1->add_new(p3, p4, globSizeA, globSizeB);
				SetGlobalProperty(pPipe1);
				SetPropertyLikePipe(pPipe, pPipe1);
				acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);

				pPipe1->put_Elevation(startZ);
				pPipe1->close();
			}
			//draw pPipe2
			TVS_Pipe*pPipe2 = pPipe2->add_new(p6, p7, globSizeA, globSizeB);
			SetGlobalProperty(pPipe2);
			SetPropertyLikePipe(pPipe, pPipe2);
			acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite);

			pPipe2->put_Elevation(nextZ);
			pPipe2->close();
			//correct PPipe
			acdbOpenAcDbEntity(pEnt, pPipe->id(), AcDb::kForWrite);
			pPipe->put_Lastpoint(p1);
			pPipe->close();
			pPipe = pPipe2;

		}
		else return false;

		return true;
	}
	static void addtrans(double &pSizeAp1,
		double &pSizeBp1,
		double &pSizeAp2,
		double &pSizeBp2,
		AcGePoint3d &A1,
		AcGePoint3d &A2,
		AcGePoint3d &A3,
		TVS_Pipe *&pipi,
		ads_point &pt1
	)
	{

		TVS_TRANS* transie;
		double pLengthTr = globalLengthTr;
		AcGeVector3d Vect = AcGeVector3d(pipi->LastPoint.x - pipi->FirstPoint.x,
			pipi->LastPoint.y - pipi->FirstPoint.y,
			pipi->LastPoint.z - pipi->FirstPoint.z);
		AcGeVector3d pVectTr;
		AcGePoint3d pFirstPoint = pipi->LastPoint;
		pVectTr = Vect.normalize()*pLengthTr;

		double pThisRoundp1,
			pThisRoundp2,
			pTransType = ftTransType,
			pThis1D = false;

		if (pSizeBp1 == 0) pThisRoundp1 = true;
		else pThisRoundp1 = false;

		if (pSizeBp2 == 0) pThisRoundp2 = true;
		else pThisRoundp2 = false;

		transie = TVS_TRANS::add_new(pSizeAp1,
			pSizeBp1,
			pSizeAp2,
			pSizeBp2,
			pLengthTr,
			pVectTr,
			pFirstPoint,
			pThisRoundp1,
			pThisRoundp2,
			pTransType,
			pThis1D);
		SetGlobalProperty(transie);
		pFirstPoint = AcGePoint3d(pFirstPoint.x + pLengthTr*Vect.x,
			pFirstPoint.y + pLengthTr*Vect.y,
			pFirstPoint.z + pLengthTr*Vect.z
		);
		AcGePoint3d pLastPoint = AcGePoint3d(pFirstPoint.x + 100 * Vect.x,
			pFirstPoint.y + 100 * Vect.y,
			pFirstPoint.z + 100 * Vect.z
		);

		bool pThis1d = false;

		pipi = pipi->add_new(pFirstPoint,
			pLastPoint,
			pSizeAp2,
			pSizeBp2,
			pThis1d,
			pThisRoundp2);
		SetGlobalProperty(pipi);
		A1 = A2;
		A2 = pLastPoint;
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;
	}


	static void Ventilation_ARXTVS_SETTINGS(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		changesize(true);
	}

	static void Ventilation_ARXTVS_DRAW(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		int index;
		AcDbObjectId id;
		TVS_Pipe::GetParamsForDraw(id, index);


		ads_point pt1, pt2;
		AcDbEntity *pEnt;
		TVS_Pipe *pipi;

		bool stat = true;
		AcGePoint3d p1, p2;
		if (index == 0)
		{
			stat = drawStep1(pt1, pt2, pipi);
			if (stat == true) stat = drawStep2(pt1, pt2, pipi);
			if (stat == true) stat = drawStep3(pt1, pt2, pipi);
		}
		else
		{

			if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite) == eOk)
			{

				if ((pipi = TVS_Pipe::cast(pEnt)) != NULL)
				{
					InstallGlobalProperty(pipi);
					if (index == 102)
					{

						AcGePoint3d buf = pipi->get_FirstPoint();
						pipi->put_FirstPoint(pipi->LastPoint);
						pipi->put_Lastpoint(buf);
					}
					pt1[0] = pipi->get_FirstPoint().x;
					pt1[1] = pipi->get_FirstPoint().y;
					pt1[2] = pipi->get_FirstPoint().z;
					pt2[0] = pipi->get_Lastpoint().x;
					pt2[1] = pipi->get_Lastpoint().y;
					pt2[2] = pipi->get_Lastpoint().z;
					pEnt->close();
					if (drawStep3(pt1, pt2, pipi) == false);
				}

			}
		}
		//TVS_Pipe::SetParamsForDraw(id,0);
		//TVS_Pipe pipie;


		//pipi=pipie.add_new(A1,A2,globSizeA,globSizeB,false,globRound);

		//if(drawStep1(pt1,pt2,pipi)==false)return;
		//if(drawStep2(pt1,pt2,pipi)==false)return;

	}
	static bool drawStep1(ads_point &pt1, ads_point &pt2, TVS_Pipe *&pipi)
	{

		int reg;
		int Astat;
		int Astat2 = false;
		ACHAR  resultss[512];
		ACHAR  resultnil[512];


		while (Astat2 == false)
		{
			Astat2 = true;
			acedInitGet(RSG_NONULL, _T("Dimenzija D"));
			Astat = acedGetPoint(NULL, _T("\nZadajte prvu tačku ili [Dimenzija]: "), pt1);
			switch (Astat)
			{
			case RTCAN:
				return false;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				changesize();
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}

		}
		return true;
	}


	static bool drawStep2(ads_point &pt1, ads_point &pt2, TVS_Pipe *&pipi)
	{


		int reg;
		int Astat;
		int Astat2 = false;
		ACHAR  resultss[512];
		ACHAR  resultnil[512];
		AcDbEntity * pEnt;
		while (Astat2 == false)
		{
			Astat2 = true;
			acedInitGet(RSG_NONULL, _T("Dimenzija D"));
			Astat = acedGetPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija]: "), pt2);
			switch (Astat)
			{
			case RTCAN:
				return false;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				changesize();
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}



		}




		AcGePoint3d A1 = asPnt3d(pt1);
		AcGePoint3d A2 = asPnt3d(pt2);

		TVS_Pipe pipie;


		pipi = pipie.add_new(A1, A2, globSizeA, globSizeB, false, globRound);

		if (acdbOpenAcDbEntity(pEnt, pipi->id(), AcDb::kForWrite) == eOk)
		{

			pipi->close();
		}
		else
		{
			acutPrintf(_T("\nSloj je zaključan"));
			return false;
		}




		return true;
	}

	static bool drawStep3(ads_point &pt1, ads_point &pt2, TVS_Pipe *&pipi)
	{

		int reg;
		int Astat;
		int Astat2 = false;
		ACHAR  resultss[512];
		ACHAR  resultnil[512];
		AcDbEntity * pEnt;

		AcGePoint3d A1 = asPnt3d(pt1);
		AcGePoint3d A2 = asPnt3d(pt2);
		AcGePoint3d A3;
		AcGeVector3d pnormvect = AcGeVector3d(0, 0, 1);
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;
		double oldsizeA, oldsizeB, oldRound;
		for (; ;)
		{


			acedInitGet(RSG_NONULL, _T("Dimenzija D Koljeno K Račva R Z Gore G Nadolje N"));
			Astat = acedGetPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Z/Gore/Nadolje]: "), pt2);
			switch (Astat)
			{
			case RTCAN:
				return false;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			case RTNORM:
				A3 = asPnt3d(pt2);
				nextpipe(A1, A2, A3, pipi, pt1);

			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				oldsizeA = globSizeA;
				oldsizeB = globSizeB;
				oldRound = globRound;
				if (changesize() == true)
				{
					addtrans(oldsizeA, oldsizeB, globSizeA, globSizeB, A1, A2, A3, pipi, pt1);
				}
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}
			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Koljeno")) == 0 || _wcsicmp(resultss, _T("K")) == 0))
			{
				reg = dCONNECTT;
				connect2(pipi, reg);
				return false;
			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Račva")) == 0 || _wcsicmp(resultss, _T("R")) == 0))
			{
				reg = dCONNECTW;
				connect2(pipi, reg);
				return false;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Z")) == 0))
			{
				reg = dZ;
				changeZ(pipi);
				A1 = A2;
				A2 = pipi->LastPoint;
				pt1[0] = A2.x;
				pt1[1] = A2.y;
				pt1[2] = A2.z;
				acdbRToS(0, 2, 2, resultss);
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Gore")) == 0 || _wcsicmp(resultss, _T("G")) == 0))
			{
				reg = dUP;
				UpDown(pipi, Form_Up);
				return false;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Nadolje")) == 0 || _wcsicmp(resultss, _T("N")) == 0))
			{
				reg = dDOWN;
				UpDown(pipi, Form_Down);
				return false;
			}

		}

	}
	// ----- Ventilation_ARX.TVS_PIPE command
	static void Ventilation_ARXTVS_PIPE(void)
	{

		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();

		ads_point pt1, pt2;
		int reg;
		int Astat;
		int Astat2 = false;
		ACHAR  resultss[512];
		ACHAR  resultnil[512];
		AcDbEntity* pEnt;

		while (Astat2 == false)
		{
			Astat2 = true;
			acedInitGet(RSG_NONULL, _T("Dimenzija D"));
			Astat = acedGetPoint(NULL, _T("\nZadajte prvu tačku ili [Dimenzija]: "), pt1);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				changesize();
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}

		}

		Astat2 = false;



		while (Astat2 == false)
		{
			Astat2 = true;
			acedInitGet(RSG_NONULL, _T("Dimenzija D"));
			Astat = acedGetPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija]: "), pt2);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				changesize();
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}



		}




		AcGePoint3d A1 = asPnt3d(pt1);
		AcGePoint3d A2 = asPnt3d(pt2);
		TVS_Pipe *pipi;
		TVS_Pipe pipie;


		pipi = pipie.add_new(A1, A2, globSizeA, globSizeB, false, globRound);
		SetGlobalProperty(pipi);
		if (acdbOpenAcDbEntity(pEnt, pipi->id(), AcDb::kForWrite) == eOk)
		{
			pipi->close();
		}
		else
		{
			acutPrintf(_T("\nSloj je zaključan"));
			return;
		}


		AcGeVector3d pnormvect = AcGeVector3d(0, 0, 1);
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;

		////////////
		AcGePoint3d A3;
		double oldsizeA, oldsizeB, oldRound;
		for (; ;)
		{


			acedInitGet(RSG_NONULL, _T("Dimenzija D Koljeno K Račva R Z Gore G Nadolje N"));
			Astat = acedGetPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Z/Gore/Nadolje]: "), pt2);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			case RTNORM:
				A3 = asPnt3d(pt2);
				nextpipe(A1, A2, A3, pipi, pt1);

			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				oldsizeA = globSizeA;
				oldsizeB = globSizeB;
				oldRound = globRound;
				if (changesize() == true)
				{
					addtrans(oldsizeA, oldsizeB, globSizeA, globSizeB, A1, A2, A3, pipi, pt1);
				}
				Astat2 = false;
				acdbRToS(0, 2, 2, resultss);
			}
			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Koljeno")) == 0 || _wcsicmp(resultss, _T("K")) == 0))
			{
				reg = dCONNECTT;
				connect2(pipi, reg);
				return;
			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Račva")) == 0 || _wcsicmp(resultss, _T("R")) == 0))
			{
				reg = dCONNECTW;
				connect2(pipi, reg);
				return;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Z")) == 0))
			{
				reg = dZ;
				changeZ(pipi);
				A1 = A2;
				A2 = pipi->LastPoint;
				pt1[0] = A2.x;
				pt1[1] = A2.y;
				pt1[2] = A2.z;
				acdbRToS(0, 2, 2, resultss);
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Gore")) == 0 || _wcsicmp(resultss, _T("G")) == 0))
			{
				reg = dUP;
				UpDown(pipi, Form_Up);
				return;
			}

			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Nadolje")) == 0 || _wcsicmp(resultss, _T("N")) == 0))
			{
				reg = dDOWN;
				UpDown(pipi, Form_Down);
				return;
			}

		}





		// Add your code for command TVSTVS_Vent_ARX.TVS_Pipe here
	}


	// - Ventilation_ARX.TVS_TRANS command (do not rename)
	static void Ventilation_ARXTVS_TRANS(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_point pt1, pt2;
		double pSizeAp1 = 500 / tvsUnits::factor();
		double pSizeBp1 = 0 / tvsUnits::factor();
		double pSizeAp2 = 300 / tvsUnits::factor();
		double pSizeBp2 = 0 / tvsUnits::factor();
		double pLengthTr = 300 / tvsUnits::factor();
		AcGeVector3d pVectTr = AcGeVector3d(1, 1, 0);
		AcGePoint3d pFirstPoint;
		AcGePoint3d pLastPoint;

		bool pThisRoundp1 = false;
		bool pThisRoundp2 = false;
		int pTransType = 3;
		bool pThis1D = false;
		TVS_TRANS transie;

		if (acedGetPoint(NULL, _T("Zadajte prvu tačku:\n"), pt1) != RTNORM)
			return;
		if (acedGetPoint(pt1, _T("Set the second point:\n"), pt2) != RTNORM)
			return;
		pFirstPoint = asPnt3d(pt1);
		pLastPoint = asPnt3d(pt2);
		pVectTr = AcGeVector3d(pLastPoint.x - pFirstPoint.x,
			pLastPoint.y - pFirstPoint.y,
			pFirstPoint.z
		);

		transie.add_new(pSizeAp1,
			pSizeBp1,
			pSizeAp2,
			pSizeBp2,
			pLengthTr,
			pVectTr,
			pFirstPoint,
			pThisRoundp1,
			pThisRoundp2,
			pTransType,
			pThis1D);



		// Add your code for command Ventilation_ARX.TVS_TRANS here
	}

	// - Ventilation_ARX.TVS_WYE command (do not rename)
	static void Ventilation_ARXTVS_WYE(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_point pt1;
		TVS_WYE wyeie;
		ads_real aSizeAotv, aSizeApr;
		double pSizeApr = 200 / tvsUnits::factor();
		double pSizeBpr = 0 / tvsUnits::factor();
		double pSizeAotv = 100 / tvsUnits::factor();
		double pSizeBotv = 0 / tvsUnits::factor();
		double pLengthPl = 150 / tvsUnits::factor();
		AcGeVector3d pVectpr = AcGeVector3d(1, 0, 0);
		AcGeVector3d pVectotv = AcGeVector3d(0, 1, 0);
		AcGePoint3d pBasepoint;
		bool pThisRoundpr = true;
		bool pThisRoundotv = true;
		bool pThis1D = false;

		if (acedGetPoint(NULL, _T("Zadajte prvu tačku:\n"), pt1) != RTNORM)
			return;
		if (acedGetReal(_T("\nUnesite širinu glavnog kanala: "), &aSizeApr) != RTNORM)
			return;
		pSizeApr = aSizeApr / tvsUnits::factor();
		if (acedGetReal(_T("\nUnesite širinu ogranka: "), &aSizeAotv) != RTNORM)
			return;
		pSizeAotv = aSizeAotv / tvsUnits::factor();

		pBasepoint = asPnt3d(pt1);
		wyeie.add_new(pSizeApr,
			pSizeBpr,
			pSizeAotv,
			pSizeBotv,
			pLengthPl,
			pVectpr,
			pVectotv,
			pBasepoint,
			pThisRoundpr,
			pThisRoundotv,
			pThis1D);
		wyeie.add_new(pSizeApr,
			pSizeBpr,
			pSizeAotv,
			pSizeBotv,
			pLengthPl,
			pVectpr,
			pVectotv,
			pBasepoint,
			pThisRoundpr,
			false,
			pThis1D);
		wyeie.add_new(pSizeApr,
			pSizeBpr,
			pSizeAotv,
			pSizeBotv,
			pLengthPl,
			pVectpr,
			pVectotv,
			pBasepoint,
			false,
			false,
			pThis1D);
		wyeie.add_new(pSizeApr,
			pSizeBpr,
			pSizeAotv,
			pSizeBotv,
			pLengthPl,
			pVectpr,
			pVectotv,
			pBasepoint,
			false,
			true,
			pThis1D);
		// Add your code for command Ventilation_ARX.TVS_WYE here
	}

	// - Ventilation_ARX.TVS_CONNECT command (do not rename)
	static void Ventilation_ARXTVS_CONNECTW(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		// Add your code for command Ventilation_ARX.TVS_CONNECT here

		double Lx, Ly, startangle;
		ads_name vozd1, vozd2, eName;
		ACHAR handle[17];
		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt1, *pEnt2 = NULL;
		AcDbObjectId id;
		TVS_WYE wyie;
		TVS_TAP * Tapie;
		TVS_Pipe * Pipi1, *Pipi2;
		double NewSiseA, NewRadius;
		resbuf *rb = NULL;
		bool ft = false;
		while (ft == false)
		{

			if (acedEntSel(_T("\nIzaberite prvi kanal: "), vozd1, pt1) == RTCAN)
				return;



			if (acdbGetObjectId(id, vozd1) == eOk)
			{

				acdbGetObjectId(id, vozd1);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt1, id, AcDb::kForWrite) == eOk)
					{
						if ((Pipi1 = TVS_Pipe::cast(pEnt1)) != NULL)
						{
							//acutPrintf(_T("\nPipe 1...ок"));
							ft = true;

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}
					pEnt1->close();
				}
				}


			}
		}
		ft = false;
		while (ft == false)
		{
			if (acedEntSel(_T("\nIzaberite drugi kanal: "), vozd2, pt2) == RTCAN)
				return;

			if (acdbGetObjectId(id, vozd2) == eOk)
			{

				acdbGetObjectId(id, vozd2);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt2, id, AcDb::kForWrite) == eOk)
					{
						if ((Pipi2 = TVS_Pipe::cast(pEnt2)) != NULL)
						{
							if (Pipi1 != Pipi2)
							{
								//acutPrintf(_T("\nPipe 2...ок"));
								ft = true;
							}
							else
							{
								acutPrintf(_T("\nIzabran je isti kanal"));

							}

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}

					pEnt2->close();
				}
				}


			}


		}

		ConnectWithWye(pEnt1, pEnt2, pt1, pt2);
	}

	static void Ventilation_ARXTVS_CONNECTT(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		// Add your code for command Ventilation_ARX.TVS_CONNECT here

		double Lx, Ly, startangle;
		ads_name vozd1, vozd2, eName;
		ACHAR handle[17];
		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt1, *pEnt2 = NULL;
		AcDbObjectId id;
		TVS_WYE wyie;
		TVS_TAP * Tapie;
		TVS_Pipe * Pipi1, *Pipi2;
		double NewSiseA, NewRadius;
		resbuf *rb = NULL;
		bool ft = false;
		while (ft == false)
		{

			if (acedEntSel(_T("\nIzaberite prvi kanal: "), vozd1, pt1) == RTCAN)
				return;



			if (acdbGetObjectId(id, vozd1) == eOk)
			{

				acdbGetObjectId(id, vozd1);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt1, id, AcDb::kForWrite) == eOk)
					{
						if ((Pipi1 = TVS_Pipe::cast(pEnt1)) != NULL)
						{
							//acutPrintf(_T("\nPipe 1...ок"));
							ft = true;

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}
					pEnt1->close();
				}
				}


			}
		}
		ft = false;
		while (ft == false)
		{
			if (acedEntSel(_T("\nIzaberite drugi kanal: "), vozd2, pt2) == RTCAN)
				return;

			if (acdbGetObjectId(id, vozd2) == eOk)
			{

				acdbGetObjectId(id, vozd2);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt2, id, AcDb::kForWrite) == eOk)
					{
						if ((Pipi2 = TVS_Pipe::cast(pEnt2)) != NULL)
						{
							if (Pipi1 != Pipi2)
							{
								//acutPrintf(_T("\nPipe 2...ок"));
								ft = true;
							}
							else
							{
								acutPrintf(_T("\nIzabran je isti kanal"));

							}

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}

					pEnt2->close();
				}
				}


			}


		}

		ConnectWithTap(pEnt1, pEnt2, pt1, pt2);
	}

	static void Ventilation_ARXTVS_CONNECTTest(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		// Add your code for command Ventilation_ARX.TVS_CONNECT here

		double Lx, Ly, startangle;
		ads_name vozd1, vozd2, eName;
		ACHAR handle[17];
		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt1, *pEnt2 = NULL;
		AcDbObjectId id1, id2;
		TVS_WYE wyie;
		TVS_TAP * Tapie;
		TVS_Pipe * Pipi1, *Pipi2;
		double NewSiseA, NewRadius;
		resbuf *rb = NULL;
		bool ft = false;
		while (ft == false)
		{

			if (acedEntSel(_T("\nIzaberite prvi kanal: "), vozd1, pt1) == RTCAN)
				return;



			if (acdbGetObjectId(id1, vozd1) == eOk)
			{

				acdbGetObjectId(id1, vozd1);

				{if (id1 != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt1, id1, AcDb::kForWrite) == eOk)
					{
						if ((Pipi1 = TVS_Pipe::cast(pEnt1)) != NULL)
						{

							//acutPrintf(_T("\nPipe 1...ок"));
							ft = true;

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}
					pEnt1->close();
				}
				}


			}
		}
		ft = false;
		while (ft == false)
		{
			if (acedEntSel(_T("\nIzaberite drugi kanal: "), vozd2, pt2) == RTCAN)
				return;

			if (acdbGetObjectId(id2, vozd2) == eOk)
			{

				acdbGetObjectId(id2, vozd2);

				{if (id2 != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt2, id2, AcDb::kForWrite) == eOk)
					{
						if ((Pipi2 = TVS_Pipe::cast(pEnt2)) != NULL)
						{
							if (Pipi1 != Pipi2)
							{
								//acutPrintf(_T("\nPipe 2...ок"));
								ft = true;
							}
							else
							{
								acutPrintf(_T("\nIzabran je isti kanal"));

							}

						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));

						return;
					}

					pEnt2->close();
				}
				}


			}


		}

		acdbOpenAcDbEntity(pEnt1, id1, AcDb::kForWrite);
		(Pipi1 = TVS_Pipe::cast(pEnt1));

		acdbOpenAcDbEntity(pEnt2, id2, AcDb::kForWrite);
		(Pipi2 = TVS_Pipe::cast(pEnt2));

		int con1end;
		Pipi1->getConnectorByIndex(conPipeEnd, con1end);

		int con2start;
		Pipi2->getConnectorByIndex(conPipeStart, con2start);

		Pipi1->connectors[con1end].connectionID = id2;
		Pipi1->connectors[con1end].cTypeAnother = Pipi2->connectors[con2start].cTypeCurrent;

		Pipi2->connectors[con2start].connectionID = id1;
		Pipi2->connectors[con2start].cTypeAnother = Pipi1->connectors[con1end].cTypeCurrent;
		Pipi1->close();
		Pipi2->close();
	}

	// - Ventilation_ARX.TVS_DUCT command (do not rename)
	static void Ventilation_ARXTVS_DUCT(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();

		// Add your code for command TVSTVS_Vent_ARX.MyCommand1 here

		//AcGePoint3d * gg=new AcGePoint3d;
		double Pi = 3.14159265358979323846;
		AcDbEntity *pEnt;
		ads_point pt1, pt2, pt3;
		TVS_Pipe *pipi;
		TVS_Pipe pipie;
		TVS_TAP tapie;
		ads_real sa, sb;
		AcDbObjectId id = AcDbObjectId::kNull;
		ads_name eName;
		double pSizeA = 250, pSizeB = 0, X1, X2, Y1, Y2, pSwectangle, radius = 150 / tvsUnits::factor(), tapradius;
		AcGePoint3d A1, A2, A3, pCenterpoint, lastpipi;
		AcGeVector3d pnormvect, pstartvect;
		bool pThisRound = false;

		if (acedGetReal(_T("\nUnesite širinu: "), &sa) != RTNORM)
		{
			return;
		}
		if (acedGetReal(_T("\nUnesite visinu: "), &sb) != RTNORM)
		{
			return;
		}
		pSizeA = sa / tvsUnits::factor();
		pSizeB = sb / tvsUnits::factor();



		if (acedGetPoint(NULL, _T("\nZadajte prvu tačku: "), pt1) != RTNORM)
			return;
		A1 = asPnt3d(pt1);
		if (acedGetPoint(pt1, _T("\nZadajte sljedeću tačku: "), pt2) != RTNORM)
			return;
		A2 = asPnt3d(pt2);
		pipi = pipie.add_new(A1, A2, pSizeA, pSizeB, false, pThisRound);

		pnormvect = AcGeVector3d(0, 0, 1);
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;


		///////////
		for (; ;)
		{
			if (acedGetPoint(pt1, _T("\nZadajte sljedeću tačku: "), pt2) != RTNORM)
				return;
			A3 = asPnt3d(pt2);
			if (pThisRound == true)
			{
				if (pSizeA * tvsUnits::factor() < 355)
				{
					tapradius = pSizeA + pSizeA / 2;
					radius = pSizeA;
				}
				else
				{
					tapradius = pSizeA / 2 + pSizeA / 2;
					radius = pSizeA / 2;
				}
			}
			else
			{
				tapradius = radius + pSizeA / 2;
			}
			Givecenterpoint(A1, A2, A3, tapradius, pCenterpoint, pSwectangle, pstartvect);
			if (correctpipes(A1, A2, A3, pSwectangle, pSizeA, radius, lastpipi, pThisRound) == true)
			{
				tapie.add_new(pSizeA, pSizeB, radius, pnormvect, pstartvect, pCenterpoint, pSwectangle, false, pThisRound);
				drawTapDirect(A1, A2, A3);
				id = pipi->id();
				acdbGetAdsName(eName, id);
				//acdbEntDel(ename);
				//pipi=pipie.add_new(A1,lastpipi,pSizeA,pSizeB);
				acdbGetObjectId(id, eName);
				acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite);
				pipi->assertWriteEnabled();
				pipi->LastPoint = lastpipi;
				pipi->close();
				pipi = pipie.add_new(A2, A3, pSizeA, pSizeB, false, pThisRound);
				A1 = A2;
				A2 = A3;

				pt1[0] = A2.x;
				pt1[1] = A2.y;
				pt1[2] = A2.z;

			}

			//pstartvect= AcGeVector3d(1,0,0);
			//pSwectangle=1.7;















		}
		// Add your code for command Ventilation_ARX.TVS_DUCT here
	}

	// - Ventilation_ARX.TVS_1D2D command (do not rename)
	static void Ventilation_ARXTVS_1D2D(void)
	{

		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();

		ads_name sset, eName;
		ACHAR  resultss[512];
		ACHAR  results1[512] = _T("2");
		ACHAR  results2[512] = _T("2d");

		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt = NULL;
		AcDbObjectId id;
		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;

		double NewSiseA;
		bool p1d;
		resbuf *rb = NULL;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;
		acedInitGet(RSG_NOZERO, _T("1 2 1d 2d"));
		if (acedGetKword(_T("\nPretvorite u [1d/2d]<2d>: "), resultss) != RTNORM)
			return;

		if (wcscmp(resultss, results1) == 0 || wcscmp(resultss, results2) == 0)
		{
			p1d = false;
		}
		else
		{
			p1d = true;
		}



		Int32 len = 0;

		acedSSLength(sset, &len);
		//consoleprint(double(len),_T("\nL: "));
		for (long i = 0; i < len; i++)
		{


			if (NULL != (acedSSName(sset, i, eName)))
			{

				//consoleprint(double(i),_T("\nd"));


				acdbGetObjectId(id, eName);
				if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite) == eOk)
					{


						if ((Pipi = TVS_Pipe::cast(pEnt)) != NULL)
						{
							Pipi->assertWriteEnabled();


							Pipi->This1D = p1d;

						}

						if ((Tapie = TVS_TAP::cast(pEnt)) != NULL)
						{
							Tapie->assertWriteEnabled();
							Tapie->This1D = p1d;

						}

						if ((Wyeie = TVS_WYE::cast(pEnt)) != NULL)
						{
							Wyeie->assertWriteEnabled();
							Wyeie->This1D = p1d;

						}

						if ((Transie = TVS_TRANS::cast(pEnt)) != NULL)
						{
							Transie->assertWriteEnabled();
							Transie->This1D = p1d;

						}


						pEnt->close();
					}

				}
			}


		}
		// Add your code for command Ventilation_ARX.TVS_1D2D here
	}

	// - Ventilation_ARX.TVS_SPEC command (do not rename)
	static void Ventilation_ARXTVS_SpecificationOld(void)
    {
        // Share the legacy table pipeline: one area/mass calculation for all commands.
        Ventilation_ARXTVS_SPEC();
    }




	// - Ventilation_ARX.TVS_Show command (do not rename)
	static void Ventilation_ARXTVS_Show(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		acutRelRb(acutBuildList(RTSTR, _T("(TVS_TEST2)"), RTNONE, RTNONE));
		//acedCommand(RTSTR,_T("(TVS_TEST2)"),RTNONE,RTNONE);
		// Add your code for command Ventilation_ARX.TVS_Show here
	}

	// - Ventilation_ARX.TVS_CUT command (do not rename)
	static void Ventilation_ARXTVS_CUT(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		AcGePoint3d pCut;
		double Lx, Ly, startangle;
		ads_name vozd1, vozd2, eName;
		ACHAR handle[17];
		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt1, *pEnt2 = NULL;
		AcDbObjectId id;
		TVS_WYE wyie;
		TVS_TAP * Tapie;
		TVS_Pipe * Pipi1, *Pipi2;
		double NewSiseA, NewRadius;
		resbuf *rb = NULL;
		AcGeLine3d Line;
		bool ft = false, s2 = false;
		AcCmColor pColor;
		AcDb::LineWeight pWeight;
		AcDbObjectId pLayer;
		AcDbObjectId pLineType;

		while (ft == false)
		{

			if (acedEntSel(_T("\nIzaberite kanal:"), vozd1, pt1) == RTCAN)
				return;

			pCut = asPnt3d(pt1);

			if (acdbGetObjectId(id, vozd1) == eOk)
			{

				acdbGetObjectId(id, vozd1);

				{if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt1, id, AcDb::kForWrite) == eOk)
					{
						if ((Pipi1 = TVS_Pipe::cast(pEnt1)) != NULL)
						{
							Line.set(Pipi1->FirstPoint, Pipi1->LastPoint);
							pCut = Line.closestPointTo(pCut);


							if ((length2p(pCut, Pipi1->FirstPoint) <= (Pipi1->Length - 1))
								&& (length2p(pCut, Pipi1->LastPoint) <= (Pipi1->Length - 1)))
							{


								Pipi2 = Pipi1->add_new(pCut,
									Pipi1->LastPoint,
									Pipi1->SizeA,
									Pipi1->SizeB,
									Pipi1->This1D,
									Pipi1->ThisRound
								);
								Pipi1->put_Lastpoint(pCut);
								s2 = true;
								pColor = Pipi1->color();
								pWeight = Pipi1->lineWeight();
								pLayer = Pipi1->layerId();
								pLineType = Pipi1->linetypeId();
							}
							Pipi1->close();

							if (s2 == true)
							{

								if (acdbOpenAcDbEntity(pEnt2, Pipi2->id(), AcDb::kForWrite) == eOk)
								{




									Pipi2->setColor(pColor);
									Pipi2->setLineWeight(pWeight);
									Pipi2->setLayer(pLayer);
									Pipi2->setLinetype(pLineType);

									Pipi2->put_Wipeout(Pipi1->Wipeout);
									Pipi2->close();

								}


								s2 = false;
								ft = true;
							}


						}
					}

					else {
						acutPrintf(_T("\nObjekat je zaključan"));


					}

				}
				}


			}
		}



		// Add your code for command Ventilation_ARX.TVS_CUT here
	}

	// - Ventilation_ARX.TVS_Change command (do not rename)





	static void Ventilation_ARXTVS_Change(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;




		Change(sset);






		// Add your code for command Ventilation_ARX.TVS_Change here
	}






	static Acad::ErrorStatus postToDatabase(AcDbVoidPtrArray eSet)
	{
		Acad::ErrorStatus es;
		AcDbBlockTable *pBtbl;
		AcDbBlockTableRecord *pBtblr;
		es = acdbHostApplicationServices()->workingDatabase()
			->getSymbolTable(pBtbl, AcDb::kForRead);
		if (es != Acad::eOk)
		{
			acutPrintf(L"\nNije moguće otvoriti tabelu blokova");
			return es;
		}
		es = pBtbl->getAt(ACDB_MODEL_SPACE, pBtblr, AcDb::kForWrite);
		if (es != Acad::eOk)
		{
			acutPrintf(L"\nNije moguće otvoriti zapis tabele blokova");
			es = pBtbl->close();
			if (es != Acad::eOk)
			{
				acutPrintf(L"\nNije moguće zatvoriti tabelu blokova");
			}
			return es;
		}

		es = pBtbl->close();
		if (es != Acad::eOk)
		{
			acutPrintf(L"\nNije moguće zatvoriti tabelu blokova");
			return es;
		}

		for (int i = 0; i < eSet.length(); i++)
		{
			AcDbObjectId ObjId;
			AcDbEntity *pNewEnt = AcDbEntity::cast((AcRxObject*)eSet[i]);
			es = pBtblr->appendAcDbEntity(ObjId, pNewEnt);
			if (es != Acad::eOk)
			{
				acutPrintf(L"\nNije moguće dodati objekat");
			}

			es = pNewEnt->close();
			if (es != Acad::eOk)
			{
				acutPrintf(L"\nNije moguće zatvoriti objekat");
			}
		}

		es = pBtblr->close();
		if (es != Acad::eOk)
		{
			acutPrintf(L"\nNije moguće zatvoriti zapis tabele blokova");
		}
		return es;
	}






	// - Ventilation_ARX.TVS_ERASE command (do not rename)
	static void Ventilation_ARXTVS_ERASE(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName;
		ACHAR  resultss[512];
		ACHAR  results1[512] = _T("2");
		ACHAR  results2[512] = _T("2d");

		ads_point pt1, pt2;
		ads_real sise = 0;
		AcDbEntity *pEnt = NULL;
		AcDbObjectId id;
		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;

		double NewSiseA;
		bool p1d;
		resbuf *rb = NULL;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;



		Int32 len = 0;

		acedSSLength(sset, &len);
		//consoleprint(double(len),_T("\nL: "));
		for (long i = 0; i < len; i++)
		{


			if (NULL != (acedSSName(sset, i, eName)))
			{

				//consoleprint(double(i),_T("\nd"));


				acdbGetObjectId(id, eName);

				bool g = false;
				if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite) == eOk)
					{


						if ((Pipi = TVS_Pipe::cast(pEnt)) != NULL)
						{

							AcDbVoidPtrArray eSet;
							Acad::ErrorStatus es = pEnt->explode(eSet);
							pEnt->close();

							//delete the original entity
							acdbEntDel(eName);

							// Add the new entities to the db
							es = postToDatabase(eSet);
							g = true;
						}

						if ((Tapie = TVS_TAP::cast(pEnt)) != NULL)
						{
							AcDbVoidPtrArray eSet;
							Acad::ErrorStatus es = pEnt->explode(eSet);
							pEnt->close();

							//delete the original entity
							acdbEntDel(eName);

							// Add the new entities to the db
							es = postToDatabase(eSet);
							g = true;
						}

						if ((Wyeie = TVS_WYE::cast(pEnt)) != NULL)
						{
							AcDbVoidPtrArray eSet;
							Acad::ErrorStatus es = pEnt->explode(eSet);
							pEnt->close();

							//delete the original entity
							acdbEntDel(eName);

							// Add the new entities to the db
							es = postToDatabase(eSet);
							g = true;
						}

						if ((Transie = TVS_TRANS::cast(pEnt)) != NULL)
						{
							AcDbVoidPtrArray eSet;
							Acad::ErrorStatus es = pEnt->explode(eSet);
							pEnt->close();

							//delete the original entity
							acdbEntDel(eName);

							// Add the new entities to the db
							es = postToDatabase(eSet);
							g = true;
						}


						if (g == false)pEnt->close();
						g = false;
					}

				}
			}


		}
		// Add your code for command Ventilation_ARX.TVS_1D2D here






		// Add your code for command Ventilation_ARX.TVS_ERASE here
	}

	static void Ventilation_ARXTVSSomeParts(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name ent;
		bool  Astat = false, status = false;
		//acedInitGet(RSG_NONULL);
		ads_point pt1, pt2;
		AcDbObjectId id;
		AcDbEntity *pEnt;
		AcDbLine*pLine;
		AcGePoint3d pstart, pend;
		while (status == false)
		{

			Astat = acedEntSel(_T("\nIzaberite liniju: "), ent, pt1);
			if (Astat == true)
			{


				if (acdbGetObjectId(id, ent) == eOk)
				{
					{if (id != AcDbObjectId::kNull)
					{
						if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForRead) == eOk)

						{
							if ((pLine = AcDbLine::cast(pEnt)) != NULL)
							{
								status = true;
								pLine->getStartPoint(pstart);
								pLine->getEndPoint(pend);
							}
							pLine->close();
						}
					}
					}
				}
			}
		}
		int * quantity = new int;
		if (acedGetInt(_T("\nBroj segmenata: "), quantity) != RTNORM) return;
		double pDist = pstart.distanceTo(pend);
		for (int i = 1; i < *quantity + 1; i++)
		{
			//AcGePoint3d base=AcGePoint3d(((*quantity-i)*pstart.x+i*pend.x)/(*quantity),((*quantity-i)*pstart.y+i*pend.y)/(*quantity),((*quantity-i)*pstart.z+i*pend.z)/(*quantity));
			AcGePoint3d base = AcGePoint3d(((*quantity * 2 - 2 * i + 1)*pstart.x + (2 * i - 1)*pend.x) / (*quantity * 2), ((*quantity * 2 - 2 * i + 1)*pstart.y + (2 * i - 1)*pend.y) / (*quantity * 2), ((*quantity * 2 - 2 * i + 1)*pstart.z + (2 * i - 1)*pend.z) / (*quantity * 2));
			SPEClist::printLine(AcGePoint3d(base.x - 200, base.y, base.z), AcGePoint3d(base.x + 200, base.y, base.z));
			SPEClist::printLine(AcGePoint3d(base.x, base.y + 200, base.z), AcGePoint3d(base.x, base.y - 200, base.z));
		}
		delete quantity;
	}

	static void setZ()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	static void UpDown(TVS_Pipe * pPipe, int stat)
	{
		AcDbEntity * pEnt;
		double startZ, nextZ, Axis;


		double x = pPipe->LastPoint.x - pPipe->FirstPoint.x;
		double y = pPipe->LastPoint.y - pPipe->FirstPoint.y;
		double z = pPipe->LastPoint.z - pPipe->FirstPoint.z;
		Axis = M_PI / 2;

		TVS_TAP* pTap = TVS_TAP::add_new(pPipe->SizeA, pPipe->SizeB, Axis, AcGeVector3d(0, 0, 1), AcGeVector3d(y, -x, 0), pPipe->LastPoint, Axis, pPipe->This1D, pPipe->ThisRound);
		SetGlobalProperty(pTap);
		SetPropertyLikePipe(pPipe, pTap);
		acdbOpenAcDbEntity(pEnt, pTap->id(), AcDb::kForWrite);
		pTap->put_Form(stat);
		pTap->close();
		pTap->draw();


		double plecho1 = abs(length2p(pTap->MA, pTap->MiddlePoint));
		double plecho2 = abs(length2p(pTap->MC, pTap->MiddlePoint));
		double dLx;

		AcGePoint3d p1, p2, p3, p4, p5, p6, p7;
		p2 = pPipe->LastPoint;
		p1 = shortlength(pPipe->FirstPoint, p2, plecho1);


		acdbOpenAcDbEntity(pEnt, pPipe->id(), AcDb::kForWrite);
		pPipe->put_Lastpoint(p1);
		pPipe->close();



	}

	static void Ventilation_ARXTVS_Flex(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		TVS_Pipe* pEnt = new TVS_Pipe();

		pEnt->FirstPoint = AcGePoint3d(0, 0, 0);
		pEnt->LastPoint = AcGePoint3d(3000, 0, 0);
		pEnt->SizeA = 100;
		pEnt->SizeB = 200;
		pEnt->This1D = false;
		pEnt->ThisRound = false;
		pEnt->Flow = 0;
		pEnt->Direct = 0;
		pEnt->Elev = 0;
		pEnt->Wipeout = false;
		pEnt->Param = 0;
		pEnt->Grani = false;
		pEnt->ElevDown = 0;
		pEnt->ElevUp = 0;
		pEnt->IsPipe = false;
		pEnt->Form = 0;
		pEnt->setFlex(true);
		//TCHAR * pPipe->Tag2=new TCHAR

		//pPipe->Tag2=ACRX_T("");
		pEnt->setLinetypeScale(acdbHostApplicationServices()->workingDatabase()->celtscale());
		//pPipe->ShowText=pShowText;
		//pPipe->SetText();
		// Post to Database
		AcDbBlockTable *pBlockTable;
		acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable,
			AcDb::kForRead);

		AcDbBlockTableRecord *pBlockTableRecord;
		pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForWrite);
		pBlockTable->close();

		AcDbObjectId retId = AcDbObjectId::kNull;
		pBlockTableRecord->appendAcDbEntity(retId, pEnt);
		pBlockTableRecord->close();
		pEnt->close();


	}
	static void  fillspecification(ads_name &sset, SpecWithAttrlist &specatrlist, ACHAR * grad, ACHAR * d)
	{
		Int32 len = 0;
		AcDbBlockReference * br;
		TVS_Entity * Ent;
		AcDbEntity *pEnt = NULL;
		ads_name  eName;
		AcDbObjectId id;
		acedSSLength(sset, &len);
		//consoleprint(double(len),_T("\nL: "));
		for (long i = 0; i < len; i++)
		{


			if (NULL != (acedSSName(sset, i, eName)))
			{

				//consoleprint(double(i),_T("\nd"));


				acdbGetObjectId(id, eName);
				if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForRead) == eOk)
					{

						pEnt->close();
						if ((br = AcDbBlockReference::cast(pEnt)) != NULL)
						{
							SPEC spec;
							wcscpy_s(spec.grad, grad);
							wcscpy_s(spec.d, d);
							if (spec.addBlock(pEnt) == true)
							{
								//spec.entityList.append(pEnt);
								specatrlist.append(spec);
							}
						}

					}
				}
			}
		}
	}
	static void  fillspecification(ads_name &sset, SPEClist &speclist, SPEClist &speclistflex, SpecWithAttrlist &specatrlist, ACHAR * grad, ACHAR * d)
	{
		Int32 len = 0;
		AcDbBlockReference * br;
		TVS_Entity * Ent;
		AcDbEntity *pEnt = NULL;
		ads_name  eName;
		AcDbObjectId id;
		acedSSLength(sset, &len);
		//consoleprint(double(len),_T("\nL: "));
		for (long i = 0; i < len; i++)
		{


			if (NULL != (acedSSName(sset, i, eName)))
			{

				//consoleprint(double(i),_T("\nd"));


				acdbGetObjectId(id, eName);
				if (id != AcDbObjectId::kNull)
				{
					if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForRead) == eOk)
					{

						if ((Ent = TVS_Entity::cast(pEnt)) != NULL)

						{

							if (Ent->isDuctFlex())
							{
								SPEC spec;
								wcscpy_s(spec.grad, grad);
								wcscpy_s(spec.d, d);
								if (spec.add(pEnt) == true)
								{
									speclistflex.append(spec);
								}
							}

							else
							{
								SPEC spec;
								wcscpy_s(spec.grad, grad);
								wcscpy_s(spec.d, d);
								if (spec.add(pEnt) == true)
								{
									speclist.append(spec);
								}
							}
						}
						pEnt->close();
						if ((br = AcDbBlockReference::cast(pEnt)) != NULL)
						{
							SPEC spec;
							wcscpy_s(spec.grad, grad);
							wcscpy_s(spec.d, d);
							if (spec.addBlock(pEnt) == true)
							{
								specatrlist.append(spec);
							}
						}

					}
				}
			}
		}
	}
	static void Ventilation_ARXTVS_SPEC(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;
		if (!tvsTable::prompt()) { acedSSFree(sset); return; }
		ads_point pt1;
		AcGePoint3d pb = AcGePoint3d(0, 0, 0);
		if (acedGetPoint(NULL, _T("\nZadajte tačku umetanja specifikacije:"), pt1) != RTNORM) { acedSSFree(sset); return; }
		pb = asPnt3d(pt1);

		SPEClist speclist, speclistflex;
		SpecWithAttrlist specatrlist;
		fillspecification(sset, speclist, speclistflex, specatrlist, L"%%d", L"%%c");
		acedSSFree(sset);
		if (speclist.specList.physicalLength() != 0)
		{
			//speclist.print();
			speclist.printSPDSForm(pb);

		}
		if (speclistflex.specList.physicalLength() != 0)
		{
			//speclist.print();
			speclistflex.printSPDSForm(pb);
		}

		if (specatrlist.specList.physicalLength() != 0)
		{
			//speclist.print();
			specatrlist.printSPDSForm(pb);
		}

	}


	static void Ventilation_ARXTVS_EXCELSPEC(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;


		long idx = 1;

		SPEClist speclist, speclistflex;
		SpecWithAttrlist specatrlist;

		fillspecification(sset, speclist, speclistflex, specatrlist, L"°", L"⌀");
		CMSExcel* m_msExcel = new CMSExcel;
		acedSetStatusBarProgressMeter(_T("Izvoz u Excel"), 0, 100);

		if (FAILED(m_msExcel->NewExcelBook(false) == E_FAIL)) { acutPrintf(_T("\n Greška")); acedRestoreStatusBar(); delete m_msExcel; return; }
		acedRestoreStatusBar();
		if (speclist.specList.physicalLength() != 0)
		{
			//speclist.print();

			speclist.printToExel(m_msExcel, idx);
			idx++;
		}
		if (speclistflex.specList.physicalLength() != 0)
		{
			//speclist.print();
			speclistflex.printToExel(m_msExcel, idx);
			idx++;
		}

		if (specatrlist.specList.physicalLength() != 0)
		{
			//speclist.print();
			specatrlist.printToExel(m_msExcel, idx);
			idx++;
		}

		m_msExcel->SetVisible(true);
		delete m_msExcel;

	}







	static bool CheckAtt(ACHAR* pName, ACHAR* tag)
	{
		AcDbDatabase *pCurDb;
		AcDbBlockTable* pBlkTbl;
		AcDbBlockTableRecord* pBlkRec;
		AcDbObjectId attId;
		Acad::ErrorStatus es;

		AcDbAttributeDefinition* pAttDef;
		// location of the AttributeDefinition in the
		// block definition


		pCurDb =
			acdbHostApplicationServices()->workingDatabase();

		es =
			pCurDb->getBlockTable(pBlkTbl, AcDb::kForRead);

		if (!pBlkTbl->has(pName))
		{
			acutPrintf(L"\nDefinicija bloka TEST ne postoji");
			pBlkTbl->close();
			return false;
		}

		es = pBlkTbl->getAt(pName, pBlkRec, AcDb::kForWrite);
		// create a AttributeDefinition

		AcDbBlockTableRecordIterator *pIterBTR = NULL;
		if ((es = pBlkRec->newIterator(pIterBTR)) == Acad::eOk) {
			for (; !pIterBTR->done(); pIterBTR->step()) {
				AcDbObjectId objSubId;
				if ((es = pIterBTR->getEntityId(objSubId)) == Acad::eOk) {
					AcDbObjectPointer<AcDbEntity> pSubEnt(objSubId, AcDb::kForRead);
					if ((es = pSubEnt.openStatus()) == Acad::eOk) {
						//
						// Здесь можно работать с примитивами в блоке
						//
						//buf=pSubEnt->isA()->name();
						//		 											acutPrintf(_T("\nПримитив: %s"),LPCTSTR(pSubEnt->isA()->name()));
						AcDbAttributeDefinition *pAttdef = AcDbAttributeDefinition::cast(pSubEnt.object());

						if (pAttdef) {
							//
							// Если это определение атрибута сделаем отдельную обработку
							//
							//acutPrintf(pAttdef->textStringConst());
							// 													acutPrintf(_T("\nTag1=%s"),LPCTSTR(pAttdef->tagConst()));
							// 													acutPrintf(_T("\nTag2=%s"),LPCTSTR(tag));
							//	const ACHAR* tagconst=LPCTSTR(pAttdef->tagConst());
							if (wcscmp(tag, pAttdef->tagConst()) == 0) {
								//	pAttDef->close();
								pBlkRec->close();
								pBlkTbl->close();

								return false;
							}
							// 														acutPrintf(_T("-> Tag=%s Value=%s IsConst=%s IsPreset=%s IsInvisible=%s isVerifiable=%s"),
							// 															LPCTSTR(pAttdef->tagConst()), LPCTSTR(pAttdef->textStringConst()),
							// 															LPCTSTR((pAttdef->isConstant()?_T("Yes"):_T("No"))),
							// 															LPCTSTR((pAttdef->isPreset()?_T("Yes"):_T("No"))),
							// 															LPCTSTR((pAttdef->isInvisible()?_T("Yes"):_T("No"))),
							// 															LPCTSTR((pAttdef->isVerifiable()?_T("Yes"):_T("No")))
							// 															);
						}
					}
					else {
						acutPrintf(_T("\nNije moguće otvoriti element u bloku! Greška: %s",
							LPCTSTR(acadErrorStatusText(es))));
					}
				}
				else {
					acutPrintf(_T("\nNije moguće dobiti AcDbObjectId elementa u bloku! Greška: %s",
						LPCTSTR(acadErrorStatusText(es))));
				}
			}
		}
		else {
			acutPrintf(_T("\nNije moguće napraviti iterator zapisa tabele blokova! Greška: %s", LPCTSTR(acadErrorStatusText(es))));
		}


		//pAttDef->close();
		pBlkRec->close();
		pBlkTbl->close();
		return true;
	}

	static void AddNewAtt(ACHAR* pName, ACHAR* tag)
	{
		AcDbDatabase *pCurDb;
		AcDbBlockTable* pBlkTbl;
		AcDbBlockTableRecord* pBlkRec;
		AcDbObjectId attId;
		Acad::ErrorStatus es;

		AcDbAttributeDefinition* pAttDef;
		// location of the AttributeDefinition in the
		// block definition
		AcGePoint3d attLoc(1.2, -0.5, 0);

		// specify the text,tag and prompt
		ACHAR text[] = { L"" };

		ACHAR prompt[] = { L"Unesite vrijednost" };

		pCurDb =
			acdbHostApplicationServices()->workingDatabase();

		es =
			pCurDb->getBlockTable(pBlkTbl, AcDb::kForRead);

		if (!pBlkTbl->has(pName))
		{
			acutPrintf(L"\nDefinicija bloka TEST ne postoji");
			pBlkTbl->close();
			return;

		}

		es = pBlkTbl->getAt(pName, pBlkRec, AcDb::kForWrite);
		// create a AttributeDefinition
		pAttDef = new AcDbAttributeDefinition(attLoc, text,
			tag, prompt);
		pAttDef->setInvisible(true);
		pAttDef->setLockPositionInBlock(true);
		// append the AttributeDefinition to the definition
		es = pBlkRec->appendAcDbEntity(attId, pAttDef);

		pAttDef->close();
		pBlkRec->close();
		pBlkTbl->close();
		//delete pAttDef;
	}

	static void Ventilation_ARXTVS_BLCKMNGR(void)
	{



		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;


		//		SPEClist speclist, speclistflex;
		SpecWithAttrlist specatrlist;
		fillspecification(sset, specatrlist, L"%%d", L"%%c");


		if (specatrlist.specList.physicalLength() != 0)
		{
			//speclist.print();

			CAcModuleResourceOverride resourceOverride;
			dgBlockManager dlg;
			dlg.blckList = specatrlist;
			INT_PTR nRet = -1;
			nRet = dlg.DoModal();
			if (nRet != IDOK) return;
			SpecWithAttrlist specatrlistnew = dlg.blckList;
			for (int i = 0; i < specatrlist.specList.length(); i++)
			{
				if (specatrlist.checkRelevations(specatrlist.specList[i], specatrlistnew.specList[i]) != Equal) specatrlistnew.specList[i].setAtribListToEnt();

			}
		}
	}

	static void Ventilation_ARXTVS_TEST(void)
	{



		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName;

		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;


		//		SPEClist speclist, speclistflex;
		SpecWithAttrlist specatrlist;
		fillspecification(sset, specatrlist, L"%%d", L"%%c");


		if (specatrlist.specList.physicalLength() != 0)
		{
			//speclist.print();

			CAcModuleResourceOverride resourceOverride;
			dgBlockManager dlg;
			dlg.blckList = specatrlist;
			INT_PTR nRet = -1;
			nRet = dlg.DoModal();
			if (nRet != IDOK) return;
			SpecWithAttrlist specatrlistnew = dlg.blckList;
			for (int i = 0; i < specatrlist.specList.length(); i++)
			{
				if (specatrlist.checkRelevations(specatrlist.specList[i], specatrlistnew.specList[i]) != Equal) specatrlistnew.specList[i].setAtribListToEnt();

			}
		}















		// 		if(ActivationErrorMessage()!=pError_Ok) return;
		// 		string ensrcretHHDId;
		// 	protection::gethddIDFromSecret(ensrcretHHDId);
		// 
		// 
		// 		
		// 		acutPrintf(_T("\n%s"),conversion::charToWchar(ensrcretHHDId.c_str()));
		// 
		// 
		// 
		// 
		// 		AcGeLine3d lin1=AcGeLine3d(AcGePoint3d(0,0,0),AcGePoint3d(2,0,0));
		// 		AcGeLine3d lin2=AcGeLine3d(AcGePoint3d(3,0,0),AcGePoint3d(0,3,0));
		// 		AcGePoint3d pnt;
		// 		double b = 0;
		// 		double a = 3/b;
		// 		bool intresect=lin1.intersectWith(lin2,pnt);
		// 		if (intresect) acutPrintf(_T("\n Да"));
		// 		else acutPrintf(_T("\n Нет"));
		// 		//SPEClist spec;
		// 		//spec.printToExel(1);
		// 
		// 
		// 
		// 		// 		CAcModuleResourceOverride resourceOverride;
		// 		// 		CDatabase cdbMyDB;
		// 		// 		cdbMyDB.Open(L"New Excel Data Source");
		// 		// 		cdbMyDB.ExecuteSQL(L"CREATE TABLE mydata (FirstName TEXT,LastName TEXT)"); 
		// 		// 		cdbMyDB.ExecuteSQL(L"INSERT INTO mydata (FirstName,LastName) VALUES('Kaev','Artem');");
		// 		// 		cdbMyDB.Close(); 
	}

	static void Ventilation_ARXTVS_AddAtrib(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_point pt1;
		ads_name ent;
		Acad::ErrorStatus es;




		int Astat;
		bool  Astat2 = false;
		ACHAR resultss[512], resultss2[512];

		Astat2 = false;
		while (Astat2 == false)
		{
			Astat = acedEntSel(_T("\nIzaberite blok za dodavanje atributa: "), ent, pt1);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;

			}
			AcDbObjectId idBlkRef;
			acdbGetObjectId(idBlkRef, ent);

			AcDbObjectPointer<AcDbBlockReference> pBlkRef(idBlkRef, AcDb::kForRead);
			if ((es = pBlkRef.openStatus()) == Acad::eOk) {
				Astat2 = true;


				ACHAR pName[512];
				pBlkRef->close();

				if (SPEC::getBlockName(pBlkRef, pName))
				{
					if (CheckAtt(pName, TagPos))
					{
						AddNewAtt(pName, TagPos);
					}
					if (CheckAtt(pName, TagName))
					{
						AddNewAtt(pName, TagName);
					}

					if (CheckAtt(pName, TagType))
					{
						AddNewAtt(pName, TagType);
					}
					if (CheckAtt(pName, TagSize))
					{
						AddNewAtt(pName, TagSize);
					}
					if (CheckAtt(pName, TagArticle))
					{
						AddNewAtt(pName, TagArticle);
					}
					if (CheckAtt(pName, TagManufacture))
					{
						AddNewAtt(pName, TagManufacture);
					}
					if (CheckAtt(pName, TagMass))
					{
						AddNewAtt(pName, TagMass);
					}
					if (CheckAtt(pName, TagCommit))
					{
						AddNewAtt(pName, TagCommit);
					}




					acedCommandS(RTSTR, _T("_ATTSYNC"), RTSTR, _T("_Name"), RTSTR, pName, RTNONE);
					return;
				}












			}
		}
	}

	// void fAddAttribute()
	// {
	// 	try
	// 	{
	// 		// Получаем объект ActiveX приложения AutoCAD, увеличивая счетчик использования
	// 		IAcadApplicationPtr pAcadApp = acedGetAcadWinApp()->GetIDispatch(TRUE);
	// 		// Теперь получим активный документ
	// 		IAcadDocument * pActiveDoc;
	// 		pAcadApp->get_ActiveDocument(&pActiveDoc);
	// 		IAcadBlockPtr pBlock = NULL;
	// 		TCHAR *pBlkName = _T("некоторое_имя_блока ");
	// 		// Создаём ActiveX-совместимую 3D-точку
	// 		AcAxPoint3d axInsPnt(0,0,0);
	// 		// Добавим имя блока
	// 		pBlock = pActiveDoc->Wblock()->Add(axInsPnt.asVariantPtr(),_bstr_t(pBlkName));
	// 		// Добавим атрибут к блоку
	// 		IAcadAttributePtr pAttDef;
	// 		pAttDef = pBlock->AddAttribute(1.0, (AcAttributeMode)0 ,
	// 			_bstr_t("Укажите имя сотрудника "), axInsPnt.asVariantPtr(),
	// 			_bstr_t("имя_сотрудника"),_bstr_t(""));
	// 		// Атрибут добавлен
	// 	}
	// 	catch(_com_error &es)
	// 	{
	// 		acutPrintf(L"\nОшибка : %s", es.ErrorMessage());
	// 	}
	//
	static void Ventilation_ARXTVS_SCALE(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name sset, eName, ent;
		ads_point pt1;
		bool isOk = false;
		int acadStatus;
		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;


		double length = 0;

		isOk = false;
		while (isOk == false)
		{
			acadStatus = acedEntSel(_T("\nIzaberite referentnu liniju: "), ent, pt1);
			switch (acadStatus)
			{
			case RTCAN:
				return;
				break;

			}
			if (acadStatus = RTNORM)
			{
				AcDbObjectId pId;
				acdbGetObjectId(pId, ent);

				AcDbObjectPointer<AcDbLine>pLine(pId, AcDb::kForRead);
				if (pLine.openStatus() == Acad::eOk)
				{
					isOk = true;
					length = pLine->endPoint().distanceTo(pLine->startPoint());
				}
			}
		}


		ads_real  len;
		isOk = false;
		while (isOk == false)
		{
			double newLength;

			acadStatus = acedGetReal(_T("\nUnesite potrebnu dužinu: "), &len);
			switch (acadStatus)
			{
			case RTCAN:
				return;
				break;

			}
			if (acadStatus = RTNORM)
			{
				newLength = len;
				isOk = true;
			}

		}
		ads_real scale = len / length;
		acedCommandS(RTSTR, _T("_SCALE"), RTPICKS, sset, RTSTR, _T(""), RTPOINT, pt1, RTREAL, scale, RTNONE);
		return;

	}


	static void Ventilation_ARXTVS_HEATFLOOR(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		ads_name eName, ent;
		ads_point pt1;
		bool isOk = false;
		int acadStatus;
		AcGePoint3dArray arr;


		double length = 0;

		isOk = false;
		while (isOk == false)
		{
			acadStatus = acedEntSel(_T("\nIzaberite poliliniju za crtanje podnog grijanja: "), ent, pt1);
			switch (acadStatus)
			{
			case RTCAN:
				return;
				break;

			}
			if (acadStatus = RTNORM)
			{
				AcDbObjectId pId;
				acdbGetObjectId(pId, ent);

				AcDbObjectPointer<AcDbPolyline>pLine(pId, AcDb::kForRead);
				if (pLine.openStatus() == Acad::eOk)
				{
					isOk = true;

					pLine->getStretchPoints(arr);
					AcDbVoidPtrArray arr1;
					pLine->getOffsetCurves(200, arr1);
					HeatFloor hf;
					time_t seconds;
					time(&seconds);
					srand(seconds);
					hf.setContour(pLine);
					hf.setStartPoint(AcGePoint3d(0, 0, 0));
					hf.step = 200;
					hf.getOffset(pLine);
					hf.drawHFLoop();
					//hf.drawOffset(pLine);
					acutPrintf(_T("\n %d"), hf.indexofrecursion);
					acutPrintf(_T("\n %d"), hf.indexofrecursion1);
					acutPrintf(_T("\n %d"), hf.indexofrecursion2);
					acutPrintf(_T("\n %d"), hf.indexofrecursion3);

					for each (AcGePoint3d var in arr)
					{
						acutPrintf(_T("\n %10.2f %10.2f %10.2f"), var.x, var.y, var.z);
					}
				}
			}
		}

		HeatFloor HF;
		AcGePoint3dArray res;
		//HF.getSimilarInsideFigureAtDistance(arr,200,res);

	// 		isOk=false;
	// 		while (isOk==false)
	// 		{
	// 			acadStatus=acedGetPoint(pt1,_T("\nУкажите точку:"),pt1);
	// 			switch (acadStatus)
	// 			{
	// 			case RTCAN:
	// 				return;
	// 				break;
	// 
	// 			}
	// 			if (acadStatus=RTNORM)
	// 			{
	// 				AcGePoint3d pt=asPnt3d(pt1);
	// 				HeatFloor HF;
	// 				if (HF.pt_in_polygon(pt,arr))
	// 				{
	// 					acutPrintf(_T("\nВнутри"));
	// 				} 
	// 				else
	// 				{
	// 					acutPrintf(_T("\nСнаружи"));
	// 				}
	// 
	// 
	// 
	// 			}
	// 		}




	// 		ads_real  len;
	// 		isOk=false;
	// 		while (isOk==false)
	// 		{
	// 			double newLength;
	// 
	// 			acadStatus=acedGetReal (_T("\nВведите Требуемую длину: "), &len) ;
	// 			switch (acadStatus)
	// 			{
	// 			case RTCAN:
	// 				return;
	// 				break;
	// 
	// 			}
	// 			if (acadStatus=RTNORM)
	// 			{
	// 				newLength=len;
	// 				isOk=true;
	// 			}
	// 
	// 		}
	// 		ads_real scale=len/length;
	// 		acedCommandS(RTSTR,_T("_SCALE"),RTPICKS,sset,RTSTR,_T(""),RTPOINT,pt1,RTREAL,scale,RTNONE);
		return;

	}

};

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CTVS_Ventilation_ARXApp)


//new
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PropertyManager, TVS_PropertyManager, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AttributesToProps, TVS_AttributesToProps, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ExcelSp, TVS_ExcelSp, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Sp, TVS_Sp, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AddProperty, TVS_AddProperty, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)




//old
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PIPE, TVS_PIPE, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_DRAW, TVS_DRAW, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TEST, TVS_TEST, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TRANS, TVS_TRANS, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_WYE, TVS_WYE, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTT, TVS_CONNECTT, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTTest, TVS_CONNECTTest, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTW, TVS_CONNECTW, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_DUCT, TVS_DUCT, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_1D2D, TVS_1D2D, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SPEC, TVS_SPEC, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_BLCKMNGR, TVS_BLCKMNGR, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_EXCELSPEC, TVS_EXCELSPEC, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SpecificationOld, TVS_SpecificationOld, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Flex, TVS_Flex, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Show, TVS_Show, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CUT, TVS_CUT, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Change, TVS_Change, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ERASE, TVS_ERASE, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_LEAD, TVS_LEAD, ACRX_CMD_TRANSPARENT, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_LEAD_Recalculate, TVS_LEAD_Recalculate, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SCALE, TVS_SCALE, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AddAtrib, TVS_AddAtrib, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVSSomeParts, TVSSomeParts, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SETTINGS, TVS_SETTINGS, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_HEATFLOOR, TVS_HEATFLOOR, ACRX_CMD_TRANSPARENT, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION, NULL)
ACED_ADSSYMBOL_ENTRY_AUTO(CTVS_Ventilation_ARXApp, MyLispFunction, false)

