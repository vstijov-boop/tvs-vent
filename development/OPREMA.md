# Oprema na mreži

Obični AutoCAD 2D blokovi (regulator protoka, PP klapna, CAV, VAV, prigušivač buke = inline, 2 priključka; rešetka, difuzor, krovna kapa, protivkišna žaluzina = terminal, 1 priključak) postaju dio TVS kanalske mreže. Blok ostaje kakav jeste; TVS u njega upisuje zapis (geometry/Equipment.h, TVS_Ventilation_ARX/EquipmentStore.h): vrsta, oznaka, priključci (tačke na osi kanala, kota = osa kanala), dimenzija, model pada pritiska (fiksni Δp, ζ, kataloška tačka Δp pri Qn), protok terminala.

## Oprema u proračunima mreže (TVS_PROTOK, TVS_PAD, TVS_DPD_IZVOZ, TVS_DIM_PRATI, TVS_PROVJERA)
- Čitanje: TVS_Ventilation_ARX/EquipmentNodes.h dodaje povezane blokove trenutnog prostora mreži; priključci se preračunavaju u jedinice crteža, a z priključka uzima z kraja kanala na koji naliježe u osnovi (kanali nose kotu u Elev). Pravila: geometry/EquipmentFlow.h.
- **TVS_PROTOK**: terminal ima protok iz zapisa; bez njega protok kanala na kome stoji (ranije upisan); oba i različita → upozorenje, važi oprema. Protok terminala se upisuje u zapis opreme, ne u kanal. Inline oprema nema protok.
- **TVS_PAD**: inline oprema je lokalni gubitak unutar dionice (kao koljeno, ne počinje novu dionicu, bez dužine), Δp po modelu (tvsEquipment::loss, pd kanala na ulazu). Terminal završava dionicu, njegov Δp je „Δp kraj. elem.“ (Fitinzi red bez ζ, Izvor „Oprema“), bez rezerve; oznaka terminala je natpis kraja u Šemi. „Za ventilator“: rezerva se ne primjenjuje na Δp krajnjih elemenata.
- **TVS_DPD_IZVOZ**: oprema ide kao oprema na DuctPressDrop dionici (Fiting.oprema = tip iz palete, položaj, oznaka): terminal na slobodnom kraju, inline na svom mjestu; fiksni Δp → dp_fix, ζ → konst / ručni ζ, katalog → dp_fix izračunat pri izvezenom Q (u DuctPressDrop-u fiksan, poruka).
- **TVS_DIM_PRATI**: povezan blok je granica dionice („oprema“), dimenzija se kroz njega ne prenosi.
- **TVS_PROVJERA**: „oprema: priključak ne naliježe na kanal“ – priključak nije na kraju kanala iste dimenzije i kote (1 mm), sa zumom.
- Ne radi (još): TVS_MOVE / TVS_STRETCH ne vide blokove; TVS_PROTOK „Primijeni“ (dimenzionisanje) ne mijenja opremu – granica kao nepoznat element.
