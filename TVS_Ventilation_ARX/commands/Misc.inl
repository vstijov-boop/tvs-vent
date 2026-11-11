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
		acedSSFree(sset);


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
			Astat = tvsCoordinates::selectEntity(_T("\nIzaberite blok za dodavanje atributa: "), ent, pt1);
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
		ads_name sset, ent;
		ads_point pt1;
		if (acedSSGet(_T(""), NULL, NULL, NULL, sset) != RTNORM)
			return;

		double length = 0;
		for (;;)
		{
			const int acadStatus = acedEntSel(_T("\nIzaberite referentnu liniju: "), ent, pt1);
			if (acadStatus == RTCAN) { acedSSFree(sset); return; }
			if (acadStatus != RTNORM) continue;   // prazan klik: ponovo
			AcDbObjectId pId;
			if (acdbGetObjectId(pId, ent) != Acad::eOk) continue;
			AcDbObjectPointer<AcDbLine>pLine(pId, AcDb::kForRead);
			if (pLine.openStatus() != Acad::eOk) { acutPrintf(_T("\nIzabrani objekat nije linija.")); continue; }
			length = pLine->endPoint().distanceTo(pLine->startPoint());
			if (length > 1e-9) break;
			acutPrintf(_T("\nLinija nema dužinu."));
		}

		ads_real len = 0;
		for (;;)
		{
			acedInitGet(RSG_NONULL | RSG_NOZERO | RSG_NONEG, NULL);
			const int acadStatus = acedGetReal(_T("\nUnesite potrebnu dužinu: "), &len);
			if (acadStatus == RTCAN) { acedSSFree(sset); return; }
			if (acadStatus == RTNORM && len > 0) break;
		}
		ads_real scale = len / length;
		acedCommandS(RTSTR, _T("_SCALE"), RTPICKS, sset, RTSTR, _T(""), RTPOINT, pt1, RTREAL, scale, RTNONE);
		acedSSFree(sset);
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
			acadStatus = tvsCoordinates::selectEntity(_T("\nIzaberite poliliniju za crtanje podnog grijanja: "), ent, pt1);
			switch (acadStatus)
			{
			case RTCAN:
				return;
				break;

			}
			if (acadStatus == RTNORM)
			{
				AcDbObjectId pId;
				if (acdbGetObjectId(pId, ent) != Acad::eOk) continue;

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
	// 			acadStatus=tvsCoordinates::getPoint(pt1,_T("\nУкажите точку:"),pt1);
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

