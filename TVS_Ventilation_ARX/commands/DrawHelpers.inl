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
#endif
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
			if (p->ThisRound || p->SizeB <= 0) { acutPrintf(L"\nTK-O/TK-G su za pravougaone kanale."); return false; }
			in.start = p->FirstPoint; in.end = p->LastPoint; in.A = p->SizeA; in.B = p->SizeB;
		}
		const double f = tvsUnits::factor();
		in.l = tvsTeeData::collarMm() / f; in.minimum = 50 / f;
		const int last = tvsTeeCommand::lastType();
		const int def = last == tvsTee::Pants || last == tvsTee::Tap ? last : tvsTee::Branch;
		wchar_t prompt[160]; swprintf_s(prompt, L"\nTip T komada [Ogranak (TK-O)/Gacasti (TK-G)/Stucna (SR5-3)] <%ls>: ", tvsTeeCommand::typeKeyword(def));
		acedInitGet(0, L"Ogranak Gacasti Stucna"); ACHAR kw[32] = L"";
		const int rc = acedGetKword(prompt, kw);
		if (rc != RTNORM && rc != RTNONE) return false;
		in.type = rc == RTNONE ? def : (_wcsicmp(kw, L"Gacasti") == 0 ? tvsTee::Pants : _wcsicmp(kw, L"Stucna") == 0 ? tvsTee::Tap : tvsTee::Branch);
		double w1 = 0, second = 0;
		const bool pants = in.type == tvsTee::Pants, tap = in.type == tvsTee::Tap;
		if (!tvsTeeCommand::askWidth(pants ? L"\nSirina desnog ogranka, mm" : L"\nSirina ogranka, mm", in.A * f, w1)) return false;
		if (!tap && !tvsTeeCommand::askWidth(pants ? L"\nSirina lijevog ogranka, mm" : L"\nSirina izlaza prolaza C, mm", (pants ? w1 / f : in.A) * f, second)) return false;
		in.w1 = w1 / f; in.second = second / f;
		if (tap) { in.l = tvsTeeCommand::tapLength(in.w1, f); in.second = 0; }
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
				r.radius2 = pants ? tvsTee::defaultRadius(r.second) : 0;
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
		A1 = pFirstPoint;   // direction of the new duct (outlet may be shifted sideways): next point continues it
		A2 = pLastPoint;
		pt1[0] = A2.x;
		pt1[1] = A2.y;
		pt1[2] = A2.z;
	}


