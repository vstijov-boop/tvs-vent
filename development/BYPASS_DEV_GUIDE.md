# DEV pravougaoni bypass

Radni tok: **nacrtaj → izaberi prepreku → pronađi → prihvati → nastavi crtanje**.

Učitati DBX pa ARX iste godine iz ovog izdvojenog paketa u novoj AutoCAD sesiji.
Ne prepisivati produkcioni bundle i ne koristiti hot unload/reload.

1. TVS postavke: mm/cm/m; kruti pravougaoni kanal. Nacrtati pravolinijski kanal
   kroz područje prepreke i dovoljno iza nje: kandidat mora stati unutar njegovih
   krajeva, uz ostatak najmanje 1 mm na oba kraja.
2. Samostalno: `TVS_BYPASS`, izabrati kanal, prepreku i Prvi/Posljednji smjer.
   Tokom `TVS_PIPE` ili `TVS_DRAW`: opcija `Obilazak` / `O`; izabrati samo prepreku.
3. Postojeći dijalog: potvrditi stvarne jedinice, eksplicitno unijeti obje izolacije
   (0 ako ih nema), Ri, razmak i ostale ulaze; `Pronađi prolaz`.
4. `Pregled u crtežu` je privremen; komandna linija nudi `[Prihvati/Dijalog/Odustani]
   <Dijalog>` (u TVS_BYPASS_PREVIEW samo `[Dijalog/Odustani]`). **`Prihvati`** ili
   **`Prihvati obilazak`** trajno upisuje isti provjereni kandidat, bez nove pretrage. Izmjena ulaza uklanja
   važenje rezultata; promijenjen kanal/prepreka ili veze odbijaju upis.
5. U crtanju poslije uspjeha odmah zadati sljedeću tačku. Nastavak je sa sačuvanog
   spoljašnjeg kraja i početne fizičke kote. Cancel/neuspjeh čuva dotadašnji nastavak.

Nastaju četiri koljena, dva kosa člana i H, dvije neizmijenjene V1 trojke i njihov
roditelj. Originalni kanal ostaje jedan spoljašnji ostatak; drugi ostatak dobija
novi objekt. Oba spoljašnja položaja/kota/presjek ostaju isti. Geometrija susjeda
i prepreka se ne mijenja. Roditelj čuva granične reference koje već postoje.

**Konektori (razvoj-bypass-fixed).** Legacy konektori nisu filed podaci
(`TVS_Pipe::dwgOutFields/dwgInFields` ih preskaču), pa ih ni korisnički UNDO ni
snimanje/otvaranje ne vraćaju. Zato writer, kao i TVS_OFFSET, ne mijenja nijedan
konektor. Ako izabrani spoljašnji kraj ima eksplicitnu vezu (postavlja je samo
TVS_CONNECTTest ili raniji DEV bypass u istoj sesiji), bypass se odbija umjesto
prevezivanja. Obični nacrtani kanali i kanali iz ponovo otvorenog crteža nemaju
povezane konektore, pa za njih bypass radi. Kontinuitet unutar sklopa je geometrijski
i zapisan u roditelju, ne u konektorima; novi ostatak ima nepovezan par konektora.

Fizički obračun kosih članova ostaje postojeći V1 (sopstveni rise svakog člana).
H je horizontalni fizički kanal. Roditelj nema količinu; sedam članova se ne broje
ponovo kroz njega. `Umetni detalj…` ostaje poseban statički nepovezan snimak.
`TVS_BYPASS_PREVIEW` zadržava raniji preview i statički detalj; za trajno prihvatanje
koristiti TVS_BYPASS ili Obilazak tokom crtanja.

Ograničenja: obični direktni horizontalni pravougaoni kruti TVS kanali, Model,
upravna prepreka i pun poprečni obuhvat postojećeg checker-a, 30/45/60, mm/cm/m.
Bez pomjeranja mreže, redukcija ili kružnih kombinacija. Dvosmislene veze,
nepoznati atributi i zaključani objekti odbijaju se kao kod offset servisa.
COPY/WBLOCK/INSERT kloniranje novih bypass članova je odbijeno u ovoj DEV verziji;
time se izbjegava kopija koja bi ostala vezana za originalnog roditelja. Stari
samostalni offseti zadržavaju svoj postojeći clone ugovor.
Kraj ostatka koji već dodiruje postojeći bypass roditelj ne smije se mijenjati;
njegov slobodni spoljašnji kraj može se koristiti za nastavak. Ako roditelj već
čuva eksplicitnu spoljašnju vezu tog ostatka, ili izabrani kraj susjeda predstavlja
njegovu granicu, novi bypass se odbija umjesto neprimjetnog kvarenja stare reference.

**Pomjeranje sklopa: cijeli sklop ili ništa.** MOVE, ROTATE oko Z i grip pomjeranje
članova obilaska/offseta dozvoljeni su samo kad su u istom izboru svi članovi sklopa
(obilazak: 4 koljena, 2 kosa komada i H; samostalni offset: V1 trojka). Djelimičan izbor,
SCALE, MIRROR i nagib se odbijaju jednom porukom na kraju komande. Ulaz/izlaz i susjedi su
obični kanali; pomjeraju se samo ako su izabrani. `TVS_BYPASS_DETALJ` računa prepreku po
trenutnom položaju i javlja kada se razlikuje od upisa.

**Skrivene ivice ukrštanja (GFX-01, varijanta 1 — samo bypass).** Writer na donji
pravi kanal ukrštanja upisuje `TVS_HIDDEN_BY_V1` (iznad: prepreka ispod H; ispod: H ispod
prepreke), u istoj transakciji. Pri crtanju se dio ivica donjeg kanala koji je u osnovi
pokriven gornjim crta isprekidano i tanko (`tvs_hidden`). Gornji kanal se čita uživo;
obrisan gornji ili donji kanal iznad gornjeg vraća pune ivice. Pomjeranje gornjeg kanala
osvježava donji tek na REGEN. Opšta vidljivost između proizvoljnih kanala nije uvedena. Statički bočni detalj je dostupan; povezani/osvježavani detalj nije uveden.

Za rollback probu u potrošnoj kopiji: prije pokretanja AutoCAD-a PROCESS env
`TVS_BYPASS_FAIL` postaviti na 1 (sedam članova), 2 (ostatak i prevezivanje),
3 (roditelj/backlink-ovi prije commita). Svaki prekid mora ostaviti izvor i susjede
bez promjena i bez novih članova/zapisa. Ukloniti env i restartovati poslije probe.
To nije Undo/Redo proba; STAB-01 ostaje odložen.

Korisnik je prijavio da INT-00 Z funkcioniše dobro. Verzija AutoCAD-a i pojedinačne
runtime stavke te korisničke probe nisu potvrđeni i ne pretpostavljaju se.

## Zaostali ili oštećeni roditelj — ciljana ispravka poslije aa12514

Read-only provjera odvaja valjanost članova od veze sa izabranim kanalom.
Potpun poznati V1 raspored referenci bez reference na izabrani kanal ne blokira
nepovezan kanal ni kada su članovi obrisani ili je roditelj ostao bez cijelog obilaska.
Pozitivna referenca iz nevažećeg zapisa konzervativno odbija operaciju.
Ako roditelj nije čitljiv ili je raspored referenci nepotpun/nepoznate verzije,
poruka VEZA NEUTVRDJENA navodi ključ roditelja i status: nezavisnost se ne može
dokazati i operacija se odbija. To može blokirati i nepovezan kanal u potpuno
oštećenom DWG-u; provjera ne prećutkuje niti automatski briše/popravlja taj zapis.
Automatski cleanup nije uveden. Nove runtime probe brisanja/save-reopen ostaju
za praktični test; regresije koriste SDK zamjene. TVS_OFFSET ne poziva pinned();
jegov postojeći servis i provjere nisu mijenjani ovom ispravkom.

## Prihvati — ispravka ponovnog kreiranja rječnika

Korisnička AutoCAD 2022 proba prijavila je BYPASS nije upisan poslije Prihvati.
Parent writer je ponovo pozivao createExtensionDictionary na šest V1 članova
koji već imaju TVS_OFFSET_MEMBER rječnik. SDK vraća eAlreadyInDb; transakcija se
prekidala. Writer sada koristi postojeći rječnik, a kreira ga samo gdje nedostaje.
Nova regresija izvršava stvarni parent writer, čuva postojeće V1 zapise i provjerava
roditelja sa svih sedam backlink-ova. Novi paket zahtijeva svježu AutoCAD sesiju;
praktično prihvatanje nove verzije još nije potvrđeno.

## razvoj-bypass-fixed

Konektori, dijagnostika, ciljane izmjene i praktična checklista:
[BYPASS_FIXED_PROVJERE.md](BYPASS_FIXED_PROVJERE.md). Rollback proba (`TVS_BYPASS_FAIL`)
radi samo u DEV paketu sa `TVS_BYPASS_FAULT_INJECTION`.

## TVS_BYPASS_DETALJ — detalj postojećeg obilaska

`TVS_BYPASS_DETALJ` → klik na bilo koji član obilaska (koljeno, kosi član ili H).
Komanda provjerava roditelja i geometriju članova, rekonstruiše presjek istim checker-om
na zapisanim D, C i s0 (bez nove pretrage), uz zapisane izolacije/razmak/rezervu i
prepreku kakva je bila pri prihvatanju, i provjerava poklapanje sa članovima na 0,001 mm.
Zatim slijedi uobičajeno umetanje: oznaka, kotiranje, tačka. Detalj je statički snimak,
nije vezan za sklop; poslije izmjene obilaska umetnuti novi. Ri i ugao se čitaju iz
koljena; minimalne dužine nisu zapisane, pa se ne provjeravaju ponovo.
