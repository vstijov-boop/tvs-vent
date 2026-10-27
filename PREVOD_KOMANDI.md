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

## Desni klik pri crtanju kanala

Ruske stavke sa korisnikove slike (`Размер`, `соедОтвод`, `соедТройник`, `Z`, `вВерх`, `вНиз`) odgovaraju opcijama tekućeg komandnog upita. U oba puta crtanja ti upiti i acedInitGet sada koriste srpske nazive iz gornje tabele; za ove stavke nije pronađen zaseban TVS kontekstni meni. Standardne stavke AutoCAD-a (Enter, Cancel, Recent Input, Pan, Zoom...) zavise od jezika samog AutoCAD-a. Prikaz i aktiviranje srpskih opcija desnim klikom još treba potvrditi u AutoCAD-u.

## Uspješne kompilacije 27.10.2026.

Izvorni commit: `b5a3d9565e6bd28b808b1956622b6ff0ba143b1d`.

- [AutoCAD 2020 x64 — paket sa prevodom komandi](https://github.com/vstijov-boop/tvs-vent/actions/runs/40523027607/artifacts/12742961791). SHA256: `cabc36034cf5f286097ff37dc9a11bb110e837ada1b5872d086486d9952fe956`.
- [AutoCAD 2022 x64 — paket sa prevodom komandi](https://github.com/vstijov-boop/tvs-vent/actions/runs/40523027599/artifacts/12742512593). SHA256: `d4d1412d9d19c2b16a1b00a9b4d88ce6a45c6760d3dcd4f092943b3811f22729`.

Obje kompilacije imaju 0 grešaka i po 2 LNK4099 upozorenja o nedostajućim Autodesk PDB simbolima. Preuzeti ZIP-ovi provjereni su po SHA256 i CRC; ARX i DBX su x64 i koriste acdb23.dll za 2020, odnosno acdb24.dll za 2022. U oba ARX-a potvrđeni su srpski upiti i registrovane srpske opcije sa dijakriticima. AutoCAD nije dostupan na razvojnom okruženju, pa konačna provjera desnog klika i rada opcija ostaje korisniku.

Pri zatvorenom AutoCAD-u sačuvati trenutno ispravnu instalaciju i zamijeniti ARX i DBX zajedno u odgovarajućem `Contents/TVS_VENT/2020/win64` ili `Contents/TVS_VENT/2022/win64` folderu postojećeg TVS.bundle. CUIX fajlovi se ovom izmjenom nisu mijenjali. Probati TVS_PIPE: D prije prve tačke, D/K/R/Z/G/N pri nastavku crtanja i odgovarajuće stavke desnog klika; TVS_CONNECTT: izbor prvog/drugog kanala; P u oznakama i specifikaciji, uključujući ponovni poziv Podešavanja.
