#ifndef Jet_h
#define Jet_h

#include "Particle.h"
#include "JetTaggingParameter.h"

class Jet : public Particle
{
public:
  Jet();
  ~Jet();

  enum class JetID
  {
    NOCUT,
    LOOSE,
    TIGHT,
    TIGHTLEPVETO,
    PUID_LOOSE,
    PUID_MEDIUM,
    PUID_TIGHT,
  };
  // setting functions
  inline void SetArea(double area) { j_area = area; };
  inline void SetJetFlavours(short pf, unsigned char hf)
  {
    j_partonFlavour = static_cast<short>(pf);
    j_hadronFlavour = static_cast<short>(hf);
  };

  inline void SetTaggerResults(RVec<float> deepjet, RVec<float> pnet, RVec<float> parT)
  {
    j_btagDeepFlav[JetTagging::JetFlavTaggerScoreType::B] = deepjet[0];
    j_btagDeepFlav[JetTagging::JetFlavTaggerScoreType::CvB] = deepjet[1];
    j_btagDeepFlav[JetTagging::JetFlavTaggerScoreType::CvL] = deepjet[2];
    j_btagDeepFlav[JetTagging::JetFlavTaggerScoreType::QvG] = deepjet[3];

    j_btagPNet[JetTagging::JetFlavTaggerScoreType::B] = pnet[0];
    j_btagPNet[JetTagging::JetFlavTaggerScoreType::CvB] = pnet[1];
    j_btagPNet[JetTagging::JetFlavTaggerScoreType::CvL] = pnet[2];
    j_btagPNet[JetTagging::JetFlavTaggerScoreType::CvNotB] = pnet[3];
    j_btagPNet[JetTagging::JetFlavTaggerScoreType::QvG] = pnet[4];
    j_btagPNet[JetTagging::JetFlavTaggerScoreType::TauVJet] = pnet[5];

    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::B] = parT[0];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::CvB] = parT[1];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::CvL] = parT[2];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::CvNotB] = parT[3];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::Ele] = parT[4];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::Mu] = parT[5];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::QvG] = parT[6];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::SvCB] = parT[7];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::SvUDG] = parT[8];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::TauVJet] = parT[9];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::probUDG] = parT[10];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::probB] = parT[11];
    j_btagUParTAK4[JetTagging::JetFlavTaggerScoreType::probBB] = parT[12];
  };
  inline void SetEnergyFractions(float cH, float nH, float nEM, float cEM, float muE)
  {
    j_chHEF = cH;
    j_neHEF = nH;
    j_neEmEF = nEM;
    j_chEmEF = cEM;
    j_muEF = muE;
  };
  inline float chHEF() const { return j_chHEF; }
  inline float neHEF() const { return j_neHEF; }
  inline float neEmEF() const { return j_neEmEF; }
  inline float chEmEF() const { return j_chEmEF; }
  inline float muEF() const { return j_muEF; }

  inline void SetMultiplicities(unsigned char nC, unsigned char nEl, unsigned char nM, unsigned char nSV) {
    // typecast before assign
    j_nConstituents = short(nC);
    j_nElectrons = short(nEl);
    j_nMuons = short(nM);
    j_nSVs = short(nSV);
  };
  inline short nConstituents() const { return j_nConstituents; }
  inline short nElectrons() const { return j_nElectrons; }
  inline short nMuons() const { return j_nMuons; }
  inline short nSVs() const { return j_nSVs; }

  // For NanoAODv13
  inline void SetHadronMultiplicities(unsigned char chMult, unsigned char neMult){
    j_chMultiplicity = chMult;
    j_neMultiplicity = neMult;
  };
  inline unsigned char chMultiplicity() const { return j_chMultiplicity; }
  inline unsigned char neMultiplicity() const { return j_neMultiplicity; }

  inline void SetMatchingIndices(short e1, short e2, short m1, short m2, short sv1, short sv2, short gj)
  {
    j_electronIdx1 = e1;
    j_electronIdx2 = e2;
    j_muonIdx1 = m1;
    j_muonIdx2 = m2;
    j_svIdx1 = sv1;
    j_svIdx2 = sv2;
    j_genJetIdx = gj;
  };

  inline void SetMatchingIndices(short e1, short e2, short m1, short m2, short sv1, short sv2)
  {
    j_electronIdx1 = e1;
    j_electronIdx2 = e2;
    j_muonIdx1 = m1;
    j_muonIdx2 = m2;
    j_svIdx1 = sv1;
    j_svIdx2 = sv2;
  };


  inline void SetOriginalIndex(int idx) { j_originalIndex = idx; };
  inline int OriginalIndex() const { return j_originalIndex; };

  inline void SetJetPuIDScore(float puIDScore) {
    j_puIDScore = puIDScore;
  };

  inline void SetCorrections(RVec<float> corrs) {
    j_PNetRegPtRawCorr = corrs[0];
    j_PNetRegPtRawCorrNeutrino = corrs[1];
    j_PNetRegPtRawRes = corrs[2];
    j_UParTAK4RegPtRawCorr = corrs[3];
    j_UParTAK4RegPtRawCorrNeutrino = corrs[4];
    j_UParTAK4RegPtRawRes = corrs[5];
    j_UParTAK4V1RegPtRawCorr = corrs[6];
    j_UParTAK4V1RegPtRawCorrNeutrino = corrs[7];
    j_UParTAK4V1RegPtRawRes = corrs[8];
    j_rawFactor = corrs[9];
  };

  inline void SetM(double jet_m) {
    j_m = jet_m;
  };
  
  inline void SetUnsmearedP4(Jet jet) {
    j_unsmearedP4 = jet;
  };
  inline double GetM() { return j_m; }
  inline int partonFlavour() const { return j_partonFlavour; };
  inline int hadronFlavour() const { return j_hadronFlavour; };
  inline int genJetIdx() const { return j_genJetIdx; };
  inline float chargedEMFraction() const { return j_chEmEF; }
  inline float chargedHadronFraction() const { return j_chHEF; }
  inline float neutralEMFraction() const { return j_neEmEF; }
  inline float neutralHadronFraction() const { return j_neHEF; }
  inline float EMFraction() const { return j_chEmEF + j_neEmEF; }
  float GetTaggerResult(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType) const;
  TLorentzVector GetUnsmearedP4() const;

private:
  // For matching indices in leptons
  int j_originalIndex; 
  // jetPuID
  float j_puIDScore;
  // corrections
  float j_PNetRegPtRawCorr;
  float j_PNetRegPtRawCorrNeutrino;
  float j_PNetRegPtRawRes;
  float j_UParTAK4RegPtRawCorr;
  float j_UParTAK4RegPtRawCorrNeutrino;
  float j_UParTAK4RegPtRawRes;
  float j_UParTAK4V1RegPtRawCorr;
  float j_UParTAK4V1RegPtRawCorrNeutrino;
  float j_UParTAK4V1RegPtRawRes;
  float j_rawFactor;
  // flav. tagging scores
  //deepjet
  unordered_map<JetTagging::JetFlavTaggerScoreType, float> j_btagDeepFlav;
  //pnet
  unordered_map<JetTagging::JetFlavTaggerScoreType, float> j_btagPNet;
  // part
  unordered_map<JetTagging::JetFlavTaggerScoreType, float> j_btagUParTAK4;
  // jet substructure
  float j_chEmEF;
  float j_chHEF;
  float j_neEmEF;
  float j_neHEF;
  float j_muEF;
  // float j_muonSubtrFactor;
  short j_nConstituents;
  short j_nElectrons;
  short j_nMuons;
  short j_nSVs;
  short j_chMultiplicity;
  short j_neMultiplicity;

  // matching information
  short j_electronIdx1;
  short j_electronIdx2;
  short j_muonIdx1;
  short j_muonIdx2;
  short j_svIdx1;
  short j_svIdx2;
  short j_genJetIdx;
  // jet Flav.
  short j_hadronFlavour;
  short j_partonFlavour;
  // others
  float j_area;
  // int j_hfadjacentEtaStripsSize;
  // int j_hfcentralEtaStripSize;
  // float j_hfsigmaEtaEta;
  // float j_hfsigmaPhiPhi;
  float j_m; // jet mass
  TLorentzVector j_unsmearedP4;
  ClassDef(Jet, 1)
};

#endif
