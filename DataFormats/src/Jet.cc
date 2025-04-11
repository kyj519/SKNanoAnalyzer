#include "Jet.h"

ClassImp(Jet)

Jet::Jet() : Particle() {
  // Jet ID
  j_looseJetId = false;
  j_tightJetID = false;
  j_tightLepVetoJetID = false;

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

//////////

bool Jet::PassID(TString ID) const {
  //TODO: implement JetID
  // if (ID == "loose")
  //   return j_looseJetId;
  // else if (ID == "tight")
  //   return j_tightJetID;
  // else if (ID == "tightLepVeto")
  //   return j_tightLepVetoJetID;
  // else if (ID == "loosePuId")
  //   return j_loosePuId;
  // else if (ID == "mediumPuId")
  //   return j_mediumPuId;
  // else if (ID == "tightPuId")
  //   return j_tightPuId;
  // else if (ID == "NOCUT") 
  //   return true;
  // else
  //   cout << "[Jet::PassID] No id : " << ID << endl;

  // exit(ENODATA);

  return false;
}

void Jet::SetJetID(unsigned char IDBit, int Run) {
  // bit 0 is loose, bit 1 is tight, bit 2 is tightLepVeto
  if(Run == 2) {
    j_looseJetId = (IDBit & 1);
    j_tightJetID = (IDBit & 2);
    j_tightLepVetoJetID = (IDBit & 4);
  }
  // due to the bug in the NanoAODv12, Below fix is necessary
  // Please refer https://gitlab.cern.ch/cms-jetmet/coordination/coordination/-/issues/117 or https://twiki.cern.ch/twiki/bin/viewauth/CMS/JetID13p6TeV#Recommendations_for_the_13_6_AN1
  // We are moving to NanoAODv13 for Run3, need different implementation
  else { // Run == 3
    j_tightJetID = false;
    if (abs(Eta()) <= 2.6) 
      j_tightJetID = (neHEF() < 0.99) && (neEmEF() < 0.9) && (chMultiplicity()+neMultiplicity() > 1) && (chHEF() > 0.01) && (chMultiplicity() > 0);
    else if (abs(Eta()) > 2.6 && abs(Eta()) <= 2.7) 
      j_tightJetID = (neHEF() < 0.90) && (neEmEF() < 0.99);
    else if (abs(Eta()) > 2.7 && abs(Eta()) <= 3.0) 
      j_tightJetID = (neHEF() < 0.99);
    else // abs(eta) > 3.0
      j_tightJetID = (neMultiplicity() >= 2) && (neEmEF() < 0.4);

    j_tightLepVetoJetID = false;
    if (abs(Eta()) <= 2.7) 
      j_tightLepVetoJetID = j_tightJetID && (muEF() < 0.8) && (chEmEF() < 0.8);
    else 
      j_tightLepVetoJetID = j_tightJetID;

    /* This is for NanoAODv12
    if (abs(eta) <= 2.7) Jet_passJetIdTight = b & (1 << 1);
    else if (abs(eta) > 2.7 && abs(eta) <= 3.0) Jet_passJetIdTight = (b & (1 << 1)) && (Jet_neHEF < 0.99);
    else if (abs(eta) > 3.0) Jet_passJetIdTight = (b & (1 << 1)) && (Jet_neEmEF < 0.4);      
    
    bool Jet_passJetIdTightLepVeto = false;      
    if (abs(eta) <= 2.7) Jet_passJetIdTightLepVeto = Jet_passJetIdTight && (Jet_muEF < 0.8) && (Jet_chEmEF < 0.8);      
    else Jet_passJetIdTightLepVeto = Jet_passJetIdTight;      
    j_looseJetId = (b & 1);      
    j_tightJetID = Jet_passJetIdTight;      
    j_tightLepVetoJetID = Jet_passJetIdTightLepVeto;      
    */
  }
}

bool Jet::PassID(JetID id) const {
  //TODO: implement JetID
  // switch(id){
  //   case JetID::LOOSE:
  //     return j_looseJetId;
  //   case JetID::TIGHT:
  //     return j_tightJetID;
  //   case JetID::TIGHTLEPVETO:
  //     return j_tightLepVetoJetID;
  //   case JetID::PUID_LOOSE:
  //     return j_loosePuId;
  //   case JetID::PUID_MEDIUM:
  //     return j_mediumPuId;
  //   case JetID::PUID_TIGHT:
  //     return j_tightPuId;
  //   case JetID::NOCUT:
  //     return true;
  //   default:
  //     break;
  // }

  return false;
}

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
