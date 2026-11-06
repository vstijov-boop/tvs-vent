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
			Astat = tvsCoordinates::getPoint(NULL, _T("\nZadajte prvu tačku ili [Dimenzija]: "), pt1);
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
			Astat = tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija]: "), pt2);
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


			acedInitGet(RSG_NONULL, _T("Dimenzija D Koljeno K Račva R Tkomad T Z Gore G Nadolje N Obilazak O"));
			Astat = tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Tkomad/Z/Gore/Nadolje/Obilazak]: "), pt2);
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
				return false;
			}


			if (Astat == RTKWORD && (_wcsicmp(resultss, _T("Račva")) == 0 || _wcsicmp(resultss, _T("R")) == 0))
			{
				reg = dCONNECTW;
				connect2(pipi, reg);
				return false;
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
