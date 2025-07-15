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
    AllJets = GetAllJets();
    AllFatJets = GetAllFatJets();
    ev = GetEvent();
    executeEventFromParameter();
    
   
}

void JpsiAnalyzer::executeEventFromParameter() {
    float JpsiMass = 3.096916; // PDG J/psi mass in GeV
    if(!ev.PassTrigger("HLT_Dimuon25_Jpsi")) return;
    if(!PassJetVetoMap(AllJets, AllMuons)) return; // Jet veto map
    Muons = SelectMuons(AllMuons, MuonIDs[0], 5., 2.4);
    std::sort(Muons.begin(), Muons.end(), AnalyzerCore::PtComparing);
    if (Muons.size() < 2) return;
    if (Muons[0].Charge() * Muons[1].Charge() > 0) return; // opposite sign
    Particle jpsi = Muons[0] + Muons[1];
    Jets = SelectJets(AllJets, Jet::JetID::TIGHT, 20., 2.5);
    FatJets = SelectFatJets(AllFatJets, FatJet::FatJetID::TIGHT, 10., 2.5);
    //select back-to-back jets
    if (Jets.size() < 2) return;

    // if (LeadingJet.DeltaPhi(SubLeadingJet) < 2.5) return; // back-to-back jets
    // if (LeadingJet.Pt() / SubLeadingJet.Pt() > 1.5 || LeadingJet.Pt() / SubLeadingJet.Pt() < 0.5) return; // pt ratio cut
    //Find jet closest to Jpsi
    size_t closestJetIndex = 0;
    float minDeltaR = 999.0;
    for (size_t i = 0; i < Jets.size(); ++i) {
        float deltaR = jpsi.DeltaR(Jets[i]);
        if (deltaR < minDeltaR) {
            minDeltaR = deltaR;
            closestJetIndex = i;
        }
    }
    Jet closestJet = Jets[closestJetIndex];
    //Find another jet farthest from above jet in terms of phi
    size_t farthestJetIndex = 0;
    float maxDeltaPhi = -999.0;
    for (size_t i = 0; i < Jets.size(); ++i) {
        if (i == closestJetIndex) continue; // skip the closest jet
        float deltaPhi = Jets[i].DeltaPhi(closestJet);
        if (deltaPhi > maxDeltaPhi) {
            maxDeltaPhi = deltaPhi;
            farthestJetIndex = i;
        }
    }
    Jet farthestJet = Jets[farthestJetIndex];

    // Fill histograms

    FillHist("JpsiMass", jpsi.M(), 1., 100, JpsiMass -1.f, JpsiMass + 1.f);
    FillHist("JpsiPt", jpsi.Pt(), 1., 50, 20.f, 100.f);
    FillHist("JpsiEta", jpsi.Eta(), 1., 50, -2.5f, 2.5f);

    FillHist("dR_Jpsi_ClosestJet", jpsi.DeltaR(closestJet), 1., 50, 0.f, 1.f);
    FillHist("dPhi_ClosestJet_FarthestJet", closestJet.DeltaPhi(farthestJet), 1., 50, -3.14f, 3.14f);
    FillHist("Jpsi_Fragmentation", jpsi.Pt() / closestJet.Pt(), 1. , 50, 0.f, 1.5f);
    FillHist("nFatJets", FatJets.size(), 1., 5, 0.f, 5.f);
    if(FatJets.size() == 0) return; // no fat jets, exit
    size_t closestFatJetIndex = 0;
    float minFatJetDeltaR = 999.0;
    for (size_t i = 0; i < FatJets.size(); ++i) {
        float deltaR = jpsi.DeltaR(FatJets[i]);
        if (deltaR < minFatJetDeltaR) {
            minFatJetDeltaR = deltaR;
            closestFatJetIndex = i;
        }
    }
    FatJet closestFatJet = FatJets[closestFatJetIndex];
    FillHist("dR_Jpsi_ClosestFatJet", jpsi.DeltaR(closestFatJet), 1., 50, 0.f, 1.f);
    FillHist("ClosestFatJet_Pt", closestFatJet.Pt(), 1., 50, 20.f, 100.f);

}
