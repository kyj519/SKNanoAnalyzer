#ifndef Gen_h
#define Gen_h

#include <memory>
#include <limits>

#include "Particle.h"
#include "GenView.h"

class Gen: public Particle {
public:

    Gen();
    Gen(std::shared_ptr<const GenSoA> storage, std::size_t index);
    ~Gen();

    //==== For empty Gen
    void SetIsEmpty(bool b);
    inline bool IsEmpty() const { return j_IsEmpty; }

    void SetIndexPIDStatus(int i, int p, int t);
    inline int Index() const { materialize(); return j_index; }
    inline int PID() const { materialize(); return j_PID; }
    inline int Status() const { materialize(); return j_status; }
    void SetMother(int mindex);
    inline int MotherIndex() const { materialize(); return j_mother_index; }

    //==== Gen Status Flags
    void SetGenStatusFlags(unsigned short f);

    inline bool isPrompt() const { materialize(); return j_status_flags & (1UL<<0); }
    inline bool isDecayedLeptonHadron() const { materialize(); return j_status_flags & (1UL<<1); }
    inline bool isTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<2); }
    inline bool isPromptTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<3); }
    inline bool isDirectTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<4); }
    inline bool isDirectPromptTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<5); }
    inline bool isDirectHadronDecayProduct() const { materialize(); return j_status_flags & (1UL<<6); }
    inline bool isHardProcess() const { materialize(); return j_status_flags & (1UL<<7); }
    inline bool fromHardProcess() const { materialize(); return j_status_flags & (1UL<<8); }
    inline bool isHardProcessTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<9); }
    inline bool isDirectHardProcessTauDecayProduct() const { materialize(); return j_status_flags & (1UL<<10); }
    inline bool fromHardProcessBeforeFSR() const { materialize(); return j_status_flags & (1UL<<11); }
    inline bool isFirstCopy() const { materialize(); return j_status_flags & (1UL<<12); }
    inline bool isLastCopy() const { materialize(); return j_status_flags & (1UL<<13); }
    inline bool isLastCopyBeforeFSR() const { materialize(); return j_status_flags & (1UL<<14); }

    virtual void Print() const;

private:

    void initializeMembers();
    void materialize() const;
    void decodeStatusFlags();

    bool j_IsEmpty;
    int j_index, j_PID, j_status;
    int j_mother_index;
    unsigned short j_status_flags;
    bool j_isPrompt, j_isDecayedLeptonHadron, j_isTauDecayProduct, j_isPromptTauDecayProduct, j_isDirectTauDecayProduct, j_isDirectPromptTauDecayProduct, j_isDirectHadronDecayProduct, j_isHardProcess, j_fromHardProcess, j_isHardProcessTauDecayProduct, j_isDirectHardProcessTauDecayProduct, j_fromHardProcessBeforeFSR, j_isFirstCopy, j_isLastCopy, j_isLastCopyBeforeFSR;

    std::shared_ptr<const GenSoA> storage_;
    std::size_t index_ = std::numeric_limits<std::size_t>::max();

    ClassDef(Gen,1)
};

#endif
