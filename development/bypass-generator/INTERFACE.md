# Generator provjereno izvodljivog obilaska — razvojni interfejs

Odobreni obim: izolovani C++11 model, bez ObjectARX-a, CAD pregleda, komande, dijaloga, trajnih podataka ili izmjene postojeće Z integracije. Ovo nije formalno verifikovana biblioteka niti dokaz rada u AutoCAD-u.

## Interfejs

`geometry/bypass/Generator.h`, namespace `tvsBypassSearch`:

```cpp
tvsBypass::Input problem(100, 100, .1); // eksplicitni minimumi u fizičkim mm
problem.angleDegrees = 45;            // korisnički izbor: samo 30, 45, 60
problem.windowLeft = -700;
problem.windowRight = 700;
tvsBypassSearch::Policy search(5, 10, 25);
// Dodatna projektna rezerva 5 mm; koraci predlaganja D=10 mm i C=25 mm.
// To nijesu numeričke tolerancije niti standard/proizvodna politika.
tvsBypassSearch::Result r = tvsBypassSearch::generate(problem, search);
if (r.hasCandidate) {
    // r.status == Found; r.checked.status == tvsBypass::Feasible
    // r.candidate može se nezavisno ponovo predati Candidate.check().
}
```

Ulaz je postojeći `tvsBypass::Input`: fizički presjek, Ri, izabrani ugao, stvarna kota ose z0, položaj i gabarit prepreke, izolacije, obavezni clearance, iznad/ispod, minimumi pravih komada i projekcije, raspoloživi prozor i opcione visinske granice. `D/C/s0` u tom ulazu se ignorišu i biraju pretragom; mogu čak biti NaN. Neaktivne floor/ceiling vrijednosti takođe se ignorišu. Aktivne granice moraju biti konačne.

Nema dodatnog A/2 ili B/2 pomaka referentne ose. Nije uveden XY režim vođenja, niti je mijenjan TVS_SETTINGS.

Rezultat:
- `Found` uz `hasCandidate=true`: najbolji sačuvani, potvrđeni kandidat i kompletni postojeći `checked` Result sa oba offset rezultata, sedam tijela, spojnim/osnim tačkama i dijagnostikom;
- `NoCandidateFoundWithinBudget`: konačna, ograničena pretraga nije našla potvrđeni kandidat; ovo nije dokaz neizvodljivosti;
- `Unresolved`: nije nađen potvrđeni kandidat, a zabilježena je numerička/granična nerazriješenost ili aritmetički izuzetak;
- `InvalidInput` / `Unsupported`: nevažeći ulaz ili nepodržan geometrijski/numerički domen.

`D,C,s0,W,passElevation,clearance` važe samo kada je hasCandidate=true. W i kota prolaza su postojeće intervalne vrijednosti. `clearance` obuhvata ogradu rastojanja provjeravača proširenu njegovom ogradom nesaglasnosti spojeva: [max(0, distance.lo-guard), distance.hi+guard], uz spoljašnje zaokruživanje. Čuvaju se attempts, proposals, seedSolveCalls, processedCells, improvements, elapsedMs i tekst termination. `minimumConfirmed` je uvijek false.

Ako nakon uspjeha dođe do budžetskog prekida, status ostaje Found, a termination objašnjava prekid. Za neuspjeh nema generičkog Infeasible statusa. Testovi poznato nemogućih primjera imaju nezavisan geometrijski dokaz; generator taj dokaz ne generalizuje.

## Algoritam i budžet

1. Validacija ulaza i postojećeg numeričkog okruženja.
2. Konzervativni početni prijedlog D iz vertikalnog zazora; doda se eksplicitni depthStepMm. Ovo je prijedlog, ne dokaz kolizije ili izvodljivosti.
3. Postojeći, neizmijenjeni solve() traži podržan početni offset, uz ograničen broj poziva. Po potrebi D raste za max(depthStepMm,D/2), do eksplicitne dubinske/visinske granice. Time se može preskočiti uži izvodljiv interval; nema tvrdnje o potpunosti pretrage.
4. Konačni skup dubina, srednjih komada i položaja. C prijedlozi: prostraniji, širina izolovane prepreke, minimum, srednji i povećani prostraniji. Pozicioniranje uključuje centriranje ograničeno prozorom, sredinu raspoloživog intervala, obje granice sa eksplicitnim inset-om i dodatne podjele.
5. Svaki kandidat za prihvatanje prolazi postojeći Candidate.check(), bez promjene clearance-a ili politike solve(). NumericalBoundary se nikada ne prihvata. Pored Feasible, donja ograda ostvarenog zazora mora dostići clearance+designReserveMm.
6. Podrazumijevano se staje na prvom potvrđenom rješenju. Opciono poboljšavanje čuva prethodni najbolji rezultat.

Podrazumijevani razvojni budžeti: 8 dubina, 5 C prijedloga, 5 položaja; 64 pripremna solve poziva, 512 kandidata za provjeru, 64 checker poziva, 300000 obrađenih ćelija, 5000 ms. Nivoi su dodatno ograničeni na 128/16/16. Maksimalna pretraživana dubina 100000 mm, inset prozora 1 mm; sve su javne postavke politike. Nula za budžet znači da taj rad nije dopušten.

attempts i proposals broje stvarno predate checker prijedloge, ne odbačene kombinacije koje ne staju u prozor. seedSolveCalls broji direktne pripremne solve pozive; dva unutrašnja poziva u svakom Candidate.check() nijesu ponovo uračunata. processedCells sabira postojeći obstacle.cells iz svakog poziva; preostali ćelijski budžet ograničava sljedeći poziv. To nije mjera svih aritmetičkih operacija: rad validacije, testovi parova i supportSlices takođe koštaju i ograničeni su svojim politikama i brojem poziva.

Vrijeme se mjeri steady_clock-om. Rok se provjerava između operacija; nije tvrdi real-time prekid aktivnog Candidate.check(). Jedan takav poziv može prekoračiti vremenski rok. Ćelijski i pozivni limiti i dalje važe. Vrijeme ranog odbijanja nepodržanog floating-point okruženja je 0: namjerno se ne radi FP aritmetika tajmera u tom okruženju.

## Ciljevi i numerička ograničenja

- Clearance je obavezno ograničenje. Izabrani ugao je ulaz. Poboljšavanje W je opcioni cilj.
- Dodatna rezerva je eksplicitna projektna politika; ne mijenja zahtijevani clearance niti numeričku toleranciju.
- improvementWToleranceMm i improvementDToleranceMm su odvojeni, podrazumijevano .01 mm.
- Kraći W se prihvata samo kada spoljašnje izračunata razlika W to potvrđuje uz W prag. Sekundarno manji D se bira samo pri tačno dokazanoj jednakosti modelskih W vrijednosti; bliskost nije zamjena za jednakost.
- Preklapajuće ograde za poređenje W ne dokazuju globalni minimum. Nema guste globalne pretrage, sertifikata optimuma ili tvrdnje da ograničeni skup obuhvata sva moguća rješenja.
- Predlaganje je obično double računanje. Dokaz prihvatanja dolazi iz postojećeg intervalnog provjeravača i njegove podržane numeričke politike; ovo ne pretvara cijelu biblioteku u formalno sertifikovan intervalni sistem.
- Ri mora biti veći od izolacije; pravougaoni kruti kanal, ista širina/visina, četiri jednaka glatka koljena bez nastavaka, horizontalni ulaz/izlaz na istoj koti, jedna upravna prepreka sa potvrđenim poprečnim preklapanjem.
- Potvrda važi za geometrijski model i pregledana kompajlerska okruženja; nema CAD runtime provjere ili provjere stvarne proizvodnje fitinga.

Stari matematički solver, intervalno jezgro i Candidate.h nijesu mijenjani ovim korakom.
