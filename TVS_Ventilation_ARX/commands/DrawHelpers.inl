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
		// Level reference is remembered; the stored value is always the axis level.
		const double elevHalf = (globSizeB > 0 ? globSizeB : globSizeA) / 2;
		dg.ElevUp = globalElevRef == 1; dg.ElevDown = globalElevRef == 2; dg.ElevMid = !(dg.ElevUp || dg.ElevDown);
		dg.Elev = globalElevMid + (globalElevRef == 1 ? elevHalf : globalElevRef == 2 ? -elevHalf : 0);
		dg.TapForm = globalTapForm;
		dg.TypeRoundTap = globalTypeRoundTap;
		dg.RadiusTypeRound = globalRadiusTypeRound;
		dg.RadiusTypeRect = globalRadiusTypeRect;
		dg.TapRadiusVariableParameter = globalTapRadiusVariableParameter;
		dg.TapRadiusConst = globalTapRadiusConst;

		dg.WipeoutLength = globalWipeoutLength;
		if (globalDuctType == DuctTypeFlex) dg.DuctFlex = TRUE; else dg.DuctFlex = FALSE;
		dg.LevelTooltip = tvsLevelTooltip::enabled() ? TRUE : FALSE;
				{ ACHAR v[16] = L""; dg.LeadAuto = (acedGetEnv(L"TVS_LEAD_AUTO", v, 16) == RTNORM && !_wcsicmp(v, L"0")) ? FALSE : TRUE; }
		dg.WholeRun = tvsSizeFollow::wholeRun() ? TRUE : FALSE;
		AcDbDatabase* capDb = acdbHostApplicationServices()->workingDatabase();
		const bool capWas = capDb && tvsDuctCaption::enabled(capDb);
		dg.DuctCaption = capWas ? TRUE : FALSE;
		const tvsSpiro::Settings spiroWas = tvsSpiro::read(capDb);
		dg.SpiroCatalog = spiroWas.catalog ? TRUE : FALSE; dg.SpiroRG = spiroWas.rg ? TRUE : FALSE;
		dg.SpiroDashed = spiroWas.dashed ? TRUE : FALSE; dg.SpiroC = spiroWas.cMm;
		dg.TransVert = tvsTransAlignCmd::defaultAlign();
		dg.TransPlan = ftTransType;

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
		tvsTeeFit::transitionSetting() = globalLengthTr * tvsUnits::factor();   // T komad: transition at a width step

		globalWipeout = dg.Wipe;
		globalGrani = dg.Grani;
		global1D = dg.D1;
		globalTapForm = dg.TapForm;
		globalTypeRoundTap = dg.TypeRoundTap;
		globalRadiusTypeRound = dg.RadiusTypeRound;
		globalRadiusTypeRect = dg.RadiusTypeRect;
		globalTapRadiusVariableParameter = dg.TapRadiusVariableParameter;
		globalTapRadiusConst = dg.TapRadiusConst;

		globalWipeoutLength = dg.WipeoutLength;

		tvsLevelTooltip::setEnabled(dg.LevelTooltip == TRUE);   // oblacic sa kotama
				acedSetEnv(L"TVS_LEAD_AUTO", dg.LeadAuto == TRUE ? L"1" : L"0");   // isto kao TVS_LEAD_AUTO
		acedSetEnv(L"TVS_DIONICA", dg.WholeRun == TRUE ? L"1" : L"0");     // TVS_DIM_PRATI: dijalog cijele dionice
		bool regen = capDb && (dg.DuctCaption == TRUE) != capWas && tvsDuctCaption::set(capDb, dg.DuctCaption == TRUE);   // isto kao TVS_NATPIS
		{	// spiro prelazi po katalogu SOKO (po crtezu)
			tvsSpiro::Settings sp; sp.catalog = dg.SpiroCatalog == TRUE; sp.rg = dg.SpiroRG == TRUE; sp.dashed = dg.SpiroDashed == TRUE; sp.cMm = dg.SpiroC;
			if (capDb && (sp.catalog != spiroWas.catalog || sp.rg != spiroWas.rg || sp.dashed != spiroWas.dashed || std::fabs(sp.cMm - spiroWas.cMm) > 1e-9)
				&& tvsSpiro::write(capDb, sp)) regen = true;
		}
		if (regen) {
			AcApDocument* capDoc = acDocManager ? acDocManager->curDocument() : NULL;
			if (capDoc) acDocManager->sendStringToExecute(capDoc, L"_.REGENALL\n", false, true, false);
		}
		tvsTransAlignCmd::setDefault(dg.TransVert);              // poravnanje novih prelaza
		if (dg.TransPlan >= 1 && dg.TransPlan <= 3) { ftTransType = dg.TransPlan; tvsTransAlignCmd::storePlan(ftTransType); }   // tip u tlocrtu
		if (dg.DuctFlex == TRUE) { globalDuctType = DuctTypeFlex; globSizeB = 0; }
		else globalDuctType = DuctTypeStill;
		if (globSizeB == 0) globRound = true;
		else globRound = false;
		// Vrh/Dno: entered value is the top/bottom edge of the (new) duct size; convert to axis.
		globalElevRef = dg.ElevUp == 1 ? 1 : dg.ElevDown == 1 ? 2 : 0;
		{
			const double half = (globSizeB > 0 ? globSizeB : globSizeA) / 2;
			globalElevMid = dg.Elev - (globalElevRef == 1 ? half : globalElevRef == 2 ? -half : 0);
			globalElev = globalElevMid;
		}
		if (a == globSizeA && b == globSizeB)
			return false;
		else
			return true;
	}


	// Elevation changed in the size dialog while drawing: offer an etaz (TVS_OFFSET triple) from
	// the current duct, at its present size; a size change follows as a transition after it.
	// The etaz targets the axis that, after that transition (its height alignment), gives the
	// level entered in the dialog. Ne: as before, the next duct starts at the new level.
	static void etazAfterSize(double oldA, double oldB, bool sized, AcGePoint3d &A1, AcGePoint3d &A2, TVS_Pipe *&pipi, ads_point &pt1)
	{
#ifdef TVS_OFFSET_INTEGRATION
		if (!pipi) return;
		double current = 0;
		{ AcDbObjectPointer<TVS_Pipe> p(pipi->objectId(), AcDb::kForRead); if (p.openStatus() != Acad::eOk) return; current = p->Elev; }
		const double f = tvsUnits::factor(), newAxis = globalElevMid;
		const double target = newAxis - (sized ? tvsTransAlign::endOffset(tvsTransAlignCmd::defaultAlign(), oldA, oldB, globSizeA, globSizeB) : 0);
		if (std::fabs(target - current) * f <= 0.5) return;
		// the Z dialog (etaz) opens with the new level proposed: U redu = etaz, Otkazi = no etaz
		acutPrintf(L"\nKota je promijenjena za %+.0f mm: dijalog etaza (Otkazi = bez etaza).", (target - current) * f);
		const double use = globalAxis > 0 && globalAxis < 90 ? globalAxis : 45;
		double accepted = 0, angle = 0;
		const tvsOffsetIntegration::Outcome r = tvsOffsetIntegration::dialogTo(pipi, target, use, accepted, angle);
		if (r == tvsOffsetIntegration::Applied) {
			A1 = pipi->FirstPoint; A2 = pipi->LastPoint; pt1[0] = A2.x; pt1[1] = A2.y; pt1[2] = A2.z;
			if (!sized) { globalElevMid = accepted; globalElev = accepted; }
			globalAxis = angle;
			const double ends = accepted + (sized ? tvsTransAlign::endOffset(tvsTransAlignCmd::defaultAlign(), oldA, oldB, globSizeA, globSizeB) : 0);
			acutPrintf(L"\nEtaz umetnut; crtanje se nastavlja na koti ose %.0f mm.", ends * f);
		}
		else acutPrintf(L"\nEtaz nije umetnut (%ls); sljedeci kanal pocinje na novoj koti.", r == tvsOffsetIntegration::NoChange ? L"nema razlike kota" : r == tvsOffsetIntegration::Cancelled ? L"otkazano" : L"vidi poruku iznad");
#else
		(void)oldA; (void)oldB; (void)sized; (void)A1; (void)A2; (void)pipi; (void)pt1;
#endif
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
		if (!reopen(pEnt)) return NULL;
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
				if (!reopen(pipi)) return;
				pipi->assertWriteEnabled();
				pipi->put_Lastpoint(A2a);
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

				if (reopen(pTap)) { pTap->erase(); pTap->close(); }

			}
		}
		else
		{


			id = pipi->id();
			acdbGetAdsName(eName, id);
			acdbGetObjectId(id, eName);
			if (!reopen(pipi)) return;
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
			lt = false;   // a pick that is not a duct asks again (before: the same pick for ever)
			while (lt == false)
			{
				stat = tvsCoordinates::selectEntity(_T("\nIzaberite kanal:"), vozd, pt1);

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
							if (pipi2 == pipi1) acutPrintf(_T("\nIzabran je kanal koji se crta; izaberite drugi kanal."));
							else ft = true;
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


#ifdef TVS_OFFSET_INTEGRATION
    static tvsOffsetIntegration::Outcome changeZ(TVS_Pipe*& pPipe){
        double elevation=0,angle=0;
        tvsOffsetIntegration::Outcome result=tvsOffsetIntegration::execute(pPipe,elevation,angle);
        if(result==tvsOffsetIntegration::Applied){globalElevMid=elevation;globalElev=elevation;globalAxis=angle;}
        if(result==tvsOffsetIntegration::Failed)acutPrintf(L"\nTVS OFFSET: operacija nije primijenjena; stanje nastavka trase nije ažurirano.");
        return result;
    }
#else
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

			if (!reopen(pTap)) return false;
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

				if (!reopen(pTap2)) return false;
				pTap2->put_Form(Form_Up);
				pTap2->put_Centerpoint(p5);
				pTap2->close();
				if (!reopen(pTap)) return false;
				pTap->put_Form(Form_Down);
				pTap->close();

			}
			else
			{
				if (!reopen(pTap)) return false;
				pTap->put_Form(Form_Up);
				pTap->put_Elevation(startZ);
				pTap->close();

				if (!reopen(pTap2)) return false;
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
				if (!reopen(pPipe1)) return false;

				pPipe1->put_Elevation(startZ);
				pPipe1->close();
			}
			//draw pPipe2
			TVS_Pipe*pPipe2 = pPipe2->add_new(p6, p7, globSizeA, globSizeB);
			SetGlobalProperty(pPipe2);
			SetPropertyLikePipe(pPipe, pPipe2);
			if (!reopen(pPipe2)) return false;

			pPipe2->put_Elevation(nextZ);
			pPipe2->close();
			//correct PPipe
			if (!reopen(pPipe)) return false;
			pPipe->put_Lastpoint(p1);
			pPipe->close();
			pPipe = pPipe2;

		}
		else return false;

		return true;
	}
#endif
	// Kosa stucna 45 while drawing a round duct (user 2026-11-26, option Tkomad): at the end of the
	// duct being drawn, leaning along the drawing direction, branch to the chosen side; with or
	// without koljeno 45 (asked every time); drawing goes on from prolaz or ogranak, the other
	// outlet gets a short duct. Geometry: geometry/TeeShape.h (lateral), koljeno: tvsTeeDialog::planElbow.
	static bool lateralDuringDrawing(AcGePoint3d &A1, AcGePoint3d &A2, TVS_Pipe *&pipi, ads_point &pt1)
	{
		tvsTrace::Scope trace_("lateralDuringDrawing");
		const AcDbObjectId pipeId = pipi->objectId();
		AcGePoint3d s0, s1; double A = 0, elev = 0; bool oneD = false;
		{
			AcDbObjectPointer<TVS_Pipe> p(pipeId, AcDb::kForRead);
			if (p.openStatus() != Acad::eOk) return false;
			s0 = p->FirstPoint; s1 = p->LastPoint; A = p->SizeA; elev = p->Elev; oneD = p->This1D;
		}
		if (oneD) { acutPrintf(L"\nKosa stucna: nije za 1D kanale."); return false; }
		const double f = tvsUnits::factor(), deg = tvsTee::lateralDeg(), pi = 3.14159265358979323846;
		AcGeVector3d u = s1 - s0; u.z = 0; if (u.length() < 1e-9) return false; u.normalize();
		double wMm = 0;
		if (!tvsTeeCommand::askWidth(L"\nKosa stucna 45° – precnik ogranka, mm", A * f, wMm)) return false;
		const double w = wMm / f;
		if (w > A + 1e-9) { acutPrintf(L"\nKosa stucna: ogranak je veci od kanala."); return false; }
		acedInitGet(0, L"Lijevo Desno"); ACHAR kw[32] = L"";
		int rc = acedGetKword(L"\nStrana ogranka [Lijevo/Desno] <Desno>: ", kw, 32);
		if (rc != RTNORM && rc != RTNONE) return false;
		const bool left = rc == RTNORM && _wcsicmp(kw, L"Lijevo") == 0;
		acedInitGet(0, L"Koljeno Bez"); kw[0] = 0;
		rc = acedGetKword(L"\nKoljeno 45° na stucni (ogranak okomito na kanal) [Koljeno/Bez] <Koljeno>: ", kw, 32);
		if (rc != RTNORM && rc != RTNONE) return false;
		const bool elbow = !(rc == RTNORM && _wcsicmp(kw, L"Bez") == 0);
		const AcGeVector3d v = left ? AcGeVector3d(-u.y, u.x, 0) : AcGeVector3d(u.y, -u.x, 0);
		AcGeVector3d d = u + v; d.normalize();
		tvsTeeData::Record r; const bool soko = tvsTeeFit::lateralRecord(r, A, w, f);   // Soko TK (L, L2) or 60 / 60 mm
		tvsTee::Tee t; t.type = tvsTee::Tap; t.round = true; t.A = A; t.C = A; t.w1 = w; t.l = r.l; t.margin = r.radius2; t.lateral = deg;
		double before = 0, after = 0; tvsTee::tapRun(t, before, after);
		double L = tvsTee::lateralLengthPl(A, w, deg, r.l);
		const AcGePoint3d B = s1 + u * before, run1 = B + u * after, face = B + d * (A / 2 + L);   // the run starts at the duct end
		double sa = A, sb = 0, ta = w, tb = 0; AcGeVector3d vpr = u, votv = d; AcGePoint3d base = B;
		TVS_WYE* pWye = TVS_WYE::add_new(sa, sb, ta, tb, L, vpr, votv, base, true, true, false);
		if (!pWye) return false;
		const AcDbObjectId wyeId = pWye->id();
		SetGlobalProperty(pWye);
		SetPropertyLikePipe(pipi, pWye);
		{
			AcDbObjectPointer<TVS_WYE> wy(wyeId, AcDb::kForWrite);
			if (wy.openStatus() != Acad::eOk) return false;
			wy->LengthPl = L; wy->Vectpr = vpr; wy->Vectotv = votv; wy->Basepoint = base;
			if (tvsTeeData::write(wy.object(), r) == Acad::eOk) wy->recordGraphicsModified();
		}
		AcGePoint3d branch = face; AcGeVector3d branchDir = d;
		if (elbow) {
			tvsTeeDialog::Section sec; sec.a = w; sec.b = 0; sec.round = true; AcGePoint3d exit; AcGeVector3d out;
			if (tvsTeeDialog::planElbow(wyeId, face, d, left ? pi / 4 : -pi / 4, sec, elev, f, exit, out)) { branch = exit; branchDir = out; }
			else acutPrintf(L"\nKosa stucna: koljeno 45 nije napravljeno; ogranak ide pod 45°.");
		}
		acedInitGet(0, L"Prolaz Ogranak"); ACHAR nk[32] = L"";
		const int nrc = acedGetKword(L"\nNastavak crtanja [Prolaz/Ogranak] <Prolaz>: ", nk);
		const bool fromBranch = nrc == RTNORM && _wcsicmp(nk, L"Ogranak") == 0;
		TVS_Pipe pipie;
		{	// short duct on the other outlet, to be continued later
			AcGePoint3d o0 = fromBranch ? run1 : branch, o1 = o0 + (fromBranch ? u : branchDir) * (300 / f);
			double wa = fromBranch ? A : w, wb = 0;
			TVS_Pipe* stub = pipie.add_new(o0, o1, wa, wb, false, true);
			if (stub) { const AcDbObjectId sid = stub->id(); SetPropertyLikePipe(pipi, stub);   // copies the size: set it back
				AcDbObjectPointer<TVS_Pipe> q(sid, AcDb::kForWrite); if (q.openStatus() == Acad::eOk) { q->ThisRound = true; q->put_SizeA(wa); q->put_SizeB(0); } }
		}
		AcGePoint3d c0 = fromBranch ? branch : run1, c1 = c0 + (fromBranch ? branchDir : u) * (100 / f);
		double ca = fromBranch ? w : A, cb = 0;
		TVS_Pipe* next = pipie.add_new(c0, c1, ca, cb, false, true);
		tvsTeeCommand::lastType() = tvsTee::Tap;
		acutPrintf(L"\nKosa račva TK 45°%ls umetnuta (%.0f / %.0f, L = %.0f mm%ls); crtanje se nastavlja iz %ls.", elbow ? L" i koljeno 45°" : L"", A * f, w * f,
			(before + after) * f, soko ? L", Soko" : L", nije u Soko tabeli: 60 / 60 mm", fromBranch ? L"ogranka" : L"prolaza");
		if (!next) return true;
		const AcDbObjectId nid = next->id();
		SetPropertyLikePipe(pipi, next);
		{ AcDbObjectPointer<TVS_Pipe> q(nid, AcDb::kForWrite); if (q.openStatus() == Acad::eOk) { q->ThisRound = true; q->put_SizeA(ca); q->put_SizeB(0); } }
		globSizeA = ca;
		pipi = next; A1 = c0; A2 = c1;
		pt1[0] = A2.x; pt1[1] = A2.y; pt1[2] = A2.z;
		return true;
	}
	// Racva Y45 (Soko "Spiro Y45", user 2026-11-26) on the end of the round duct being drawn: inlet face
	// at the duct end, branches at 45 deg right (d3) / left (d4), L2 from the Soko table (else d3 +
	// spigot); optional 45 deg elbows on both branches; the other branch gets a 300 mm duct.
	static bool wyeDuringDrawing(AcGePoint3d &A1, AcGePoint3d &A2, TVS_Pipe *&pipi, ads_point &pt1)
	{
		tvsTrace::Scope trace_("wyeDuringDrawing");
		const AcDbObjectId pipeId = pipi->objectId();
		AcGePoint3d s0, s1; double A = 0, elev = 0; bool oneD = false;
		{
			AcDbObjectPointer<TVS_Pipe> p(pipeId, AcDb::kForRead);
			if (p.openStatus() != Acad::eOk) return false;
			s0 = p->FirstPoint; s1 = p->LastPoint; A = p->SizeA; elev = p->Elev; oneD = p->This1D;
		}
		if (oneD) { acutPrintf(L"\nRačva Y45: nije za 1D kanale."); return false; }
		const double f = tvsUnits::factor(), deg = tvsTee::lateralDeg(), pi = 3.14159265358979323846;
		AcGeVector3d u = s1 - s0; u.z = 0; if (u.length() < 1e-9) return false; u.normalize();
		const int soko = tvsSokoPants::branchFor(A * f);
		double w1Mm = 0, w2Mm = 0;
		if (!tvsTeeCommand::askWidth(L"\nRačva Y45 – prečnik desnog ogranka, mm", soko > 0 ? soko : A * f, w1Mm)) return false;
		if (!tvsTeeCommand::askWidth(L"\nRačva Y45 – prečnik lijevog ogranka, mm", w1Mm, w2Mm)) return false;
		const double w1 = w1Mm / f, w2 = w2Mm / f;
		if (w1 > A + 1e-9 || w2 > A + 1e-9) { acutPrintf(L"\nRačva Y45: ogranak je veći od ulaza."); return false; }
		tvsTeeData::Record r; double L = 0; const bool table = tvsTeeFit::wyeRecord(r, A, w1, w2, f, L);
		tvsTee::Tee t; t.type = tvsTee::Pants; t.round = true; t.A = A; t.w1 = w1; t.w2 = w2; t.l = r.l; t.margin = r.radius2; t.lateral = deg;
		if (!tvsTee::valid(t)) { acutPrintf(L"\nRačva Y45 nije moguća: ogranci su premali za ulaz %.0f mm.", A * f); return false; }
		acedInitGet(0, L"Okomito Paralelno Bez"); ACHAR kw[32] = L"";
		int rc = acedGetKword(L"\nKoljena 45° na ogranke [Okomito/Paralelno/Bez] <Bez>: ", kw, 32);
		if (rc != RTNORM && rc != RTNONE) return false;
		const int elbows = rc == RTNORM && _wcsicmp(kw, L"Okomito") == 0 ? 1 : rc == RTNORM && _wcsicmp(kw, L"Paralelno") == 0 ? 2 : 0;
		const AcGeVector3d ex(u.y, -u.x, 0);   // +x of the shape = right branch (w1)
		tvsTee::P lp[3], ld[3]; tvsTee::wyePorts(t, lp, ld);
		const double yO = tvsTee::wyeAxisY(A, w1, r.radius2, deg);
		double sa = w1, sb = 0, ta = A, tb = 0; AcGeVector3d vpr = ex, votv = -u; AcGePoint3d base = s1 + u * yO;
		TVS_WYE* pWye = TVS_WYE::add_new(sa, sb, ta, tb, L, vpr, votv, base, true, true, false);
		if (!pWye) return false;
		const AcDbObjectId wyeId = pWye->id();
		SetGlobalProperty(pWye);
		SetPropertyLikePipe(pipi, pWye);
		{
			AcDbObjectPointer<TVS_WYE> wy(wyeId, AcDb::kForWrite);
			if (wy.openStatus() != Acad::eOk) return false;
			wy->SizeApr = w1; wy->SizeBpr = 0; wy->SizeAotv = A; wy->SizeBotv = 0; wy->ThisRoundpr = true; wy->ThisRoundotv = true;
			wy->LengthPl = L; wy->Vectpr = vpr; wy->Vectotv = votv; wy->Basepoint = base;
			if (tvsTeeData::write(wy.object(), r) == Acad::eOk) wy->recordGraphicsModified();
		}
		// outlets: 0 = right (w1), 1 = left (w2)
		AcGePoint3d at[2]; AcGeVector3d dir[2]; const double wd[2] = { w1, w2 };
		for (int k = 0; k < 2; ++k) {
			const int q = k == 0 ? 1 : 0;   // shape port 1 = +x (right), 0 = -x (left)
			at[k] = s1 + ex * lp[q].x + u * lp[q].y; dir[k] = ex * ld[q].x + u * ld[q].y; dir[k].normalize();
			if (elbows) {   // right: perpendicular turns clockwise, left counter-clockwise; parallel the other way
				const double turn = (k == 0 ? -1 : 1) * (elbows == 1 ? 1 : -1) * pi / 4;
				tvsTeeDialog::Section sec; sec.a = wd[k]; sec.b = 0; sec.round = true; AcGePoint3d exit; AcGeVector3d out;
				if (tvsTeeDialog::planElbow(wyeId, at[k], dir[k], turn, sec, elev, f, exit, out)) { at[k] = exit; dir[k] = out; }
				else acutPrintf(L"\nRačva Y45: koljeno 45° nije napravljeno; ogranak ide pod 45°.");
			}
		}
		acedInitGet(0, L"Desni Lijevi"); ACHAR nk[32] = L"";
		const int nrc = acedGetKword(L"\nNastavak crtanja [Desni/Lijevi] <Desni>: ", nk);
		const int cont = nrc == RTNORM && _wcsicmp(nk, L"Lijevi") == 0 ? 1 : 0, other = 1 - cont;
		TVS_Pipe pipie;
		{	// short duct on the other branch, to be continued later
			AcGePoint3d o0 = at[other], o1 = o0 + dir[other] * (300 / f); double wa = wd[other], wb = 0;
			TVS_Pipe* stub = pipie.add_new(o0, o1, wa, wb, false, true);
			if (stub) { const AcDbObjectId sid = stub->id(); SetPropertyLikePipe(pipi, stub);   // copies the size: set it back
				AcDbObjectPointer<TVS_Pipe> q(sid, AcDb::kForWrite); if (q.openStatus() == Acad::eOk) { q->ThisRound = true; q->put_SizeA(wa); q->put_SizeB(0); } }
		}
		AcGePoint3d c0 = at[cont], c1 = c0 + dir[cont] * (100 / f);
		double ca = wd[cont], cb = 0;
		TVS_Pipe* next = pipie.add_new(c0, c1, ca, cb, false, true);
		tvsTeeCommand::lastType() = tvsTee::Pants;
		acutPrintf(L"\nRačva Y45%ls umetnuta (Ø%.0f / Ø%.0f, Ø%.0f, L2 = %.0f mm%ls); crtanje se nastavlja iz %ls ogranka.", elbows ? L" i koljena 45°" : L"",
			A * f, w1 * f, w2 * f, r.l * f, table ? L", Soko" : L", nije u Soko tabeli: d3 + nipl", cont ? L"lijevog" : L"desnog");
		if (!next) return true;
		const AcDbObjectId nid = next->id();
		SetPropertyLikePipe(pipi, next);
		{ AcDbObjectPointer<TVS_Pipe> q(nid, AcDb::kForWrite); if (q.openStatus() == Acad::eOk) { q->ThisRound = true; q->put_SizeA(ca); q->put_SizeB(0); } }
		globSizeA = ca;
		pipi = next; A1 = c0; A2 = c1;
		pt1[0] = A2.x; pt1[1] = A2.y; pt1[2] = A2.z;
		return true;
	}
	// Round duct: kosa racva TK (stucna 45) or racva Y45 (asked every time).
	static bool roundTeeDuringDrawing(AcGePoint3d &A1, AcGePoint3d &A2, TVS_Pipe *&pipi, ads_point &pt1)
	{
		const bool wasWye = tvsTeeCommand::lastType() == tvsTee::Pants;
		acedInitGet(0, L"Kosa Y45"); ACHAR kw[32] = L"";
		const int rc = acedGetKword(wasWye ? L"\nRačva [Kosa/Y45] <Y45>: " : L"\nRačva [Kosa/Y45] <Kosa>: ", kw, 32);
		if (rc != RTNORM && rc != RTNONE) return false;
		const bool wye = rc == RTNONE ? wasWye : _wcsicmp(kw, L"Y45") == 0;
		return wye ? wyeDuringDrawing(A1, A2, pipi, pt1) : lateralDuringDrawing(A1, A2, pipi, pt1);
	}
	// T komad faza 2 (TVS_DRAW / TVS_PIPE option Tkomad): TK-O / TK-G placed on the duct being
	// drawn with a preview (TeeInsert.h); the duct is extended or shortened to the inlet face,
	// the other outlet gets a short duct, drawing continues from the chosen outlet.
	static bool teeDuringDrawing(AcGePoint3d &A1, AcGePoint3d &A2, TVS_Pipe *&pipi, ads_point &pt1)
	{
		if (!pipi) return false;
		const AcDbObjectId pipeId = pipi->objectId();
		tvsTeeInsert::Input in;
		{
			AcDbObjectPointer<TVS_Pipe> p(pipeId, AcDb::kForRead);
			if (p.openStatus() != Acad::eOk) return false;
			if (p->ThisRound || p->SizeB <= 0) { p.close(); return roundTeeDuringDrawing(A1, A2, pipi, pt1); }   // okrugli: kosa stucna / Y45
			in.start = p->FirstPoint; in.end = p->LastPoint; in.A = p->SizeA; in.B = p->SizeB;
		}
		const double f = tvsUnits::factor();
		in.l = tvsTeeData::collarMm() / f; in.minimum = 50 / f;
		const int last = tvsTeeCommand::lastType();
		const int def = last == tvsTee::Pants || last == tvsTee::Tap ? last : tvsTee::Branch;
		acutPrintf(L"\nOgranak = TK-O, Gacasti = TK-G, Stucna = SR5-3.");
		wchar_t prompt[160]; swprintf_s(prompt, L"\nTip T komada [Ogranak/Gacasti/Stucna] <%ls>: ", tvsTeeCommand::typeKeyword(def));
		acedInitGet(0, L"Ogranak Gacasti Stucna TK-O TK-G SR5-3"); ACHAR kw[32] = L"";
		const int rc = acedGetKword(prompt, kw);
		if (rc == RTNORM) tvsTeeCommand::typeAlias(kw, 32);
		if (rc != RTNORM && rc != RTNONE) return false;
		in.type = rc == RTNONE ? def : (_wcsicmp(kw, L"Gacasti") == 0 ? tvsTee::Pants : _wcsicmp(kw, L"Stucna") == 0 ? tvsTee::Tap : tvsTee::Branch);
		double w1 = 0, second = 0;
		const bool pants = in.type == tvsTee::Pants, tap = in.type == tvsTee::Tap;
		if (!tvsTeeCommand::askWidth(pants ? L"\nSirina desnog ogranka, mm" : L"\nSirina ogranka, mm", in.A * f, w1)) return false;
		if (!tap && !tvsTeeCommand::askWidth(pants ? L"\nSirina lijevog ogranka, mm" : L"\nSirina izlaza prolaza C, mm", (pants ? w1 / f : in.A) * f, second)) return false;
		in.w1 = w1 / f; in.second = second / f;
		if (tap) { in.l = tvsTeeCommand::tapLength(in.w1, f); in.second = 0; in.margin = tvsTeeCommand::tapMargin(f); }
		if (pants && !tvsTee::pantsFeasible(in.A, in.w1, in.second, in.l)) {
			acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).", in.A * f);
			return false;
		}
		tvsTeeInsert::Plan plan;
		for (;;) {
			tvsTeeInsert::Jig jig(in);
			if (jig.drag() != AcEdJig::kNormal) return false;
			if (jig.valid()) { plan = jig.result(); break; }
			acutPrintf(L"\nT komad se ne moze postaviti na tom mjestu.");
		}
		// duct up to the inlet face
		{
			AcDbObjectPointer<TVS_Pipe> p(pipeId, AcDb::kForWrite);
			if (p.openStatus() != Acad::eOk || p->put_Lastpoint(plan.face) != Acad::eOk) { acutPrintf(L"\nKanal nije prilagodjen; T komad nije umetnut."); return false; }
			p->recordGraphicsModified();
		}
		double sApr = plan.SizeApr, sBpr = in.B, sAotv = plan.SizeAotv, sBotv = in.B, len = plan.LengthPl;
		AcGeVector3d vpr = plan.Vectpr, votv = plan.Vectotv; AcGePoint3d base = plan.Basepoint;
		TVS_WYE* pWye = TVS_WYE::add_new(sApr, sBpr, sAotv, sBotv, len, vpr, votv, base, false, false, pipi->This1D);
		if (!pWye) return false;
		const AcDbObjectId wyeId = pWye->id();
		SetGlobalProperty(pWye);
		SetPropertyLikePipe(pipi, pWye);
		{
			AcDbObjectPointer<TVS_WYE> w(wyeId, AcDb::kForWrite);
			if (w.openStatus() == Acad::eOk) {
				tvsTeeData::Record r; r.type = in.type; r.l = in.l; r.second = pants ? plan.tee.w2 : tap ? 0 : plan.tee.C;
				r.radius2 = pants ? tvsTee::defaultRadius(r.second) : tap ? in.margin : 0;
				if (tvsTeeData::write(w.object(), r) == Acad::eOk) w->recordGraphicsModified();
			}
		}
		tvsTeeCommand::lastType() = in.type;
		// continuation
		acedInitGet(0, pants ? L"Desni Lijevi" : L"Prolaz Ogranak"); ACHAR nk[32] = L"";
		const int nrc = acedGetKword(pants ? L"\nNastavak crtanja [Desni/Lijevi] <Desni>: " : L"\nNastavak crtanja [Prolaz/Ogranak] <Prolaz>: ", nk);
		int cont = 0;
		if (nrc == RTNORM && (_wcsicmp(nk, L"Lijevi") == 0 || _wcsicmp(nk, L"Ogranak") == 0)) cont = 1;
		TVS_Pipe pipie;
		const int other = 1 - cont;
		{	// short duct on the other outlet, to be continued later
			AcGePoint3d s0 = plan.port[other], s1 = s0 + plan.portDir[other] * (300 / f);
			double wa = plan.portWidth[other], wb = in.B;
			TVS_Pipe* stub = pipie.add_new(s0, s1, wa, wb, false, false);
			if (stub) { const AcDbObjectId sid = stub->id(); SetPropertyLikePipe(pipi, stub);
				AcDbObjectPointer<TVS_Pipe> s(sid, AcDb::kForWrite); if (s.openStatus() == Acad::eOk) s->put_SizeA(plan.portWidth[other]); }
		}
		AcGePoint3d c0 = plan.port[cont], c1 = c0 + plan.portDir[cont] * (100 / f);
		double ca = plan.portWidth[cont], cb = in.B;
		TVS_Pipe* next = pipie.add_new(c0, c1, ca, cb, false, false);
		if (!next) return true;
		const AcDbObjectId nid = next->id();
		SetPropertyLikePipe(pipi, next);
		{ AcDbObjectPointer<TVS_Pipe> s(nid, AcDb::kForWrite); if (s.openStatus() == Acad::eOk) s->put_SizeA(ca); }
		globSizeA = ca;   // drawing goes on with the outlet width
		pipi = next; A1 = c0; A2 = c1;
		pt1[0] = A2.x; pt1[1] = A2.y; pt1[2] = A2.z;
		acutPrintf(L"\n%ls umetnut%ls; crtanje se nastavlja iz %ls.", pants ? L"Gacasti T komad TK-G" : tap ? L"Stucna SR5-3" : L"T komad sa ogrankom TK-O", tap ? L"a" : L"",
			pants ? (cont ? L"lijevog ogranka" : L"desnog ogranka") : (cont ? L"ogranka" : L"prolaza"));
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
		double pLengthTr = tvsSpiro::drawingLength(NULL, pSizeAp1, pSizeBp1 == 0, pSizeAp2, pSizeBp2 == 0, globalLengthTr, globalDuctType == DuctTypeFlex);   // spiro: po katalogu SOKO
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
		const double axisBefore = pipi->Elev;   // axis of the duct before the transition (its start)
		SetGlobalProperty(transie);
		{	// poravnanje po visini (TVS_SETTINGS): the transition starts at the axis of the duct
			// before it; the duct after it continues at that axis + the end offset of the alignment
			// (osa: same axis), whatever the elevation reference (Vrh / Centar / Dno) of the dialog.
			const int align = tvsTransAlignCmd::defaultAlign();
			AcDbObjectPointer<TVS_TRANS> t(transie->objectId(), AcDb::kForWrite);
			if (t.openStatus() == Acad::eOk) {
				t->put_Elevation(axisBefore);
				if (align != tvsTransAlign::Axis) tvsTransAlign::write(t.object(), align);
				t->recordGraphicsModified();
			}
			globalElevMid = axisBefore + tvsTransAlign::endOffset(align, pSizeAp1, pSizeBp1, pSizeAp2, pSizeBp2);
			globalElev = globalElevMid;
		}
		pFirstPoint = AcGePoint3d(pFirstPoint.x + pLengthTr*Vect.x,
			pFirstPoint.y + pLengthTr*Vect.y,
			pFirstPoint.z + pLengthTr*Vect.z
		);
		{	// tip u tlocrtu 2 / 3 (ravno uz desnu / lijevu ivicu): the outlet axis is shifted sideways,
			// as TVS_TRANS draws it (right = (Vect.y, -Vect.x)); the next duct starts there
			AcGeVector3d right(Vect.y, -Vect.x, 0);
			if (right.length() > 1e-12) {
				right.normalize();
				const double s = (pSizeAp1 - pSizeAp2) / 2;
				if (pTransType == 2) pFirstPoint += right * s;
				else if (pTransType == 3) pFirstPoint -= right * s;
			}
		}
				const double stub = 100.0 / tvsUnits::factor();   // first 100 mm of the next duct (drawing units: mm, cm or m)
		AcGePoint3d pLastPoint = AcGePoint3d(pFirstPoint.x + stub * Vect.x,
			pFirstPoint.y + stub * Vect.y,
			pFirstPoint.z + stub * Vect.z
		);

		bool pThis1d = false;

		pipi = pipi->add_new(pFirstPoint,
			pLastPoint,
			pSizeAp2,
			pSizeBp2,
			pThis1d,
			pThisRoundp2);
		SetGlobalProperty(pipi);
		A1 = pFirstPoint;   // direction of the new duct (outlet may be shifted sideways): next point continues it
		A2 = pLastPoint;
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;
	}


