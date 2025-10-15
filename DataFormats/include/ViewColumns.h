#ifndef VIEWCOLUMNS_H
#define VIEWCOLUMNS_H

#include <cstddef>

#include "BranchManager.h"

// Generic column view referencing a BranchVector without materialising data.
template <typename T>
class ColumnView {
public:
    using BranchType = BranchVector<T, int>;

    ColumnView() = default;

    void bind(const BranchType *branch) { data = branch; }

    const T &operator[](std::size_t idx) const {
        static const T fallback{};
        return data ? (*data)[idx] : fallback;
    }

    std::size_t size() const { return data ? data->size() : 0; }
    bool empty() const { return size() == 0; }

    const BranchType *branch() const { return data; }

private:
    const BranchType *data = nullptr;
};

// Boolean specialisation using the dedicated BranchVector<bool> implementation.
class BoolColumnView {
public:
    using BranchType = BranchVector<bool, int>;

    BoolColumnView() = default;

    void bind(const BranchType *branch) { data = branch; }

    bool operator[](std::size_t idx) const {
        return data ? static_cast<bool>((*data)[idx]) : false;
    }

    std::size_t size() const { return data ? data->size() : 0; }
    bool empty() const { return size() == 0; }

    const BranchType *branch() const { return data; }

private:
    const BranchType *data = nullptr;
};

#endif // VIEWCOLUMNS_H
