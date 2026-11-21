"""Pregled 2026-11b, prijedlog 3: jedna radnja = jedan UNDO korak i, gdje je moguce, sve ili nista.
ActionTransaction.h se prevodi sa zamjenama za SDK (g++ -std=c++14 -Wall -Wextra -Werror):
abort bez commit-a, dodavanje u transakciju, provjera zakljucanog sloja. Zamjene ne dokazuju
stvarni AutoCAD rollback - to je rucni test u TESTIRANJE.md. Ostalo: provjere u izvoru."""
import pathlib,re,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
ARX=ROOT/'TVS_Ventilation_ARX'
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
STUBS=r'''
#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
namespace Acad {enum ErrorStatus {eOk,eNotApplicable,eOnLockedLayer,eFail};}
namespace AcDb {enum OpenMode {kForRead,kForWrite};}
struct AcDbObjectId {int id;AcDbObjectId(int v=0):id(v){}bool isNull()const{return id==0;}};
static int liveParts=0;
struct AcDbObject {bool erased,closed;AcDbObject():erased(false),closed(false){}virtual ~AcDbObject(){}
 Acad::ErrorStatus erase(){erased=true;return Acad::eOk;}Acad::ErrorStatus close(){closed=true;return Acad::eOk;}};
struct AcDbEntity:AcDbObject {int layer;AcDbEntity():layer(1){++liveParts;}~AcDbEntity(){--liveParts;}AcDbObjectId layerId()const{return layer;}};
struct AcDbLayerTableRecord {bool locked;bool isLocked()const{return locked;}};
static int appends=0,failAppend=0;
struct AcDbBlockTableRecord:AcDbObject {Acad::ErrorStatus appendAcDbEntity(AcDbObjectId& id,AcDbEntity*){
 ++appends;if(appends==failAppend)return Acad::eFail;id=1000+appends;return Acad::eOk;}};
struct AcTransactionManager {int depth,aborts,ends,starts,added,failGet,failAdd;bool failStart;
 AcTransactionManager():depth(0),aborts(0),ends(0),starts(0),added(0),failGet(0),failAdd(0),failStart(false){}
 void* startTransaction(){if(failStart)return NULL;++depth;++starts;return this;}
 Acad::ErrorStatus endTransaction(){--depth;++ends;return Acad::eOk;}
 Acad::ErrorStatus abortTransaction(){--depth;++aborts;return Acad::eOk;}
 Acad::ErrorStatus getObject(AcDbObject*& o,AcDbObjectId id,AcDb::OpenMode){
  static AcDbBlockTableRecord space;o=NULL;if(id.id==failGet)return Acad::eOnLockedLayer;o=&space;return Acad::eOk;}
 Acad::ErrorStatus addNewlyCreatedDBRObject(AcDbObject*,bool add){assert(add);++added;return added==failAdd?Acad::eFail:Acad::eOk;}
};
static AcTransactionManager theManager;
#define actrTransactionManager (&theManager)
static std::map<int,AcDbEntity*> entities;static std::map<int,AcDbLayerTableRecord> layers;
template<class T> struct Opener;
template<> struct Opener<AcDbEntity> {static AcDbEntity* get(int id){return entities.count(id)?entities[id]:NULL;}};
template<> struct Opener<AcDbLayerTableRecord> {static AcDbLayerTableRecord* get(int id){return layers.count(id)?&layers[id]:NULL;}};
template<class T> class AcDbObjectPointer {T* p;public:
 AcDbObjectPointer(AcDbObjectId id,AcDb::OpenMode):p(Opener<T>::get(id.id)){}
 Acad::ErrorStatus openStatus()const{return p?Acad::eOk:Acad::eFail;}T* operator->()const{return p;}};
'''
MAIN=r'''
int main(){
 using tvsAction::Transaction;
 {Transaction tx;assert(tx.good()&&theManager.depth==1);
  {Transaction inner;assert(inner.good()&&theManager.depth==2);assert(inner.commit());}   // nested
  assert(theManager.depth==1);}
 assert(theManager.depth==0&&theManager.aborts==1&&theManager.ends==1);                // no commit: abort
 {Transaction tx;AcDbObjectId id;
  AcDbBlockTableRecord* space=static_cast<AcDbBlockTableRecord*>(tx.open(5,AcDb::kForWrite));assert(space);
  AcDbEntity* part=new AcDbEntity();assert(tx.append(space,part,id)&&id.id>0&&!part->closed);
  failAppend=appends+1;assert(!tx.append(space,new AcDbEntity(),id)&&tx.status==Acad::eFail);   // not appended: deleted
  theManager.failAdd=theManager.added+1;AcDbEntity* lost=new AcDbEntity();
  assert(!tx.append(space,lost,id)&&lost->erased&&lost->closed);                        // appended, not in tx: erased
  assert(!tx.append(NULL,new AcDbEntity(),id)&&tx.status==Acad::eNotApplicable);
  theManager.failGet=7;assert(!tx.open(7,AcDb::kForWrite)&&tx.status==Acad::eOnLockedLayer);
  assert(tx.commit()&&!tx.good()&&!tx.commit());delete part;delete lost;}
 assert(theManager.depth==0&&liveParts==0);
 theManager.failStart=true;{Transaction tx;assert(!tx.good()&&!tx.open(5,AcDb::kForRead)&&!tx.commit());
  AcDbObjectId id;assert(!tx.append(NULL,new AcDbEntity(),id));}theManager.failStart=false;
 assert(liveParts==0&&theManager.depth==0);
 AcDbEntity a,b;b.layer=2;entities[1]=&a;entities[2]=&b;layers[1].locked=false;layers[2].locked=true;
 assert(tvsAction::writable(AcDbObjectId(0))&&tvsAction::writable(AcDbObjectId(1)));
 assert(!tvsAction::writable(AcDbObjectId(2))&&!tvsAction::writable(AcDbObjectId(9)));   // locked / missing
 std::vector<AcDbObjectId> ok,bad;ok.push_back(1);ok.push_back(0);bad=ok;bad.push_back(2);
 assert(tvsAction::writable(ok)&&!tvsAction::writable(bad));
 std::cout<<"PASS ActionTransaction: abort without commit, nesting, append ownership, failures, writable preflight\n";
}
'''
class ActionTransaction(unittest.TestCase):
 def test_header_with_sdk_substitutes(self):
  h=(ARX/'ActionTransaction.h').read_bytes()
  self.assertTrue(all(c<128 for c in h),'ActionTransaction.h: samo ASCII')
  with tempfile.TemporaryDirectory() as d:
   d=pathlib.Path(d)
   for n in ('actrans.h','dbobjptr.h','dbsymtb.h'):(d/n).write_text('#pragma once\n')
   (d/'t.cpp').write_text(STUBS+'#include "ActionTransaction.h"\n'+MAIN)
   r=subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-I',str(d),'-I',str(ARX),str(d/'t.cpp'),'-o',str(d/'t')],capture_output=True,text=True)
   self.assertEqual(r.returncode,0,r.stdout+r.stderr)
   r=subprocess.run([str(d/'t')],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr);print(r.stdout.strip())
 def test_no_windows_macro_names(self):
  h=text('TVS_Ventilation_ARX/ActionTransaction.h')
  for w in ('near','far','small','min','max'):self.assertIsNone(re.search(r'\b%s\b'%w,h),w)
 def test_commands_are_one_undo_step(self):   # ACRX_CMD_MODAL: AutoCAD records the command as one UNDO step
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  for c in ('TVS_ERASE','TVS_RAZBIJ','TVS_TK_TIP','TVS_DIM_PRATI','TVS_Change','TVS_OFFSET'):
   m=re.search(r'ACED_ARXCOMMAND_ENTRY_AUTO\(CTVS_Ventilation_ARXApp, Ventilation_ARX, %s, %s, ([^,]*),'%(c,c),e);self.assertTrue(m,c)
   self.assertIn('ACRX_CMD_MODAL',m.group(1),c)
  self.assertIn('L"TVS_BYPASS",L"TVS_BYPASS",ACRX_CMD_MODAL',e)
  self.assertIn('#include "ActionTransaction.h"',e)
 def test_svojstva_settles_in_its_command(self):   # Svojstva + dionica = jedan UNDO korak
  s=text('TVS_Ventilation_ARX/commands/Edit.inl');g=s[s.index('static void Ventilation_ARXTVS_Change(void)'):s.index('static Acad::ErrorStatus postToDatabase(')]
  self.assertIn('if (tvsTeeDialog::forSelection(sset, IDD_TEETYPE)) { acedSSFree(sset); tvsSizeFollow::run(IDD_RUN, IDD_TEETYPE); return; }',g)
  self.assertLess(g.index('Change(sset);'),g.rindex('tvsSizeFollow::run(IDD_RUN, IDD_TEETYPE);'))
  self.assertLess(g.index('syncTVSGlobalUnits();'),g.index('tvsSizeFollow::run('))
 def test_explode_one_transaction_per_element(self):
  s=text('TVS_Ventilation_ARX/commands/Edit.inl');g=s[s.index('static Acad::ErrorStatus postToDatabase('):s.index('static void Ventilation_ARXTVSSomeParts(void)')]
  self.assertNotIn('acdbOpenObject(',g);self.assertNotIn('acdbOpenAcDbEntity(',g);self.assertNotIn('->close()',g)
  self.assertIn('tx.append(space, part, partId)',g)
  i=g.index('tvsAction::Transaction tx;');j=g.index('es = postToDatabase(tx, eSet, owner);')
  k=g.index('if (es == Acad::eOk) es = pEnt->erase();');l=g.index('if (es == Acad::eOk && !tx.commit()) es = tx.status;')
  self.assertTrue(i<j<k<l)
  self.assertIn('AcDbEntity::cast(tx.open(id, AcDb::kForWrite))',g)
 def test_tee_dialog_preflight(self):
  t=text('TVS_Ventilation_ARX/TeeTypeDialog.h')
  self.assertIn('inline bool writable(const Source& s){',t);self.assertIn('if(tvsAction::writable(ids))return true;',t)
  o=t[t.index('inline bool open(const AcDbObjectId& id,UINT idd){'):];self.assertLess(o.index('if(!writable(s))return true;'),o.index('dg.DoModal()'))
  a=t[t.index('inline bool openArriving('):];self.assertLess(a.index('if(!writable(s))return false;'),a.index('dg.DoModal()'))
 def test_dionica_shared_preflight(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h');self.assertIn('#include "ActionTransaction.h"',s)
  self.assertIn('if(!tvsAction::writable(w)){acutPrintf(L"\\nDionica nije promijenjena: element dionice je na zaključanom sloju',s)
  self.assertLess(s.index('// D7:'),s.index('if(!tvsAction::writable(w))'));self.assertLess(s.index('if(!tvsAction::writable(w))'),s.index('// write: etazi, elbows'))
if __name__=='__main__':unittest.main()
