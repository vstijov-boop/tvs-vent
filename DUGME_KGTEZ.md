# Dugme Masa kanala — KGTEZ

U `Installer/TVS_TOOLS/TVS_TOOLS.cuix` dodata je komanda KGTEZ, sa nazivom „Masa kanala“. Nalazi se u toolbaru, ribbon panelu i padajućem meniju Kalkulator. Providne ikone 16×16 i 32×32 su ugrađene u CUIX; kopije za održavanje nalaze se u `Installer/TVS_TOOLS/icons/`.

Makro prati konvenciju postojećih TVS_TOOLS LISP komandi: ako c:KGTEZ nije učitan, učitava LISP sa potvrđene putanje, pa šalje komandu KGTEZ. LISP nije dostavljen i nije dio repozitorijuma; njegov sadržaj nije mijenjan.

```text
^C^C^P(if (not c:KGTEZ)(load "C:/ProgramData/Autodesk/ApplicationPlugins/TVS.bundle/Contents/TVS_TOOLS/Resources/KGTEZ.lsp"));^PKGTEZ;
```

## Zamjena

1. Zatvoriti sve AutoCAD prozore.
2. Sačuvati trenutno ispravan TVS_TOOLS.cuix van foldera ApplicationPlugins.
3. Novi TVS_TOOLS.cuix kopirati u `C:/ProgramData/Autodesk/ApplicationPlugins/TVS.bundle/Contents/TVS_TOOLS/Resources`, uz zamjenu postojećeg.
4. KGTEZ.lsp ostaje u tom istom folderu, kao na korisnikovoj slici. ARX/DBX i TVS_VENT.cuix se ovom izmjenom ne mijenjaju.
5. Pokrenuti AutoCAD i kliknuti teg „Masa kanala“ u Kalkulatoru. Ako je ostao stari izgled/ikone, pri zatvorenom AutoCAD-u preimenovati TVS_TOOLS.mnr u TVS_TOOLS.mnr.old pa ponovo pokrenuti AutoCAD. Ako je potrebno osvježavanje učitane grupe, koristiti CUILOAD za TVS_TOOLS.cuix.

## Provjere

ZIP CRC i parsiranje svih XML/CUI dijelova prolaze. Svih 14 ranijih makroa, njihova polja, reference dugmadi i 15 ranijih bitmapa su očuvani. Novi makro ima po jednu referencu u toolbaru, meniju i ribbonu; provjerene su interne veze i obje ugrađene RGBA PNG ikone. Izgled ikona pregledan na svijetloj i tamnoj pozadini. Nije bilo izmjena u programskom kodu niti potrebe za kompilacijom ARX/DBX. Učitavanje CUIX-a i pokretanje korisnikovog KGTEZ.lsp još treba potvrditi u AutoCAD-u.

Ikona je izvedena iz odobrenog predloga generisanog ugrađenim alatom za slike: metalni teg sa omčom i natpisom „kg“, čeličnosiva i plava boja, providna pozadina. Izvršeno je prilagođavanje dimenzijama dugmadi bez promjene motiva.

SHA256 CUIX: `2d954611207be73f4093eed62bffd94dca96e0039c74649647bceab0d3e52600`.
