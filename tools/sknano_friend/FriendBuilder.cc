#include "FriendBuilder.h"

#include <ROOT/REntry.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReadOptions.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleTypes.hxx>
#include <ROOT/RNTupleWriteOptions.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RVec.hxx>
#include <Compression.h>
#include <TFile.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace sknano_friend {
namespace {

namespace fs = std::filesystem;

// Shared with AnalyzerFramework/RNTupleSource.h; the tool must not depend on
// the framework, so the three contract constants are repeated here.
constexpr const char *kStatusField = "SKFriend_status";
constexpr const char *kInfoNTuple = "SKFriendInfo";
constexpr std::uint32_t kFormatVersion = 1;

using Key = std::tuple<std::uint32_t, std::uint32_t, std::uint64_t>;
using Lumi = std::pair<std::uint32_t, std::uint32_t>;

struct KeyHash {
    std::size_t operator()(const Key &key) const noexcept {
        const auto [run, lumi, event] = key;
        std::size_t seed = std::hash<std::uint64_t>{}(event);
        seed ^= std::hash<std::uint32_t>{}(lumi) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<std::uint32_t>{}(run) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct LumiHash {
    std::size_t operator()(const Lumi &lumi) const noexcept {
        return (static_cast<std::size_t>(lumi.first) << 32) ^ lumi.second;
    }
};

struct Loc {
    std::uint32_t aux = 0;
    std::uint64_t entry = 0;
};

std::string CollectionOf(const std::string &field) {
    const auto underscore = field.find('_');
    if (underscore != std::string::npos)
        return field.substr(0, underscore);
    if (field.size() > 1 && field[0] == 'n')
        return field.substr(1);
    return field;
}

bool IsReserved(const std::string &name) {
    return name == "run" || name == "luminosityBlock" || name == "event" ||
           name == kStatusField;
}

bool Contains(const std::vector<std::string> &list, const std::string &value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}

std::map<std::string, std::string> TopLevelTypes(ROOT::RNTupleReader &reader) {
    std::map<std::string, std::string> types;
    for (const auto &field : reader.GetDescriptor().GetTopLevelFields())
        types.emplace(field.GetFieldName(), field.GetTypeName());
    return types;
}

// Cardinality fields are read-only projections of a collection's offsets;
// the friend stores the count itself.  RNTupleCardinality<T> is a struct
// holding one T, so the value memory can be copied byte for byte.
static_assert(sizeof(ROOT::RNTupleCardinality<std::uint32_t>) == sizeof(std::uint32_t));
static_assert(sizeof(ROOT::RNTupleCardinality<std::uint64_t>) == sizeof(std::uint64_t));

std::string StorageType(const std::string &type) {
    if (type == "ROOT::RNTupleCardinality<std::uint64_t>")
        return "std::uint64_t";
    if (type.rfind("ROOT::RNTupleCardinality<", 0) == 0)
        return "std::uint32_t";
    return type;
}

std::unique_ptr<ROOT::RNTupleModel>
MakeModel(const std::vector<std::pair<std::string, std::string>> &fields) {
    auto model = ROOT::RNTupleModel::CreateBare();
    for (const auto &[name, type] : fields)
        model->AddField(ROOT::RFieldBase::Create(name, type).Unwrap());
    return model;
}

std::uint64_t CountValue(const ROOT::REntry &entry, ROOT::RFieldToken token,
                         const std::string &type) {
    const void *raw = entry.GetPtr<void>(token).get();
    if (StorageType(type) == "std::uint64_t")
        return *static_cast<const std::uint64_t *>(raw);
    return *static_cast<const std::uint32_t *>(raw);
}

template <typename T>
std::vector<T> ReadColumn(ROOT::RNTupleReader &reader, const std::string &name) {
    auto view = reader.GetView<T>(name);
    std::vector<T> values(reader.GetNEntries());
    for (std::uint64_t i = 0; i < values.size(); ++i)
        values[i] = view(i);
    return values;
}

std::vector<std::uint64_t> ReadCount(ROOT::RNTupleReader &reader, const std::string &name,
                                     const std::string &type) {
    std::vector<std::uint64_t> values(reader.GetNEntries());
    if (type == "ROOT::RNTupleCardinality<std::uint64_t>") {
        auto view = reader.GetView<ROOT::RNTupleCardinality<std::uint64_t>>(name);
        for (std::uint64_t i = 0; i < values.size(); ++i) values[i] = view(i);
    } else if (type.rfind("ROOT::RNTupleCardinality<", 0) == 0) {
        auto view = reader.GetView<ROOT::RNTupleCardinality<std::uint32_t>>(name);
        for (std::uint64_t i = 0; i < values.size(); ++i) values[i] = view(i);
    } else if (type == "std::uint64_t") {
        auto view = reader.GetView<std::uint64_t>(name);
        for (std::uint64_t i = 0; i < values.size(); ++i) values[i] = view(i);
    } else if (type == "std::uint32_t") {
        auto view = reader.GetView<std::uint32_t>(name);
        for (std::uint64_t i = 0; i < values.size(); ++i) values[i] = view(i);
    } else {
        throw std::runtime_error("count field '" + name + "' has unsupported type " + type);
    }
    return values;
}

std::string KeyString(std::uint32_t run, std::uint32_t lumi, std::uint64_t event) {
    return std::to_string(run) + ":" + std::to_string(lumi) + ":" + std::to_string(event);
}

ROOT::RNTupleReadOptions SequentialOptions() {
    ROOT::RNTupleReadOptions options;
    options.SetClusterCache(ROOT::RNTupleReadOptions::EClusterCache::kOff);
    return options;
}

// The NanoAOD Jet table keeps jets with corrected pt > 15 GeV and stores pt
// with 10 mantissa bits, so a jet just above the cut is stored as exactly
// 15.0.  Two productions computing that pt a last bit apart can put such a
// jet on different sides of the cut (Hadron lambda-polarization
// docs/data_friend_schema_fix.md section 8); that, and only that, is aligned.
constexpr float kJetTableCut = 15.f;

// Jet pt, eta, phi (and rawFactor) of one entry.
std::vector<ROOT::RVec<float>> ReadJets(const std::string &ntuple, const std::string &file,
                                        std::uint64_t entry, bool rawFactor) {
    std::vector<std::pair<std::string, std::string>> fields;
    for (const char *name : {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_rawFactor"})
        fields.emplace_back(name, "ROOT::VecOps::RVec<float>");
    if (!rawFactor)
        fields.pop_back();
    auto reader = ROOT::RNTupleReader::Open(MakeModel(fields), ntuple, file);
    auto values = reader->CreateEntry();
    reader->LoadEntry(entry, *values);
    std::vector<ROOT::RVec<float>> jets;
    for (const auto &field : fields)
        jets.push_back(*values->GetPtr<ROOT::RVec<float>>(field.first));
    return jets;
}

// +1 when the aux list has one jet more, -1 when the central one has, 0 when
// the lists differ in any other way: the extra jet must be the last one and
// stored at exactly the cut, every other jet bit-identical in all of them.
int ExtraJetAtCut(const std::vector<ROOT::RVec<float>> &central,
                  const std::vector<ROOT::RVec<float>> &aux) {
    const auto nc = central[0].size(), na = aux[0].size();
    if (nc + 1 != na && na + 1 != nc)
        return 0;
    const auto common = std::min(nc, na);
    for (std::size_t k = 0; k < central.size(); ++k)
        if (central[k].size() != nc || aux[k].size() != na ||
            (common && std::memcmp(central[k].data(), aux[k].data(), common * sizeof(float))))
            return 0;
    const auto side = na > nc ? 1 : -1;
    return (side > 0 ? aux[0] : central[0])[common] == kJetTableCut ? side : 0;
}

// Resizes an RVec field value in place; new elements are value-initialised
// (0, false).  False when the type is none of T.
template <typename... T>
bool ResizeRVec(void *value, const std::string &type, std::size_t size) {
    return ((type == ROOT::RField<ROOT::RVec<T>>::TypeName() &&
             (static_cast<ROOT::RVec<T> *>(value)->resize(size), true)) || ...);
}

} // namespace

nlohmann::json BuildReport::toJson() const {
    nlohmann::json lumis = nlohmann::json::array();
    for (const auto &[run, lumi] : missingLumis)
        lumis.push_back({run, lumi});
    nlohmann::json runs = nlohmann::json::object();
    for (const auto &[file, sums] : auxRuns)
        runs[file] = {{"count", sums.count}, {"sumw", sums.sumw}, {"sumw2", sums.sumw2}};
    nlohmann::json aligned = nlohmann::json::array();
    for (const auto &[key, side] : jetAligned)
        aligned.push_back({{"event", key}, {"extra_jet_in", side > 0 ? "aux" : "central"}});
    return {
        {"filtered", filtered},
        {"aux_runs", runs},
        {"aux_matched", auxMatched},
        {"central_entries", centralEntries},
        {"n_status", {nStatus[0], nStatus[1], nStatus[2]}},
        {"aux_only", auxOnly},
        {"missing_lumis", lumis},
        {"missing_sumw", missingSumW},
        {"missing_sumsign", missingSumSign},
        {"missing_count", missingCount},
        {"fields", fields},
        {"checked_counts", checkedCounts},
        {"collections", collections},
        {"jet_aligned", jetAligned.size()},
        {"jet_aligned_events", aligned},
        {"source_files", sourceFiles},
        {"seconds", seconds},
    };
}

nlohmann::json CheckReport::toJson() const {
    return {
        {"entries", entries},
        {"mismatches", mismatches},
        {"n_status", {nStatus[0], nStatus[1], nStatus[2]}},
        {"info", info},
    };
}

BuildReport BuildFriend(const BuildOptions &options) {
    const auto started = std::chrono::steady_clock::now();
    BuildReport report;
    report.sourceFiles = options.aux;
    report.filtered = options.filtered;
    report.collections = options.collections;
    if (options.aux.empty())
        throw std::runtime_error("no aux files given");

    // 1. The central file: event keys, lumis, the fields it already has.
    auto central = ROOT::RNTupleReader::Open(options.ntupleName, options.central,
                                             SequentialOptions());
    const std::uint64_t entries = central->GetNEntries();
    report.centralEntries = entries;
    const auto centralTypes = TopLevelTypes(*central);
    const auto runs = ReadColumn<std::uint32_t>(*central, "run");
    const auto lumis = ReadColumn<std::uint32_t>(*central, "luminosityBlock");
    const auto events = ReadColumn<std::uint64_t>(*central, "event");
    std::vector<float> genWeight;
    if (options.isMC && centralTypes.count("genWeight"))
        genWeight = ReadColumn<float>(*central, "genWeight");
    std::unordered_set<Lumi, LumiHash> centralLumis;
    for (std::uint64_t i = 0; i < entries; ++i)
        centralLumis.emplace(runs[i], lumis[i]);

    // 2. Aux files: where each of our events is.  For MC the Runs table counts
    //    the events read before any filter; comparing it with the events
    //    written checks the production's declared filtering, and its sums are
    //    the normalisation of a filtered production.
    std::unordered_map<Key, Loc, KeyHash> locations;
    locations.reserve(entries);
    std::vector<std::map<std::string, std::string>> auxFileTypes;
    std::int64_t auxRead = 0, auxWritten = 0;
    // Filtered data has no pre-filter count, but a lumi its LuminosityBlocks
    // never list was certainly not processed: those events are lost, not
    // rejected, and their luminosity has to go.
    const bool lumiBookkeeping = options.filtered && !options.isMC;
    std::unordered_set<Lumi, LumiHash> processedLumis;
    for (std::uint32_t a = 0; a < options.aux.size(); ++a) {
        const auto &file = options.aux[a];
        auto reader = ROOT::RNTupleReader::Open(options.ntupleName, file, SequentialOptions());
        if (lumiBookkeeping) {
            auto lumiTable = ROOT::RNTupleReader::Open("LuminosityBlocks", file, SequentialOptions());
            auto run = lumiTable->GetView<std::uint32_t>("run");
            auto lumi = lumiTable->GetView<std::uint32_t>("luminosityBlock");
            for (std::uint64_t i = 0; i < lumiTable->GetNEntries(); ++i)
                processedLumis.emplace(run(i), lumi(i));
        }
        if (options.isMC) {
            auto runsTable = ROOT::RNTupleReader::Open("Runs", file, SequentialOptions());
            auto count = runsTable->GetView<std::int64_t>("genEventCount");
            auto sumw = runsTable->GetView<double>("genEventSumw");
            auto sumw2 = runsTable->GetView<double>("genEventSumw2");
            RunsSums sums;
            for (std::uint64_t i = 0; i < runsTable->GetNEntries(); ++i) {
                sums.count += count(i);
                sums.sumw += sumw(i);
                sums.sumw2 += sumw2(i);
            }
            report.auxRuns[file] = sums;
            auxRead += sums.count;
            auxWritten += static_cast<std::int64_t>(reader->GetNEntries());
            if (!options.filtered && sums.count != static_cast<std::int64_t>(reader->GetNEntries()))
                throw std::runtime_error(
                    file + " read " + std::to_string(sums.count) + " events but wrote " +
                    std::to_string(reader->GetNEntries()) +
                    ": the production filters events; declare it filtered");
        }
        auxFileTypes.push_back(TopLevelTypes(*reader));
        auto run = reader->GetView<std::uint32_t>("run");
        auto lumi = reader->GetView<std::uint32_t>("luminosityBlock");
        auto event = reader->GetView<std::uint64_t>("event");
        for (std::uint64_t e = 0; e < reader->GetNEntries(); ++e) {
            const Lumi id{run(e), lumi(e)};
            if (!centralLumis.count(id))
                continue;
            const Key key{id.first, id.second, event(e)};
            const auto [it, inserted] = locations.emplace(key, Loc{a, e});
            if (!inserted)
                throw std::runtime_error(
                    "duplicate event " + KeyString(id.first, id.second, event(e)) +
                    " in " + options.aux[it->second.aux] + " and " + file);
        }
    }

    // 3. Which aux fields the friend carries, and which counts to cross-check.
    //    The first aux file decides; the others need only what it takes.
    const auto &auxTypes = auxFileTypes.front();
    std::vector<std::pair<std::string, std::string>> selected; // name, aux type
    for (const auto &[name, type] : auxTypes) {
        if (IsReserved(name) || type.empty() || name.rfind("_", 0) == 0)
            continue;
        const auto collection = CollectionOf(name);
        if (Contains(options.excludePrefixes, collection))
            continue;
        const bool overridden = Contains(options.overrides, collection);
        if (!overridden && !options.collections.empty() &&
            !Contains(options.collections, collection))
            continue;
        if (centralTypes.count(name) && !overridden)
            continue;
        selected.emplace_back(name, type);
    }
    if (selected.empty())
        throw std::runtime_error("no custom fields found in " + options.aux[0]);
    std::vector<std::string> checkedCounts;
    for (const auto &[name, type] : selected) {
        const auto collection = CollectionOf(name);
        const auto count = "n" + collection;
        if (Contains(options.overrides, collection) || count == name ||
            !centralTypes.count(count) || !auxTypes.count(count) ||
            Contains(checkedCounts, count))
            continue;
        checkedCounts.push_back(count);
    }
    // Every aux file must hold the keys, the carried fields and the checked
    // counts with the first file's types.  Other fields may differ: in data
    // the L1_*/HLT_* set follows the trigger menu of the runs a file holds.
    // Without declared collections a trigger bit the central file lacks is
    // carried, hence required too.
    std::vector<std::string> required{"run", "luminosityBlock", "event"};
    for (const auto &[name, type] : selected)
        required.push_back(name);
    required.insert(required.end(), checkedCounts.begin(), checkedCounts.end());
    for (std::uint32_t a = 1; a < options.aux.size(); ++a)
        for (const auto &name : required) {
            const auto found = auxFileTypes[a].find(name);
            if (found == auxFileTypes[a].end())
                throw std::runtime_error("aux file " + options.aux[a] + " lacks field " + name +
                                         ", which the friend needs (" + auxTypes.at(name) +
                                         " in " + options.aux[0] + ")");
            if (found->second != auxTypes.at(name))
                throw std::runtime_error("aux file " + options.aux[a] + " has field " + name +
                                         " as " + found->second + ", which the friend needs as " +
                                         auxTypes.at(name) + " (" + options.aux[0] + ")");
        }
    for (const auto &[name, type] : selected)
        report.fields.push_back(name);
    report.checkedCounts = checkedCounts;
    std::vector<std::vector<std::uint64_t>> centralCounts;
    for (const auto &count : checkedCounts)
        centralCounts.push_back(ReadCount(*central, count, centralTypes.at(count)));
    central.reset();

    if (options.isMC && options.filtered && auxRead == auxWritten)
        throw std::runtime_error(
            "declared filtered, but the aux files wrote every event they read (" +
            std::to_string(auxRead) + ")");

    // 4. Status of every central entry: 0 present; 1 absent from a filtered
    //    production, i.e. rejected by its filter (no loss is assumed there);
    //    2 absent from an unfiltered one, i.e. lost.  MC drops lost events one
    //    by one and subtracts their genWeight; data can only subtract
    //    luminosity per lumi, so a lumi missing any event is dropped whole.
    std::vector<std::uint8_t> status(entries, 0);
    std::vector<Loc> target(entries);
    std::set<Lumi> missingLumis;
    std::uint64_t matched = 0;
    for (std::uint64_t i = 0; i < entries; ++i) {
        const auto found = locations.find(Key{runs[i], lumis[i], events[i]});
        if (found != locations.end()) {
            target[i] = found->second;
            ++matched;
            ++report.auxMatched[options.aux[found->second.aux]];
        } else if (lumiBookkeeping && !processedLumis.count(Lumi{runs[i], lumis[i]})) {
            status[i] = 2;
            missingLumis.emplace(runs[i], lumis[i]);
        } else if (options.filtered) {
            status[i] = 1;
        } else {
            status[i] = 2;
            missingLumis.emplace(runs[i], lumis[i]);
        }
    }
    if (!options.isMC && !options.filtered && !missingLumis.empty()) {
        std::set<Lumi> partial;
        for (std::uint64_t i = 0; i < entries; ++i) {
            const Lumi id{runs[i], lumis[i]};
            if (status[i] == 0 && missingLumis.count(id)) {
                partial.insert(id);
                status[i] = 2;
            }
        }
        // ponytail: data has no pre-filter event count, so a filtered data
        // production is recognised only by the pattern it leaves -- missing
        // events in many lumis rather than whole lumis lost with a failed job.
        // A per-lumi count from the producer would make this exact.
        if (partial.size() > std::max<std::size_t>(5, centralLumis.size() / 100))
            throw std::runtime_error(
                std::to_string(partial.size()) + " of " + std::to_string(centralLumis.size()) +
                " lumis of " + options.central + " are only partly in the production: "
                "it looks filtered, and friends require an unfiltered one");
    }
    for (std::uint64_t i = 0; i < entries; ++i) {
        ++report.nStatus[status[i]];
        if (status[i] != 2)
            continue;
        ++report.missingCount;
        if (!genWeight.empty()) {
            report.missingSumW += genWeight[i];
            report.missingSumSign += genWeight[i] > 0 ? 1. : (genWeight[i] < 0 ? -1. : 0.);
        }
    }
    report.auxOnly = locations.size() - matched;
    report.missingLumis.assign(missingLumis.begin(), missingLumis.end());
    locations.clear();

    // 5. Stage: read each aux file in its own order (sequential I/O), buffer a
    //    chunk of entries, write them back out sorted by central entry.  The
    //    central file is sorted by event number while a multi-threaded cmsRun
    //    scatters events, so reading the aux files in central order would
    //    thrash a cluster per entry.
    const fs::path scratch = fs::path(options.scratch) /
        ("sknano_friend_" + std::to_string(::getpid()));
    fs::create_directories(scratch);
    struct Chunk {
        std::uint32_t aux = 0;
        std::vector<std::pair<std::uint64_t, std::uint64_t>> items; // aux entry, central entry
        fs::path path;
    };
    std::vector<Chunk> chunks;
    {
        std::vector<std::vector<std::pair<std::uint64_t, std::uint64_t>>> perAux(options.aux.size());
        for (std::uint64_t i = 0; i < entries; ++i)
            if (status[i] == 0)
                perAux[target[i].aux].emplace_back(target[i].entry, i);
        for (std::uint32_t a = 0; a < perAux.size(); ++a) {
            auto &items = perAux[a];
            std::sort(items.begin(), items.end());
            for (std::size_t begin = 0; begin < items.size(); begin += options.chunkEntries) {
                Chunk chunk;
                chunk.aux = a;
                chunk.items.assign(items.begin() + begin,
                                   items.begin() + std::min(items.size(), begin + options.chunkEntries));
                chunk.path = scratch / ("stage_" + std::to_string(chunks.size()) + ".root");
                chunks.push_back(std::move(chunk));
            }
        }
    }
    std::vector<std::uint32_t> stageChunk(entries, 0);
    std::vector<std::uint64_t> stagePos(entries, 0);
    std::vector<std::pair<std::string, std::string>> bufferFields = selected;
    for (const auto &count : checkedCounts)
        if (std::none_of(selected.begin(), selected.end(),
                         [&](const auto &f) { return f.first == count; }))
            bufferFields.emplace_back(count, auxTypes.at(count));
    std::vector<std::pair<std::string, std::string>> storedFields, jetFields;
    for (const auto &[name, type] : selected) {
        storedFields.emplace_back(name, StorageType(type));
        if (CollectionOf(name) == "Jet")
            jetFields.emplace_back(name, type);
    }
    const bool rawFactor = centralTypes.count("Jet_rawFactor") > 0;
    // LZ4 rather than uncompressed: the stage files are read back once,
    // sequentially, and on a worker whose scratch is RAM-backed their size
    // is what the job's memory limit sees (5 GB per 243k-entry file raw).
    ROOT::RNTupleWriteOptions stageOptions;
    stageOptions.SetCompression(ROOT::RCompressionSetting::EAlgorithm::kLZ4, 1);
    stageOptions.SetApproxZippedClusterSize(16 * 1024 * 1024);
    for (std::uint32_t c = 0; c < chunks.size(); ++c) {
        auto &chunk = chunks[c];
        auto reader = ROOT::RNTupleReader::Open(MakeModel(bufferFields), options.ntupleName,
                                                options.aux[chunk.aux]);
        std::vector<std::unique_ptr<ROOT::REntry>> buffer;
        buffer.reserve(chunk.items.size());
        std::vector<ROOT::RFieldToken> countTokens;
        for (const auto &count : checkedCounts)
            countTokens.push_back(reader->GetModel().GetToken(count));
        for (const auto &[auxEntry, centralEntry] : chunk.items) {
            auto entry = reader->CreateEntry();
            reader->LoadEntry(auxEntry, *entry);
            for (std::size_t k = 0; k < checkedCounts.size(); ++k) {
                const auto expected = centralCounts[k][centralEntry];
                const auto actual = CountValue(*entry, countTokens[k], auxTypes.at(checkedCounts[k]));
                if (expected == actual)
                    continue;
                const auto key = KeyString(runs[centralEntry], lumis[centralEntry], events[centralEntry]);
                const auto &auxFile = options.aux[chunk.aux];
                const bool raw = rawFactor && auxFileTypes[chunk.aux].count("Jet_rawFactor");
                // A jet at the Jet-table cut on one side only: the extension
                // columns follow the central list, the aux extra entry is
                // dropped and a missing one is value-initialised.
                const int side =
                    checkedCounts[k] == "nJet" && (expected + 1 == actual || actual + 1 == expected)
                        ? ExtraJetAtCut(ReadJets(options.ntupleName, options.central, centralEntry, raw),
                                        ReadJets(options.ntupleName, auxFile, auxEntry, raw))
                        : 0;
                if (side == 0)
                    throw std::runtime_error(
                        checkedCounts[k] + " differs for event " + key + ": central " +
                        std::to_string(expected) + ", aux " + std::to_string(actual) + " (" +
                        auxFile + "); the extension columns of that collection cannot be attached");
                for (const auto &[name, type] : jetFields)
                    if (!ResizeRVec<float, double, bool, std::int8_t, std::uint8_t, std::int16_t,
                                    std::uint16_t, std::int32_t, std::uint32_t, std::int64_t,
                                    std::uint64_t>(entry->GetPtr<void>(name).get(), type, expected))
                        throw std::runtime_error("cannot align " + name + " of type " + type +
                                                 " for event " + key);
                report.jetAligned.emplace_back(key, side);
                std::cerr << "sknano_friend: " << (options.isMC ? "MC " : "") << "event " << key
                          << ": one jet at the 15 GeV cut only in the "
                          << (side > 0 ? "aux file " + auxFile : "central file")
                          << "; Jet extension aligned to the central list\n";
            }
            buffer.push_back(std::move(entry));
        }
        std::vector<std::size_t> order(chunk.items.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
            return chunk.items[lhs].second < chunk.items[rhs].second;
        });
        auto writer = ROOT::RNTupleWriter::Recreate(MakeModel(storedFields), "Stage",
                                                    chunk.path.string(), stageOptions);
        auto out = writer->CreateEntry();
        std::vector<std::pair<ROOT::RFieldToken, ROOT::RFieldToken>> tokens; // out, buffer
        for (const auto &[name, type] : storedFields)
            tokens.emplace_back(out->GetToken(name), reader->GetModel().GetToken(name));
        for (std::uint64_t rank = 0; rank < order.size(); ++rank) {
            const auto &entry = *buffer[order[rank]];
            for (const auto &[outToken, inToken] : tokens)
                out->BindRawPtr<void>(outToken, entry.GetPtr<void>(inToken).get());
            writer->Fill(*out);
            const auto centralEntry = chunk.items[order[rank]].second;
            stageChunk[centralEntry] = c;
            stagePos[centralEntry] = rank;
        }
        chunk.items.clear();
        chunk.items.shrink_to_fit();
    }

    // 6. Final pass in central order: every stage file is consumed front to
    //    back, so all reads stay sequential.
    const fs::path outPath(options.out);
    const fs::path tmpPath = outPath.string() + ".tmp";
    fs::create_directories(outPath.parent_path());
    {
        std::vector<std::pair<std::string, std::string>> finalFields{
            {"run", "std::uint32_t"},
            {"luminosityBlock", "std::uint32_t"},
            {"event", "std::uint64_t"},
            {kStatusField, "std::uint8_t"},
        };
        finalFields.insert(finalFields.end(), storedFields.begin(), storedFields.end());
        auto writer = ROOT::RNTupleWriter::Recreate(MakeModel(finalFields), options.ntupleName,
                                                    tmpPath.string());
        auto data = writer->CreateEntry();
        auto empty = writer->CreateEntry();
        std::vector<std::unique_ptr<ROOT::RNTupleReader>> stages;
        std::vector<std::unique_ptr<ROOT::REntry>> stageEntries;
        std::vector<std::vector<ROOT::RFieldToken>> stageTokens;
        for (const auto &chunk : chunks) {
            stages.push_back(ROOT::RNTupleReader::Open("Stage", chunk.path.string(),
                                                       SequentialOptions()));
            stageEntries.push_back(stages.back()->CreateEntry());
            std::vector<ROOT::RFieldToken> tokens;
            for (const auto &[name, type] : storedFields)
                tokens.push_back(stages.back()->GetModel().GetToken(name));
            stageTokens.push_back(std::move(tokens));
        }
        std::vector<ROOT::RFieldToken> dataTokens;
        for (const auto &[name, type] : storedFields)
            dataTokens.push_back(data->GetToken(name));
        const auto setKeys = [&](ROOT::REntry &entry, std::uint64_t i) {
            *entry.GetPtr<std::uint32_t>("run") = runs[i];
            *entry.GetPtr<std::uint32_t>("luminosityBlock") = lumis[i];
            *entry.GetPtr<std::uint64_t>("event") = events[i];
            *entry.GetPtr<std::uint8_t>(kStatusField) = status[i];
        };
        for (std::uint64_t i = 0; i < entries; ++i) {
            if (status[i] != 0) {
                setKeys(*empty, i);
                writer->Fill(*empty);
                continue;
            }
            const auto c = stageChunk[i];
            auto &entry = *stageEntries[c];
            stages[c]->LoadEntry(stagePos[i], entry);
            for (std::size_t f = 0; f < dataTokens.size(); ++f)
                data->BindRawPtr<void>(dataTokens[f], entry.GetPtr<void>(stageTokens[c][f]).get());
            setKeys(*data, i);
            writer->Fill(*data);
        }
    }
    std::error_code ignored;
    fs::remove_all(scratch, ignored);

    // 7. The record the framework checks on attach.
    {
        auto model = ROOT::RNTupleModel::Create();
        auto version = model->MakeField<std::uint32_t>("formatVersion");
        auto production = model->MakeField<std::string>("production");
        auto centralFile = model->MakeField<std::string>("centralFile");
        auto centralEntries = model->MakeField<std::uint64_t>("centralEntries");
        auto overrides = model->MakeField<std::vector<std::string>>("overrides");
        auto fields = model->MakeField<std::vector<std::string>>("fields");
        auto sourceFiles = model->MakeField<std::vector<std::string>>("sourceFiles");
        auto nStatus = model->MakeField<std::vector<std::uint64_t>>("nStatus");
        auto missingRuns = model->MakeField<std::vector<std::uint32_t>>("missingRuns");
        auto missingLumisField = model->MakeField<std::vector<std::uint32_t>>("missingLumis");
        auto missingSumW = model->MakeField<double>("missingSumW");
        auto missingSumSign = model->MakeField<double>("missingSumSign");
        auto missingCount = model->MakeField<std::uint64_t>("missingCount");
        auto filterNote = model->MakeField<std::string>("filterNote");
        std::unique_ptr<TFile> file(TFile::Open(tmpPath.c_str(), "UPDATE"));
        if (!file || file->IsZombie())
            throw std::runtime_error("cannot reopen " + tmpPath.string());
        auto writer = ROOT::RNTupleWriter::Append(std::move(model), kInfoNTuple, *file);
        *version = kFormatVersion;
        *production = options.production;
        *centralFile = options.central;
        *centralEntries = entries;
        *overrides = options.overrides;
        *fields = report.fields;
        *sourceFiles = options.aux;
        *nStatus = {report.nStatus[0], report.nStatus[1], report.nStatus[2]};
        for (const auto &[run, lumi] : report.missingLumis) {
            missingRuns->push_back(run);
            missingLumisField->push_back(lumi);
        }
        *missingSumW = report.missingSumW;
        *missingSumSign = report.missingSumSign;
        *missingCount = report.missingCount;
        *filterNote = options.filterNote;
        writer->Fill();
        writer.reset();
        file->Close();
    }
    fs::rename(tmpPath, outPath);
    report.seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    return report;
}

CheckReport CheckFriend(const std::string &friendFile, const std::string &centralFile,
                        const std::string &ntupleName) {
    CheckReport report;
    auto central = ROOT::RNTupleReader::Open(ntupleName, centralFile, SequentialOptions());
    auto friendReader = ROOT::RNTupleReader::Open(ntupleName, friendFile, SequentialOptions());
    report.entries = central->GetNEntries();
    if (friendReader->GetNEntries() != report.entries)
        throw std::runtime_error("entry count differs: central " +
                                 std::to_string(report.entries) + ", friend " +
                                 std::to_string(friendReader->GetNEntries()));
    auto cRun = central->GetView<std::uint32_t>("run");
    auto cLumi = central->GetView<std::uint32_t>("luminosityBlock");
    auto cEvent = central->GetView<std::uint64_t>("event");
    auto fRun = friendReader->GetView<std::uint32_t>("run");
    auto fLumi = friendReader->GetView<std::uint32_t>("luminosityBlock");
    auto fEvent = friendReader->GetView<std::uint64_t>("event");
    auto fStatus = friendReader->GetView<std::uint8_t>(kStatusField);
    for (std::uint64_t i = 0; i < report.entries; ++i) {
        if (cRun(i) != fRun(i) || cLumi(i) != fLumi(i) || cEvent(i) != fEvent(i))
            ++report.mismatches;
        const auto status = fStatus(i);
        ++report.nStatus[status < 3 ? status : 2];
    }
    auto info = ROOT::RNTupleReader::Open(kInfoNTuple, friendFile);
    if (info->GetNEntries() != 1)
        throw std::runtime_error(std::string(kInfoNTuple) + " must hold exactly one entry");
    report.info = {
        {"format_version", info->GetView<std::uint32_t>("formatVersion")(0)},
        {"production", info->GetView<std::string>("production")(0)},
        {"central_file", info->GetView<std::string>("centralFile")(0)},
        {"central_entries", info->GetView<std::uint64_t>("centralEntries")(0)},
        {"overrides", info->GetView<std::vector<std::string>>("overrides")(0)},
        {"fields", info->GetView<std::vector<std::string>>("fields")(0)},
        {"source_files", info->GetView<std::vector<std::string>>("sourceFiles")(0)},
        {"n_status", info->GetView<std::vector<std::uint64_t>>("nStatus")(0)},
        {"missing_runs", info->GetView<std::vector<std::uint32_t>>("missingRuns")(0)},
        {"missing_lumis", info->GetView<std::vector<std::uint32_t>>("missingLumis")(0)},
        {"missing_sumw", info->GetView<double>("missingSumW")(0)},
        {"missing_sumsign", info->GetView<double>("missingSumSign")(0)},
        {"missing_count", info->GetView<std::uint64_t>("missingCount")(0)},
        {"filter_note", info->GetView<std::string>("filterNote")(0)},
    };
    return report;
}

} // namespace sknano_friend
