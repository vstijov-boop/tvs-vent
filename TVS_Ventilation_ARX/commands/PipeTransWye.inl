	// ----- Ventilation_ARX.TVS_PIPE command
		static void Ventilation_ARXTVS_PIPE(void)
	{

		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		if (!currentLayerWritable()) return;

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
			Astat = tvsCoordinates::getPoint(NULL, _T("\nZadajte prvu tačku ili [Dimenzija]: "), pt1);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			case RTNORM:
				break;
			default:   // RTERROR / RTNONE ...: no point was given
				return;
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
			Astat = tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija]: "), pt2);
			switch (Astat)
			{
			case RTCAN:
				return;
				break;
			case RTKWORD:
				acedGetInput(resultss);
				break;
			case RTNORM:
				break;
			default:   // RTERROR / RTNONE ...: no point was given
				return;
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


			acedInitGet(RSG_NONULL, _T("Dimenzija D Koljeno K Račva R Tkomad T Z Gore G Nadolje N Obilazak O"));
			Astat = tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Tkomad/Z/Gore/Nadolje/Obilazak]: "), pt2);
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
				break;
			default:   // RTERROR ...
				return;
			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Dimenzija")) == 0 || _wcsicmp(resultss, _T("D")) == 0))
			{
				reg = dSIZE;
				oldsizeA = globSizeA;
				oldsizeB = globSizeB;
				oldRound = globRound;
				const bool sized = changesize();
				etazAfterSize(oldsizeA, oldsizeB, sized, A1, A2, pipi, pt1);   // kota promijenjena u dijalogu: ponudi etaz
				if (sized)
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
			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Tkomad")) == 0 || _wcsicmp(resultss, _T("T")) == 0))
			{
				teeDuringDrawing(A1, A2, pipi, pt1);   // faza 2: TK-O / TK-G in the duct being drawn
			}

#ifdef TVS_BYPASS_PREVIEW
            if(Astat==RTKWORD&&(_wcsicmp(resultss,L"Obilazak")==0||_wcsicmp(resultss,L"O")==0)){
                TVSBypassOutput result;
                if(tvsBypassDuringDrawing(pipi->objectId(),result,tvsBypassRadiusSettings())){
                    // Endpoints from a short read-open of the new outlet by ObjectId; the legacy
                    // loop keeps carrying a pointer, so pipi still takes the committed object.
                    AcDbObjectPointer<TVS_Pipe> next(result.id,AcDb::kForRead);
                    if(next.openStatus()==Acad::eOk){A1=next->FirstPoint;A2=next->LastPoint;}
                    else{A1=result.pipe->FirstPoint;A2=result.pipe->LastPoint;}
                    pipi=result.pipe;globalElevMid=result.elevation;globalElev=result.elevation;globalAxis=result.angle;
                    pt1[0]=A2.x;pt1[1]=A2.y;pt1[2]=A2.z;
                }
            }
#endif
			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Z")) == 0))
			{
				reg = dZ;
#ifdef TVS_OFFSET_INTEGRATION
                if(changeZ(pipi)==tvsOffsetIntegration::Applied){
                    A1=pipi->FirstPoint;A2=pipi->LastPoint;
                    pt1[0]=A2.x;pt1[1]=A2.y;pt1[2]=A2.z;
                }
#else
				changeZ(pipi);
				A1 = A2;
				A2 = pipi->LastPoint;
				pt1[0] = A2.x;
				pt1[1] = A2.y;
				pt1[2] = A2.z;
#endif
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

		if (tvsCoordinates::getPoint(NULL, _T("Zadajte prvu tačku:\n"), pt1) != RTNORM)
			return;
		if (tvsCoordinates::getPoint(pt1, _T("Set the second point:\n"), pt2) != RTNORM)
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

		if (tvsCoordinates::getPoint(NULL, _T("Zadajte prvu tačku:\n"), pt1) != RTNORM)
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

