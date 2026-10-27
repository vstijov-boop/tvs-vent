# Prevod komandne linije na srpski latinicom

Prevedeni su aktivni upiti za izbor objekata, unos tačaka, širina i dužina, poruke o zaključanim objektima, greške i status izvoza. Globalni nazivi komandi (TVS_PIPE, TVS_CONNECTT itd.) ostaju isti.

Primjeri:

- `TVS_CONNECTT`: `Izaberite prvi kanal:` / `Izaberite drugi kanal:`
- `TVS_PIPE`: `Zadajte prvu tačku ili [Dimenzija]:`
- Nastavak crtanja: `Zadajte sljedeću tačku ili [Dimenzija/Koljeno/Račva/Z/Gore/Nadolje]:`

| Opcija | Prečica | Radnja |
| --- | --- | --- |
| Dimenzija | D | Promjena dimenzije kanala |
| Koljeno | K | Spajanje koljenom |
| Račva | R | Spajanje račvom |
| Z | Z | Promjena kote |
| Gore | G | Vertikalno nagore |
| Nadolje | N | Vertikalno nadolje |
| Podešavanja | P | Podešavanja oznake ili specifikacije, u odgovarajućoj komandi |

Prečice se prihvataju malim i velikim slovima. Registracija opcija i poređenje dobijenog unosa koriste iste nazive. Ruske opcije i stare prečice ruskog rasporeda tastature zamijenjene su ovim opcijama; postojeće skripte koje ih šalju treba prilagoditi. Izbor opcija obrađuje se samo kada AutoCAD vrati RTKWORD, da unos koordinata ne ponovi prethodnu opciju. Podešavanja specifikacije registruju se pri svakom ponovnom upitu.

Komentari, interni ključevi svojstava i proračuni ostaju neizmijenjeni. `[P]` u formatu višestruke oznake i dalje nema implementiran jedinični pad pritiska; prečica P za Podešavanja je odvojena funkcija.

Provjera: pregled svih aktivnih poziva komandnih API-ja u ARX/DBX bez ruskih literala, očuvani komentari i argumenti formatiranja, izvršena provjera svih 16 stvarnih uslova obrade opcija (puni nazivi, prečice, mala slova, nepoznata opcija i unos koordinata). Kompilacije i konačna provjera rada komandi u AutoCAD-u evidentiraju se odvojeno.
