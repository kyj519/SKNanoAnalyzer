#include "ExampleViewRun.h"

#include <algorithm>
#include <cmath>
#include <vector>

ExampleViewRun::ExampleViewRun() {}
ExampleViewRun::~ExampleViewRun() {}

void ExampleViewRun::initializeAnalyzer() {
    if (DataEra == "2016preVFP" || DataEra == "2016postVFP" || DataEra == "2018") {
        IsoMuTriggerName = "HLT_IsoMu24";
        TriggerSafePtCut = 26.f;
    } else if (DataEra == "2017") {
        IsoMuTriggerName = "HLT_IsoMu27";
        TriggerSafePtCut = 29.f;
    } else if (DataEra == "2022" || DataEra == "2022EE" || DataEra == "2024") {
        IsoMuTriggerName = "HLT_IsoMu24";
        TriggerSafePtCut = 26.f;
    } else if (DataEra == "2023") {
        IsoMuTriggerName = ""; // Run-3 special case
        TriggerSafePtCut = 26.f;
    } else {
        IsoMuTriggerName = "";
        TriggerSafePtCut = 0.f;
    }

    if (!myCorr)
        myCorr = new MyCorrection(DataEra, DataPeriod, IsDATA ? DataStream : MCSample, IsDATA);
}

void ExampleViewRun::executeEvent() {
    const float baseWeight = IsDATA ? 1.f : MCweight();

    Event ev = GetEvent();
    if (!IsoMuTriggerName.IsNull() && !IsoMuTriggerName.IsWhitespace()) {
        if (!ev.PassTrigger(IsoMuTriggerName))
            return;
    }

    MuonViewCollection muons = GetAllMuonViews();
    if (muons.size() < 2)
        return;

    const float muonPtCut = std::max(TriggerSafePtCut, 20.f);
    std::vector<std::size_t> mediumMuons = SelectMuonIndices(muons, Muon::MuonID::POG_MEDIUM, muonPtCut, 2.4f);

    std::sort(mediumMuons.begin(), mediumMuons.end(), [&](std::size_t a, std::size_t b) {
        return muons[a].Pt() > muons[b].Pt();
    });

    FillHist("ViewExample/NMuon", static_cast<float>(mediumMuons.size()), baseWeight, 6, 0., 6.);
    if (mediumMuons.size() < 2)
        return;

    const auto &lead = muons[mediumMuons[0]];
    const auto &sublead = muons[mediumMuons[1]];
    TLorentzVector dimu = lead.P4() + sublead.P4();
    FillHist("ViewExample/DimuonMass", dimu.M(), baseWeight, 120, 60., 120.);

    JetViewCollection jets = GetAllJetViews();
    std::size_t tightJetCount = 0;
    for (std::size_t i = 0; i < jets.size(); ++i) {
        const auto &jet = jets[i];
        if (jet.Pt() < 30.)
            continue;
        if (std::fabs(jet.Eta()) > 2.5)
            continue;
        ++tightJetCount;
    }
    FillHist("ViewExample/JetMultiplicity", static_cast<float>(tightJetCount), baseWeight, 10, 0., 10.);
}
