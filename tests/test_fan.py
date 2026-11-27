"""Ventilator u mrezi (korisnik 27.11.): usis / potis rucno sa privremenim natpisima, TVS_PROTOK / TVS_PAD
po stranama (usis = odvod, potis = dovod) i zbir; niz opreme (stavka 27) kanal - FV - ventilator - FV - kanal."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Fan(unittest.TestCase):
 def test_record(self):
  e=text('geometry/Equipment.h')
  self.assertIn('Louvre=9,Fan=10};',e);self.assertIn('inline int kindCount(){return 11;}',e)
  self.assertIn('if(r.suction>=0&&r.ports==2)put(L"usis",num(r.suction));',e)
 def test_labels_and_command(self):
  c=text('TVS_Ventilation_ARX/FanCommand.h')
  self.assertIn('L"USIS",L"POTIS"',c);self.assertIn('[Prihvati/Zamijeni] <Prihvati>',c);self.assertIn('addTransient(e,kAcGiDirectShortTerm',c)
  a=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');self.assertIn('TVS_VENTILATOR, TVS_VENTILATOR, ACRX_CMD_MODAL, NULL)',a)
  self.assertIn('tvsFan::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  l=text('TVS_Ventilation_ARX/EquipmentLinkCommand.h')
  self.assertIn('tvsFan::isFan(r)&&r.suction<0)tvsFan::chooseSide(linkedNow[k]);',l)
  self.assertIn('if(rowOf[k]>=0){if(rowDone.insert(rowOf[k]).second)linkRow(rowOf[k]);continue;}',l)
  self.assertIn('lk::chain(feet,r,o.tol,parts)',l);self.assertIn('if(leansWrong(r,index)){refuse(t,id,name,NotLeaning,0,false);',l)
  self.assertIn('for(size_t k=0;k<lean.size();++k){out.push_back(lean[k]);index.push_back(-1);}',l);self.assertIn('if(lk::dot(dir,lk::sub(c,e.at))<=0)continue;',l)
  self.assertIn('keepSuction(old,rec);',l)
 def test_sides(self):
  f=text('TVS_Ventilation_ARX/FlowCommand.h')
  self.assertIn('out[i].ports[fanCut().port].x+=1e9;',f);self.assertIn('FanScope scope(fanId,side==0?suction:1-suction);',f)
  self.assertIn('if(fanCut().port>=0&&net.nodes[root].id==fanCut().id)port=fanCut().port;',f)
  p=text('TVS_Ventilation_ARX/PressureCommand.h')
  self.assertIn('tvsFlow::FanScope scope(fanId,k==0?1-suction:suction);',p)
  self.assertIn('compute(n,r0,tvsFlow::fanCut().port,k==0,',p)
  self.assertIn('book=tvsSheet::fanBook(tvsSheet::build(res[0],iu,ou,L"usis"),tvsSheet::build(res[1],ip,op,L"potis"),info,q[0],q[1],tag);',p)
  self.assertIn('if(static_cast<int>(i)==root&&net.linked[j]&&tvsFan::isFan(net.records[j]))continue;',p)
if __name__=='__main__':unittest.main()
