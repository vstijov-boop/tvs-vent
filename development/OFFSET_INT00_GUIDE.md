# INT-00 — samostalni i zajednički offset (DEV)

Odobren je samo INT-00. Nema trajnog bypass-a, nove bypass/parent šeme, povezanog
detalja ili kružnih presjeka. `TVS_OFFSET_MEMBER` v1 i fizički obračun su isti.
Nova komanda `TVS_OFFSET` postoji samo uz `TVS_OFFSET_INTEGRATION` u DEV ARX/DBX paru.
Produkcioni bundle ne prepisivati. Novu verziju učitati poslije zatvaranja i
ponovnog pokretanja AutoCAD-a, DBX pa ARX odgovarajuće godine.

## TVS_OFFSET

1. U kopiji probnog DWG-a, Model prostor, izabrati obični pravi horizontalni kruti
   pravougaoni TVS kanal. Podesiti mm/cm/m i konstantni Ri ili k*B kroz postojeće
   TVS postavke. Za referentni 400x200, Ri=100 koristiti k=0.5.
2. Izabrati `Prvi` ili `Posljednji` slobodan kraj. Suprotni kraj i originalni
   identitet objekta ostaju isti; original se samo skraćuje na izabranom kraju.
3. Postojeći dijalog prikazuje početnu apsolutnu kotu iz stvarnog kanala. Ne
   mijenjati početnu kotu; zadati završnu apsolutnu kotu i ugao. Dijalog je u mm
   nezavisno od crtačkih jedinica. Ri se razrješava iz važećih TVS postavki.
4. Zeleni transient pregled pokazuje buduću projektovanu osu i spojeve; žuti tekst
   daje njihove apsolutne fizičke kote. Original je još vidljiv. Komandna linija
   daje A/B, Ri, ugao, P, rise i fizički L. Ovo nije prikaz punih limenih kontura
   niti potvrda prostornog izbjegavanja drugih tijela.
5. `Primijeni` potvrđuje jednu atomsku operaciju. Enter, `Otkazi` ili Esc otkazuju.
   Izmijenjen izvor/svojstva/jedinice/veze ili zaključan sloj se ponovo provjeravaju
   prije upisa. Poslije uspjeha postoje dva koljena, kosi član i izlazni nastavak
   dužine 300 mm na novoj koti; originalni ostatak zadržava svoj identitet.

Samostalna komanda ne mijenja aktivnu trasu, globalnu kotu/ugao ili režim crtanja.
Ne pokušava nastaviti crtanje umjesto korisnika. Povezan izabrani kraj se odbija,
uključujući stvarni geometrijski kontakt bez eksplicitnog connectionID.

## Postojeća opcija Z — oba pozivaoca

`TVS_PIPE` i `TVS_DRAW` / drawStep3 i dalje nude `Z`. Oba pozivaju isti interact,
snapshot, fizički plan, adapter, validaciju i writer kao samostalna komanda.
Sada i Z daje pregled i eksplicitnu potvrdu. Samo nakon Applied adapter crtanja
postavlja izlazni objekt, novu globalnu kotu/ugao i A1/A2/pt1. Cancelled, NoChange
i Failed ne mijenjaju taj nastavak. Jednake kote ne kreiraju objekte.

Referenca za poređenje: 400x200, Ri=100, -400 → 0 mm, 45°:
P=282.842712474619 mm, riseStraight=282.842712474619 mm, L=400 mm.
Količina kosog člana 0.4 m, površina 0.48 m², masa 2.2608 kg pri 0.6 mm bez rezerve.
To je testni orakul, ne tvrdnja da su svi runtime potrošači ovdje izmjereni.

## Konektori, smjer i svojstva

- Izabrani kraj mora biti slobodan i bez geometrijskog kontakta. Dvosmislene,
  jednosmjerne, pogrešno tipizirane i netangentne veze se odbijaju. Unutrašnja
  račva/priključak na izabranom kanalu se odbija. Susjed se ne otvara za upis.
- Suprotna eksplicitna veza prve verzije podržana je za recipročni pipe port;
  geometrijski položaj/kota/presjek/smjer moraju se slagati. Poznati implicitni
  priključci postojećih ravanskih fittinga i offset koljena mogu ostati implicitni.
  Nedovoljno opisiva TVS geometrija u Model-u se konzervativno odbija.
- Kod Prvi se originalni FirstPoint skraćuje; LastPoint ostaje. Kod Posljednji
  obrnuto. Originalni endpoint redosljed i konektorske uloge nikad se ne zamjenjuju.
  Novi članovi orijentisani su od fiksnog kraja ka izabranom izlazu. Flow je ista
  nenegativna skalarna vrijednost. Direct različit od 0 i poseban Form nisu podržani;
  ne nagađa se semantika smjera niti negira protok.
- Prikazna svojstva preko AcDbEntity setPropertiesFrom, Flow, Grani, Wipeout,
  WipeoutLength i This1D prenose se novim članovima. Materijal je AcDbEntity
  materijal; lim/debljina/masa se i dalje izvode postojećim fizičkim obračunom.
- TVS_Property proizvođač i zajednička napomena prenose se. Jedinstvena pozicija
  ostaje samo na originalu. Ručni naziv/tip/artikal/presjek/jedinica, nenulta ručna
  količina/masa, nepoznate XData aplikacije ili neprazan extension dictionary
  odbijaju operaciju. Ništa se ne briše ili tiho preimenuje. Postojeći MLeader
  ostaje vezan za originalni ostatak, ne proizvoljan novi član.
- Legacy TVS_Pipe/TAP filer ne snima konektorski vektor. Ova faza to ne mijenja i
  ne obećava novu trajnu topološku šemu. Originalni vektor ostaje netaknut,
  uključujući suprotni priključak; null-ID free-port cache nije autoritet za položaj.
  Geometrijski portovi dolaze iz stvarnih tačaka/parametara, bez redraw keša.
  Novi neoznačeni izlaz i susjedi zato i dalje koriste legacy implicitne kontakte.

## Probe za pregled isporuke — ne izvršavati Redo

Na kopiji provjeriti oba kraja i oba smjera kote, 30/45/60, A!=B, mm/cm/m i rotiran
UCS. Porediti standalone i Z za iste fizičke ulaze. Z posebno kroz TVS_PIPE i
TVS_DRAW: nakon Applied nastaviti sljedeći segment sa stvarnog izlaza/nove kote;
nakon Cancel/NoChange/Failed nastaviti sa starog kraja/kote.

Negativne probe: povezan izabrani kraj, pokvarena recipročna veza, nepoznati
atributi, premala visina/kratak ulaz, 90°, zaključan sloj i promjena ulaza tokom
preview-a. Potvrditi bez novih članova ili promjene originala/susjeda.

`TVS_OFFSET_FAIL_NEXT` priprema jedan razvojni prekid sljedećeg potvrđenog offset/Z
upisa nakon novih članova, v1 zapisa i skraćenja izvora, prije commita. Abort mora
vratiti i originalne tačke; nema novih objekata/zapisa. Rani Cancel/odbijanje ne
troši injekciju. Ovo nije Undo/Redo proba. Za reset bez uspješnog pokušaja restart.

Runtime provjere evidentirati zasebno od SDK kompilacije i testova sa zamjenama.
STAB-01 ostaje odložen; nema novih Redo istraga/proba i reaktori se ne mijenjaju.
AutoCAD 2020 i 2022 runtime status ne izjednačavati s build prolazom.

## Granice

Samo Model, obični TVS_Pipe, kruti pravougaoni, horizontalna crtačka ravan,
mm/cm/m, pozitivan Ri i podržani ugao bez 90°. DEV minimumi: Lmin=100 mm,
minProjection=0.1 mm, ostatak ulaza najmanje 1 mm. Nema mrežnog pomjeranja,
parametarskog editovanja stare trojke, novih presjeka, automatskog povezivanja
dokumentacije ili proračuna kolizije cijele okoline. COPY ograničenja stare
integracije i statičkih detalja ostaju ista.

Prije buduće faze 1 potrebno je kratko poređenje ranije prihvaćenog roditelja nad
dvije v1 trojke i H naspram posebnog sedmočlanog ugovora, pa preporuka i odobrenje.
Plan iz 80c53c6 nije odobrenje promjene tog smjera; nijedna bypass šema nije uvedena.
