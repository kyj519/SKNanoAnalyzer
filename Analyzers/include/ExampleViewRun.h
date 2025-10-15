#ifndef ExampleViewRun_h
#define ExampleViewRun_h

#include "AnalyzerCore.h"

class ExampleViewRun : public AnalyzerCore {
public:
    ExampleViewRun();
    ~ExampleViewRun();

    void initializeAnalyzer() override;
    void executeEvent() override;

private:
    TString IsoMuTriggerName;
    float TriggerSafePtCut = 0.f;
};

#endif
