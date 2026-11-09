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



		if (tvsCoordinates::getPoint(NULL, _T("\nZadajte prvu tačku: "), pt1) != RTNORM)
			return;
		A1 = asPnt3d(pt1);
		if (tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku: "), pt2) != RTNORM)
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
			if (tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku: "), pt2) != RTNORM)
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
		{
			const int rc = acedGetKword(_T("\nPretvorite u [1d/2d]<2d>: "), resultss);
			if (rc == RTNONE) wcscpy_s(resultss, _T("2d"));   // Enter = podrazumijevano
			else if (rc != RTNORM) { acedSSFree(sset); return; }
		}

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
		acedSSFree(sset);
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

			if (tvsCoordinates::selectEntity(_T("\nIzaberite kanal:"), vozd1, pt1) == RTCAN)
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
		// one T komad: the TVS_TK_TIP dialog (TeeTypeDialog.h) instead of the general Svojstva dialog
		if (tvsTeeDialog::forSelection(sset, IDD_TEETYPE)) { acedSSFree(sset); return; }

		Change(sset);
		acedSSFree(sset);






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
		acedSSFree(sset);






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

			const int picked = tvsCoordinates::selectEntity(_T("\nIzaberite liniju: "), ent, pt1);
			if (picked == RTCAN) return;
			Astat = picked == RTNORM;
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
							pEnt->close();
						}
					}
					}
				}
			}
		}
		int * quantity = new int;
		if (acedGetInt(_T("\nBroj segmenata: "), quantity) != RTNORM || *quantity < 1) { delete quantity; return; }
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
