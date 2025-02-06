#ifndef MttQuestionnaire_DL_h
#define MttQuestionnaire_DL_h

#include "MttQuestionnaire.h"

class MttQuestionnaire_DL : public MttQuestionnaire 
{
public:
    bool PassBaseLineSelection(bool remove_flavtagging_cut = false) override;
    inline bool CheckChannel() override
    {
        if (channel != Channel::MM && channel != Channel::EE && channel != Channel::ME) return false;
        return true;
    };

    inline float MCNormalization() override { 
        if (channel == Channel::MM){
            return MCweight()*ev.GetTriggerLumi(Mu_Trigger[DataEra.Data()]);
        }
        else if (channel == Channel::EE){
            return MCweight()*ev.GetTriggerLumi(El_Trigger[DataEra.Data()]);
        }
        else if (channel == Channel::ME){
            return MCweight()*(ev.GetTriggerLumi(Mu_Trigger[DataEra.Data()]));
        }
        else throw std::runtime_error("Invalid channel");
    }

    inline std::string GetRegionString() override
    {
        return "CR_DL";
    }

    MttQuestionnaire_DL();
    ~MttQuestionnaire_DL() override = default;
    unique_ptr<CorrectionSet> cset_TriggerSF;

};

#endif
