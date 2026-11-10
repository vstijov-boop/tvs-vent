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
#include "BypassPreviewCommand.h"
#include "Func.h"
#include "CoordinateInput.h"
#include "TVSMoveCommand.h"
#include "AlignCommand.h"
#ifdef TVS_OFFSET_PROOF
#include "TVSTestOffset.h"
#endif
#include "tdbl.h"

#include "dgPipeSize.h"
#include "dgChangeZ.h"
#include "OffsetSessionState.h"
#include "OffsetIntegrationCommand.h"
#include "OffsetIntegrationReactor.h"
#include "OffsetIntegrationDiagnostics.h"
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
#include "TvsLinetypes.h"
#include "FlexCuffCommand.h"
#include "MLeaderStyleTools.h"
#include "TeeCommand.h"
#include "TeeInsert.h"
#include "MatchCommand.h"
#include "SectionCommand.h"
#include "SectionDialog.h"
#include "LevelTooltip.h"
#include "../TVS_Ventilation_DBX/SpiroCatalog.h"
#include "../TVS_Ventilation_DBX/PortGrips.h"
#include "TransAlignCommand.h"
#include "SizeFollow.h"
#include "ElbowAngleCommand.h"
#include "ScaleWindow.h"
#include "ExportCommand.h"
#include "TeeTypeDialog.h"

#include "TVSLeadCommand.h"
#include "TVSCommandLeadRecalculate.h"
#include "TVSCommandPropertyManager.h"
#include "TVSCommandAttributesToProperties.h"
#include "TVSCommandAddProperties.h"
#include "TVSCommandExcelSpecification.h"
#include "TVSCommandSpecification.h"
#include "TablePresentation.h"
#include "OffsetSelectionCheck.h"
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

#ifdef TVS_BYPASS_PREVIEW
// Live radius mode from the shared Settings.h globals (Settings.cpp).
static TVSBypassRadius tvsBypassRadiusSettings(){TVSBypassRadius r;r.mode=globalRadiusTypeRect;r.roundMode=globalRadiusTypeRound;r.typeRoundTap=globalTypeRoundTap;r.k=globalTapRadiusVariableParameter;r.constant=globalTapRadiusConst;return r;}
static void tvsBypassCommandEntry(){tvsBypassCommand(tvsBypassRadiusSettings());}
static void tvsBypassPreviewCommandEntry(){tvsBypassPreviewCommand(tvsBypassRadiusSettings());}
#endif
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
		tvsLevelTooltip::start();
		tvsSizeFollow::start();   // prelaz i kanal dijele dimenziju na spoju (TVS_DIM_PRATI)
		tvsScaleWindow::start();  // jedinice crteža pri kopiranju / umetanju (ScaleWindow.h)
		tvsSizeFollow::enableTeeFollow(IDD_RUN, IDD_TEETYPE);   // TVS_TK_TIP: dionica iza kanala nove dimenzije
		ftTransType = tvsTransAlignCmd::loadPlan();   // tip novih prelaza u tlocrtu (TVS_SETTINGS)
#ifdef TVS_BYPASS_PREVIEW
        #ifdef TVS_DEV
        if(acedRegCmds->addCommand(L"TVS_BYPASS_DEV",L"TVS_BYPASS_DETAIL_TEST",L"TVS_BYPASS_DETAIL_TEST",ACRX_CMD_MODAL,&tvsBypassDetailTestCommand)!=Acad::eOk)
            acutPrintf(L"\nDEV: registracija fixture testa nije uspjela.");
        if(acedRegCmds->addCommand(L"TVS_BYPASS_DEV",L"TVS_BYPASS_DETAIL_CHECK",L"TVS_BYPASS_DETAIL_CHECK",ACRX_CMD_MODAL,&tvsBypassDetailCheckCommand)!=Acad::eOk)
            acutPrintf(L"\nDEV: registracija provjere detalja nije uspjela.");
#endif
        if(acedRegCmds->addCommand(L"TVS_BYPASS_DEV",L"TVS_BYPASS",L"TVS_BYPASS",ACRX_CMD_MODAL,&tvsBypassCommandEntry)!=Acad::eOk)
            acutPrintf(L"\nDEV: registracija TVS_BYPASS nije uspjela.");
        if(acedRegCmds->addCommand(L"TVS_BYPASS_DEV",L"TVS_BYPASS_PREVIEW",L"TVS_BYPASS_PREVIEW",ACRX_CMD_MODAL,&tvsBypassPreviewCommandEntry)!=Acad::eOk)
            acutPrintf(L"\nDEV: registracija TVS_BYPASS_PREVIEW nije uspjela.");
        if(acedRegCmds->addCommand(L"TVS_BYPASS_DEV",L"TVS_BYPASS_DETALJ",L"TVS_BYPASS_DETALJ",ACRX_CMD_MODAL,&tvsBypassExistingDetailCommand)!=Acad::eOk)
            acutPrintf(L"\nDEV: registracija TVS_BYPASS_DETALJ nije uspjela.");
#endif
		PropertyAdmin::initialize();
#ifdef TVS_OFFSET_INTEGRATION
        acedEditor->addReactor(&offsetReactor());
        tvsOffsetRefit::attachAll();   // etaz follows a changed duct elevation (TVS_ETAZ_KOTA)
#endif
        tvsLinetypes::ensureCurrent(); // drawing already open when the ARX is loaded
        { const MLeaderSettings& ml = Settings::get()->mLeader; // TVS label style available to AutoCAD too
          tvsMLeaderStyle::ensureCurrentDocument(ml.style, ml.styleSource.IsEmpty() ? tvsMLeaderStyle::bundledSource() : ml.styleSource); }
        // TODO: Add your initialization code here

		return (retCode);
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg(void *pkt) {
#ifdef TVS_BYPASS_PREVIEW
        acedRegCmds->removeGroup(L"TVS_BYPASS_DEV");
#endif
        PropertyAdmin::uninitialize();
        tvsLevelTooltip::stop();
        tvsSizeFollow::stop();
        tvsScaleWindow::stop();
#ifdef TVS_OFFSET_INTEGRATION
        acedEditor->removeReactor(&offsetReactor());tvsOffsetData::clearClonePermits();tvsOffsetRefit::detachAll();
#endif
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
        tvsLinetypes::ensureCurrent(); // hidden/centre lines dashed even without TVS_EnterPoint.lsp
        tvsSizeFollow::attach(acdbHostApplicationServices()->workingDatabase());
#ifdef TVS_OFFSET_INTEGRATION
        tvsOffsetRefit::attach(acdbHostApplicationServices()->workingDatabase());
#endif
        { const MLeaderSettings& ml = Settings::get()->mLeader; // TVS label style available to AutoCAD too
          tvsMLeaderStyle::ensureCurrentDocument(ml.style, ml.styleSource.IsEmpty() ? tvsMLeaderStyle::bundledSource() : ml.styleSource); }

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




	#include "commands/ConnectHelpers.inl"   // spajanje dva kanala (prelaz, koljeno, T komad) i svojstva novih elemenata
	#include "commands/DrawHelpers.inl"   // crtanje: dimenzije, etaz, koljeno, nastavak, kota, T komad tokom crtanja
	#include "commands/Draw.inl"   // TVS_SETTINGS, TVS_DRAW
	#include "commands/NewCommands.inl"   // TVS_OFFSET, TVS_IZVOZ, TVS_PRESJEK, TVS_MATCH, TVS_TK_TIP, TVS_STRETCH, TVS_MOVE ...
	#include "commands/PipeTransWye.inl"   // TVS_PIPE, TVS_TRANS, TVS_WYE
	#include "commands/Connect.inl"   // TVS_CONNECTW, TVS_CONNECTT
	#include "commands/Edit.inl"   // TVS_DUCT, TVS_1D2D, TVS_CUT, TVS_Change, TVS_ERASE, TVS_Flex ...
	#include "commands/Specification.inl"   // specifikacija, atributi, TVS_BLCKMNGR
	#include "commands/Misc.inl"   // TVS_TEST, TVS_AddAtrib, TVS_SCALE, TVS_HEATFLOOR
};

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CTVS_Ventilation_ARXApp)


//new
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PropertyManager, TVS_PropertyManager, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AttributesToProps, TVS_AttributesToProps, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ExcelSp, TVS_ExcelSp, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Sp, TVS_Sp, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AddProperty, TVS_AddProperty, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)




ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_MOVE, TVS_MOVE, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ALIGN, TVS_ALIGN, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_KOLJENA_UGAO, TVS_KOLJENA_UGAO, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PRERACUNAJ, TVS_PRERACUNAJ, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)

//old
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PIPE, TVS_PIPE, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_DRAW, TVS_DRAW, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTT, TVS_CONNECTT, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTW, TVS_CONNECTW, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_1D2D, TVS_1D2D, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SPEC, TVS_SPEC, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_BLCKMNGR, TVS_BLCKMNGR, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_EXCELSPEC, TVS_EXCELSPEC, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CUT, TVS_CUT, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Change, TVS_Change, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ERASE, TVS_ERASE, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_RAZBIJ, TVS_RAZBIJ, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_LEAD, TVS_LEAD, ACRX_CMD_MODAL, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_LEAD_Recalculate, TVS_LEAD_Recalculate, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)

ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SCALE, TVS_SCALE, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_AddAtrib, TVS_AddAtrib, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SETTINGS, TVS_SETTINGS, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_HEATFLOOR, TVS_HEATFLOOR, ACRX_CMD_MODAL, NULL)



ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_STRETCH, TVS_STRETCH, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_FLEX_MANZETNE, TVS_FLEX_MANZETNE, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_FLEX_MANZETNA, TVS_FLEX_MANZETNA, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TK_TIP, TVS_TK_TIP, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET | ACRX_CMD_REDRAW, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PRELAZ_KOTA, TVS_PRELAZ_KOTA, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_DIM_PRATI, TVS_DIM_PRATI, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_LEAD_AUTO, TVS_LEAD_AUTO, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_IZVOZ, TVS_IZVOZ, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_KOTE, TVS_KOTE, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_NATPIS, TVS_NATPIS, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PRESJEK, TVS_PRESJEK, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_MATCH, TVS_MATCH, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)

// Razvojne / test komande: samo u razvojnom buildu (TVS_DEV), ne u izdanju.
#ifdef TVS_DEV
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TEST, TVS_TEST, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TRANS, TVS_TRANS, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_WYE, TVS_WYE, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_CONNECTTest, TVS_CONNECTTest, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_DUCT, TVS_DUCT, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_SpecificationOld, TVS_SpecificationOld, ACRX_CMD_TRANSPARENT | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Flex, TVS_Flex, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_Show, TVS_Show, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVSSomeParts, TVSSomeParts, ACRX_CMD_TRANSPARENT, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyCommand, MyCommandLocal, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MyPickFirst, MyPickFirstLocal, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, TVSMyGroup, MySessionCmd, MySessionCmdLocal, ACRX_CMD_MODAL | ACRX_CMD_SESSION, NULL)
ACED_ADSSYMBOL_ENTRY_AUTO(CTVS_Ventilation_ARXApp, MyLispFunction, false)
#endif

#if defined(TVS_OFFSET_PROOF)&&defined(TVS_DEV)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TEST_OFFSET, TVS_TEST_OFFSET, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TEST_OFFSET_CHECK, TVS_TEST_OFFSET_CHECK, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_TEST_OFFSET_FAIL, TVS_TEST_OFFSET_FAIL, ACRX_CMD_MODAL, NULL)
#endif

#ifdef TVS_OFFSET_INTEGRATION
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OFFSET_STATUS, TVS_OFFSET_STATUS, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OFFSET, TVS_OFFSET, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_ETAZ_KOTA, TVS_ETAZ_KOTA, ACRX_CMD_MODAL, NULL)
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OFFSET_POLICY, TVS_OFFSET_POLICY, ACRX_CMD_MODAL, NULL)
#ifdef TVS_DEV
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OFFSET_TEST_BAD, TVS_OFFSET_TEST_BAD, ACRX_CMD_MODAL, NULL)
#endif
#endif

#ifdef TVS_OFFSET_INTEGRATION
#ifdef TVS_DEV
ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OFFSET_FAIL_NEXT, TVS_OFFSET_FAIL_NEXT, ACRX_CMD_MODAL, NULL)
#endif
#endif

