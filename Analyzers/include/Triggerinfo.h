#ifndef Triggerinfo_h
#define Triggerinfo_h
#include <memory>
#include "TTreeReaderWrappers.h"   // Bool_t 래퍼에 필요
#include "TString.h"

struct TriggerInfo {
    std::unique_ptr<TTreeReaderValueWrapper<Bool_t>> hlt;
    float lumi = 0.f;
    bool  alwaysTrue = false;
};

using TriggerMap_t =
    std::map<TString, std::unique_ptr<TriggerInfo>>;

#endif // Triggerinfo_h