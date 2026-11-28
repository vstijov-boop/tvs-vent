	// K4: both ducts are chosen by id; the old loops compared pointers of closed objects and
	// passed pointers closed in the loop. The ducts must be writable (locked layer -> stop).
	// tee (TVS_CONNECTW, plan 26): the second pick may be a T komad (krstasti komad / XK) - *tee = true.
	static bool selectTwoPipes(AcDbObjectId& id1, AcDbObjectId& id2, ads_point pt1, ads_point pt2, bool* tee = NULL)
	{
		if (tee) *tee = false;
		const ACHAR* prompts[2] = { _T("\nIzaberite prvi kanal: "), tee ? _T("\nIzaberite drugi kanal (ili T komad – krstasti komad / XK): ") : _T("\nIzaberite drugi kanal: ") };
		AcDbObjectId* ids[2] = { &id1, &id2 };
		double* pts[2] = { pt1, pt2 };
		for (int k = 0; k < 2; ++k)
		{
			for (;;)
			{
				ads_name e;
				if (tvsCoordinates::selectEntity(prompts[k], e, pts[k]) == RTCAN) return false;
				AcDbObjectId id;
				if (acdbGetObjectId(id, e) != Acad::eOk || id.isNull()) continue;
				AcDbObjectPointer<TVS_Pipe> p(id, AcDb::kForWrite);
				if (p.openStatus() == Acad::eNotThatKindOfClass && k == 1 && tee && id.objectClass() == TVS_WYE::desc()) { id2 = id; *tee = true; break; }
				if (p.openStatus() == Acad::eNotThatKindOfClass) continue;
				if (p.openStatus() != Acad::eOk) { acutPrintf(_T("\nObjekat je zaključan")); return false; }
				if (k == 1 && id == id1) { acutPrintf(_T("\nIzabran je isti kanal")); continue; }
				*ids[k] = id;
				break;
			}
		}
		return true;
	}
	// The connect helpers take the entities (they re-open them for write themselves).
	static bool entitiesOf(const AcDbObjectId& id1, const AcDbObjectId& id2, AcDbEntity*& e1, AcDbEntity*& e2)
	{
		if (acdbOpenAcDbEntity(e1, id1, AcDb::kForRead) != Acad::eOk) return false;
		e1->close();
		if (acdbOpenAcDbEntity(e2, id2, AcDb::kForRead) != Acad::eOk) return false;
		e2->close();
		return true;
	}
	// - Ventilation_ARX.TVS_CONNECT command (do not rename)
	static void Ventilation_ARXTVS_CONNECTW(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		if (!currentLayerWritable()) return;
		ads_point pt1, pt2;
		AcDbObjectId id1, id2;
		AcDbEntity *pEnt1 = NULL, *pEnt2 = NULL;
		bool tee = false;
		if (!selectTwoPipes(id1, id2, pt1, pt2, &tee)) return;
		if (tee) { tvsTeeCrossCmd::connect(id1, id2); return; }   // krstasti komad / XK (plan 26)
		if (!entitiesOf(id1, id2, pEnt1, pEnt2)) return;
		ConnectWithWye(pEnt1, pEnt2, pt1, pt2);
	}

	static void Ventilation_ARXTVS_CONNECTT(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		if (!currentLayerWritable()) return;
		ads_point pt1, pt2;
		AcDbObjectId id1, id2;
		AcDbEntity *pEnt1 = NULL, *pEnt2 = NULL;
		if (!selectTwoPipes(id1, id2, pt1, pt2) || !entitiesOf(id1, id2, pEnt1, pEnt2)) return;
		ConnectWithTap(pEnt1, pEnt2, pt1, pt2);
	}


	static void Ventilation_ARXTVS_CONNECTTest(void)
	{
		if (ActivationErrorMessage() != pError_Ok) return;
		syncTVSGlobalUnits();
		if (!currentLayerWritable()) return;
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

			if (tvsCoordinates::selectEntity(_T("\nIzaberite prvi kanal: "), vozd1, pt1) == RTCAN)
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
			if (tvsCoordinates::selectEntity(_T("\nIzaberite drugi kanal: "), vozd2, pt2) == RTCAN)
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

				if (acdbOpenAcDbEntity(pEnt1, id1, AcDb::kForWrite) != eOk) return;
		if (acdbOpenAcDbEntity(pEnt2, id2, AcDb::kForWrite) != eOk) { pEnt1->close(); return; }
		Pipi1 = TVS_Pipe::cast(pEnt1); Pipi2 = TVS_Pipe::cast(pEnt2);
		if (!Pipi1 || !Pipi2) { pEnt1->close(); pEnt2->close(); return; }


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

