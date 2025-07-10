#ifndef GoldenJsonParser_h
#define GoldenJsonParser_h

#include <nlohmann/json.hpp>   
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

class GoldenJsonParser {
public:
    explicit GoldenJsonParser(const std::string& jsonPath) {
        std::ifstream jf(jsonPath);
        if (!jf)
            throw std::runtime_error("Cannot open golden JSON file " + jsonPath);

        nlohmann::json j;       // 파싱
        jf >> j;

        for (const auto& [runStr, ranges] : j.items()) {
            int run = std::stoi(runStr);
            auto& vec = table_[run];
            vec.reserve(ranges.size());
            for (const auto& interval : ranges) {
                vec.emplace_back(interval[0].get<int>(), interval[1].get<int>());
            }
            std::sort(vec.begin(), vec.end());            
        }
    }

    inline bool isGood(int run, int lumi) const {
        auto it = table_.find(run);
        if (it == table_.end()) return false;            

        const auto& intervals = it->second;
        size_t lo = 0, hi = intervals.size();
        while (lo < hi) {
            size_t mid = (lo + hi) >> 1;
            if (lumi < intervals[mid].first) {
                hi = mid;
            } else if (lumi > intervals[mid].second) {
                lo = mid + 1;
            } else {
                return true;                              
            }
        }
        return false;
    }

private:
    std::unordered_map<int, std::vector<std::pair<int,int>>> table_;
};

#endif