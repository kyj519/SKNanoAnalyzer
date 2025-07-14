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

    //[[nodiscard]]             
    bool init(TTreeReader& reader, const char* branchName) {
        auto* tree = reader.GetTree();
        if (!tree || !tree->GetBranch(branchName)) return false;

        tree->SetBranchStatus(branchName, 1);
        myArray = std::make_unique<TTreeReaderArray<T>>(reader, branchName);
        return true;
    }

    const T& operator[](std::size_t i) const noexcept {
        static const T dummy{};                    
        if (!myArray || i >= myArray->GetSize()) return dummy;
        return (*myArray)[i];
    }

    std::size_t size() const noexcept {
        return myArray ? myArray->GetSize() : 0;
    }

    bool valid() const noexcept { return static_cast<bool>(myArray); }

    void reset() noexcept { myArray.reset(); }

private:
    std::unique_ptr<TTreeReaderArray<T>> myArray;
};

template <typename T>
class TTreeReaderValueWrapper {
public:
    TTreeReaderValueWrapper() = default;

    //[[nodiscard]]
    bool init(TTreeReader& reader, const char* branchName) {
        auto* tree = reader.GetTree();
        if (!tree || !tree->GetBranch(branchName)) return false;

        tree->SetBranchStatus(branchName, 1);
        myValue = std::make_unique<TTreeReaderValue<T>>(reader, branchName);
        return true;
    }

    const T& get() const noexcept {
        static const T dummy{};
        return myValue ? **myValue : dummy;
    }

    operator const T&() const noexcept { return get(); }

    bool valid() const noexcept { return static_cast<bool>(myValue); }

    void reset() noexcept { myValue.reset(); }

private:
    std::unique_ptr<TTreeReaderValue<T>> myValue;
};

#endif