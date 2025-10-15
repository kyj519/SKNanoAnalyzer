#ifndef Muon_h
#define Muon_h

#include <bitset>
#include <memory>
#include <limits>

#include "TString.h"
#include "Lepton.h"
#include "MuonView.h"

class AnalyzerCore;

// Need update
// - TuneP object
// - momentum scale
// - rochester correction
// - chi2
// - truth matching info
// - Analysis dependent IDs

class Muon: public Lepton {
public:
    Muon();
    Muon(std::shared_ptr<const MuonSoA> storage, std::size_t index);
    ~Muon();

    enum class Property {
        TkRelIso,
        PfRelIso03,
        PfRelIso04,
        MiniPFRelIso,
        Dxy,
        DxyErr,
        Dz,
        DzErr,
        Ip3d,
        Sip3d,
        TrackerLayers,
        GenPartFlav,
        GenPartIdx,
        JetIdx,
        LooseId,
        MediumId,
        MediumPromptId,
        TightId,
        SoftId,
        SoftMvaId,
        TriggerLooseId,
        HighPtId,
        MiniIsoId,
        MultiIsoId,
        MvaMuId,
        PfIsoId,
        PuppiIsoId,
        TkIsoId,
        SoftMva,
        MvaLowPt,
        MvaPrompt,
        Count
    };

    using EnsureCallback = void (*)(void*, Muon&, Property);
    void AttachLazyPayload(void *context, EnsureCallback callback, int index) const;
    void DetachLazyPayload() const;
    bool HasLazyPayload() const { return static_cast<bool>(lazy_); }

    // Boolean IDs
    enum class BooleanID {NONE, LOOSE, MEDIUM, MEDIUMPROMPT, TIGHT, SOFT, SOFTMVA, TRIGGERLOOSE};

    void SetBIDBit(BooleanID id, bool idbit);
    inline bool isPOGTightId() const { ensure(Property::TightId); return j_tightId; }
    inline bool isPOGMediumId() const { ensure(Property::MediumId); return j_mediumId; }
    inline bool isPOGMediumPromptId() const { ensure(Property::MediumPromptId); return j_mediumPromptId; }
    inline bool isPOGLooseId() const { ensure(Property::LooseId); return j_looseId; }
    inline bool isPOGSoftId() const { ensure(Property::SoftId); return j_softId; }
    inline bool isPOGSoftMvaId() const { ensure(Property::SoftMvaId); return j_softMvaId; }
    inline bool isPOGTriggerIdLoose() const { ensure(Property::TriggerLooseId); return j_triggerIdLoose; }

    // Muon type methods
    void SetIsTracker(bool isTracker) { j_isTracker = isTracker; }
    void SetIsStandalone(bool isStandalone) { j_isStandalone = isStandalone; }
    void SetIsGlobal(bool isGlobal) { j_isGlobal = isGlobal; }
    inline bool isTracker() const { return j_isTracker; }
    inline bool isStandalone() const { return j_isStandalone; }
    inline bool isGlobal() const { return j_isGlobal; }

    // Unsigned char IDs
    enum class WorkingPointID {NONE, HIGHPT, MINIISO, MULTIISO, MVAMU, PFISO, PUPPIISO, TKISO};
    enum class WorkingPoint {NONE, VLOOSE, LOOSE, MEDIUM, TIGHT, VTIGHT, VVTIGHT};

    enum class MuonID
    {
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

    void SetWIDBit(WorkingPointID id, unsigned char value);
    inline WorkingPoint HighPtId() const { ensure(Property::HighPtId); return static_cast<WorkingPoint>(j_highPtId); }
    inline WorkingPoint MiniIsoId() const { ensure(Property::MiniIsoId); return static_cast<WorkingPoint>(j_miniIsoId); }
    inline WorkingPoint MultiIsoId() const { ensure(Property::MultiIsoId); return static_cast<WorkingPoint>(j_multiIsoId); }
    inline WorkingPoint MvaMuId() const { ensure(Property::MvaMuId); return static_cast<WorkingPoint>(j_mvaMuId); }
    inline WorkingPoint PfIsoId() const { ensure(Property::PfIsoId); return static_cast<WorkingPoint>(j_pfIsoId); }
    inline WorkingPoint PuppiIsoId() const { ensure(Property::PuppiIsoId); return static_cast<WorkingPoint>(j_puppiIsoId); }
    inline WorkingPoint TkIsoId() const { ensure(Property::TkIsoId); return static_cast<WorkingPoint>(j_tkIsoId); }

    void SetNTrackerLayers(int n) { j_nTrackerLayers = n; markLoaded(Property::TrackerLayers); }
    inline int nTrackerLayers() const { ensure(Property::TrackerLayers); return j_nTrackerLayers; }
    void SetMiniAODPt(float pt) { j_miniAODPt = pt; }
    inline float MiniAODPt() const { return j_miniAODPt; }
    void SetMomentumScaleUpDown(float up, float down) { j_momentumScaleUp = up; j_momentumScaleDown = down; }
    inline float MomentumScaleUp() const { return j_momentumScaleUp; }
    inline float MomentumScaleDown() const { return j_momentumScaleDown; }

    // Override base-class accessors to enable lazy filling
    float TkRelIso() const { ensure(Property::TkRelIso); return Lepton::TkRelIso(); }
    float PfRelIso03() const { ensure(Property::PfRelIso03); return Lepton::PfRelIso03(); }
    float PfRelIso04() const { ensure(Property::PfRelIso04); return Lepton::PfRelIso04(); }
    float MiniPFRelIso() const { ensure(Property::MiniPFRelIso); return Lepton::MiniPFRelIso(); }
    float dXY() const { ensure(Property::Dxy); return Lepton::dXY(); }
    float dXYerr() const { ensure(Property::DxyErr); return Lepton::dXYerr(); }
    float dZ() const { ensure(Property::Dz); return Lepton::dZ(); }
    float dZerr() const { ensure(Property::DzErr); return Lepton::dZerr(); }
    float IP3D() const { ensure(Property::Ip3d); return Lepton::IP3D(); }
    float SIP3D() const { ensure(Property::Sip3d); return Lepton::SIP3D(); }

    // MVA ID scores
    enum class MVAID {NONE, SOFTMVA, MVALOWPT, MVAPROMPT};

    void SetMVAID(MVAID id, float score);
    inline float SoftMva() const { ensure(Property::SoftMva); return j_softMva; }
    inline float MvaLowPt() const { ensure(Property::MvaLowPt); return j_mvaLowPt; }
    inline float MvaTTH() const { ensure(Property::MvaPrompt); return j_mvaPrompt; }

    void SetGenPartIdx(short genPartIdx) { j_genPartIdx = genPartIdx; markLoaded(Property::GenPartIdx); }
    inline short GenPartIdx() const { ensure(Property::GenPartIdx); return j_genPartIdx; }

    void SetGenPartFlav(unsigned char genPartFlav) { j_genPartFlav = genPartFlav; markLoaded(Property::GenPartFlav); }
    inline unsigned char GenPartFlav() const { ensure(Property::GenPartFlav); return j_genPartFlav; }

    void SetJetIdx(short jetIdx) { j_jetIdx = jetIdx; markLoaded(Property::JetIdx); }
    inline short JetIdx() const { ensure(Property::JetIdx); return j_jetIdx; }

    // ID helper functions
    bool PassID(const MuonID ID) const;
    bool PassID(const TString ID) const;

    // Private IDs
    bool Pass_HcToWATight() const;
    bool Pass_HcToWALoose() const;

private:
    void ensure(Property property) const;
    void markLoaded(Property property) const;

    struct LazyPayload {
        void *context = nullptr;
        EnsureCallback callback = nullptr;
        int index = -1;
        mutable std::bitset<static_cast<std::size_t>(Property::Count)> loaded;
    };

    void initializeMembers();
    void materialize() const;
    bool isMaterialized() const { return !storage_; }
    void loadFromStorage(Property property) const;

    std::shared_ptr<const MuonSoA> storage_;
    std::size_t index_ = std::numeric_limits<std::size_t>::max();

    mutable std::bitset<static_cast<std::size_t>(Property::Count)> cachedProperties_;
    mutable std::shared_ptr<LazyPayload> lazy_;

    bool j_isTracker, j_isStandalone, j_isGlobal;
    bool j_looseId, j_mediumId, j_mediumPromptId, j_tightId, j_softId, j_softMvaId, j_triggerIdLoose;
    unsigned char j_highPtId, j_miniIsoId, j_multiIsoId, j_mvaMuId, j_pfIsoId, j_puppiIsoId, j_tkIsoId;
    float j_softMva, j_mvaLowPt, j_mvaPrompt;
    int j_nTrackerLayers;
    float j_miniAODPt, j_momentumScaleUp, j_momentumScaleDown;
    short j_genPartIdx;
    unsigned char j_genPartFlav;
    short j_jetIdx;

    friend class AnalyzerCore;
    ClassDef(Muon, 1);
};

#endif
