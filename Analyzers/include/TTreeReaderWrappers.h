#ifndef TTreeReaderWrappers_h
#define TTreeReaderWrappers_h
#include <memory>
#include <TTreeReader.h>  
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>


template <typename T>
class TTreeReaderArrayWrapper {
public:
    TTreeReaderArrayWrapper() = default;

    bool init(TTreeReader& reader, const char* branchName) {
        branch_ = branchName;            
        auto* tree = reader.GetTree();
        if (!tree || !tree->GetBranch(branchName)) {
            state_ = State::BranchMissing;
            return false;
        }

        tree->SetBranchStatus(branchName, 1);
        myArray = std::make_unique<TTreeReaderArray<T>>(reader, branchName);
        state_  = State::Initialised;
        return true;
    }

    const T& operator[](std::size_t i) const {      
        switch (state_) {
            case State::Initialised:
                if (i >= myArray->GetSize())
                    std::string msg = "[TTreeReaderArrayWrapper::" + branch_ + "] Index out of range"
                    throw std::out_of_range(msg);
                return (*myArray)[i];

            case State::Uninitialised:
                std::string msg = "[TTreeReaderArrayWrapper::" + branch_ + "] Not initialised";
                throw std::logic_error(msg);

            case State::BranchMissing:
                std::string msg = "[TTreeReaderArrayWrapper::" + branch_ + "] Your Analyzer is trying to access a branch that does not exist in the TTree";
                throw std::runtime_error(msg);
        }
        // silences -Wreturn-type
        throw std::logic_error("Unknown state");
    }

    std::size_t size() const noexcept {
        return (state_ == State::Initialised) ? myArray->GetSize() : 0;
    }

    bool valid() const noexcept { return state_ == State::Initialised; }

    void reset() noexcept {
        myArray.reset();
        state_ = State::Uninitialised;
        branch_.clear();
    }

private:
    enum class State { Uninitialised, Initialised, BranchMissing };

    State                                 state_{State::Uninitialised};
    std::unique_ptr<TTreeReaderArray<T>>  myArray;
    std::string                           branch_;   
};

template <typename T>
class TTreeReaderValueWrapper {
public:
    TTreeReaderValueWrapper() = default;

    bool init(TTreeReader& reader, const char* branchName) {
        branch_ = branchName;  // 예외 메시지용
        auto* tree = reader.GetTree();
        if (!tree || !tree->GetBranch(branchName)) {
            state_ = State::BranchMissing;
            return false;
        }

        tree->SetBranchStatus(branchName, 1);
        myValue = std::make_unique<TTreeReaderValue<T>>(reader, branchName);
        state_  = State::Initialised;
        return true;
    }

    const T& get() const {
        switch (state_) {
            case State::Initialised:
                return **myValue;  
            case State::Uninitialised:
                std::string msg = "[TTreeReaderValueWrapper::" + branch_ + "] Not initialised";
                throw std::logic_error(msg);
            case State::BranchMissing:
                std::string msg = "[TTreeReaderValueWrapper::" + branch_ + "] Your Analyzer is trying to access a branch that does not exist in the TTree";
                throw std::runtime_error(msg);
        }
        throw std::logic_error("TTreeReaderValueWrapper::get(): unknown state.");
    }

    operator const T&() const { return get(); }

    bool valid() const noexcept { return state_ == State::Initialised; }

    void reset() noexcept {
        myValue.reset();
        state_ = State::Uninitialised;
        branch_.clear();
    }

private:
    enum class State { Uninitialised, Initialised, BranchMissing };

    State                                  state_{State::Uninitialised};
    std::unique_ptr<TTreeReaderValue<T>>   myValue;
    std::string                            branch_;
};

#endif