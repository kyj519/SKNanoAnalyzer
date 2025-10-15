#ifndef GENVIEW_H
#define GENVIEW_H

#include <memory>
#include <cstddef>
#include <vector>

#include "TLorentzVector.h"
#include "ViewColumns.h"

struct GenSoA {
    ColumnView<float> pt;
    ColumnView<float> eta;
    ColumnView<float> phi;
    ColumnView<float> mass;
    ColumnView<int> pdgId;
    ColumnView<int> status;
    ColumnView<short> motherIdx;
    ColumnView<unsigned short> statusFlags;

    std::size_t size() const { return pt.size(); }
};

class GenView {
public:
    GenView() = default;
    GenView(std::shared_ptr<const GenSoA> storage, std::size_t index)
        : store(std::move(storage)), idx(index) {}

    bool valid() const { return static_cast<bool>(store) && idx < store->size(); }

    float Pt() const { return store->pt[idx]; }
    float Eta() const { return store->eta[idx]; }
    float Phi() const { return store->phi[idx]; }
    float Mass() const { return store->mass[idx]; }
    int PdgId() const { return store->pdgId[idx]; }
    int Status() const { return store->status[idx]; }
    int MotherIndex() const { return static_cast<int>(store->motherIdx[idx]); }
    unsigned short StatusFlags() const { return store->statusFlags[idx]; }

    TLorentzVector P4() const {
        TLorentzVector v;
        v.SetPtEtaPhiM(Pt(), Eta(), Phi(), Mass());
        return v;
    }

private:
    std::shared_ptr<const GenSoA> store;
    std::size_t idx = 0;
};

class GenViewCollection {
public:
    GenViewCollection() = default;
    explicit GenViewCollection(std::shared_ptr<GenSoA> payload)
        : storage_(std::move(payload)) {
        if (storage_) {
            const std::size_t n = storage_->size();
            views.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                views.emplace_back(storage_, i);
            }
        }
    }

    const GenView &operator[](std::size_t index) const { return views[index]; }
    std::size_t size() const { return views.size(); }
    bool empty() const { return views.empty(); }

    auto begin() const { return views.begin(); }
    auto end() const { return views.end(); }

    const std::shared_ptr<GenSoA> &storage() const { return storage_; }

private:
    std::shared_ptr<GenSoA> storage_;
    std::vector<GenView> views;
};

#endif // GENVIEW_H
