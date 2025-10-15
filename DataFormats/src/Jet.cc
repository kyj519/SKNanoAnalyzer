#include "Jet.h"
#include <stdexcept>
#include <limits>
#include <iostream>

ClassImp(Jet)

void Jet::initializeMembers() {
  storage_.reset();
  index_ = std::numeric_limits<std::size_t>::max();
  cachedProperties_.reset();
  lazy_.reset();

  jet_rawPt = -999.f;
  jet_originalPt = -999.f;
  j_originalIndex = -1;
  j_puIDScore = -999.f;
  j_PNetRegPtRawCorr = -999.f;
  j_PNetRegPtRawCorrNeutrino = -999.f;
  j_PNetRegPtRawRes = -999.f;
  j_UParTAK4RegPtRawCorr = -999.f;
  j_UParTAK4RegPtRawCorrNeutrino = -999.f;
  j_UParTAK4RegPtRawRes = -999.f;
  j_UParTAK4V1RegPtRawCorr = -999.f;
  j_UParTAK4V1RegPtRawCorrNeutrino = -999.f;
  j_UParTAK4V1RegPtRawRes = -999.f;
  j_rawFactor = -999.f;

  j_btagDeepFlav.clear();
  j_btagPNet.clear();
  j_btagUParTAK4.clear();

  j_chEmEF = -999.f;
  j_chHEF = -999.f;
  j_neEmEF = -999.f;
  j_neHEF = -999.f;
  j_muEF = -999.f;
  j_nConstituents = -999;
  j_nElectrons = -999;
  j_nMuons = -999;
  j_nSVs = -999;
  j_chMultiplicity = -1;
  j_neMultiplicity = -1;

  j_electronIdx1 = -999;
  j_electronIdx2 = -999;
  j_muonIdx1 = -999;
  j_muonIdx2 = -999;
  j_svIdx1 = -999;
  j_svIdx2 = -999;
  j_genJetIdx = -999;

  j_hadronFlavour = -999;
  j_partonFlavour = -999;

  j_area = -999.f;
  j_m = -999.f;
  j_unsmearedP4.SetPxPyPzE(-999., -999., -999., -999.);
  j_constituents.clear();
}

Jet::Jet() : Particle() {
  initializeMembers();
}

Jet::Jet(std::shared_ptr<const JetSoA> storage, std::size_t index) : Particle() {
  initializeMembers();
  storage_ = std::move(storage);
  index_ = index;
  if (storage_) {
    const float pt = storage_->pt[index_];
    const float eta = storage_->eta[index_];
    const float phi = storage_->phi[index_];
    const float mass = storage_->mass[index_];
    SetPtEtaPhiM(pt, eta, phi, mass);

    jet_originalPt = storage_->pt[index_];
    jet_rawPt = storage_->pt[index_] * (1.f - storage_->rawFactor[index_]);
    j_rawFactor = storage_->rawFactor[index_];
    j_area = storage_->area[index_];

    SetEnergyFractions(storage_->chHEF[index_], storage_->neHEF[index_], storage_->neEmEF[index_], storage_->chEmEF[index_], storage_->muEF[index_]);
    SetHadronMultiplicities(storage_->chMultiplicity[index_], storage_->neMultiplicity[index_]);
    SetMultiplicities(storage_->nConstituents[index_], storage_->nElectrons[index_], storage_->nMuons[index_], storage_->nSVs[index_]);

    const short e1 = storage_->electronIdx1[index_];
    const short e2 = storage_->electronIdx2[index_];
    const short m1 = storage_->muonIdx1[index_];
    const short m2 = storage_->muonIdx2[index_];
    const short sv1 = storage_->svIdx1[index_];
    const short sv2 = storage_->svIdx2[index_];
    const short gj = storage_->genJetIdx[index_];
    SetMatchingIndices(e1, e2, m1, m2, sv1, sv2, gj);

    SetJetFlavours(storage_->partonFlavour[index_], static_cast<unsigned char>(storage_->hadronFlavour[index_]));

    j_PNetRegPtRawCorr = storage_->pnetRegPtRawCorr[index_];
    j_PNetRegPtRawCorrNeutrino = storage_->pnetRegPtRawCorrNeutrino[index_];
    j_PNetRegPtRawRes = storage_->pnetRegPtRawRes[index_];
    j_UParTAK4RegPtRawCorr = storage_->uparTAK4RegPtRawCorr[index_];
    j_UParTAK4RegPtRawCorrNeutrino = storage_->uparTAK4RegPtRawCorrNeutrino[index_];
    j_UParTAK4RegPtRawRes = storage_->uparTAK4RegPtRawRes[index_];
    j_UParTAK4V1RegPtRawCorr = storage_->uparTAK4V1RegPtRawCorr[index_];
    j_UParTAK4V1RegPtRawCorrNeutrino = storage_->uparTAK4V1RegPtRawCorrNeutrino[index_];
    j_UParTAK4V1RegPtRawRes = storage_->uparTAK4V1RegPtRawRes[index_];

    j_puIDScore = storage_->puIdDisc[index_];
  }
}

Jet::~Jet() {}

void Jet::AttachLazyPayload(void *context, EnsureCallback callback, int index) const {
  materialize();
  if (!lazy_) {
    lazy_ = std::make_shared<LazyPayload>();
  }
  lazy_->context = context;
  lazy_->callback = callback;
  lazy_->index = index;
  lazy_->loaded = cachedProperties_;
}

void Jet::DetachLazyPayload() const {
  lazy_.reset();
}

void Jet::ensure(Property property) const {
  const auto bit = static_cast<std::size_t>(property);
  if (cachedProperties_.test(bit))
    return;

  if (storage_) {
    loadFromStorage(property);
    return;
  }

  if (!lazy_ || !lazy_->callback)
    return;

  lazy_->callback(lazy_->context, const_cast<Jet &>(*this), property);
  markLoaded(property);
}

void Jet::markLoaded(Property property) const {
  const auto bit = static_cast<std::size_t>(property);
  cachedProperties_.set(bit);
  if (lazy_)
    lazy_->loaded.set(bit);
}

void Jet::materialize() const {
  if (!storage_)
    return;

  auto self = const_cast<Jet *>(this);
  const auto idx = index_;

  const float pt = storage_->pt[idx];
  const float eta = storage_->eta[idx];
  const float phi = storage_->phi[idx];
  const float mass = storage_->mass[idx];
  self->SetPtEtaPhiM(pt, eta, phi, mass);

  self->jet_originalPt = storage_->pt[idx];
  self->jet_rawPt = storage_->pt[idx] * (1.f - storage_->rawFactor[idx]);
  self->j_rawFactor = storage_->rawFactor[idx];
  self->j_area = storage_->area[idx];

  self->SetEnergyFractions(storage_->chHEF[idx], storage_->neHEF[idx], storage_->neEmEF[idx], storage_->chEmEF[idx], storage_->muEF[idx]);
  self->SetHadronMultiplicities(storage_->chMultiplicity[idx], storage_->neMultiplicity[idx]);
  self->SetMultiplicities(storage_->nConstituents[idx], storage_->nElectrons[idx], storage_->nMuons[idx], storage_->nSVs[idx]);

  const short e1 = storage_->electronIdx1[idx];
  const short e2 = storage_->electronIdx2[idx];
  const short m1 = storage_->muonIdx1[idx];
  const short m2 = storage_->muonIdx2[idx];
  const short sv1 = storage_->svIdx1[idx];
  const short sv2 = storage_->svIdx2[idx];
  const short gj = storage_->genJetIdx[idx];
  self->SetMatchingIndices(e1, e2, m1, m2, sv1, sv2, gj);

  self->SetJetFlavours(storage_->partonFlavour[idx], static_cast<unsigned char>(storage_->hadronFlavour[idx]));

  self->j_PNetRegPtRawCorr = storage_->pnetRegPtRawCorr[idx];
  self->j_PNetRegPtRawCorrNeutrino = storage_->pnetRegPtRawCorrNeutrino[idx];
  self->j_PNetRegPtRawRes = storage_->pnetRegPtRawRes[idx];
  self->j_UParTAK4RegPtRawCorr = storage_->uparTAK4RegPtRawCorr[idx];
  self->j_UParTAK4RegPtRawCorrNeutrino = storage_->uparTAK4RegPtRawCorrNeutrino[idx];
  self->j_UParTAK4RegPtRawRes = storage_->uparTAK4RegPtRawRes[idx];
  self->j_UParTAK4V1RegPtRawCorr = storage_->uparTAK4V1RegPtRawCorr[idx];
  self->j_UParTAK4V1RegPtRawCorrNeutrino = storage_->uparTAK4V1RegPtRawCorrNeutrino[idx];
  self->j_UParTAK4V1RegPtRawRes = storage_->uparTAK4V1RegPtRawRes[idx];
  self->j_puIDScore = storage_->puIdDisc[idx];

  for (std::size_t bit = 0; bit < static_cast<std::size_t>(Property::Count); ++bit) {
    if (!cachedProperties_.test(bit))
      loadFromStorage(static_cast<Property>(bit));
  }

  self->storage_.reset();
  self->index_ = std::numeric_limits<std::size_t>::max();
}

void Jet::loadFromStorage(Property property) const {
  if (!storage_)
    return;

  const auto idx = index_;
  auto self = const_cast<Jet *>(this);

  using Tagger = JetTagging::JetFlavTagger;
  using Score = JetTagging::JetFlavTaggerScoreType;

  switch (property) {
  case Property::DeepFlavB:
    self->SetTaggerScore(Tagger::DeepJet, Score::B, storage_->deepFlavB[idx]);
    break;
  case Property::DeepFlavCvB:
    self->SetTaggerScore(Tagger::DeepJet, Score::CvB, storage_->deepFlavCvB[idx]);
    break;
  case Property::DeepFlavCvL:
    self->SetTaggerScore(Tagger::DeepJet, Score::CvL, storage_->deepFlavCvL[idx]);
    break;
  case Property::DeepFlavQG:
    self->SetTaggerScore(Tagger::DeepJet, Score::QvG, storage_->deepFlavQG[idx]);
    break;
  case Property::PNetB:
    self->SetTaggerScore(Tagger::ParticleNet, Score::B, storage_->pnetB[idx]);
    break;
  case Property::PNetCvB:
    self->SetTaggerScore(Tagger::ParticleNet, Score::CvB, storage_->pnetCvB[idx]);
    break;
  case Property::PNetCvL:
    self->SetTaggerScore(Tagger::ParticleNet, Score::CvL, storage_->pnetCvL[idx]);
    break;
  case Property::PNetCvNotB:
    self->SetTaggerScore(Tagger::ParticleNet, Score::CvNotB, storage_->pnetCvNotB[idx]);
    break;
  case Property::PNetQvG:
    self->SetTaggerScore(Tagger::ParticleNet, Score::QvG, storage_->pnetQvG[idx]);
    break;
  case Property::PNetTauVJet:
    self->SetTaggerScore(Tagger::ParticleNet, Score::TauVJet, storage_->pnetTauVJet[idx]);
    break;
  case Property::ParTB:
    self->SetTaggerScore(Tagger::ParT, Score::B, storage_->uparTAK4B[idx]);
    break;
  case Property::ParTCvB:
    self->SetTaggerScore(Tagger::ParT, Score::CvB, storage_->uparTAK4CvB[idx]);
    break;
  case Property::ParTCvL:
    self->SetTaggerScore(Tagger::ParT, Score::CvL, storage_->uparTAK4CvL[idx]);
    break;
  case Property::ParTCvNotB:
    self->SetTaggerScore(Tagger::ParT, Score::CvNotB, storage_->uparTAK4CvNotB[idx]);
    break;
  case Property::ParTEle:
    self->SetTaggerScore(Tagger::ParT, Score::Ele, storage_->uparTAK4Ele[idx]);
    break;
  case Property::ParTMu:
    self->SetTaggerScore(Tagger::ParT, Score::Mu, storage_->uparTAK4Mu[idx]);
    break;
  case Property::ParTQvG:
    self->SetTaggerScore(Tagger::ParT, Score::QvG, storage_->uparTAK4QvG[idx]);
    break;
  case Property::ParTSvCB:
    self->SetTaggerScore(Tagger::ParT, Score::SvCB, storage_->uparTAK4SvCB[idx]);
    break;
  case Property::ParTSvUDG:
    self->SetTaggerScore(Tagger::ParT, Score::SvUDG, storage_->uparTAK4SvUDG[idx]);
    break;
  case Property::ParTTauVJet:
    self->SetTaggerScore(Tagger::ParT, Score::TauVJet, storage_->uparTAK4TauVJet[idx]);
    break;
  case Property::ParTUDG:
    self->SetTaggerScore(Tagger::ParT, Score::probUDG, storage_->uparTAK4UDG[idx]);
    break;
  case Property::ParTProbB:
    self->SetTaggerScore(Tagger::ParT, Score::probB, storage_->uparTAK4ProbB[idx]);
    break;
  case Property::ParTProbBB:
    self->SetTaggerScore(Tagger::ParT, Score::probBB, storage_->uparTAK4ProbBB[idx]);
    break;
  default:
    break;
  }

  markLoaded(property);
}

Jet::Property Jet::propertyFor(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType scoreType) {
  using Tagger = JetTagging::JetFlavTagger;
  using Score = JetTagging::JetFlavTaggerScoreType;
  switch (tagger) {
    case Tagger::DeepJet:
      switch (scoreType) {
        case Score::B: return Property::DeepFlavB;
        case Score::CvB: return Property::DeepFlavCvB;
        case Score::CvL: return Property::DeepFlavCvL;
        case Score::QvG: return Property::DeepFlavQG;
        default: break;
      }
      break;
    case Tagger::ParticleNet:
      switch (scoreType) {
        case Score::B: return Property::PNetB;
        case Score::CvB: return Property::PNetCvB;
        case Score::CvL: return Property::PNetCvL;
        case Score::CvNotB: return Property::PNetCvNotB;
        case Score::QvG: return Property::PNetQvG;
        case Score::TauVJet: return Property::PNetTauVJet;
        default: break;
      }
      break;
    case Tagger::ParT:
      switch (scoreType) {
        case Score::B: return Property::ParTB;
        case Score::CvB: return Property::ParTCvB;
        case Score::CvL: return Property::ParTCvL;
        case Score::CvNotB: return Property::ParTCvNotB;
        case Score::Ele: return Property::ParTEle;
        case Score::Mu: return Property::ParTMu;
        case Score::QvG: return Property::ParTQvG;
        case Score::SvCB: return Property::ParTSvCB;
        case Score::SvUDG: return Property::ParTSvUDG;
        case Score::TauVJet: return Property::ParTTauVJet;
        case Score::probUDG: return Property::ParTUDG;
        case Score::probB: return Property::ParTProbB;
        case Score::probBB: return Property::ParTProbBB;
        default: break;
      }
      break;
    default:
      break;
  }
  throw std::runtime_error("[Jet::propertyFor] Unsupported tagger/score combination");
}

void Jet::ensureScore(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType scoreType) const {
  ensure(propertyFor(tagger, scoreType));
}

void Jet::SetTaggerScore(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType scoreType, float value) {
  switch (tagger) {
    case JetTagging::JetFlavTagger::DeepJet:
      j_btagDeepFlav[scoreType] = value;
      break;
    case JetTagging::JetFlavTagger::ParticleNet:
      j_btagPNet[scoreType] = value;
      break;
    case JetTagging::JetFlavTagger::ParT:
      j_btagUParTAK4[scoreType] = value;
      break;
    default:
      break;
  }
  markLoaded(propertyFor(tagger, scoreType));
}

float Jet::GetTaggerResult(JetTagging::JetFlavTagger tagger, JetTagging::JetFlavTaggerScoreType scoreType) const {
    ensureScore(tagger, scoreType);
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
    auto it = taggerResult->find(scoreType);
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
