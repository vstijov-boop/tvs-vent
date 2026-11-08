	static bool addTrans(TVS_Pipe *pPipe1,
		TVS_Pipe  *pPipe2, AcGePoint3d &point1, AcGePoint3d &midp, AcGePoint3d &point2)
	{
		if (length2p(pPipe1->FirstPoint, point1) > length2p(pPipe1->LastPoint, point1))
		{
			AcDbEntity* pEnt;
			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_Lastpoint(pPipe1->FirstPoint);
				pPipe1->put_FirstPoint(point1);
				pPipe1->close();

			}
		}

		if (length2p(pPipe2->FirstPoint, point2) < length2p(pPipe2->LastPoint, point2))
		{
			AcDbEntity* pEnt;
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {
				pPipe2->put_FirstPoint(pPipe2->LastPoint);
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();
			}
		}


		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcDbEntity* pEnt;
		if (GroseStatus == TVSEnt1isGrose)
		{

			TVS_TRANS * pTrans;
			TVS_Pipe* GPipe = pPipe1;
			TVS_Pipe* SPipe = pPipe2;


			double spiroLt = tvsSpiro::drawingLength(NULL, GPipe->SizeA, GPipe->ThisRound, SPipe->SizeA, SPipe->ThisRound, globalLengthTr, GPipe->getDuctType() == DuctTypeFlex);   // spiro: po katalogu SOKO
			pTrans = pTrans->add_new(GPipe->SizeA,
				GPipe->SizeB,
				SPipe->SizeA,
				SPipe->SizeB,
				spiroLt,
				AcGeVector3d(-GPipe->FirstPoint.x + GPipe->LastPoint.x, -GPipe->FirstPoint.y + GPipe->LastPoint.y, -GPipe->FirstPoint.z + GPipe->LastPoint.z).normalize(),
				GPipe->FirstPoint,
				GPipe->ThisRound,
				SPipe->ThisRound,
				globalLengthTr,
				GPipe->This1D
			);
			if (acdbOpenAcDbEntity(pEnt, pTrans->id(), AcDb::kForWrite) == eOk) {
				pTrans->put_Length(spiroLt);
				pTrans->close();
			}
			SetGlobalProperty(pTrans);
			SetPropertyLikePipe(SPipe, pTrans);
			SetPropertyLikePipe(SPipe, GPipe);
			AcGePoint3d lastPoint = GPipe->LastPoint;

			point1 = shortlength(midp, point1, spiroLt);
			return true;
		}
		if (GroseStatus == TVSEnt2isGrose)
		{

			TVS_TRANS * pTrans;
			TVS_Pipe* GPipe = pPipe2;
			TVS_Pipe* SPipe = pPipe1;


			double spiroLt = tvsSpiro::drawingLength(NULL, GPipe->SizeA, GPipe->ThisRound, SPipe->SizeA, SPipe->ThisRound, globalLengthTr, GPipe->getDuctType() == DuctTypeFlex);   // spiro: po katalogu SOKO
			pTrans = pTrans->add_new(GPipe->SizeA,
				GPipe->SizeB,
				SPipe->SizeA,
				SPipe->SizeB,
				spiroLt,
				AcGeVector3d(GPipe->FirstPoint.x - GPipe->LastPoint.x, GPipe->FirstPoint.y - GPipe->LastPoint.y, GPipe->FirstPoint.z - GPipe->LastPoint.z).normalize(),
				GPipe->LastPoint,
				GPipe->ThisRound,
				SPipe->ThisRound,
				globalLengthTr,
				GPipe->This1D
			);
			if (acdbOpenAcDbEntity(pEnt, pTrans->id(), AcDb::kForWrite) == eOk) {
				pTrans->put_Length(spiroLt);
				pTrans->close();
			}
			SetGlobalProperty(pTrans);
			SetPropertyLikePipe(SPipe, pTrans);
			SetPropertyLikePipe(SPipe, GPipe);

			point2 = shortlength(midp, point2, spiroLt);
			return true;

		}

		return false;
	}



	static bool ConnectWithTap(AcDbEntity *pEnt1,
		AcDbEntity  *pEnt2, ads_point pnt1, ads_point pnt2)

	{
		//Проверка
		if (TVSClassCheck(pEnt1) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}

		if (TVSClassCheck(pEnt2) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}


		TVS_Pipe* pPipe1 = TVS_Pipe::cast(pEnt1);
		TVS_Pipe* pPipe2 = TVS_Pipe::cast(pEnt2);
		AcGePoint3d p1 = pPipe1->get_FirstPoint();
		AcGePoint3d p2 = pPipe1->get_Lastpoint();
		AcGePoint3d p3 = pPipe2->get_FirstPoint();
		AcGePoint3d p4 = pPipe2->get_Lastpoint();

		AcGeLine3d line1 = AcGeLine3d(p1, p2);
		AcGeLine3d  line2 = AcGeLine3d(p3, p4);
		AcGeTol dop;
		dop.setEqualPoint(0.01 / tvsUnits::factor());
		int LineStatus = TVSisIntersection;
		int SizeStatus;
		if (line1.isParallelTo(line2, dop)) { LineStatus = TVSisParallel; acutPrintf(_T("\n Paralelni")); }
		if (line1.isColinearTo(line2, dop)) { LineStatus = TVSisLinear; acutPrintf(_T("\n Kolinearni")); }
		if (line1.isPerpendicularTo(line2, dop)) { LineStatus = TVSisPerpendicular; acutPrintf(_T("\n Upravni")); }
		if ((pPipe1->SizeA == pPipe2->SizeA) && (pPipe1->SizeA == pPipe2->SizeA)) SizeStatus = TVSisSameSize;
		else SizeStatus = TVSisNoSameSize;
		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcGePoint3d pointSelect1 = line1.closestPointTo(asPnt3d(pnt1));
		AcGePoint3d pointSelect2 = line2.closestPointTo(asPnt3d(pnt2));

#pragma region  line intersection
		if (LineStatus != TVSisParallel&&LineStatus != TVSisLinear)
		{
			AcGePoint3d midp, point1, point2;

			line1.intersectWith(line2, midp);
			//find point1
			if (midp.distanceTo(p1) <= p1.distanceTo(p2) && midp.distanceTo(p2) <= p1.distanceTo(p2))// midp check between p3 p4
			{
				if (midp.distanceTo(p1) <= pointSelect1.distanceTo(p1)) point1 = p2;
				else point1 = p1;
			}
			else
			{
				if (midp.distanceTo(p1) >= midp.distanceTo(p2)) point1 = p1;
				else point1 = p2;
			}//end findpoint1

			//find point2
			if (midp.distanceTo(p3) <= p3.distanceTo(p4) && midp.distanceTo(p4) <= p3.distanceTo(p4)) // midp check between p3 p4
			{
				if (midp.distanceTo(p3) <= pointSelect2.distanceTo(p3)) point2 = p4;
				else point2 = p3;
			}
			else
			{
				if (midp.distanceTo(p3) >= midp.distanceTo(p4)) point2 = p3;
				else point2 = p4;
			}//end findpoint2
			// Validate before changing either duct: zero-length or collinear legs cannot form an elbow.
            double elbowAngle = 0.0;
            AcGeVector3d elbowStart;
            if (!GiveStartvectorAndAngle(point1, midp, point2, elbowAngle, elbowStart)) {
                acutPrintf(L"\nNije moguće napraviti koljeno: provjerite smjerove i krajnje tačke kanala.");
                return false;
            }
			///Tap with point1 midp point2;

			AcDbEntity * pEnt;

			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_FirstPoint(point1);
				pPipe1->put_Lastpoint(midp);
				pPipe1->close();

			}
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {


				pPipe2->put_FirstPoint(midp);
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();
			}
#pragma region		//insert trans
			if (GroseStatus != TVSEntitiesisSame)
			{

				addTrans(pPipe1, pPipe2, point1, midp, point2);


			}
#pragma endregion
			TVS_TAP* pTap = drawTapDirect(point1, midp, point2);
			if (!pTap) return false;
			SetGlobalProperty(pTap);
			SetPropertyLikePipe(pPipe1, pTap);

			if (acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite) == eOk) {
				pPipe1->put_FirstPoint(point1);
				pPipe1->put_Lastpoint(shortlength(point1, midp, length2p(pTap->MA, pTap->MiddlePoint)));
				pPipe1->close();

			}
			if (acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite) == eOk) {


				pPipe2->put_FirstPoint(shortlength(point2, midp, length2p(pTap->MA, pTap->MiddlePoint)));
				pPipe2->put_Lastpoint(point2);
				pPipe2->close();

			}




		} //end if line intersection
#pragma endregion 

#pragma region linear
		if (LineStatus == TVSisLinear)
		{
			AcGePoint3d kr1;
			AcGePoint3d kr2;
			AcGePoint3d midp;
			if (max(length2p(p1, p3), length2p(p1, p4)) > max(length2p(p2, p3), length2p(p2, p4)))
			{
				kr1 = p1; midp = p2;
			}
			else { kr1 = p2; midp = p1; }

			if (length2p(p1, p4) > length2p(p1, p3)) kr2 = p4;
			else kr2 = p3;

			AcDbEntity * pEnt;

			//put trans

			addTrans(pPipe1, pPipe2, kr1, midp, kr2);

			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();
			pPipe1->put_FirstPoint(kr1);
			pPipe1->put_Lastpoint(kr2);
			pPipe1->close();

			ads_name eName;
			acdbGetAdsName(eName, pPipe2->id());
			acdbEntDel(eName);

		}
#pragma endregion 
		if (LineStatus == TVSisParallel) return false;
		return true;
	}


	static bool ConnectWithWye(AcDbEntity *pEnt1,
		AcDbEntity  *pEnt2, ads_point pnt1, ads_point pnt2)

	{
		//Проверка
		if (TVSClassCheck(pEnt1) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}

		if (TVSClassCheck(pEnt2) != isTVS_Pipe)
		{
			acutPrintf(_T("\n Izabran je neodgovarajući objekat"));
			return false;
		}


		TVS_Pipe* pPipe1 = TVS_Pipe::cast(pEnt1);
		TVS_Pipe* pPipe2 = TVS_Pipe::cast(pEnt2);
		AcGePoint3d p1 = pPipe1->get_FirstPoint();
		AcGePoint3d p2 = pPipe1->get_Lastpoint();
		AcGePoint3d p3 = pPipe2->get_FirstPoint();
		AcGePoint3d p4 = pPipe2->get_Lastpoint();

		AcGeLine3d line1 = AcGeLine3d(p1, p2);
		AcGeLine3d  line2 = AcGeLine3d(p3, p4);
		AcGeTol dop;
		dop.setEqualPoint(0.01 / tvsUnits::factor());
		int LineStatus = TVSisIntersection;
		int SizeStatus;
		if (line1.isParallelTo(line2, dop)) { LineStatus = TVSisParallel; acutPrintf(_T("\n Paralelni")); }
		if (line1.isColinearTo(line2, dop)) { LineStatus = TVSisLinear; acutPrintf(_T("\n Kolinearni")); }
		if (line1.isPerpendicularTo(line2, dop)) { LineStatus = TVSisPerpendicular; acutPrintf(_T("\n Upravni")); }
		if ((pPipe1->SizeA == pPipe2->SizeA) && (pPipe1->SizeA == pPipe2->SizeA)) SizeStatus = TVSisSameSize;
		else SizeStatus = TVSisNoSameSize;
		int GroseStatus = whyIsGrose(pPipe1, pPipe2);
		AcGePoint3d pointSelect1 = line1.closestPointTo(asPnt3d(pnt1));
		AcGePoint3d pointSelect2 = line2.closestPointTo(asPnt3d(pnt2));
		AcDbEntity * pEnt;
		AcDbObjectId wyeId;
#pragma region  line intersection
		if (LineStatus == TVSisPerpendicular || LineStatus == TVSisIntersection)
		{
			int teeType = tvsTee::Straight; double teeSecond = 0;
			const AcGeVector3d mainFlow = p4 - p3;   // main duct as drawn = air flow (stucna entry upstream)
			// rectangular main: TK-O / TK-G / stucna for a rectangular branch, stucna for a round one
			if (!pPipe2->ThisRound && pPipe2->SizeB > 0 && (pPipe1->ThisRound || pPipe1->SizeB > 0)
				&& !tvsTeeCommand::askType(teeType, teeSecond, pPipe2->SizeA, pPipe1->SizeA, tvsUnits::factor(), pPipe1->ThisRound))
				return false;
			// stucna with a round branch: the branch axis must be at the axis elevation of the main (Elev = axis)
			if (teeType == tvsTee::Tap && pPipe1->ThisRound && !pPipe2->ThisRound
				&& std::fabs(pPipe1->Elev - pPipe2->Elev) * tvsUnits::factor() > 0.1)
			{
				const double f = tvsUnits::factor();
				wchar_t text[400];
				swprintf_s(text, L"Okrugli ogranak nije na koti ose pravougaonog kanala.\n\n"
					L"Osa pravougaonog kanala: %.0f mm\nOsa okruglog kanala: %.0f mm\n\n"
					L"Postavite okrugli kanal na kotu ose pravougaonog kanala (Elev). Spoj nije napravljen.", pPipe2->Elev * f, pPipe1->Elev * f);
				acedAlert(text);
				return false;
			}

			AcGePoint3d midp, point1, point2, intersectpoint;

			line1.intersectWith(line2, midp);
			//find point1
			if (max(midp.distanceTo(p1), midp.distanceTo(p2)) < p1.distanceTo(p2))
			{
				if (p2.distanceTo(pointSelect1) < p2.distanceTo(midp))
				{
					AcGePoint3d temp;
					temp = p2;
					p2 = p1;
					p1 = temp;
					acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
					pPipe1->assertWriteEnabled();
					pPipe1->put_FirstPoint(p1);
					pPipe1->put_Lastpoint(p2);
					pPipe1->close();
				}

			}
			else
			{
				if (p2.distanceTo(midp) > p1.distanceTo(midp))
				{
					AcGePoint3d temp;
					temp = p2;
					p2 = p1;
					p1 = temp;
					acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
					pPipe1->assertWriteEnabled();
					pPipe1->put_FirstPoint(p1);
					pPipe1->put_Lastpoint(p2);
					pPipe1->close();
				}
			}

			intersectpoint = line2.closestPointTo(p2);

			//made perpendicular

			if (LineStatus == TVSisIntersection)
			{
				TVS_Pipe* tempPipe;
				tempPipe = tempPipe->add_new(p2, intersectpoint, globSizeA, globSizeB);
				SetPropertyLikePipe(pPipe1, tempPipe);
				ads_point pt1;
				ads_point pt2;





				ConnectWithTap(pPipe1, tempPipe, asDblArray(p1), asDblArray(intersectpoint));
				pPipe1 = tempPipe;
				p1 = p2;
				p2 = intersectpoint;

			}



			if (length2p(p3, intersectpoint) < length2p(p4, intersectpoint))
			{
				AcGePoint3d temp;
				temp = p4;
				p4 = p3;
				p3 = temp;
			}




			acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite);
			pPipe2->assertWriteEnabled();
			pPipe2->put_FirstPoint(p3);
			pPipe2->put_Lastpoint(intersectpoint);
			pPipe2->close();

			TVS_WYE* pWye;
			pWye = pWye->add_new(pPipe2->SizeA,
				pPipe2->SizeB,
				pPipe1->SizeA,
				pPipe1->SizeB,
				globalLengthW,
				AcGeVector3d(-pPipe2->FirstPoint.x + pPipe2->LastPoint.x, -pPipe2->FirstPoint.y + pPipe2->LastPoint.y, -pPipe2->FirstPoint.z + pPipe2->LastPoint.z).normalize(),
				AcGeVector3d(p1.x - p2.x, p1.y - p2.y, p1.z - p2.z).normalize(),
				intersectpoint,
				pPipe2->ThisRound,
				pPipe1->ThisRound,
				pPipe2->This1D);
			SetGlobalProperty(pWye);
			SetPropertyLikePipe(pPipe2, pWye);
			wyeId = pWye->id();


			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();

			pPipe1->put_Lastpoint(shortlength(p1, intersectpoint, pPipe2->SizeA / 2 + pWye->LengthPl));
			pPipe1->close();
			acdbOpenAcDbEntity(pEnt, pPipe2->id(), AcDb::kForWrite);
			pPipe2->assertWriteEnabled();
			pPipe2->put_Lastpoint(shortlength(p3, intersectpoint, pPipe1->SizeA / 2 + pWye->LengthPl));
			pPipe2->close();
			//if intersection between p3 and p4
			if (max(length2p(p3, intersectpoint), length2p(p4, intersectpoint)) < length2p(p3, p4))
			{
				TVS_Pipe* tempPipe;
				tempPipe = tempPipe->add_new(intersectpoint, p4, globSizeA, globSizeB);
				SetPropertyLikePipe(pPipe2, tempPipe);
				acdbOpenAcDbEntity(pEnt, tempPipe->id(), AcDb::kForWrite);
				tempPipe->assertWriteEnabled();
				tempPipe->put_FirstPoint(shortlength(p4, intersectpoint, pPipe1->SizeA / 2 + pWye->LengthPl));
				tempPipe->close();
			}



			tvsTeeCommand::applyType(wyeId, teeType, teeSecond, mainFlow);   // faza 2: created directly as TK-O / TK-G / stucna
		} //end if line intersection
#pragma endregion 

#pragma region linear
		if (LineStatus == TVSisLinear)
		{
			AcGePoint3d kr1;
			AcGePoint3d kr2;
			AcGePoint3d midp;
			if (max(length2p(p1, p3), length2p(p1, p4)) > max(length2p(p2, p3), length2p(p2, p4)))
			{
				kr1 = p1; midp = p2;
			}
			else { kr1 = p2; midp = p1; }

			if (length2p(p1, p4) > length2p(p1, p3)) kr2 = p4;
			else kr2 = p3;

			AcDbEntity * pEnt;

			//put trans

			addTrans(pPipe1, pPipe2, kr1, midp, kr2);

			acdbOpenAcDbEntity(pEnt, pPipe1->id(), AcDb::kForWrite);
			pPipe1->assertWriteEnabled();
			pPipe1->put_FirstPoint(kr1);
			pPipe1->put_Lastpoint(kr2);
			pPipe1->close();

			ads_name eName;
			acdbGetAdsName(eName, pPipe2->id());
			acdbEntDel(eName);

		}
#pragma endregion 
		if (LineStatus == TVSisParallel) return false;
		return true;
	}


	// 
	// 	static bool ConnectPutTrans(AcDbEntity *pEnt1, AcDbEntity *pEnt2, int pointStatus)
	// 
	// 	{
	// 		//Проверка
	// 		if (TVSClassCheck(pEnt1)!=isTVS_Pipe)
	// 		{
	// 			acutPrintf(_T("\n Выбран не тот обьект"));
	// 			return false;
	// 		}
	// 
	// 		if (TVSClassCheck(pEnt2)!=isTVS_Pipe)
	// 		{
	// 			acutPrintf(_T("\n Выбран не тот обьект"));
	// 			return false;
	// 		}
	// 
	// 
	// 		TVS_Pipe* pPipe1=TVS_Pipe::cast(pEnt1);
	// 		TVS_Pipe* pPipe2=TVS_Pipe::cast(pEnt2);
	// 		AcGePoint3d p1 = pPipe1->get_FirstPoint();
	// 		AcGePoint3d p2 = pPipe1->get_Lastpoint();
	// 		AcGePoint3d p3 = pPipe2->get_FirstPoint();
	// 		AcGePoint3d p4 = pPipe2->get_Lastpoint();
	// 
	// 		AcGeLine3d line1=AcGeLine3d (p1,p2);
	// 		AcGeLine3d  line2=AcGeLine3d (p3,p4);
	// 		AcGeTol dop;
	// 		dop.setEqualPoint(0.01);
	// 		int LineStatus=TVSisIntersection;
	// 		int SizeStatus;
	// 		if(line1.isParallelTo(line2,dop)) {LineStatus=TVSisParallel; acutPrintf(_T("\n Параллельны"));} 
	// 		if(line1.isColinearTo(line2,dop)) {LineStatus=TVSisLinear; acutPrintf(_T("\n Линейны"));} 
	// 		if(line1.isPerpendicularTo(line2,dop)) {LineStatus=TVSisPerpendicular; acutPrintf(_T("\n Перпендикулярны"));} 
	// 		if((pPipe1->SizeA==pPipe2->SizeA)&&(pPipe1->SizeA==pPipe2->SizeA)) SizeStatus=TVSisSameSize;
	// 		else SizeStatus=TVSisNoSameSize;
	// 		int GroseStatus=whyIsGrose(pPipe1,pPipe2);
	// 
	// 
	// #pragma region //if pipes various
	// 		
	// 			if (GroseStatus=TVSisNoSameSize)
	// 			{
	// 
	// 
	// 				if (pointStatus==TVSFirstPoint)
	// 				{
	// 
	// 					p1=
	// 				}
	// 
	// 
	// 
	// 				TVS_TAP* pTap=drawTapDirect(point1,midp,point2);
	// 				SetGlobalProperty(pTap);
	// 				SetPropertyLikePipe(pPipe1,pTap);
	// 				AcDbEntity * pEnt;
	// 				if (acdbOpenAcDbEntity(pEnt,pPipe1->id(),AcDb::kForWrite)==eOk){
	// 					pPipe1->put_FirstPoint(point1);
	// 					pPipe1->put_Lastpoint(shortlength(point1, midp,length2p(pTap->MA,pTap->MiddlePoint)));
	// 					pPipe1->close();
	// 
	// 				}
	// 				if (acdbOpenAcDbEntity(pEnt,pPipe2->id(),AcDb::kForWrite)==eOk){
	// 
	// 
	// 					pPipe2->put_FirstPoint(shortlength(point2, midp,length2p(pTap->MA,pTap->MiddlePoint)));
	// 					pPipe2->put_Lastpoint(point2);
	// 					pPipe2->close();
	// 				
	// 
	// 			}
	// 		} //end if line intersection
	// #pragma endregion
	// 
	// 	}
	// 

	static void SetPropertyLikePipe(TVS_Pipe *pPipe, TVS_Entity* pEnt)
	{
		AcDbEntity * pAEnt;
		if (acdbOpenAcDbEntity(pAEnt, pEnt->id(), AcDb::kForWrite) == eOk) {
			pEnt->put_SizeA(pPipe->get_SizeA());
			pEnt->put_SizeB(pPipe->get_SizeB());
			pEnt->put_Wipeout(pPipe->get_Wipeout());
			pEnt->put_Grani(pPipe->get_Grani());
			pEnt->put_This1D(pPipe->get_This1D());
			pEnt->put_Flow(pPipe->get_Flow());
			pEnt->put_Elevation(pPipe->get_Elevation());
			pEnt->setDuctType(pPipe->getDuctType());
			pEnt->setColor(pPipe->color());
			pEnt->setLineWeight(pPipe->lineWeight());
			pEnt->setLayer(pPipe->layer());
			pEnt->setLinetype(pPipe->linetype());
			pEnt->setLinetypeScale(pPipe->linetypeScale());
			pEnt->draw();
			pEnt->close();
		}
		else
		{
			acutPrintf(_T("\n Objekat je zaključan: svojstva se ne mogu kopirati"));
		}
	}
	static void InstallGlobalProperty(TVS_Entity *pEnt)
	{
		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;
		AcDbObjectId	id = pEnt->id();
		ads_name eName;


		acdbGetAdsName(eName, id);
		acdbGetObjectId(id, eName);
		AcDbEntity* pnt;


		//acdbOpenAcDbEntity(pnt,id,AcDb::kForRead);




		globSizeA = pEnt->get_SizeA();
		globSizeB = pEnt->get_SizeB();
		globalWipeout = pEnt->get_Wipeout();
		globalGrani = pEnt->get_Grani();
		global1D = pEnt->get_This1D();
		globalFlow = pEnt->get_Flow();
		globalDuctType = pEnt->getDuctType();
		globalElevMid = pEnt->get_Elevation();


		//	pEnt->close();
	}
	static void SetGlobalProperty(TVS_Entity *pEnt)
	{

		TVS_Pipe * Pipi;
		TVS_TAP * Tapie;
		TVS_WYE* Wyeie;
		TVS_TRANS * Transie;
		AcDbObjectId	id = pEnt->id();
		ads_name eName;


		// 	acdbGetAdsName(eName,id);
		// 	acdbGetObjectId(id,eName);
		AcDbEntity* pnt;


		if (acdbOpenAcDbEntity(pnt, pEnt->id(), AcDb::kForWrite) == eOk)


		{



			pEnt->put_SizeA(globSizeA);
			pEnt->put_SizeB(globSizeB);
			pEnt->put_Wipeout(globalWipeout);
			pEnt->put_WipeoutLength(globalWipeoutLength);
			pEnt->put_Grani(globalGrani);
			pEnt->put_This1D(global1D);
			pEnt->put_Flow(globalFlow);
			pEnt->put_Elevation(globalElevMid);
			pEnt->setDuctType(globalDuctType);


			if ((Pipi = TVS_Pipe::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Pipi->put_SizeA(SizeA);
				// 			if (cSizeB==false) Pipi->put_SizeB(SizeB);
				// 			if (cFlow==false) Pipi->put_Flow(Flow);
				// 			if (cGrani==false) Pipi->put_Grani(Grani);
				// 			if (cD1==false) Pipi->put_This1D(D1);
				// 			if (cWipe==false) Pipi->put_Wipeout(Wipe);
				// 			if (cElev==false) Pipi->put_Elevation(getElev(Elev,Emode,Pipi->SizeA,Pipi->SizeB));
				//


			}

			if ((Tapie = TVS_TAP::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Tapie->put_SizeA(SizeA);
				// 			if (cSizeB==false) Tapie->put_SizeB(SizeB);
				// 			if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Tapie->put_This1D(D1);
				// 			if (cWipe==false) Tapie->put_Wipeout(Wipe);
				// 			if (cElev==false) Tapie->put_Elevation(getElev(Elev,Emode,Tapie->SizeA,Tapie->SizeB));
				// 			//
				// 
				// 
				// 			if (cTapForm==false) Tapie->put_Form(TapForm);
				Tapie->put_TypeRoundTap(globalTypeRoundTap);
				Tapie->put_RadiusTypeRound(globalRadiusTypeRound);
				Tapie->put_RadiusTypeRect(globalRadiusTypeRect);
				Tapie->put_RadiusVariableParameter(globalTapRadiusVariableParameter);
				Tapie->put_RadiusConst(globalTapRadiusConst);
				// 			if (cSwectangle==false) Tapie->put_Swectangle(Swectangle);

			}

			if ((Wyeie = TVS_WYE::cast(pEnt)) != NULL)
			{


				// 			///
				// 			if (cSizeA==false) Wyeie->put_SizeApr(SizeA);
				// 			if (cSizeB==false) Wyeie->put_SizeBpr(SizeB);
				// 			if (cSizeA2==false) Wyeie->put_SizeAotv(SizeA2);
				// 			if (cSizeB2==false) Wyeie->put_SizeBotv(SizeB2);
				// 			if (cLengthW==false) Wyeie->put_Length(LengthW);
				// 			//if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Wyeie->put_This1D(D1);
				// 			if (cWipe==false) Wyeie->put_Wipeout(Wipe);
				// 			if (cElev==false) Wyeie->put_Elevation(getElev(Elev,Emode,Wyeie->SizeApr,Wyeie->SizeBpr));
				// 			//


			}

			if ((Transie = TVS_TRANS::cast(pEnt)) != NULL)
			{
				// 			///
				// 			if (cSizeA==false) Transie->put_SizeAp1(SizeA);
				// 			if (cSizeB==false) Transie->put_SizeBp1(SizeB);
				// 			if (cSizeA2==false) Transie->put_SizeAp2(SizeA2);
				// 			if (cSizeB2==false) Transie->put_SizeBp2(SizeB2);
				// 			if (cLengthTr==false) Transie->put_Length(LengthTr);
				// 			//if (cFlow==false) Tapie->put_Flow(Flow);
				// 
				// 			if (cD1==false) Transie->put_This1D(D1);
				// 			if (cWipe==false) Transie->put_Wipeout(Wipe);
				// 			if (cElev==false) Transie->put_Elevation(getElev(Elev,Emode,Transie->SizeAp1,Transie->SizeBp1));
				// 			//



			}

			pEnt->draw();
			pEnt->close();
		}



	}





