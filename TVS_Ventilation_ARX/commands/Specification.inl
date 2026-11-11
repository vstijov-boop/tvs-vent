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
									// Cuffs marked by TVS_FLEX_MANZETNE: one piece per marked end.
									bool cuffStart = false, cuffEnd = false;
									if (tvsFlexCuff::read(pEnt, cuffStart, cuffEnd) && (cuffStart || cuffEnd)) {
										SPEC cuff;
										wcscpy_s(cuff.grad, grad);
										wcscpy_s(cuff.d, d);
										cuff.addCuffs(pEnt, spec.SizeA, int(cuffStart) + int(cuffEnd));
										speclistflex.append(cuff);
									}
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
#ifdef TVS_OFFSET_INTEGRATION
        if(!offsetSelectionValid(sset)){acedSSFree(sset);return;}
#endif
		if (!tvsTable::prompt()) { acedSSFree(sset); return; }
		ads_point pt1;
		AcGePoint3d pb = AcGePoint3d(0, 0, 0);
		if (tvsCoordinates::getPoint(NULL, _T("\nZadajte tačku umetanja specifikacije:"), pt1) != RTNORM) { acedSSFree(sset); return; }
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
#ifdef TVS_OFFSET_INTEGRATION
        if(!offsetSelectionValid(sset)){acedSSFree(sset);return;}
#endif


		long idx = 1;

		SPEClist speclist, speclistflex;
		SpecWithAttrlist specatrlist;

		fillspecification(sset, speclist, speclistflex, specatrlist, L"°", L"⌀");
		acedSSFree(sset);
		CMSExcel* m_msExcel = new CMSExcel;
		acedSetStatusBarProgressMeter(_T("Izvoz u Excel"), 0, 100);

		if (FAILED(m_msExcel->NewExcelBook(false))) { acutPrintf(_T("\nExcel nije dostupan (nije instaliran ili se ne može pokrenuti); izvoz nije urađen.")); acedRestoreStatusBar(); delete m_msExcel; return; }
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
			acutPrintf(L"\nDefinicija bloka \"%s\" ne postoji.", pName);
			pBlkTbl->close();
			return false;
		}

		es = pBlkTbl->getAt(pName, pBlkRec, AcDb::kForRead);
		if (es != Acad::eOk) { pBlkTbl->close(); return false; }

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
									pSubEnt.close();
									delete pIterBTR;
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
						acutPrintf(_T("\nNije moguće otvoriti element u bloku! Greška: %s"),
							LPCTSTR(acadErrorStatusText(es)));
					}
				}
				else {
					acutPrintf(_T("\nNije moguće dobiti AcDbObjectId elementa u bloku! Greška: %s"),
						LPCTSTR(acadErrorStatusText(es)));
				}
			}
		}
		else {
			acutPrintf(_T("\nNije moguće napraviti iterator zapisa tabele blokova! Greška: %s"), LPCTSTR(acadErrorStatusText(es)));
		}


		delete pIterBTR;
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
			acutPrintf(L"\nDefinicija bloka \"%s\" ne postoji.", pName);
			pBlkTbl->close();
			return;

		}

		es = pBlkTbl->getAt(pName, pBlkRec, AcDb::kForWrite);
		if (es != Acad::eOk) { pBlkTbl->close(); acutPrintf(L"\nBlok \"%s\" se ne može mijenjati (zaključan?).", pName); return; }
		// create a AttributeDefinition
		pAttDef = new AcDbAttributeDefinition(attLoc, text,
			tag, prompt);
		pAttDef->setInvisible(true);
		pAttDef->setLockPositionInBlock(true);
		// append the AttributeDefinition to the definition
		es = pBlkRec->appendAcDbEntity(attId, pAttDef);

		if (es == Acad::eOk) pAttDef->close(); else delete pAttDef;
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
	}

