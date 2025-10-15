#ifndef JETVIEW_H
#define JETVIEW_H

#include <memory>
#include <cstddef>
#include <vector>

#include "TLorentzVector.h"
#include "ViewColumns.h"

struct JetSoA {
    ColumnView<float> pt;
    ColumnView<float> eta;
    ColumnView<float> phi;
    ColumnView<float> mass;
    ColumnView<float> rawFactor;
    ColumnView<float> area;
    ColumnView<float> chHEF;
    ColumnView<float> neHEF;
    ColumnView<float> neEmEF;
    ColumnView<float> chEmEF;
    ColumnView<float> muEF;
    ColumnView<short> partonFlavour;
    ColumnView<unsigned char> hadronFlavour;
    ColumnView<unsigned char> chMultiplicity;
    ColumnView<unsigned char> neMultiplicity;
    ColumnView<unsigned char> nConstituents;
    ColumnView<unsigned char> nElectrons;
    ColumnView<unsigned char> nMuons;
    ColumnView<unsigned char> nSVs;
    ColumnView<short> electronIdx1;
    ColumnView<short> electronIdx2;
    ColumnView<short> muonIdx1;
    ColumnView<short> muonIdx2;
    ColumnView<short> svIdx1;
    ColumnView<short> svIdx2;
    ColumnView<short> genJetIdx;
    ColumnView<float> deepFlavB;
    ColumnView<float> deepFlavCvB;
    ColumnView<float> deepFlavCvL;
    ColumnView<float> deepFlavQG;
    ColumnView<float> pnetB;
    ColumnView<float> pnetCvB;
    ColumnView<float> pnetCvL;
    ColumnView<float> pnetCvNotB;
    ColumnView<float> pnetQvG;
    ColumnView<float> pnetTauVJet;
    ColumnView<float> uparTAK4B;
    ColumnView<float> uparTAK4CvB;
    ColumnView<float> uparTAK4CvL;
    ColumnView<float> uparTAK4CvNotB;
    ColumnView<float> uparTAK4Ele;
    ColumnView<float> uparTAK4Mu;
    ColumnView<float> uparTAK4QvG;
    ColumnView<float> uparTAK4SvCB;
    ColumnView<float> uparTAK4SvUDG;
    ColumnView<float> uparTAK4TauVJet;
    ColumnView<float> uparTAK4UDG;
    ColumnView<float> uparTAK4ProbB;
    ColumnView<float> uparTAK4ProbBB;
    ColumnView<float> pnetRegPtRawCorr;
    ColumnView<float> pnetRegPtRawCorrNeutrino;
    ColumnView<float> pnetRegPtRawRes;
    ColumnView<float> uparTAK4RegPtRawCorr;
    ColumnView<float> uparTAK4RegPtRawCorrNeutrino;
    ColumnView<float> uparTAK4RegPtRawRes;
    ColumnView<float> uparTAK4V1RegPtRawCorr;
    ColumnView<float> uparTAK4V1RegPtRawCorrNeutrino;
    ColumnView<float> uparTAK4V1RegPtRawRes;
    ColumnView<float> puIdDisc;

    std::size_t size() const { return pt.size(); }
};

class JetView {
public:
    JetView() = default;
    JetView(std::shared_ptr<const JetSoA> storage, std::size_t index)
        : store(std::move(storage)), idx(index) {}

    bool valid() const { return static_cast<bool>(store) && idx < store->size(); }

    float Pt() const { return store->pt[idx]; }
    float Eta() const { return store->eta[idx]; }
    float Phi() const { return store->phi[idx]; }
    float Mass() const { return store->mass[idx]; }
    float RawFactor() const { return store->rawFactor[idx]; }
    float Area() const { return store->area[idx]; }
    float ChHEF() const { return store->chHEF[idx]; }
    float NeHEF() const { return store->neHEF[idx]; }
    float NeEmEF() const { return store->neEmEF[idx]; }
    float ChEmEF() const { return store->chEmEF[idx]; }
    float MuEF() const { return store->muEF[idx]; }
    short PartonFlavour() const { return store->partonFlavour[idx]; }
    unsigned char HadronFlavour() const { return store->hadronFlavour[idx]; }
    unsigned char ChMultiplicity() const { return store->chMultiplicity[idx]; }
    unsigned char NeMultiplicity() const { return store->neMultiplicity[idx]; }
    unsigned char NConstituents() const { return store->nConstituents[idx]; }
    unsigned char NElectrons() const { return store->nElectrons[idx]; }
    unsigned char NMuons() const { return store->nMuons[idx]; }
    unsigned char NSVs() const { return store->nSVs[idx]; }
    short ElectronIdx1() const { return store->electronIdx1[idx]; }
    short ElectronIdx2() const { return store->electronIdx2[idx]; }
    short MuonIdx1() const { return store->muonIdx1[idx]; }
    short MuonIdx2() const { return store->muonIdx2[idx]; }
    short SvIdx1() const { return store->svIdx1[idx]; }
    short SvIdx2() const { return store->svIdx2[idx]; }
    short GenJetIdx() const { return store->genJetIdx[idx]; }
    float PNetRegPtRawCorr() const { return store->pnetRegPtRawCorr[idx]; }
    float PNetRegPtRawCorrNeutrino() const { return store->pnetRegPtRawCorrNeutrino[idx]; }
    float PNetRegPtRawRes() const { return store->pnetRegPtRawRes[idx]; }
    float UParTAK4RegPtRawCorr() const { return store->uparTAK4RegPtRawCorr[idx]; }
    float UParTAK4RegPtRawCorrNeutrino() const { return store->uparTAK4RegPtRawCorrNeutrino[idx]; }
    float UParTAK4RegPtRawRes() const { return store->uparTAK4RegPtRawRes[idx]; }
    float UParTAK4V1RegPtRawCorr() const { return store->uparTAK4V1RegPtRawCorr[idx]; }
    float UParTAK4V1RegPtRawCorrNeutrino() const { return store->uparTAK4V1RegPtRawCorrNeutrino[idx]; }
    float UParTAK4V1RegPtRawRes() const { return store->uparTAK4V1RegPtRawRes[idx]; }
    float PuIdDisc() const { return store->puIdDisc[idx]; }

    TLorentzVector P4() const {
        TLorentzVector v;
        v.SetPtEtaPhiM(Pt(), Eta(), Phi(), Mass());
        return v;
    }

private:
    std::shared_ptr<const JetSoA> store;
    std::size_t idx = 0;
};

class JetViewCollection {
public:
    JetViewCollection() = default;
    explicit JetViewCollection(std::shared_ptr<JetSoA> payload)
        : storage_(std::move(payload)) {
        if (storage_) {
            const std::size_t n = storage_->size();
            views.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                views.emplace_back(storage_, i);
            }
        }
    }

    const JetView &operator[](std::size_t index) const { return views[index]; }
    std::size_t size() const { return views.size(); }
    bool empty() const { return views.empty(); }

    auto begin() const { return views.begin(); }
    auto end() const { return views.end(); }

    const std::shared_ptr<JetSoA> &storage() const { return storage_; }

private:
    std::shared_ptr<JetSoA> storage_;
    std::vector<JetView> views;
};

#endif // JETVIEW_H
