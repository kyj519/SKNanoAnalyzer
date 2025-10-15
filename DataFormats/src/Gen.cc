#include "Gen.h"
#include <utility>
#include <limits>

ClassImp(Gen)

void Gen::initializeMembers() {
    storage_.reset();
    index_ = std::numeric_limits<std::size_t>::max();

    j_IsEmpty = true;
    j_index = -1;
    j_PID = 0;
    j_status = 0;
    j_mother_index = -1;
    j_status_flags = 0;

    j_isPrompt = false;
    j_isDecayedLeptonHadron = false;
    j_isTauDecayProduct = false;
    j_isPromptTauDecayProduct = false;
    j_isDirectTauDecayProduct = false;
    j_isDirectPromptTauDecayProduct = false;
    j_isDirectHadronDecayProduct = false;
    j_isHardProcess = false;
    j_fromHardProcess = false;
    j_isHardProcessTauDecayProduct = false;
    j_isDirectHardProcessTauDecayProduct = false;
    j_fromHardProcessBeforeFSR = false;
    j_isFirstCopy = false;
    j_isLastCopy = false;
    j_isLastCopyBeforeFSR = false;
}

Gen::Gen() : Particle() {
    initializeMembers();
}

Gen::Gen(std::shared_ptr<const GenSoA> storage, std::size_t index) : Particle() {
    initializeMembers();
    storage_ = storage;
    index_ = index;
    materialize();
}

Gen::~Gen() {}

void Gen::materialize() const {
    if (!storage_)
        return;

    auto self = const_cast<Gen *>(this);
    self->SetIsEmpty(false);
    self->SetPtEtaPhiM(storage_->pt[index_], storage_->eta[index_], storage_->phi[index_], storage_->mass[index_]);
    self->SetIndexPIDStatus(static_cast<int>(index_), storage_->pdgId[index_], storage_->status[index_]);
    self->SetMother(storage_->motherIdx[index_]);
    self->SetGenStatusFlags(storage_->statusFlags[index_]);

    self->storage_.reset();
    self->index_ = std::numeric_limits<std::size_t>::max();
}

void Gen::SetIsEmpty(bool b){
    j_IsEmpty = b;
}

void Gen::SetIndexPIDStatus(int i, int p, int t){
    j_index = i;
    j_PID = p;
    j_status = t;
}

void Gen::SetMother(int mindex){
    j_mother_index = mindex;
}

void Gen::SetGenStatusFlags(unsigned short f){
    j_status_flags = f;
    decodeStatusFlags();
}

void Gen::decodeStatusFlags() {
    j_isPrompt = j_status_flags & (1UL << 0);
    j_isDecayedLeptonHadron = j_status_flags & (1UL << 1);
    j_isTauDecayProduct = j_status_flags & (1UL << 2);
    j_isPromptTauDecayProduct = j_status_flags & (1UL << 3);
    j_isDirectTauDecayProduct = j_status_flags & (1UL << 4);
    j_isDirectPromptTauDecayProduct = j_status_flags & (1UL << 5);
    j_isDirectHadronDecayProduct = j_status_flags & (1UL << 6);
    j_isHardProcess = j_status_flags & (1UL << 7);
    j_fromHardProcess = j_status_flags & (1UL << 8);
    j_isHardProcessTauDecayProduct = j_status_flags & (1UL << 9);
    j_isDirectHardProcessTauDecayProduct = j_status_flags & (1UL << 10);
    j_fromHardProcessBeforeFSR = j_status_flags & (1UL << 11);
    j_isFirstCopy = j_status_flags & (1UL << 12);
    j_isLastCopy = j_status_flags & (1UL << 13);
    j_isLastCopyBeforeFSR = j_status_flags & (1UL << 14);
}

void Gen::Print() const{
    materialize();
    cout << "(Index, PID, Status, MotherIndex, Pt, Eta, Phi, M) = " << Index() << "	" << PID() << "	" << Status() << "	" << MotherIndex() << "	" << Pt() << "	" << Eta() << "	" << Phi() << "	" << M() << endl;
}
