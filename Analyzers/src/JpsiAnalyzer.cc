#include "JpsiAnalyzer.h"

JpsiAnalyzer::JpsiAnalyzer() {}
JpsiAnalyzer::~JpsiAnalyzer() {}

void JpsiAnalyzer::initializeAnalyzer() {
    //==== Example 1
    RunSyst = HasFlag("RunSyst");
    cout << "[JpsiAnalyzer::initializeAnalyzer] RunSyst = " << RunSyst << endl;


    MuonIDs = {Muon::MuonID::POG_SOFT};
    MuonIDSFKeys = {"NUM_TightID_DEN_TrackerMuons"};

    if (DataEra == "2024") {
        TriggerName = "HLT_Dimuon25_Jpsi";
    } else {
        cerr << "[JpsiAnalyzer::initializeAnalyzer] DataEra is not set properly" << endl;
        exit(EXIT_FAILURE);
    }

    myCorr = new MyCorrection(DataEra, IsDATA?DataStream:MCSample ,IsDATA);
   
}

void JpsiAnalyzer::executeEvent() {

    AllMuons = GetAllMuons();
    ev = GetEvent();
    executeEventFromParameter();
    
   
}

void JpsiAnalyzer::executeEventFromParameter() {
    float JpsiMass = 3.096916; // PDG J/psi mass in GeV
    if(!ev.PassTrigger("HLT_Dimuon25_Jpsi")) return;
    Muons = SelectMuons(AllMuons, MuonIDs[0], 5., 2.4);
    std::sort(Muons.begin(), Muons.end(), AnalyzerCore::PtComparing);
    if (Muons.size() < 2) return;
    if (Muons[0].Charge() * Muons[1].Charge() > 0) return; // opposite sign
    Particle jpsi = Muons[0] + Muons[1];
    FillHist("JpsiMass", jpsi.M(), 1., 100, JpsiMass -1.f, JpsiMass + 1.f);
    FillHist("JpsiPt", jpsi.Pt(), 1., 50, 20.f, 100.f);
    FillHist("JpsiEta", jpsi.Eta(), 1., 50, -2.5f, 2.5f);

    

}
