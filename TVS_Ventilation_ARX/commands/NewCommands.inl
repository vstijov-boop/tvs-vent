#ifdef TVS_OFFSET_PROOF
#ifdef TVS_OFFSET_INTEGRATION
    static void Ventilation_ARXTVS_OFFSET(){syncTVSGlobalUnits();tvsOffsetIntegration::standalone();}   // jedinice ovog crteža
    // TVS_ETAZ_KOTA: etaz follows a changed duct elevation (run automatically, OffsetRefit.h).
    static void Ventilation_ARXTVS_ETAZ_KOTA(){if(ActivationErrorMessage()!=pError_Ok)return;syncTVSGlobalUnits();tvsOffsetRefit::run();}
    static void Ventilation_ARXTVS_OFFSET_FAIL_NEXT(){tvsOffsetIntegration::failNext()=true;acutPrintf(L"\nDEV: sljedeci potvrdeni offset/Z prekida transakciju poslije podataka i kraja izvora.");}
    static void Ventilation_ARXTVS_OFFSET_STATUS(){tvsOffsetDiagnostics::status();}
    static void Ventilation_ARXTVS_OFFSET_POLICY(){tvsOffsetDiagnostics::policy();}
    static void Ventilation_ARXTVS_OFFSET_TEST_BAD(){tvsOffsetDiagnostics::corruptTest();}
#endif
    static void Ventilation_ARXTVS_TEST_OFFSET(){tvsOffsetProof::create();}
    static void Ventilation_ARXTVS_TEST_OFFSET_CHECK(){tvsOffsetProof::check();}
    static void Ventilation_ARXTVS_TEST_OFFSET_FAIL(){tvsOffsetProof::create(true);}
#endif
    // Flex hose cuffs: marks ends that do not continue into flex (display + specification).
    static void Ventilation_ARXTVS_FLEX_MANZETNE(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsFlexCuffCommand::run();
    }
    // T komad TK-O / TK-G (faza 1: type of an existing TVS T-komad).
    // TVS_PRELAZ_KOTA: transition alignment by height; asks which side of the network stays.
    // TVS_IZVOZ: copy of the drawing with TVS elements replaced by their drawn shape (ExportCommand.h).
    static void Ventilation_ARXTVS_IZVOZ(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsExport::run();
    }
    // TVS_LEAD_AUTO: TVS labels of elements changed by a command get their text again after it
    // (TVS_DIM_PRATI, SizeFollow.h); on by default, kept in the AutoCAD profile (TVS_LEAD_AUTO env).
    static void Ventilation_ARXTVS_LEAD_AUTO(void)
    {
        const bool on=tvsSizeFollow::autoLabels();
        wchar_t prompt[160];swprintf_s(prompt,L"\nAutomatsko ažuriranje TVS oznaka poslije izmjene elementa [Uklj/Isklj] <%ls>: ",on?L"Uklj":L"Isklj");
        acedInitGet(0,L"Uklj Isklj");ACHAR kw[16]=L"";const int rc=acedGetKword(prompt,kw,16);
        if(rc!=RTNORM&&rc!=RTNONE)return;
        const bool next=rc==RTNONE?on:_wcsicmp(kw,L"Uklj")==0;
        acedSetEnv(L"TVS_LEAD_AUTO",next?L"1":L"0");
        acutPrintf(next?L"\nTVS oznake se ažuriraju same poslije izmjene elementa.":L"\nAutomatsko ažuriranje TVS oznaka isključeno (TVS_LEAD_Recalculate ručno).");
    }
    // TVS_DIM_PRATI: duct / transition sizes at their joints follow an edit (run automatically, SizeFollow.h).
    static void Ventilation_ARXTVS_DIM_PRATI(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsSizeFollow::run(IDD_RUN, IDD_TEETYPE);
    }
    static void Ventilation_ARXTVS_PRELAZ_KOTA(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsTransAlignCmd::run();
    }
    // TVS_NATPIS: on-screen duct captions size / flow / velocity on/off (DuctCaption.h, never plotted).
    static void Ventilation_ARXTVS_NATPIS(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsLevelTooltip::toggleCaptions();
    }
    // TVS_KOTE: on-screen elevation labels on/off (LevelLabels.h, not plotted).
    static void Ventilation_ARXTVS_KOTE(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsLevelTooltip::toggleLabels();
    }
    // TVS_PRESJEK: poprecni presjek of the TVS network (SectionCommand.h).
    static void Ventilation_ARXTVS_PRESJEK(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsSectionDialog::run(IDD_SECTION);   // dialog (SectionDialog.h)
    }
    // TVS_MATCH: TVS properties of a source duct onto selected elements (MatchCommand.h).
    static void Ventilation_ARXTVS_MATCH(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsMatch::run();
    }
    static void Ventilation_ARXTVS_TK_TIP(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsTeeDialog::run(IDD_TEETYPE);   // dialog (TeeTypeDialog.h); prompt flow stays for drawing (Racva)
    }
    static void Ventilation_ARXTVS_FLEX_MANZETNA(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsFlexCuffCommand::toggle();
    }
    static void Ventilation_ARXTVS_STRETCH(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsMove::executeStretch();
    }
    // Connected editing is modal so one command owns the complete undo operation.
    static void Ventilation_ARXTVS_MOVE(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsMove::execute();
    }
    // TVS_PROVJERA: network check with zoom to each problem (CheckCommand.h).
    static void Ventilation_ARXTVS_PROVJERA(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsCheck::execute();
    }
    // TVS_PROTOK: flows from the terminals summed to the unit (FlowCommand.h).
    static void Ventilation_ARXTVS_PROTOK(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsFlow::execute();
    }
    // TVS_PRERACUNAJ: elements pasted 1:1 from a drawing in other units -> this drawing's units (ScaleWindow.h).
    static void Ventilation_ARXTVS_PRERACUNAJ(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsScaleWindow::recalculate();
    }
    // TVS_KOLJENA_UGAO: elbows a little off 15 / 22,5 / 30 / 45 / 60 / 75 / 90 deg back to it (ElbowAngleCommand.h).
    static void Ventilation_ARXTVS_KOLJENA_UGAO(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsElbowAngle::execute();
    }
    // TVS_ALIGN: one-off alignment to a LINE / polyline segment / XLINE / RAY (AlignCommand.h).
    static void Ventilation_ARXTVS_ALIGN(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsAlignCmd::execute();
    }

