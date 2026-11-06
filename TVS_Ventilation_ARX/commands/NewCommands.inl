#ifdef TVS_OFFSET_PROOF
#ifdef TVS_OFFSET_INTEGRATION
    static void Ventilation_ARXTVS_OFFSET(){tvsOffsetIntegration::standalone();}
    // TVS_ETAZ_KOTA: etaz follows a changed duct elevation (run automatically, OffsetRefit.h).
    static void Ventilation_ARXTVS_ETAZ_KOTA(){if(ActivationErrorMessage()!=pError_Ok)return;tvsOffsetRefit::run();}
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
    // TVS_DIM_PRATI: duct / transition sizes at their joints follow an edit (run automatically, SizeFollow.h).
    static void Ventilation_ARXTVS_DIM_PRATI(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        tvsSizeFollow::run();
    }
    static void Ventilation_ARXTVS_PRELAZ_KOTA(void)
    {
        if (ActivationErrorMessage() != pError_Ok) return;
        syncTVSGlobalUnits();
        tvsTransAlignCmd::run();
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
        tvsSectionCmd::run();
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
        tvsTeeCommand::setType();
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

