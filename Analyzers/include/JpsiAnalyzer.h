#ifndef JpsiAnalyzer_h 
#define JpsiAnalyzer_h

#include "AnalyzerCore.h"
#include "SystematicHelper.h"


class JpsiAnalyzer : public AnalyzerCore {
public:
    void initializeAnalyzer();
    void executeEvent();
    void executeEventFromParameter();

    bool RunSyst;
    bool RunNewPDF;
    bool RunXSecSyst;

    TString TriggerName;
    float TriggerSafePtCut;

    RVec<TString> MuonIDSFKeys;
    RVec<Muon::MuonID> MuonIDs;
    RVec<Muon> AllMuons;
    RVec<Jet> AllJets;
    Event ev;

    float weight_Prefire;

    unique_ptr<SystematicHelper> systHelper;

    RVec<Muon> Muons;

    JpsiAnalyzer();
    ~JpsiAnalyzer();
};

#endif

