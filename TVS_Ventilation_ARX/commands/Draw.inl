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
		tvsJointFix::AfterDrawing teePorts;   // new ducts on a TK-O / TK-G port: prelaz there (JointFix.h)
		if (!currentLayerWritable()) { TVS_Pipe::SetParamsForDraw(AcDbObjectId::kNull, 0); return; }
		int index;
		AcDbObjectId id;
		TVS_Pipe::GetParamsForDraw(id, index);
		TVS_Pipe::SetParamsForDraw(AcDbObjectId::kNull, 0);   // one grip start only; a plain TVS_Draw starts fresh
		pendingStep3().clear(); pendingTee().clear();   // grip menu answers belong to this start only
		if (index >= tvsPortGrip::base && index < tvsPortGrip::base + 3)
		{	// connector grip on a free fitting port (PortGrips.h): the first duct leaves the port along its axis
			drawFromPort(id, index - tvsPortGrip::base);
			return;
		}


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
					if (index == 101 || index == 102)
					{	// grip menu (user 2026-11-26); 101 sits beyond LastPoint, 102 beyond FirstPoint
						pEnt->close();
						const int m = ductEndMenu(id, index == 101);
						if (m != 1) return;
						pEnt = NULL;
						if (acdbOpenAcDbEntity(pEnt, id, AcDb::kForWrite) != eOk) return;
						if ((pipi = TVS_Pipe::cast(pEnt)) == NULL) { pEnt->close(); return; }
					}
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
					drawStep3(pt1, pt2, pipi);
				}

			}
		}
		//TVS_Pipe::SetParamsForDraw(id,0);
		//TVS_Pipe pipie;


		//pipi=pipie.add_new(A1,A2,globSizeA,globSizeB,false,globRound);

		//if(drawStep1(pt1,pt2,pipi)==false)return;
		//if(drawStep2(pt1,pt2,pipi)==false)return;

	}
	// Grip menu (plan 19 faza 2, user 2026-11-26): a popup at the cursor when a connector grip is
	// clicked - Nastavi crtanje / Nastavi drugom dimenzijom (prelaz) / Umetni koljeno (ugao, strana).
	// Returns 0 cancel, 1 draw, 2 dimension, 100 + deg (+ 1000 left) for an elbow.
	// Duct end (duct = true) also: Spoji koljenom / račvom, T komad (podmeni po obliku), Z, Gore,
	// Nadolje, Obilazak - the options of the drawing prompt (drawStep3), ids 3..8 and 21..25.
	static int portGripMenu(bool duct = false, bool round = false)
	{
		HMENU menu = CreatePopupMenu(), elbow = CreatePopupMenu(), tee = CreatePopupMenu(), level = CreatePopupMenu();
		if (!menu || !elbow || !tee || !level) { for (HMENU h : { menu, elbow, tee, level }) if (h) DestroyMenu(h); return 1; }
		const int degs[4] = { 90, 60, 45, 30 };
		for (int i = 0; i < 4; ++i) {
			wchar_t t[32];
			swprintf_s(t, L"%d° lijevo", degs[i]); AppendMenuW(elbow, MF_STRING, 1100 + degs[i], t);
			swprintf_s(t, L"%d° desno", degs[i]); AppendMenuW(elbow, MF_STRING, 100 + degs[i], t);
		}
		AppendMenuW(menu, MF_STRING, 1, L"Nastavi crtanje");
		AppendMenuW(menu, MF_STRING, 2, L"Nastavi drugom dimenzijom (prelaz)...");
		AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(elbow), L"Umetni koljeno");
		if (duct) {
			if (round) { AppendMenuW(tee, MF_STRING, 24, L"Kosa račva TK 45°"); AppendMenuW(tee, MF_STRING, 25, L"Račva Y45"); }
			else { AppendMenuW(tee, MF_STRING, 21, L"TK-O sa ogrankom"); AppendMenuW(tee, MF_STRING, 22, L"TK-G gaćasti"); AppendMenuW(tee, MF_STRING, 23, L"Štucna 45° SR5-3"); }
			AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(tee), round ? L"Račva" : L"T komad");
			AppendMenuW(level, MF_STRING, 5, L"Z (etaž)...");
			AppendMenuW(level, MF_STRING, 6, L"Koljeno gore");
			AppendMenuW(level, MF_STRING, 7, L"Koljeno nadolje");
#ifdef TVS_BYPASS_PREVIEW
			AppendMenuW(level, MF_STRING, 8, L"Obilazak...");
#endif
			AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(level), L"Promjena visine");
			AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
			AppendMenuW(menu, MF_STRING, 3, L"Spoji sa kanalom koljenom...");
			AppendMenuW(menu, MF_STRING, 4, L"Spoji sa kanalom račvom...");
		}
		else { DestroyMenu(tee); DestroyMenu(level); }
		AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
		AppendMenuW(menu, MF_STRING, 0, L"Odustani");
		SetMenuDefaultItem(menu, 1, FALSE);
		POINT pt; GetCursorPos(&pt);
		HWND owner = adsw_acadMainWnd();
		SetForegroundWindow(owner);
		const int cmd = static_cast<int>(TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x + 8, pt.y + 8, 0, owner, NULL));
		PostMessage(owner, WM_NULL, 0, 0);
		DestroyMenu(menu);   // destroys the submenus too
		return cmd;
	}
	// Connector grip on a free fitting port: options before the first duct
	//   Dimenzija - another size / shape: a prelaz right at the port (spiro by catalogue), then that size;
	//   Koljeno   - 30 / 45 / 60 / 90 deg left / right in plan at the port, the duct leaves the elbow.
	// The first duct leaves along the (last) direction; a point to the side is projected onto it.
	static void drawFromPort(const AcDbObjectId& id, int k)
	{
		tvsTrace::Scope trace_("drawFromPort");
		const int choice = portGripMenu();
		if (choice == 0) { acutPrintf(_T("\nOdustano.")); return; }
		tvsPortGrip::Port P[4]; int n = 0; double elev = 0; AcDbDatabase* db = NULL;
		{
			AcDbEntity* e = NULL;
			if (acdbOpenAcDbEntity(e, id, AcDb::kForRead) != Acad::eOk) return;
			TVS_Entity* t = TVS_Entity::cast(e);
			if (t) { n = tvsPortGrip::ports(t, P); elev = t->get_Elevation(); db = t->database(); InstallGlobalProperty(t); }
			e->close();
		}
		if (k < 0 || k >= n || !db) return;
		tvsTeePort::Section sec; sec.a = P[k].a; sec.b = P[k].b; sec.round = P[k].round;
		drawFromEnd(id, P[k].p, P[k].dir, sec, elev, db, choice);
	}
	// Grip menu on a duct end (user 2026-11-26): Nastavi crtanje = the usual continuation (drawStep3);
	// another size / an elbow start right at the duct end, as on a fitting port. 0: cancelled,
	// 1: continue as before (the caller), 2: done here.
	static int ductEndMenu(const AcDbObjectId& id, bool atLast)
	{
		bool round = false;
		{ AcDbObjectPointer<TVS_Pipe> p(id, AcDb::kForRead); if (p.openStatus() == Acad::eOk) round = p->ThisRound || p->SizeB <= 0; }
		const int choice = portGripMenu(true, round);
		if (choice == 0) { acutPrintf(_T("\nOdustano.")); return 0; }
		if (choice == 1) return 1;
		static const wchar_t* step[] = { NULL, NULL, NULL, L"Koljeno", L"Račva", L"Z", L"Gore", L"Nadolje", L"Obilazak" };
		if (choice >= 3 && choice <= 8) { pendingStep3() = step[choice]; return 1; }   // as typed at the drawing prompt
		if (choice >= 21 && choice <= 25) {
			static const wchar_t* type[] = { L"Ogranak", L"Gacasti", L"Stucna", L"Kosa", L"Y45" };
			pendingStep3() = L"Tkomad"; pendingTee() = type[choice - 21]; return 1; }
		AcGePoint3d at; AcGeVector3d dir; tvsTeePort::Section sec; double elev = 0; AcDbDatabase* db = NULL;
		{
			AcDbObjectPointer<TVS_Pipe> p(id, AcDb::kForRead);
			if (p.openStatus() != Acad::eOk) return 0;
			if (p->This1D || p->DuctType == DuctTypeFlex) { acutPrintf(_T("\n1D / flex kanal: nastavlja se obično crtanje.")); return 1; }
			at = atLast ? p->LastPoint : p->FirstPoint; dir = atLast ? p->LastPoint - p->FirstPoint : p->FirstPoint - p->LastPoint; dir.z = 0;
			if (dir.length() < 1e-9) return 1;
			dir.normalize();
			sec.a = p->SizeA; sec.round = p->ThisRound || p->SizeB <= 0; sec.b = sec.round ? 0 : p->SizeB;
			elev = p->get_Elevation(); db = p->database(); InstallGlobalProperty(p.object());
		}
		if (!db) return 0;
		drawFromEnd(id, at, dir, sec, elev, db, choice);
		return 2;
	}
	static void drawFromEnd(const AcDbObjectId& id, AcGePoint3d at, AcGeVector3d dir, tvsTeePort::Section sec, double elev, AcDbDatabase* db, int choice)
	{
		const double f = tvsUnits::factor(db);
		globSizeA = sec.a; globSizeB = sec.round ? 0 : sec.b; globRound = sec.round; globalElevMid = elev; globalElev = elev;
		ads_point pick;
		AcGePoint3d end;
		auto dimension = [&]() {   // prelaz right at the port, then the new size
			if (!changesize()) return;
			globalElevMid = elev; globalElev = elev;   // the plan prelaz keeps the elevation of the fitting
			tvsTeePort::Section to; to.a = globSizeA; to.round = globRound; to.b = globRound ? 0 : globSizeB;
			if (tvsTeePort::same(sec, to, 1e-9)) return;
			const double L = tvsTeePort::prelazLength(db, sec, to, f);
			if (!tvsTeeDialog::prelaz(db, id, at, dir, sec, to, L, elev)) { acutPrintf(_T("\nPrelaz nije napravljen.")); return; }
			wchar_t a1[32], a2[32]; tvsTeePort::sizeText(a1, 32, sec, f); tvsTeePort::sizeText(a2, 32, to, f);
			acutPrintf(_T("\nPrelaz %ls -> %ls mm, dužine %.0f mm."), a1, a2, L * f);
			at += dir * L; sec = to;
		};
		auto elbow = [&](double deg, bool left) {
			const double ang = deg * 3.14159265358979323846 / 180;
			AcGePoint3d exit; AcGeVector3d out;
			if (!tvsTeeDialog::planElbow(id, at, dir, left ? ang : -ang, sec, elev, f, exit, out)) { acutPrintf(_T("\nKoljeno nije napravljeno.")); return; }
			acutPrintf(_T("\nKoljeno %.0f° %ls."), deg, left ? _T("lijevo") : _T("desno"));
			at = exit; dir = out;
		};
		if (choice == 2) dimension();
		else if (choice >= 100) elbow(choice % 1000 - 100, choice >= 1000);
		for (;;)
		{
			ads_point base = { at.x, at.y, at.z };
			acedInitGet(0, _T("Dimenzija D Koljeno K"));
			const int st = tvsCoordinates::getPoint(base, _T("\nKraj prvog kanala (u pravcu priključka) ili [Dimenzija/Koljeno]: "), pick);
			if (st == RTKWORD)
			{
				ACHAR kw[64] = _T(""); acedGetInput(kw);
				if (_wcsicmp(kw, _T("Dimenzija")) == 0 || _wcsicmp(kw, _T("D")) == 0) dimension();
				else if (_wcsicmp(kw, _T("Koljeno")) == 0 || _wcsicmp(kw, _T("K")) == 0)
				{
					ACHAR ak[16] = _T(""); acedInitGet(0, _T("30 45 60 90"));
					const int sa = acedGetKword(_T("\nUgao koljena [30/45/60/90] <90>: "), ak, 16);
					if (sa != RTNORM && sa != RTNONE) continue;
					const double deg = sa == RTNORM ? _wtof(ak) : 90;
					ACHAR sk[16] = _T(""); acedInitGet(0, _T("Lijevo Desno"));
					const int ss = acedGetKword(_T("\nSmjer [Lijevo/Desno] <Desno>: "), sk, 16);
					if (ss != RTNORM && ss != RTNONE) continue;
					elbow(deg, ss == RTNORM && _wcsicmp(sk, _T("Lijevo")) == 0);
				}
				continue;
			}
			if (st != RTNORM) return;
			AcGeVector3d v = asPnt3d(pick) - at; v.z = 0;
			const double t = v.dotProduct(dir);
			if (t > 1e-6) { end = at + dir * t; break; }
			acutPrintf(_T("\nTačka mora biti ispred priključka (u smjeru nastavka)."));
		}
		globSizeA = sec.a; globSizeB = sec.round ? 0 : sec.b; globRound = sec.round;
		AcGePoint3d A1 = at, A2 = end;
		TVS_Pipe pipie;
		TVS_Pipe* pipi = pipie.add_new(A1, A2, globSizeA, globSizeB, false, globRound);
		AcDbEntity* pEnt = NULL;
		if (!pipi || acdbOpenAcDbEntity(pEnt, pipi->id(), AcDb::kForWrite) != eOk) { acutPrintf(_T("\nSloj je zaključan")); return; }
		pipi->close();
		ads_point pt1 = { A1.x, A1.y, A1.z }, pt2 = { A2.x, A2.y, A2.z };
		drawStep3(pt1, pt2, pipi);
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
			case RTNORM:
				break;
			default:   // RTERROR / RTNONE ...: no point was given
				return false;
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
			case RTNORM:
				break;
			default:   // RTERROR / RTNONE ...: no point was given
				return false;
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

	// An option chosen in the grip menu: answers the first drawing prompt of drawStep3 once.
	static std::wstring& pendingStep3() { static std::wstring s; return s; }
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


			bool fromMenu = false;
			if (!pendingStep3().empty()) { wcscpy_s(resultss, pendingStep3().c_str()); pendingStep3().clear(); Astat = RTKWORD; fromMenu = true; }   // grip menu
			else {
			acedInitGet(RSG_NONULL, _T("Dimenzija D Koljeno K Račva R Tkomad T Z Gore G Nadolje N Obilazak O"));
			Astat = tvsCoordinates::getPoint(pt1, _T("\nZadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Tkomad/Z/Gore/Nadolje/Obilazak]: "), pt2);
			}
			switch (Astat)
			{
			case RTCAN:
				return false;
				break;
			case RTKWORD:
				if (!fromMenu) acedGetInput(resultss);
				break;
			case RTNORM:
				A3 = asPnt3d(pt2);
				nextpipe(A1, A2, A3, pipi, pt1);
				break;
			default:   // RTERROR ...
				return false;
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
