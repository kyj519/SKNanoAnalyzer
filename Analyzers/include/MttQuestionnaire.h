#ifndef MttQuestionnaire_h
#define MttQuestionnaire_h

#include <future>
#include <tuple>
#include <memory>
#include <set>
#include <correction.h>
#include <variant>
#include <functional>

#include "AnalyzerCore.h"
#include "TKinFitter.h"
#include "TFitParticlePt.h"
#include "TFitParticleEtEtaPhi.h"
#include "TFitParticleMCCart.h"
#include "TFitConstraintMGaus.h"
#include "TFitConstraintM.h"
#include "TFitConstraintEp.h"
#include "TMatrixD.h"
#include "MttQuestionnaireParameters.h"
using correction::CorrectionSet;
#include "SystematicHelper.h"

using FloatArray = std::vector<float>;
using FloatArrays = std::vector<FloatArray>;
using IntArray = std::vector<int>;
using BoolArray = std::vector<uint8_t>;
using VariousArray = std::variant<FloatArray, IntArray, BoolArray>;

class MttQuestionnaire : public AnalyzerCore
{
public:
    inline bool isPIDUpTypeQuark(int pdg) { return (abs(pdg) == 2 || abs(pdg) == 4 || abs(pdg) == 6); }
    inline bool isPIDDownTypeQuark(int pdg) { return (abs(pdg) == 1 || abs(pdg) == 3 || abs(pdg) == 5); }
    inline bool isPIDLepton(int pdg) { return (abs(pdg) == 11 || abs(pdg) == 13 || abs(pdg) == 15); }
    inline bool isPIDNeutrino(int pdg) { return (abs(pdg) == 12 || abs(pdg) == 14 || abs(pdg) == 16); }
    inline bool isDaughterOf(int idx, int m_idx)
    {
        while (idx >= 0)
        {
            int midx = AllGens[idx].MotherIndex();
            if (midx < 0)
                break; // No valid mother
            if (midx == m_idx)
                return true; // Found the W in the chain
            idx = midx;
        }
        return false;
    };
    inline int GetSimplePID(int pid, bool usesign = false)
    {
        int simple_pid = 0;
        if (abs(pid) == 5)
            simple_pid = 5;
        else if (abs(pid) == 4)
            simple_pid = 4;
        else
            simple_pid = 0;
        if (!usesign)
            return simple_pid;
        if (pid < 0)
            return -simple_pid;
        return simple_pid;
    }
    float LeptonTriggerWeight(bool isEle, const Correction::variation syst = Correction::variation::nom, const TString &source = "total");
    void Clear();
    short GetPassedBTaggingWP(const Jet &jet);
    short GetPassedCTaggingWP(const Jet &jet);
    void SetChannel();
    bool virtual CheckChannel() = 0;
    void initializeAnalyzer();
    void executeEvent();
    void executeEventFromParameter();
    bool virtual PassBaseLineSelection(bool remove_flavtagging_cut = false) = 0;
    void virtual FillHistogramsAtThisPoint(const TString &histPrefix, float weight = 1.f);
    void virtual FillKinematicFitterResult(const TString &histPrefix, float weight);
    void virtual GetKineMaticFitterResult(const int strategy) = 0;
    void SetTTbarId();
    void virtual SkimTree() = 0;
    void SetSystematicLambda(bool remove_flavtagging_sf = false);
    RVec<int> virtual FindTTbarJetIndices();
    float virtual MCNormalization() = 0;
    std::string virtual GetRegionString() = 0;


    inline std::string GetTTHFPostFix()
    {
        if (ttbj || ttbb)
            return "+B";
        if (ttcc)
            return "+C";
        if (ttLF)
            return "+LF";
        return "";
    }



    RVec<RVec<unsigned int>> virtual GetPermutations(const RVec<Jet> &jets);
    MttQuestionnaire();
    virtual ~MttQuestionnaire() = default;

    // Objects
    RVec<Muon> AllMuons;
    RVec<Electron> AllElectrons;
    RVec<Jet> AllJets;
    RVec<Gen> AllGens;
    RVec<LHE> AllLHEs;
    RVec<GenJet> AllGenJets;
    // Selected Objects
    RVec<Jet> Jets;
    RVec<Electron> Electrons_Veto;
    RVec<Electron> Electrons;
    RVec<Muon> Muons_Veto;
    RVec<Muon> Muons;
    Lepton lepton;
    RVec<Lepton> leptons;
    Event ev;
    Particle MET;
    RVec<int> ttbar_jet_indices;
    RVec<int> ttbar_AllGenJets_indices;
    // event info
    float HT;
    short n_jets;
    short n_b_tagged_jets;
    short n_c_tagged_jets;
    short n_partonFlav_b_jets;
    short n_partonFlav_c_jets;
    bool find_all_jets;
    bool find_all_genjets;
    int tt_decay_code;
    bool ttbj;
    bool ttbb;
    bool ttcc;
    bool ttLF;
    std::unique_ptr<SystematicHelper> systHelper;

    enum class Channel
    {
        MM,
        ME,
        EE,
        Mu,
        El,
        FH
    };

    inline TString GetChannelString(Channel ch)
    {
        switch (ch)
        {
        case Channel::MM:
            return "MM";
        case Channel::ME:
            return "ME";
        case Channel::EE:
            return "EE";
        case Channel::Mu:
            return "Mu";
        case Channel::El:
            return "El";
        case Channel::FH:
            return "FH";
        default:
            return "";
        }
    }

    Channel channel;

};

#endif
