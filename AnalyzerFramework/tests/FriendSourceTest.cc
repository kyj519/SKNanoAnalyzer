#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <typeindex>
#include <unistd.h>
#include <unordered_set>
#include <vector>

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleWriteOptions.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RVec.hxx>
#include <TFile.h>

#include <AnalyzerFramework/RNTupleSource.h>

#include "AnalysisException.h"

// Round trip of the friend contract through RNTupleSource alone: BranchManager
// and the typed handles only ever call hasField/fieldType/make*Column, so what
// is checked here is exactly what an analyzer sees.
namespace {

namespace fs = std::filesystem;
using ROOT::VecOps::RVec;

constexpr std::uint64_t kEntries = 4;

struct FriendSpec {
    std::uint64_t entries = kEntries;
    std::vector<std::string> overrides{"GenPart"};
    std::vector<std::uint8_t> status{0, 0, 1, 2};
    bool breakKeyAtEntry2 = false;
    bool carryJetPt = false; // a duplicate that is not an override
    std::uint32_t formatVersion = SKNano::kFriendFormatVersion;
    std::uint64_t centralEntries = kEntries;
};

void WritePrimary(const std::string &path) {
    auto model = ROOT::RNTupleModel::Create();
    auto run = model->MakeField<std::uint32_t>("run");
    auto lumi = model->MakeField<std::uint32_t>("luminosityBlock");
    auto event = model->MakeField<std::uint64_t>("event");
    auto nJet = model->MakeField<std::uint32_t>("nJet");
    auto jetPt = model->MakeField<RVec<float>>("Jet_pt");
    auto nGenPart = model->MakeField<std::uint32_t>("nGenPart");
    auto genPt = model->MakeField<RVec<float>>("GenPart_pt");
    auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "Events", path);
    for (std::uint64_t i = 0; i < kEntries; ++i) {
        *run = 1;
        *lumi = 10 + static_cast<std::uint32_t>(i / 2);
        *event = 100 + i;
        *jetPt = {10.f + i, 20.f + i};
        *nJet = jetPt->size();
        *genPt = {1.f + i};
        *nGenPart = genPt->size();
        writer->Fill();
    }
}

void WriteFriend(const std::string &path, const FriendSpec &spec) {
    {
        auto model = ROOT::RNTupleModel::Create();
        auto run = model->MakeField<std::uint32_t>("run");
        auto lumi = model->MakeField<std::uint32_t>("luminosityBlock");
        auto event = model->MakeField<std::uint64_t>("event");
        auto status = model->MakeField<std::uint8_t>(SKNano::kFriendStatusField);
        auto nLambda = model->MakeField<std::uint32_t>("nLambda");
        auto lambdaPt = model->MakeField<RVec<float>>("Lambda_pt");
        auto nGenPart = model->MakeField<std::uint32_t>("nGenPart");
        auto genPt = model->MakeField<RVec<float>>("GenPart_pt");
        std::shared_ptr<RVec<float>> jetPt;
        if (spec.carryJetPt)
            jetPt = model->MakeField<RVec<float>>("Jet_pt");
        auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "Events", path);
        for (std::uint64_t i = 0; i < spec.entries; ++i) {
            *run = 1;
            *lumi = 10 + static_cast<std::uint32_t>(i / 2);
            *event = 100 + i + (spec.breakKeyAtEntry2 && i == 2 ? 1000 : 0);
            *status = i < spec.status.size() ? spec.status[i] : 0;
            if (*status == 0) {
                *lambdaPt = {5.f + i};
                *genPt = {1.f + i, 2.f + i, 3.f + i}; // more rows than the primary
            } else {
                lambdaPt->clear();
                genPt->clear();
            }
            *nLambda = lambdaPt->size();
            *nGenPart = genPt->size();
            if (jetPt)
                *jetPt = {0.f};
            writer->Fill();
        }
    }
    auto model = ROOT::RNTupleModel::Create();
    auto version = model->MakeField<std::uint32_t>("formatVersion");
    auto overrides = model->MakeField<std::vector<std::string>>("overrides");
    auto centralEntries = model->MakeField<std::uint64_t>("centralEntries");
    std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "UPDATE"));
    auto writer = ROOT::RNTupleWriter::Append(std::move(model),
                                              SKNano::kFriendInfoNTuple, *file);
    *version = spec.formatVersion;
    *overrides = spec.overrides;
    *centralEntries = spec.centralEntries;
    writer->Fill();
    writer.reset();
    file->Close();
}

class FriendSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               ("sknano-friend-" + std::to_string(::getpid()));
        std::error_code error;
        fs::create_directories(dir_, error);
        if (error)
            GTEST_SKIP() << "no writable temporary directory: " << error.message();
        primary_ = (dir_ / "primary.root").string();
        WritePrimary(primary_);
    }
    void TearDown() override {
        std::error_code error;
        fs::remove_all(dir_, error);
    }
    std::string friendFile(const std::string &name, const FriendSpec &spec) {
        const auto path = (dir_ / name).string();
        WriteFriend(path, spec);
        return path;
    }
    static std::vector<float> Read(const SKNano::RNTupleVectorColumn &column,
                                   std::uint64_t entry) {
        const auto span = column.read(entry);
        const auto *data = static_cast<const float *>(span.data);
        return {data, data + span.size};
    }

    fs::path dir_;
    std::string primary_;
};

TEST_F(FriendSourceTest, FriendFieldsAreServedNextToPrimaryOnes) {
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    source.addFriend(friendFile("friend.root", FriendSpec{}));
    ASSERT_EQ(source.friendCount(), 1U);

    EXPECT_TRUE(source.hasField("Jet_pt"));
    EXPECT_TRUE(source.hasField("Lambda_pt"));
    EXPECT_TRUE(source.hasField("nLambda"));
    EXPECT_EQ(source.fieldType("Lambda_pt"), "ROOT::VecOps::RVec<float>");

    auto jetPt = source.makeVectorColumn("Jet_pt", std::type_index(typeid(float)));
    auto lambdaPt = source.makeVectorColumn("Lambda_pt", std::type_index(typeid(float)));
    auto nLambda = source.makeScalarColumn("nLambda", std::type_index(typeid(unsigned int)));
    for (std::uint64_t entry = 0; entry < kEntries; ++entry) {
        source.noteEntry(entry);
        EXPECT_EQ(Read(*jetPt, entry), (std::vector<float>{10.f + entry, 20.f + entry}));
        unsigned int count = 99;
        nLambda->read(entry, &count);
        if (entry < 2) {
            EXPECT_EQ(Read(*lambdaPt, entry), std::vector<float>{5.f + entry});
            EXPECT_EQ(count, 1U);
        } else {
            EXPECT_TRUE(Read(*lambdaPt, entry).empty());
            EXPECT_EQ(count, 0U);
        }
    }
}

TEST_F(FriendSourceTest, DeclaredOverrideReplacesThePrimaryCollection) {
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    source.addFriend(friendFile("friend.root", FriendSpec{}));
    auto genPt = source.makeVectorColumn("GenPart_pt", std::type_index(typeid(float)));
    auto nGenPart = source.makeScalarColumn("nGenPart", std::type_index(typeid(unsigned int)));
    EXPECT_EQ(Read(*genPt, 1), (std::vector<float>{2.f, 3.f, 4.f}));
    unsigned int count = 0;
    nGenPart->read(1, &count);
    EXPECT_EQ(count, 3U);
}

TEST_F(FriendSourceTest, ReservedFieldsStayWithThePrimary) {
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    source.addFriend(friendFile("friend.root", FriendSpec{}));
    EXPECT_FALSE(source.hasField(SKNano::kFriendStatusField));
    EXPECT_TRUE(source.hasField("event"));
    auto event = source.makeScalarColumn("event", std::type_index(typeid(ULong64_t)));
    ULong64_t value = 0;
    event->read(3, &value);
    EXPECT_EQ(value, 103U);
}

TEST_F(FriendSourceTest, StatusFollowsTheFriendColumn) {
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    EXPECT_EQ(source.friendStatus(0), 0);
    source.addFriend(friendFile("friend.root", FriendSpec{}));
    EXPECT_EQ(source.friendStatus(0), 0);
    EXPECT_EQ(source.friendStatus(2), 1);
    EXPECT_EQ(source.friendStatus(3), 2);
}

TEST_F(FriendSourceTest, EntryCountMismatchIsRejectedOnAttach) {
    FriendSpec spec;
    spec.entries = kEntries - 1;
    spec.centralEntries = kEntries - 1;
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    EXPECT_THROW(source.addFriend(friendFile("short.root", spec)), SKNano::ConfigError);
    EXPECT_EQ(source.friendCount(), 0U);
}

TEST_F(FriendSourceTest, MisalignedKeyIsCaughtAtThatEntry) {
    FriendSpec spec;
    spec.breakKeyAtEntry2 = true;
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    source.addFriend(friendFile("broken.root", spec));
    EXPECT_NO_THROW(source.noteEntry(0));
    EXPECT_NO_THROW(source.noteEntry(1));
    EXPECT_THROW(source.noteEntry(2), SKNano::ConfigError);
}

TEST_F(FriendSourceTest, UndeclaredDuplicateIsRejected) {
    FriendSpec spec;
    spec.carryJetPt = true;
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    EXPECT_THROW(source.addFriend(friendFile("dup.root", spec)), SKNano::ConfigError);
}

TEST_F(FriendSourceTest, UnknownFormatVersionIsRejected) {
    FriendSpec spec;
    spec.formatVersion = SKNano::kFriendFormatVersion + 1;
    SKNano::RNTupleSource source;
    source.open("Events", primary_);
    EXPECT_THROW(source.addFriend(friendFile("v2.root", spec)), SKNano::ConfigError);
}

// Read-ahead bytes of one file: three uncompressed clusters laid out A|B|C,
// B never active.  Default merges A..C across B; SKNANO_INPUT_PREFETCH_SKIP_INACTIVE
// reads exactly the active pages, and with nothing active at open waits until
// the activated set has settled.
std::uint64_t PrefetchBytes(const std::string &path, bool skip,
                            const std::unordered_set<std::string> &atOpen,
                            const std::unordered_set<std::string> &afterEntry0) {
    if (skip)
        ::setenv("SKNANO_INPUT_PREFETCH_SKIP_INACTIVE", "1", 1);
    SKNano::RNTupleSource source;
    source.open("Events", path, false, true, 2, &atOpen);
    ::unsetenv("SKNANO_INPUT_PREFETCH_SKIP_INACTIVE");
    source.noteEntry(0, &atOpen);
    for (std::uint64_t entry = 1; entry < 200; ++entry) // past the settle hold
        source.noteEntry(entry, &afterEntry0);
    for (int wait = 0; wait < 10000 && source.readStats().prefetchClusters < 3; ++wait)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_EQ(source.readStats().prefetchClusters, 3U);
    return source.readStats().prefetchBytes;
}

std::uint64_t PageBytes(const std::string &path, const std::string &name) {
    auto reader = ROOT::RNTupleReader::Open("Events", path);
    const auto &descriptor = reader->GetDescriptor();
    std::uint64_t bytes = 0;
    std::function<void(const ROOT::RFieldDescriptor &)> add =
        [&](const ROOT::RFieldDescriptor &field) {
            for (const auto &column : descriptor.GetColumnIterable(field))
                for (const auto &cluster : descriptor.GetClusterIterable())
                    for (const auto &page :
                         cluster.GetPageRange(column.GetPhysicalId()).GetPageInfos())
                        bytes += page.GetLocator().GetNBytesOnStorage();
            for (const auto &child : descriptor.GetFieldIterable(field))
                add(child);
        };
    add(descriptor.GetFieldDescriptor(descriptor.FindFieldId(name)));
    return bytes;
}

TEST_F(FriendSourceTest, PrefetchSkipModeReadsOnlyActivePages) {
    const auto path = (dir_ / "layout.root").string();
    {
        auto model = ROOT::RNTupleModel::Create();
        std::vector<std::shared_ptr<RVec<float>>> fields;
        for (const char *name : {"A_x", "B_x", "C_x"})
            fields.push_back(model->MakeField<RVec<float>>(name));
        // CustomNanoAOD style: P_x is a projection reading A_x's columns.
        model->AddProjectedField(std::make_unique<ROOT::RField<RVec<float>>>("P_x"),
                                 [](const std::string &name) {
                                     return name == "P_x" ? "A_x" : "A_x._0";
                                 }).ThrowOnError();
        ROOT::RNTupleWriteOptions options;
        options.SetCompression(0);
        auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "Events", path, options);
        for (int entry = 0; entry < 3000; ++entry) {
            for (std::size_t i = 0; i < fields.size(); ++i) // distinct: RNTuple dedups pages
                fields[i]->assign(64 + i, static_cast<float>(3 * entry + i));
            writer->Fill();
            if (entry % 1000 == 999)
                writer->CommitCluster();
        }
    }
    const auto a = PageBytes(path, "A_x"), b = PageBytes(path, "B_x"),
               c = PageBytes(path, "C_x");
    const std::unordered_set<std::string> none, ac{"A_x", "C_x"}, onlyA{"A_x"},
        projection{"P_x"};
    // Merged neighbours also cover the 8-byte checksum of each page.
    const auto skipAC = PrefetchBytes(path, true, ac, ac);
    EXPECT_GE(skipAC, a + c);
    EXPECT_LT(skipAC, a + c + 1024);
    EXPECT_GE(PrefetchBytes(path, false, ac, ac), a + b + c); // B streamed through
    const auto skipA = PrefetchBytes(path, true, none, onlyA);
    EXPECT_GE(skipA, a);
    EXPECT_LT(skipA, a + 1024);
    EXPECT_EQ(PrefetchBytes(path, true, projection, projection), skipA);
    EXPECT_GE(PrefetchBytes(path, false, projection, projection), a); // default too
}

} // namespace
