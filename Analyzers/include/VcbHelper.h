#include <ROOT/RVec.hxx>
#include "Jet.h"
#include "JetTaggingParameter.h"
using ROOT::VecOps::RVec;

enum class VcbCat { N0, L0, C0, C1, C2, C3, C4, B0, B1, B2, B3, B4 };

// ---------------- thresholds (half-open [low, high), 마지막 구간만 high 포함) ----------------
static constexpr double HF_T1 = 0.264, HF_T2 = 0.448, HF_T3 = 0.767;                        // HFvLF
static constexpr double BVC_T1 = 0.01,  BVC_T2 = 0.028, BVC_T3 = 0.094, BVC_T4 = 0.69,      // BvC
                        BVC_T5 = 0.918, BVC_T6 = 0.978, BVC_T7 = 0.994;

// branchless binning (side="right")
inline int bin_hf(double x) {
  return int(x >= HF_T1) + int(x >= HF_T2) + int(x >= HF_T3);   // 0..3
}
inline int bin_bvc(double y) {
  return int(y >= BVC_T1) + int(y >= BVC_T2) + int(y >= BVC_T3) +
         int(y >= BVC_T4) + int(y >= BVC_T5) + int(y >= BVC_T6) +
         int(y >= BVC_T7);                                      // 0..7
}

// ---------------- HFvLF, BvC (네 파이썬 로직 그대로) ----------------
inline std::pair<double,double> HFvLF_BvC_from_ParT(const Jet& j) {
  using Tagger = JetTagging::JetFlavTagger;
  using Score  = JetTagging::JetFlavTaggerScoreType;

  const double probudg = j.GetTaggerResult(Tagger::ParT, Score::probUDG);
  const double SvUDG   = j.GetTaggerResult(Tagger::ParT, Score::SvUDG);
  const double CvL     = j.GetTaggerResult(Tagger::ParT, Score::CvL);
  const double CvB     = j.GetTaggerResult(Tagger::ParT, Score::CvB);

  const double probs = (SvUDG > 0.0 && probudg > 0.0 && SvUDG < 1.0)
                         ? SvUDG * probudg / (1.0 - SvUDG) : -1.0;

  const double probc = (CvL > 0.0 && CvL < 1.0 && probs > 0.0 && probudg > 0.0)
                         ? CvL * (probs + probudg) / (1.0 - CvL) : -1.0;

  const double probbbblepb = (CvB > 0.0 && probc > 0.0)
                               ? (1.0 - CvB) * probc / CvB : -1.0;

  const double BvC = (CvB > 0.0) ? (1.0 - CvB) : -1.0;

  const double HFvLF = (probbbblepb > 0.0 && probc > 0.0 && probs > 0.0 && probudg > 0.0)
                         ? (probbbblepb + probc) / (probbbblepb + probc + probs + probudg)
                         : -1.0;

  return {HFvLF, BvC};
}

// ---------------- 카테고리 결정 (2×1D bin + LUT) ----------------
inline VcbCat classify_from_scores(double hf, double bvc) {
  if (!(hf >= 0.0 && hf <= 1.0 && bvc >= 0.0 && bvc <= 1.0)) return VcbCat::N0;

  const int ih = bin_hf(hf);
  if (ih == 0) return VcbCat::L0;
  if (ih == 1) return VcbCat::C0;
  if (ih == 2) return VcbCat::C1;

  // ih == 3 → 상단 행: BvC로 세분
  static constexpr VcbCat TOP_MAP[8] = {VcbCat::C4, VcbCat::C3, VcbCat::C2, VcbCat::B0,
                                     VcbCat::B1, VcbCat::B2, VcbCat::B3, VcbCat::B4};
  const int jb = bin_bvc(bvc);
  return TOP_MAP[jb];
}

// ---------------- 메인: RVec<Jet> → RVec<VcbCat> ----------------
inline RVec<VcbCat> ClassifyJets_ParT(const RVec<Jet>& jets) {
  RVec<VcbCat> out; out.reserve(jets.size());
  for (const auto& j : jets) {
    auto [hf, bvc] = HFvLF_BvC_from_ParT(j);
    out.emplace_back(classify_from_scores(hf, bvc));
  }
  return out;
}