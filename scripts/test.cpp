#include <algorithm>
void test() {
    Vcb_SL module;
    //MeasureJetTaggingR module;
    module.SetTreeName("Events");
    module.LogEvery = 1000;
    module.IsDATA = false;
    module.MCSample = "TTLJ_powheg";
    module.xsec = 405.685352016;
    module.sumW = 25677969408.0;
    module.sumSign = 76333768.0;
    module.SetEra("2023BPix");
    module.Userflags = {
        "Mu",
        "doBreakdown"
        //"Training",
        //"SL"
    };
    module.AddFile("/gv0/DATA/SKNano/Run3NanoAODv12/2022/MC/TTtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8/240702_161307/0000/tree_101.root");
    module.SetOutfilePath("hists_101.root");
    module.MaxEvent = std::max(1, static_cast<int>(module.fChain->GetEntries()/100));
    module.Init();
    module.initializeAnalyzer();
    module.Loop();
    module.WriteHist();
}