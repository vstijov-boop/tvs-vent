
#include "StdAfx.h"
#include "SPEC.h"
#include "TablePresentation.h"
#include "../TVS_Ventilation_DBX/SheetMetal.h"

#include <cmath>
#include<iostream>
using namespace std;

#define None 0
#define PipeRound 1
#define PipeRect 2
#define TapRound 3
#define TapRect 4
#define Trans 5
#define Wye 6
#define TypeInt 0
#define TypeDouble1 1
#define TypeDouble2 2

#pragma once

void SPEC::setAcharType(int val)
{
	switch (val)
	{
	case TypeInt:
		wcscpy_s(achartype,_T("%d"));
	case TypeDouble1:
		wcscpy_s(achartype,_T("%10.1f "));
	case TypeDouble2:
		wcscpy_s(achartype,_T("%10.2f "));
	default:
		wcscpy_s(achartype,_T(""));
	}
}

SPEC::SPEC(void) : sheetMass(0.0), sheetThickness(0.0), sheetUnitMass(0.0)
{
	wcscpy_s(d,_T("%%c"));
	wcscpy_s(grad,_T("%%d"));
}

SPEC::~SPEC(void)
{
}


void SPEC::setParam1(double param,int type)
{
	param1=param;
	param1Type=type;
}

void SPEC::setParam2(double param,int type)
{
	param2=param;
	param2Type=type;
}

bool SPEC::add(AcDbEntity * pEnt)
{
	TVS_Pipe * pPipe;
	TVS_TAP * pTap;
	TVS_WYE * pWye;
	TVS_TRANS * pTrans;
	TVS_Entity *Ent;
	status=None;
	SizeA=0;
	SizeB=0;
	SizeA2=0;
	SizeA3=0;
	Swectangle=0;
	Length=0;
	Area=0;
	sheetMass = sheetThickness = sheetUnitMass = 0.0;



	if (acdbOpenAcDbEntity(pEnt,pEnt->id(),AcDb::kForRead)==eOk)
	{
		if(Ent = TVS_Entity::cast(pEnt))
		{


#pragma region Pipe
			if(pPipe = TVS_Pipe::cast(pEnt))
			{

				SizeA=tvsUnits::mm(pPipe->get_SizeA(), pPipe);
				SizeB=tvsUnits::mm(pPipe->get_SizeB(), pPipe);
				Length=tvsUnits::mm(pPipe->get_Length(), pPipe);
				setUnit1(_T("m"));
				setUnit2(_T("m²"));
				if (pPipe->DuctType==DuctTypeFlex)
				{
					status=PipeRound;
					setName(_T("Fleksibilni kanal"));
					setLable(d);
					appendLable(SizeA);
					Area=Length*M_PI*SizeA/1000000;
					setParam1(Length/1000, TypeDouble1);
					setParam2(Area, TypeDouble2);
				}
				else
				{
					if (SizeB==0) 
					{
						status=PipeRound;
						setName(_T("Okrugli kanal"));
						setLable(d);
						appendLable(SizeA);
						Area=Length*M_PI*SizeA/1000000;

					}
					else 
					{ 
						status=PipeRect;
						setName(_T("Pravougaoni kanal"));
						if (SizeB>SizeA)
						{
							SizeB=tvsUnits::mm(pPipe->SizeA, pPipe);
							SizeA=tvsUnits::mm(pPipe->SizeB, pPipe);
						}
						setLable(max(SizeA,SizeB));
						appendLable(_T("x"));
						appendLable(min(SizeA,SizeB));
						Area=Length*(SizeB+SizeA)*2/1000000;
					}
					setParam1(Length/1000, TypeDouble1);
					setParam2(Area, TypeDouble2);
				}
			}
#pragma endregion

#pragma region Tap
			if(pTap = TVS_TAP::cast(pEnt))
			{




				if (pTap->DuctType==DuctTypeFlex)
				{
					SizeA=tvsUnits::mm(pTap->get_SizeA(), pTap);
					SizeB=tvsUnits::mm(pTap->get_SizeB(), pTap);

					setUnit1(_T("m"));
					setUnit2(_T("m²"));

					status=PipeRound;
					setName(_T("Fleksibilni kanal"));
					setLable(d);
					appendLable(SizeA);
					Length=M_PI*tvsUnits::mm(pTap->Radius, pTap)*pTap->Swectangle/2/M_PI/1000;
					Area=Length*M_PI*SizeA/1000;

					setParam1(Length, TypeDouble1);
					setParam2(Area, TypeDouble2);
				}
				else
				{
					Swectangle=5*floor((pTap->Swectangle+(2*M_PI/180))/5*180/M_PI);

					
					SizeA=tvsUnits::mm(pTap->get_SizeA(), pTap);
					SizeB=tvsUnits::mm(pTap->get_SizeB(), pTap);

					setUnit1(_T("kom"));
					setUnit2(_T("m²"));

					if (SizeB==0) 
					{
						status=TapRound;
						setName(_T("Okruglo koljeno"));
						setLable(d);
						appendLable(SizeA);
						Area=M_PI*2*SizeA*tvsUnits::mm(pTap->Radius, pTap)/1000000*pTap->Swectangle;
						if ((pTap->RadiusTypeRound==TypeRoundTap_TapSection)&&(SizeA<=355))
						{
							Area=Area+M_PI*SizeA*2/10000;
						}

					}
					else 
					{ 
						if (pTap->Form == Form_Direct)
						{
					
							SizeA=tvsUnits::mm(pTap->get_SizeA(), pTap);
							SizeB=tvsUnits::mm(pTap->get_SizeB(), pTap);
						}else{
						
							SizeB=tvsUnits::mm(pTap->get_SizeA(), pTap);
							SizeA=tvsUnits::mm(pTap->get_SizeB(), pTap);
						}
						status=TapRect;
						setName(_T("Pravougaono koljeno"));
						setLable(SizeA);
						appendLable(_T("x"));
						appendLable(SizeB);
                        Area=tvsSheetMetal::elbowArea(tvsUnits::mm(pTap->SizeA,pTap),
                            tvsUnits::mm(pTap->SizeB,pTap),tvsUnits::mm(pTap->Radius,pTap),pTap->Swectangle);
					}
					appendLable(_T("("));
					appendLable(Swectangle);
					appendLable(grad);
					appendLable(_T(")"));
					setParam1(1, TypeDouble1);
					setParam2(Area, TypeDouble2);
				}
			}
#pragma endregion
#pragma region Wye
			if(pWye = TVS_WYE::cast(pEnt))
			{
				status=Wye;
				SizeA=tvsUnits::mm(pWye->get_SizeApr(), pWye);
				SizeB=tvsUnits::mm(pWye->get_SizeBpr(), pWye);
				SizeA2=tvsUnits::mm(pWye->get_SizeAotv(), pWye);
				SizeA3=tvsUnits::mm(pWye->get_SizeBotv(), pWye);
				Swectangle=tvsUnits::mm(pWye->LengthPl, pWye);
				setUnit1(_T("kom"));
				setUnit2(_T("m²"));

				if (SizeB==0) 
				{
					setName(_T("T-komad"));
					setLable(d);
					appendLable(SizeA);
					Area=(SizeA2+tvsUnits::mm(pWye->LengthPl, pWye)*2)*M_PI*SizeA/1000000;
				}
				else 
				{ 
					setName(_T("T-komad"));
					setLable(SizeA);
					appendLable(_T("x"));
					appendLable(SizeB);
					Area=(SizeA2+tvsUnits::mm(pWye->LengthPl, pWye)*2)*(SizeA+SizeB)/500000;
				}
				appendLable(_T("-"));
				if (SizeA3==0) 
				{
					appendLable(d);
					appendLable(SizeA2);
					Area=Area+(tvsUnits::mm(pWye->LengthPl, pWye))*M_PI*SizeA2/1000000-sCircle(SizeA2/1000);
				}
				else 
				{ 
					setName(_T("T-komad"));
					appendLable(SizeA2);
					appendLable(_T("x"));
					appendLable(SizeA3);
					Area=Area+(tvsUnits::mm(pWye->LengthPl, pWye))*(SizeA2+SizeA3)/500000-SizeA2*SizeA3/1000000;
				}

				appendLable(_T(" (L="));
				appendLable(tvsUnits::mm(pWye->LengthPl, pWye));
				appendLable(_T(" mm)"));
				setParam1(1, TypeDouble1);
				setParam2(Area, TypeDouble2);
			}
#pragma endregion
#pragma region Trans
			if(pTrans = TVS_TRANS::cast(pEnt))
			{
				status=Trans;
				Swectangle=tvsUnits::mm(pTrans->LengthTr, pTrans);
				SizeA=tvsUnits::mm(pTrans->get_SizeAp1(), pTrans);
				SizeB=tvsUnits::mm(pTrans->get_SizeBp1(), pTrans);
				SizeA2=tvsUnits::mm(pTrans->get_SizeAp2(), pTrans);
				SizeA3=tvsUnits::mm(pTrans->get_SizeBp2(), pTrans);
				if (SizeB==0)
				{
					if (SizeA3==0)
					{
						if (SizeA<SizeA2)
						{
							SizeA=tvsUnits::mm(pTrans->get_SizeAp2(), pTrans);
							SizeB=tvsUnits::mm(pTrans->get_SizeBp2(), pTrans);
							SizeA2=tvsUnits::mm(pTrans->get_SizeAp1(), pTrans);
							SizeA3=tvsUnits::mm(pTrans->get_SizeBp1(), pTrans);
						}
					}
				}
				else
				{
					if ((SizeA3==0)||(SizeA<SizeA2))
					{

						SizeA=tvsUnits::mm(pTrans->get_SizeAp2(), pTrans);
						SizeB=tvsUnits::mm(pTrans->get_SizeBp2(), pTrans);
						SizeA2=tvsUnits::mm(pTrans->get_SizeAp1(), pTrans);
						SizeA3=tvsUnits::mm(pTrans->get_SizeBp1(), pTrans);
					}

				}

				setUnit1(_T("kom"));
				setUnit2(_T("m²"));
				double radp1, radp2;
				if (SizeB==0) 
				{
					setName(_T("Prelaz"));
					setLable(d);
					appendLable(SizeA);
					radp1=SizeA/2;
				}
				else 
				{ 
					setName(_T("Prelaz"));
					setLable(SizeA);
					appendLable(_T("x"));
					appendLable(SizeB);
					radp1=(SizeB+SizeA)/(M_PI);
				}
				appendLable(_T("-"));
				if (SizeA3==0) 
				{
					appendLable(d);
					appendLable(SizeA2);
					radp2=SizeA2/2;

				}
				else 
				{ 

					appendLable(SizeA2);
					appendLable(_T("x"));
					appendLable(SizeA3);
					radp2=(SizeA2+SizeA3)/(M_PI);

				}
				double dobr=sqrt(pow(radp1-radp2,2)+pow(tvsUnits::mm(pTrans->LengthTr, pTrans),2));
				Area=M_PI*(radp1+radp2)*dobr/1000000;
				appendLable(_T(" (L="));
				appendLable(tvsUnits::mm(pTrans->LengthTr, pTrans));
				appendLable(_T(" mm)"));
				setParam1(1, TypeDouble1);
				setParam2(Area, TypeDouble2);
			}
#pragma endregion
            if (Ent->getSheetMetal(Area,sheetThickness,sheetMass)) {
                setParam2(Area,TypeDouble2);
                sheetUnitMass = param1 > 0.0 ? sheetMass / param1 : 0.0;
            }
			//if (status!=None) printResult();


			pEnt->close();
			return true;
		}
		pEnt->close();	
	}

	else return false;
}

void SPEC::setName(const ACHAR * pName)
{
	wcscpy_s(name,pName);
}

void SPEC::setLable(const ACHAR * pAchar)
{
	wcscpy_s(lable,pAchar);
}


void SPEC::setLable(double val)
{
	acdbRToS(val,2,2,lable);
	
}

void SPEC::appendLable( const ACHAR * pAchar )
{
	wcscat_s(lable,pAchar);
}


void SPEC::appendLable( double par )
{
	ACHAR buffer [512] ;
	acdbRToS(par,2,2,buffer);
	wcscat_s(lable,buffer);
}

void SPEC::setUnit1(const ACHAR * pAchar)
{
	wcscpy_s(unit1,pAchar);
}

void SPEC::setUnit2(const ACHAR * pAchar)
{
	wcscpy_s(unit2,pAchar);
}


void SPEC::printResult()
{
	setParamChars();
	acutPrintf(_T("\n%s %s  %s  %s  %s  %s"), name,lable,param1char, unit1, param2char, unit2);
	// 	acutPrintf(_T("%s "),param1char);
	// 	acutPrintf(_T("%s "),unit1);
	// 	acutPrintf(_T("%s "),param2char);
	// 	acutPrintf(_T("%s"),unit2);




}


void SPEC::printResultChar()
{
	setParamChars();
	acutPrintf(_T("\n%s %s  %s  %s  %s  %s  %s  %s  %s"),sPos, sName,sTypeSize,sManufacture,sArticle, sUnit, param1char, sMass,sCommit);
}

// int SPEC::toInt( const ACHAR * pAchar )
// {
// 
// }


const ACHAR * SPEC::toChar( double val )
{
	const ACHAR* str=new ACHAR[512];
	ACHAR  buffer[512];
	acdbRToS(val,2,2,buffer);
	str=buffer;
	return buffer;
}


void SPEC::setParamChars()
{
	acdbRToS(param1,2,param1Type,param1char);
	acdbRToS(param2,2,param2Type,param2char);
	acdbRToS(param1,2,param1Type,sValue);
}


double SPEC::sCircle(double diam)
{
	return M_PI*diam*diam/4;
}

double SPEC::lCircle(double diam)
{
	return M_PI*diam;
}

bool SPEC::GetAtt(AcDbEntity* pEnt, ACHAR* tag, ACHAR  *pVal)
{

	AcDbDatabase *pCurDb;
	AcDbBlockTable* pBlkTbl;
	AcDbBlockTableRecord* pBlkRec;
	AcDbObjectId attId;
	Acad::ErrorStatus es;
	AcDbBlockReference * br;
	AcDbAttributeDefinition* pAttDef;
	// location of the AttributeDefinition in the
	// block definition
	ACHAR* pName;


	if (acdbOpenAcDbEntity(pEnt,pEnt->id(),AcDb::kForWrite)==eOk)

	{
		if ( (br = AcDbBlockReference::cast(pEnt)) != NULL )
		{	



			AcDbObjectIterator *pAttrIter = br->attributeIterator();
			if (pAttrIter) {
				for (pAttrIter->start();!pAttrIter->done();pAttrIter->step()) {
					AcDbObjectId objAttrId = pAttrIter->objectId();
					AcDbObjectPointer<AcDbAttribute> pAttr(objAttrId,AcDb::kForRead);

					if ((es = pAttr.openStatus()) == Acad::eOk) {
						//
						// Здесь можно получить информацию об атрибуте
						//

						if (wcscmp(tag,pAttr->tagConst())==0) {
							//	pAttDef->close();
							pAttr->close();
							pEnt->close();
							//acutPrintf(_T("\nАтрибут: Tag=%s Value=%s "),
							//	LPCTSTR(pAttr->tagConst()),LPCTSTR(pAttr->textStringConst()));

							wcscpy_s(pVal,512,pAttr->textStringConst());
							return true;
						}

						pAttr->close();
					} else {
						acutPrintf(_T("\nNije moguće otvoriti atribut bloka! Greška: %s", LPCTSTR(acadErrorStatusText(es))));
					}
				}
			}
		}
		pEnt->close();
	}












	return false;

}
bool SPEC::addBlock(AcDbEntity * pEnt)
{

	if(GetAtt(pEnt,TagName,sName)&&GetAtt(pEnt,TagSize,sSize)&&GetAtt(pEnt,TagType,sType))
	{
		wcscpy_s(sPos,_T(""));
		//wcscpy_s(sName,_T(""));
		wcscpy_s(sTypeSize,_T(""));
		wcscpy_s(sArticle,_T(""));
		wcscpy_s(sManufacture,_T(""));
		wcscpy_s(sUnit,_T("kom"));
		wcscpy_s(sValue,_T(""));
		wcscpy_s(sMass,_T(""));
		wcscpy_s(sCommit,_T(""));

		if (wcscmp(sType,_T(""))!=0) wcscpy_s(sTypeSize,sType);
		if ((wcscmp(sType,_T(""))!=0)&&(wcscmp(sSize,_T(""))!=0)) wcscat_s(sTypeSize,_T("-"));
		if (wcscmp(sSize,_T(""))!=0) wcscat_s(sTypeSize,sSize);

		GetAtt(pEnt,TagPos,sPos);
		GetAtt(pEnt,TagManufacture,sManufacture);
		GetAtt(pEnt,TagMass,sMass);
		GetAtt(pEnt,TagArticle,sArticle);
		GetAtt(pEnt,TagCommit,sCommit);
		setParam1(1, TypeInt);
		setParam2(0, TypeInt);
		if ((wcscmp(sPos,_T(""))==0)&&
			(wcscmp(sName,_T(""))==0)&&
			(wcscmp(sTypeSize,_T(""))==0)&&
			(wcscmp(sArticle,_T(""))==0)&&
			(wcscmp(sManufacture,_T(""))==0)&&
			(wcscmp(sMass,_T(""))==0)&&
			(wcscmp(sCommit,_T(""))==0)
			) getBlockName(pEnt,sName);
		sEnt=pEnt;
		entityList.append(sEnt);
		return true;
	}
	else return false;
}


bool SPEC::setAtribToEnt(AcDbEntity* pEnt, ACHAR* tag, ACHAR  *pVal)
{


		Acad::ErrorStatus es;


		AcDbObjectPointer<AcDbBlockReference> pBlkRef(pEnt->id(),AcDb::kForWrite);
		if ((es = pBlkRef.openStatus()) != Acad::eOk) {
			acutPrintf(_T("\nGreška pri otvaranju BlockReference: %s"), acadErrorStatusText(es)); return false;
		}
		AcDbObjectIterator *pAttrIter = pBlkRef->attributeIterator();
		if (pAttrIter) {
			for (pAttrIter->start();!pAttrIter->done();pAttrIter->step()) {
				AcDbObjectId objAttrId = pAttrIter->objectId();
				AcDbObjectPointer<AcDbAttribute> pAttr(objAttrId,AcDb::kForWrite);

				if ((es = pAttr.openStatus()) == Acad::eOk) {
					//
					// Здесь можно получить информацию об атрибуте
					//

					if (wcscmp(tag,pAttr->tagConst())==0) {


						pAttr->setTextString(pVal);
						return true;
					}

				} else {
					acutPrintf(_T("\nNije moguće otvoriti atribut bloka! Greška: %s", LPCTSTR(acadErrorStatusText(es))));
				}
			}
		}

	}



bool SPEC::setAtribListToEnt()
{
	for each (AcDbEntity* pEnt in entityList)
	{
		setAtribToEnt(pEnt, TagPos, sPos);
		setAtribToEnt(pEnt, TagName, sName);
		setAtribToEnt(pEnt, TagType, sType);
		setAtribToEnt(pEnt, TagSize, sSize);
		setAtribToEnt(pEnt, TagArticle, sArticle);
		setAtribToEnt(pEnt, TagManufacture, sManufacture);
		setAtribToEnt(pEnt, TagMass, sMass);
		setAtribToEnt(pEnt, TagCommit, sCommit);
	}
	return true;
}

bool SPEC::getBlockName(AcDbEntity* pEnt , ACHAR *pName)
{

	Acad::ErrorStatus es;


	AcDbObjectPointer<AcDbBlockReference> pBlkRef(pEnt->id(),AcDb::kForRead);
	if ((es = pBlkRef.openStatus()) != Acad::eOk) {
		acutPrintf(_T("\nGreška pri otvaranju BlockReference: %s"), acadErrorStatusText(es)); return false;
	}
	AcDbBlockTableRecordPointer pBTR(pBlkRef->blockTableRecord(),AcDb::kForRead);
	if ((es = pBTR.openStatus()) != Acad::eOk) {
		acutPrintf(_T("\nGreška pri otvaranju BlockTableRecord: %s"), acadErrorStatusText(es)); return false;
	}
	const ACHAR* sBTRName = NULL; pBTR->getName(sBTRName); 
	if (sBTRName) wcscpy_s(pName,512,sBTRName);

	
	///DynBlock

	


	AcDbDynBlockReference dynblkRef(pEnt->id());
	if (!dynblkRef.isDynamicBlock()) {

		acutPrintf(_T("\nOvo nije dinamički blok!")); return true;

	}

	AcDbBlockTableRecordPointer pDynBTR(dynblkRef.dynamicBlockTableRecord(),AcDb::kForRead);

	if ((es = pDynBTR.openStatus()) != Acad::eOk) {

		acutPrintf(_T("\nGreška pri otvaranju dinamičkog BlockTableRecord: %s"), acadErrorStatusText(es)); return true;

	}

	const ACHAR* sDynBTRName = NULL; pDynBTR->getName(sDynBTRName); 

	if (sDynBTRName) wcscpy_s(pName,512,sDynBTRName);







	return true;
}


void SPEC:: add(double pSizeA,
				double pSizeB,
				bool pThisRect,
				double pLength,
				double pSwectangle,
				double pSizeA2,
				double pSizeA3
				)
{
	SizeA=pSizeA;
	SizeB=pSizeB;
	ThisRect=pThisRect;
	Length=pLength;
	SizeA2=pSizeA2;
	SizeA3=pSizeA3;
	Swectangle=pSwectangle;
	Area=0;
	sheetMass = sheetThickness = sheetUnitMass = 0.0;

}


SPEClist::SPEClist(void)
{

}

SPEClist::~SPEClist(void)
{

}


void SPEClist::append(SPEC line)
{
	// 	acutPrintf(_T("\n123"));
	// 	print();
	if (specList.logicalLength()==0)
	{
		specList.append(line);
		return;
	}

	length=specList.logicalLength();
	int i = 0;
	while (i<length)
	{
		int currentStatus=checkRelevations(specList[i],line);
		if (currentStatus==Equal)
		{
			specList[i].param1=specList[i].param1+line.param1;
			specList[i].param2=specList[i].param2+line.param2;
            specList[i].sheetMass += line.sheetMass;
			specList[i].entityList.append(line.sEnt);
			return;
		}

		if (currentStatus==Larger)
		{
			specList.insertAt(i,line);
			return;
		}
		i++;
	}
	specList.append(line);
}

int SPEClist::checkRelevations(SPEC param1, SPEC param2)
{
	double parameters1[6],parameters2[6];;
	parameters1[0]=param1.status;
	parameters2[0]=param2.status;

	parameters1[1]=param1.SizeA;
	parameters2[1]=param2.SizeA;

	parameters1[2]=param1.SizeB;
	parameters2[2]=param2.SizeB;

	parameters1[3]=param1.SizeA2;
	parameters2[3]=param2.SizeA2;

	parameters1[4]=param1.SizeA3;
	parameters2[4]=param2.SizeA3;

	parameters1[5]=param1.Swectangle;
	parameters2[5]=param2.Swectangle;
	int i=0;

	while (i<6)
	{
		if (parameters1[i]<parameters2[i])
		{
			return Less;
		}
		if (parameters1[i]>parameters2[i])
		{
			return Larger;
		}
		i++;
	}


    if (param1.sheetThickness < param2.sheetThickness) return Less;
    if (param1.sheetThickness > param2.sheetThickness) return Larger;
    const double massTolerance = 1e-9 * (1.0 + fabs(param2.sheetUnitMass));
    if (param1.sheetUnitMass < param2.sheetUnitMass - massTolerance) return Less;
    if (param1.sheetUnitMass > param2.sheetUnitMass + massTolerance) return Larger;
	return Equal;

}

void SPEClist::print()
{
	length=specList.logicalLength();
	for (int i=0; i<length;i++)
	{
		specList[i].printResult();
	}
}





void SPEClist::printSPDSForm(AcGePoint3d &cent)
{
	//double columnswidth[9]={2000,13000,6000,3500,4500,2000,2000,2500,4000};
	double columnswidth[11]={0, 2000,15000,21000,24500,29000,31000,33000,35500,37500,41500};
	double columndistanse[7]={2000,15000,29000,31000,33000,35500,37500};
	const double units = 1.0 / tvsTable::current().templateFactor();
	for (double& c : columnswidth) c /= units;
	for (double& c : columndistanse) c /= units;
	double rowhight=800 / units;
	double otstupX=100 / units;
	double otstupY=150 / units;
	int length = specList.logicalLength();
	AcGePoint3d curcnt, pos1, pos2, pos3,pos4,pos5,pos6,pos7;
	curcnt=cent;



	//zagolovok
	pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
	printText(pos1,_T("Naziv"));
	printText(pos2,_T("Dimenzije"));
	printText(pos3,_T("Jed. mjere"));
	printText(pos4,_T("Količina"));
	printText(pos5,_T("Jed. mjere 2"));
	printText(pos6,_T("Površina, m²"));
    pos7=AcGePoint3d(curcnt.x+columndistanse[6]+otstupX,curcnt.y+otstupY,curcnt.z);
    printText(pos7,_T("Masa, kg"));
	curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);

	//print all table text
	for (int i =0; i<length;i++)
	{
		pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
		specList[i].setParamChars();
		printText(pos1,specList[i].name);
		printText(pos2,specList[i].lable);
		printText(pos3,specList[i].unit1);
		printText(pos4,specList[i].param1char);
		printText(pos5,specList[i].unit2);
		printText(pos6,specList[i].param2char);
        pos7=AcGePoint3d(curcnt.x+columndistanse[6]+otstupX,curcnt.y+otstupY,curcnt.z);
        if (specList[i].sheetThickness > 0.0) {
            ACHAR massText[64]; acdbRToS(specList[i].sheetMass,2,3,massText);
            printText(pos7,massText);
        }
		curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);
	}

	/// total
	double summlenght=0, summArea=0, totalMass=0;
	ACHAR  sA[512];

	for (int i =0; i<length;i++)
	{
		//summlenght+=specList[i].param1;
		summArea+=specList[i].param2;
        totalMass+=specList[i].sheetMass;
	}
	acdbRToS(summArea,2,2,sA);
	//pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
	//pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
	//pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
	//pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
	printText(pos5,_T("Ukupno:"));
	//printText(pos2,_T("Dimenzije"));
	//printText(pos3,_T("Jed. mjere"));
	//printText(pos4,_T("Količina"));
	//printText(pos5,_T("Jed. mjere 2"));
	printText(pos6,sA);
    pos7=AcGePoint3d(curcnt.x+columndistanse[6]+otstupX,curcnt.y+otstupY,curcnt.z);
    ACHAR massText[64]; acdbRToS(totalMass,2,3,massText); printText(pos7,massText);
	curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);

	///print table

	double TableLenght=41500 / units;
	double Hight=length*rowhight;
	//vertical Lines

	AcGePoint3d start,end;
	for each (double c in columnswidth)
	{
		start=AcGePoint3d(cent.x+c,cent.y+rowhight,cent.z);
		end=AcGePoint3d(cent.x+c,cent.y-rowhight*(length+1),cent.z);
		printLine(start,end);
	}
	//horizontal Lines

	for (int i =-1; i<length+2;i++)
	{
		start=AcGePoint3d(cent.x,cent.y-rowhight*i,cent.z);
		end=AcGePoint3d(cent.x+TableLenght,cent.y-rowhight*i,cent.z);
		printLine(start,end);
	}
	cent=AcGePoint3d(cent.x,cent.y-rowhight*(length+3),cent.z);
	print();
	acutPrintf(_T("\nUkupno m²: %s"),sA);
}

void SPEClist::printText( AcGePoint3d cent, const ACHAR * pAchar )
{
	AcDbDatabase *db=acdbHostApplicationServices()->workingDatabase();

	AcDbObjectId tId=db->textstyle();
	///
	AcDbText *text1 = new AcDbText (cent, pAchar,tId,tvsTable::current().textHeight(),0 );

	AcDbBlockTable *pBlockTable;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable,
		AcDb::kForRead);

	AcDbBlockTableRecord *pBlockTableRecord;
	acdbOpenObject(pBlockTableRecord, db->currentSpaceId(),AcDb::kForWrite);
	pBlockTable->close();

	AcDbObjectId retId = AcDbObjectId::kNull;
	pBlockTableRecord->appendAcDbEntity(retId, text1);
	pBlockTableRecord->close();
	text1->close();
	//

}

void SPEClist::printLine( AcGePoint3d start, AcGePoint3d end )
{
	AcDbDatabase *db=acdbHostApplicationServices()->workingDatabase();

	AcDbObjectId tId=db->textstyle();
	///
	AcDbLine *pEnt = new AcDbLine (start,end);

	AcDbBlockTable *pBlockTable;
	acdbHostApplicationServices()->workingDatabase()->getSymbolTable(pBlockTable,
		AcDb::kForRead);

	AcDbBlockTableRecord *pBlockTableRecord;
	acdbOpenObject(pBlockTableRecord, db->currentSpaceId(),AcDb::kForWrite);
	pBlockTable->close();
	pEnt->setLineWeight(AcDb::kLnWt015);
	AcDbObjectId retId = AcDbObjectId::kNull;
	pBlockTableRecord->appendAcDbEntity(retId, pEnt);
	pBlockTableRecord->close();
	pEnt->close();
	//
}


bool SPEClist::printToExel(CMSExcel* m_msExcel,long &idx)
{
	


	
	int length = specList.logicalLength();
	ACHAR*col1={L"A"};
	ACHAR*col2={L"B"};
	ACHAR*col3={L"C"};
	ACHAR*col4={L"D"};
	ACHAR*col5={L"E"};
	ACHAR*col6={L"F"};
	ACHAR*col7={L"G"};
	ACHAR*col8={L"H"};
	ACHAR*col9={L"I"};
    ACHAR*col10={L"J"};

	//m_msExcel->SetExcelCellFormat(L"A1:i20000",L"Text");



	m_msExcel->printExelText(col2,idx,_T("Naziv"));
	m_msExcel->printExelText(col3,idx,_T("Dimenzije"));
	m_msExcel->printExelText(col6,idx,_T("Jed. mjere"));
	m_msExcel->printExelText(col7,idx,_T("Količina"));
	m_msExcel->printExelText(col8,idx,_T("Jed. mjere 2"));
	m_msExcel->printExelText(col9,idx,_T("Površina, m²"));
    m_msExcel->printExelText(col10,idx,_T("Masa, kg"));


 acedSetStatusBarProgressMeter(_T("Izvoz podataka u Excel"),0,length);
	idx++;
	//print all table text
	for (int i =0; i<length;i++)
	{

		 acedSetStatusBarProgressMeterPos( i);
		specList[i].setParamChars();

		m_msExcel->printExelText(col2,idx,specList[i].name);
		m_msExcel->printExelText(col3,idx,specList[i].lable);
		m_msExcel->printExelText(col6,idx,specList[i].unit1);
		m_msExcel->printExelText(col7,idx,specList[i].param1char);
		m_msExcel->printExelText(col8,idx,specList[i].unit2);
		m_msExcel->printExelText(col9,idx,specList[i].param2char);
        if (specList[i].sheetThickness > 0.0) {
            ACHAR massText[64]; acdbRToS(specList[i].sheetMass,2,3,massText);
            m_msExcel->printExelText(col10,idx,massText);
        }



		idx++;
	}
		acedRestoreStatusBar();

	/// total
	double summlenght=0, summArea=0, totalMass=0;
	ACHAR  sA[512];

	for (int i =0; i<length;i++)
	{
		//summlenght+=specList[i].param1;
		summArea+=specList[i].param2;
        totalMass+=specList[i].sheetMass;
	}
	acdbRToS(summArea,2,2,sA);

	m_msExcel->printExelText(col8,idx,_T("Ukupno:"));
	m_msExcel->printExelText(col9,idx,sA);
    ACHAR massText[64]; acdbRToS(totalMass,2,3,massText);
    m_msExcel->printExelText(col10,idx,massText);

	idx++;

	return true;
}





SpecWithAttrlist::SpecWithAttrlist(void)
{

}

SpecWithAttrlist::~SpecWithAttrlist(void)
{

}



int SpecWithAttrlist::checkRelevations(SPEC param1, SPEC param2)
{
	int ret;
	ret=checkCharRelevations(param1.sPos,param2.sPos);
	if(ret==Equal) ret=checkCharRelevations(param1.sName,param2.sName);
	if(ret==Equal) ret=checkCharRelevations(param1.sType,param2.sType);
	if(ret==Equal) ret=checkCharRelevations(param1.sSize,param2.sSize);
	if(ret==Equal) ret=checkCharRelevations(param1.sArticle,param2.sArticle);
	if(ret==Equal) ret=checkCharRelevations(param1.sManufacture,param2.sManufacture);
	if(ret==Equal) ret=checkCharRelevations(param1.sMass,param2.sMass);
	if(ret==Equal) ret=checkCharRelevations(param1.sCommit,param2.sCommit);
	return ret;
}

int SpecWithAttrlist::checkCharRelevations(ACHAR * param1, ACHAR * param2)
{
	if(wcscmp(param1,param2)<0) return Less;
	if(wcscmp(param1,param2)==0) return Equal;
	if(wcscmp(param1,param2)>0) return Larger;
}

void SpecWithAttrlist::printSPDSForm(AcGePoint3d &cent)
{

	//double columnswidth[9]={2000,13000,6000,3500,4500,2000,2000,2500,4000};
	double columnswidth[10]={0, 2000,15000,21000,24500,29000,31000,33000,35500,39500};
	double columndistanse[9]={0,2000,15000,21000,24500,29000,31000,33000,35500};
	const double units = 1.0 / tvsTable::current().templateFactor();
	for (double& c : columnswidth) c /= units;
	for (double& c : columndistanse) c /= units;
	double rowhight=800 / units;
	double otstupX=100 / units;
	double otstupY=150 / units;
	int length = specList.logicalLength();
	AcGePoint3d curcnt, pos1, pos2, pos3,pos4,pos5,pos6,pos7,pos8,pos9;
	curcnt=cent;



	//zagolovok
	pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos7=AcGePoint3d(curcnt.x+columndistanse[6]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos8=AcGePoint3d(curcnt.x+columndistanse[7]+otstupX,curcnt.y+otstupY,curcnt.z);
	pos9=AcGePoint3d(curcnt.x+columndistanse[8]+otstupX,curcnt.y+otstupY,curcnt.z);
	printText(pos1,_T("Pozicija"));
	printText(pos2,_T("Naziv"));
	printText(pos3,_T("Tip i dimenzije"));
	printText(pos4,_T("Šifra"));
	printText(pos5,_T("Proizvođač"));
	printText(pos6,_T("Jed. mjere"));
	printText(pos7,_T("Količina"));
	printText(pos8,_T("Masa"));
	printText(pos9,_T("Napomena"));
	curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);

	//print all table text
	for (int i =0; i<length;i++)
	{

		pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos7=AcGePoint3d(curcnt.x+columndistanse[6]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos8=AcGePoint3d(curcnt.x+columndistanse[7]+otstupX,curcnt.y+otstupY,curcnt.z);
		pos9=AcGePoint3d(curcnt.x+columndistanse[8]+otstupX,curcnt.y+otstupY,curcnt.z);
		//specList[i].setParamChars();
		//printText(pos1,specList[i].name);
		specList[i].setParamChars();
		if(wcscmp(specList[i].sPos,_T(""))!=0) printText(pos1,specList[i].sPos);
		if(wcscmp(specList[i].sName,_T(""))!=0) printText(pos2,specList[i].sName);
		if(wcscmp(specList[i].sTypeSize,_T(""))!=0) printText(pos3,specList[i].sTypeSize);
		if(wcscmp(specList[i].sArticle,_T(""))!=0) printText(pos4,specList[i].sArticle);
		if(wcscmp(specList[i].sManufacture,_T(""))!=0) printText(pos5,specList[i].sManufacture);
		if(wcscmp(specList[i].sUnit,_T(""))!=0) printText(pos6,specList[i].sUnit);

		printText(pos7,specList[i].param1char);
		if(wcscmp(specList[i].sMass,_T(""))!=0) printText(pos8,specList[i].sMass);
		if(wcscmp(specList[i].sCommit,_T(""))!=0) printText(pos9,specList[i].sCommit);
		curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);
	}

	// 		/// total
	// 		double summlenght=0, summArea=0;
	// 		ACHAR  sA[512];
	// 
	// 		for (int i =0; i<length;i++)
	// 		{
	// 			//summlenght+=specList[i].param1;
	// 			summArea+=specList[i].param2;
	// 		}
	// 		acdbRToS(summArea,2,2,sA);
	// 		//pos1=AcGePoint3d(curcnt.x+columndistanse[0]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		//pos2=AcGePoint3d(curcnt.x+columndistanse[1]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		//pos3=AcGePoint3d(curcnt.x+columndistanse[2]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		//pos4=AcGePoint3d(curcnt.x+columndistanse[3]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		pos5=AcGePoint3d(curcnt.x+columndistanse[4]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		pos6=AcGePoint3d(curcnt.x+columndistanse[5]+otstupX,curcnt.y+otstupY,curcnt.z);
	// 		printText(pos5,_T("Ukupno:"));
	// 		//printText(pos2,_T("Dimenzije"));
	// 		//printText(pos3,_T("Jed. mjere"));
	// 		//printText(pos4,_T("Količina"));
	// 		//printText(pos5,_T("Jed. mjere 2"));
	// 		printText(pos6,sA);
	// 		curcnt=AcGePoint3d(curcnt.x,curcnt.y-rowhight,curcnt.z);

	///print table

	double TableLenght=39500 / units;
	double Hight=length*rowhight;
	//vertical Lines

	AcGePoint3d start,end;
	for each (double c in columnswidth)
	{
		start=AcGePoint3d(cent.x+c,cent.y+rowhight,cent.z);
		end=AcGePoint3d(cent.x+c,cent.y-rowhight*(length+1),cent.z);
		printLine(start,end);
	}
	//horizontal Lines

	for (int i =-1; i<length+2;i++)
	{
		start=AcGePoint3d(cent.x,cent.y-rowhight*i,cent.z);
		end=AcGePoint3d(cent.x+TableLenght,cent.y-rowhight*i,cent.z);
		printLine(start,end);
	}
	cent=AcGePoint3d(cent.x,cent.y-rowhight*(length+3),cent.z);
	print();
	//acutPrintf(_T("\nUkupno m²: %s"),sA);
}


bool SpecWithAttrlist::printToExel(CMSExcel* m_msExcel,long &idx)
{

	ACHAR*col1={L"A"};
	ACHAR*col2={L"B"};
	ACHAR*col3={L"C"};
	ACHAR*col4={L"D"};
	ACHAR*col5={L"E"};
	ACHAR*col6={L"F"};
	ACHAR*col7={L"G"};
	ACHAR*col8={L"H"};
	ACHAR*col9={L"I"};

	
	int length = specList.logicalLength();


	m_msExcel->printExelText(col1,idx,_T("Pozicija"));
	m_msExcel->printExelText(col2,idx,_T("Naziv"));
	m_msExcel->printExelText(col3,idx,_T("Tip i dimenzije"));
	m_msExcel->printExelText(col4,idx,_T("Šifra"));
	m_msExcel->printExelText(col5,idx,_T("Proizvođač"));
	m_msExcel->printExelText(col6,idx,_T("Jed. mjere"));
	m_msExcel->printExelText(col7,idx,_T("Količina"));
	m_msExcel->printExelText(col8,idx,_T("Masa"));
	m_msExcel->printExelText(col9,idx,_T("Napomena"));

	int status;
	 acedSetStatusBarProgressMeter(_T("Izvoz podataka u Excel"),0,length);
	 
	idx++;
	//print all table text
	for (int i =0; i<length;i++)
	{

		specList[i].setParamChars();

		
	
	 acedSetStatusBarProgressMeterPos( i);
	
		if(wcscmp(specList[i].sPos,_T(""))!=0) m_msExcel->printExelText(col1,idx,specList[i].sPos);
		if(wcscmp(specList[i].sName,_T(""))!=0) m_msExcel->printExelText(col2,idx,specList[i].sName);
		if(wcscmp(specList[i].sTypeSize,_T(""))!=0) m_msExcel->printExelText(col3,idx,specList[i].sTypeSize);
		if(wcscmp(specList[i].sArticle,_T(""))!=0) m_msExcel->printExelText(col4,idx,specList[i].sArticle);
		if(wcscmp(specList[i].sManufacture,_T(""))!=0) m_msExcel->printExelText(col5,idx,specList[i].sManufacture);
		if(wcscmp(specList[i].sUnit,_T(""))!=0) m_msExcel->printExelText(col6,idx,specList[i].sUnit);
		if(wcscmp(specList[i].param1char,_T(""))!=0) m_msExcel->printExelText(col7,idx,specList[i].param1char);
		if(wcscmp(specList[i].sMass,_T(""))!=0) m_msExcel->printExelText(col8,idx,specList[i].sMass);
		if(wcscmp(specList[i].sCommit,_T(""))!=0) m_msExcel->printExelText(col9,idx,specList[i].sCommit);



		idx++;
	}


	acedRestoreStatusBar();

	idx++;

	return true;
}

void SpecWithAttrlist::print()
{
	length=specList.logicalLength();
	for (int i=0; i<length;i++)
	{
		specList[i].printResultChar();
	}
}


