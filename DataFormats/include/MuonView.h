#ifndef MUONVIEW_H
#define MUONVIEW_H

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "TLorentzVector.h"
#include "TString.h"
#include "ViewColumns.h"

/**
 * Lightweight view over nanoAOD muon branches.
 * Exposes read-only accessors mirroring the high-level Muon API
 * without materialising per-object payloads.
 */

struct MuonSoA {
    ColumnView<float> pt;
    ColumnView<float> eta;
    ColumnView<float> phi;
    ColumnView<float> mass;
    ColumnView<int> charge;
    ColumnView<float> tkRelIso;
    ColumnView<float> pfRelIso03;
    ColumnView<float> pfRelIso04;
    ColumnView<float> miniPFRelIsoAll;
    ColumnView<float> dxy;
    ColumnView<float> dxyErr;
    ColumnView<float> dz;
    ColumnView<float> dzErr;
    ColumnView<float> ip3d;
    ColumnView<float> sip3d;
    ColumnView<unsigned char> highPtId;
    BoolColumnView looseId;
    BoolColumnView mediumId;
    BoolColumnView mediumPromptId;
    BoolColumnView tightId;
    BoolColumnView softId;
    BoolColumnView softMvaId;
    BoolColumnView triggerLooseId;
    ColumnView<unsigned char> miniIsoId;
    ColumnView<unsigned char> multiIsoId;
    ColumnView<unsigned char> mvaMuId;
    ColumnView<unsigned char> pfIsoId;
    ColumnView<unsigned char> puppiIsoId;
    ColumnView<unsigned char> tkIsoId;
    ColumnView<unsigned char> nTrackerLayers;
    ColumnView<float> softMva;
    ColumnView<float> mvaLowPt;
    ColumnView<float> mvaPrompt;
    ColumnView<unsigned char> genPartFlav;
    ColumnView<short> genPartIdx;
    ColumnView<short> jetIdx;

    std::vector<float> correctedPt;
    std::vector<float> miniAODPt;
    std::vector<float> momentumScaleUp;
    std::vector<float> momentumScaleDown;

    std::size_t size() const { return correctedPt.size(); }
};

class MuonView {
public:
    enum class WorkingPoint : unsigned char {
        NONE = 0,
        VLOOSE = 1,
        LOOSE = 2,
        MEDIUM = 3,
        TIGHT = 4,
        VTIGHT = 5,
        VVTIGHT = 6
    };

    enum class MuonID {
        NOCUT,
        POG_TIGHT,
        POG_MEDIUM,
        POG_MEDIUM_PROMPT,
        POG_LOOSE,
        POG_SOFT,
        POG_SOFT_MVA,
        POG_TRIGGER_LOOSE,
        POG_TRACKER_HIGH_PT,
        POG_GLOBAL_HIGH_PT,
        POG_MINISO_LOOSE,
        POG_MINISO_MEDIUM,
        POG_MINISO_TIGHT,
        POG_MINISO_VTIGHT,
        POG_MULTISO_LOOSE,
        POG_MULTISO_MEDIUM,
        POG_MVA_MU_MEDIUM,
        POG_MVA_MU_TIGHT,
        POG_PFISO_VLOOSE,
        POG_PFISO_LOOSE,
        POG_PFISO_MEDIUM,
        POG_PFISO_TIGHT,
        POG_PFISO_VTIGHT,
        POG_PFISO_VVTIGHT,
        POG_PUPPIISO_LOOSE,
        POG_PUPPIISO_MEDIUM,
        POG_PUPPIISO_TIGHT,
        POG_TKISO_LOOSE,
        POG_TKISO_TIGHT
    };

    MuonView() = default;
    MuonView(std::shared_ptr<const MuonSoA> data, std::size_t index)
        : store(std::move(data)), idx(index) {}

    bool valid() const { return static_cast<bool>(store) && idx < store->size(); }

    float Pt() const { return store->correctedPt[idx]; }
    float Eta() const { return store->eta[idx]; }
    float Phi() const { return store->phi[idx]; }
    float M() const { return store->mass[idx]; }
    int Charge() const { return store->charge[idx]; }

    float MiniAODPt() const { return store->miniAODPt[idx]; }
    float MomentumScaleUp() const { return store->momentumScaleUp[idx]; }
    float MomentumScaleDown() const { return store->momentumScaleDown[idx]; }

    float TkRelIso() const { return store->tkRelIso[idx]; }
    float PfRelIso03() const { return store->pfRelIso03[idx]; }
    float PfRelIso04() const { return store->pfRelIso04[idx]; }
    float MiniPFRelIso() const { return store->miniPFRelIsoAll[idx]; }
    float dXY() const { return store->dxy[idx]; }
    float dXYerr() const { return store->dxyErr[idx]; }
    float dZ() const { return store->dz[idx]; }
    float dZerr() const { return store->dzErr[idx]; }
    float IP3D() const { return store->ip3d[idx]; }
    float SIP3D() const { return store->sip3d[idx]; }

    unsigned char nTrackerLayers() const { return store->nTrackerLayers[idx]; }
    float SoftMva() const { return store->softMva[idx]; }
    float MvaLowPt() const { return store->mvaLowPt[idx]; }
    float MvaPrompt() const { return store->mvaPrompt[idx]; }

    unsigned char GenPartFlav() const { return store->genPartFlav[idx]; }
    short GenPartIdx() const { return store->genPartIdx[idx]; }
    short JetIdx() const { return store->jetIdx[idx]; }

    TLorentzVector P4() const {
        TLorentzVector v;
        v.SetPtEtaPhiM(Pt(), Eta(), Phi(), M());
        return v;
    }

    float DeltaR(const TLorentzVector &other) const { return P4().DeltaR(other); }
    float DeltaR(const MuonView &other) const { return P4().DeltaR(other.P4()); }

    bool isPOGTightId() const { return store->tightId[idx]; }
    bool isPOGMediumId() const { return store->mediumId[idx]; }
    bool isPOGMediumPromptId() const { return store->mediumPromptId[idx]; }
    bool isPOGLooseId() const { return store->looseId[idx]; }
    bool isPOGSoftId() const { return store->softId[idx]; }
    bool isPOGSoftMvaId() const { return store->softMvaId[idx]; }
    bool isPOGTriggerIdLoose() const { return store->triggerLooseId[idx]; }

    bool PassID(MuonID id) const;
    bool PassID(const TString &id) const;

private:
    bool workingPointAtLeast(const ColumnView<unsigned char> &column, WorkingPoint target) const {
        return column[idx] >= static_cast<unsigned char>(target);
    }

    bool Pass_HcToWATight() const;
    bool Pass_HcToWALoose() const;

    std::shared_ptr<const MuonSoA> store;
    std::size_t idx = 0;
};

class MuonViewCollection {
public:
    MuonViewCollection() = default;
    explicit MuonViewCollection(std::shared_ptr<MuonSoA> data);

    const MuonView &operator[](std::size_t index) const { return views[index]; }
    std::size_t size() const { return views.size(); }

    auto begin() const { return views.begin(); }
    auto end() const { return views.end(); }

    const std::shared_ptr<MuonSoA> &storage() const { return payload; }

private:
    std::shared_ptr<MuonSoA> payload;
    std::vector<MuonView> views;
};

#endif // MUONVIEW_H
