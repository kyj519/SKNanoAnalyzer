#include "Jet.h"

ClassImp(Jet)

Jet::Jet() : Particle() {

  // Corrections
  j_PNetRegPtRawCorr = -999.0;
  j_PNetRegPtRawCorrNeutrino = -999.0;
  j_PNetRegPtRawRes = -999.0;
  j_UParTAK4RegPtRawCorr = -999.0;
  j_UParTAK4RegPtRawCorrNeutrino = -999.0;
  j_UParTAK4RegPtRawRes = -999.0;
  j_UParTAK4V1RegPtRawCorr = -999.0;
  j_UParTAK4V1RegPtRawCorrNeutrino = -999.0;
  j_UParTAK4V1RegPtRawRes = -999.0;
  j_rawFactor = -999.0;

  // Jet Substructure
  j_chEmEF = -999.0;
  j_chHEF = -999.0;
  j_neEmEF = -999.0;
  j_neHEF = -999.0;
  j_muEF = -999.0;
  // j_muonSubtrFactor = -999.0;
  j_nConstituents = -999;
  j_nElectrons = -999;
  j_nMuons = -999;
  j_nSVs = -999;

  // Matching Information
  j_electronIdx1 = -999;
  j_electronIdx2 = -999;
  j_muonIdx1 = -999;
  j_muonIdx2 = -999;
  j_svIdx1 = -999;
  j_svIdx2 = -999;
  j_genJetIdx = -999;

  // Jet Flavour
  j_hadronFlavour = -999;
  j_partonFlavour = -999;

  // Others
  j_area = -999.0;
  j_originalIndex = -1;
  // j_hfadjacentEtaStripsSize = -999;
  // j_hfcentralEtaStripSize = -999;
  // j_hfsigmaEtaEta = -999.0;
  // j_hfsigmaPhiPhi = -999.0;
  j_m = -999.0;
  j_unsmearedP4.SetPxPyPzE(-999., -999., -999., -999.);
}

Jet::~Jet() {}



float Jet::GetTaggerResult(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType scoreType) const {
    // First inspect correct tagger - score type is given
    // declare as null reference
    const unordered_map<JetTagging::JetFlavTaggerScoreType, float> *taggerResult = nullptr;
    switch (tagger)
    {
    case JetTagging::JetFlavTagger::DeepJet:
        taggerResult = &j_btagDeepFlav;
        break;
    case JetTagging::JetFlavTagger::ParticleNet:
        taggerResult = &j_btagPNet;
        break;
    case JetTagging::JetFlavTagger::ParT:
        taggerResult = &j_btagUParTAK4;
        break;

    default:
        cout << "[FatJet::GetTaggerResult] No tagger " << JetTagging::GetTaggerCorrectionLibStr(tagger) << endl;
        exit(ENODATA);
    }
    // Now inspect correct score type
    auto it = taggerResult->find(scoreType); // <--- use '->' for pointer access
    if (it == taggerResult->end())
    {
        cout << "[FatJet::GetTaggerResult] No tagger score type "
            << JetTagging::GetJetFlavTaggerScoreTypeStr(scoreType) 
            << " for tagger " << JetTagging::GetTaggerCorrectionLibStr(tagger) << endl;
        exit(ENODATA);
    }
    return it->second;
}

TLorentzVector Jet::GetUnsmearedP4() const{
  return j_unsmearedP4;
}
