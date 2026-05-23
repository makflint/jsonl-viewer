#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "../third_party/json.hpp"

struct FieldStats {
    int present_count = 0;
    int null_count = 0;
    std::map<std::string, int> type_counts;
    std::vector<std::pair<std::string, int>> top_values;
    std::optional<double> numeric_min, numeric_max;
    std::optional<double> array_avg_length, array_max_length;
};

struct ColumnNode {
    std::string name;
    std::string path;
    std::string kind;
    std::vector<ColumnNode> children;
    FieldStats stats;
};

struct RawSchema {
    std::vector<ColumnNode> columns;
    int record_count = 0;
};

[[nodiscard]] inline RawSchema analyze_raw_schema(const std::vector<nlohmann::json>& entries) {
    RawSchema schema;
    schema.record_count = static_cast<int>(entries.size());
    return schema;
}
