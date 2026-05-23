#include "../third_party/catch_amalgamated.hpp"
#include "../src/schema.hpp"

TEST_CASE("analyze_raw_schema returns empty schema for empty input") {
    std::vector<nlohmann::json> entries;

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.record_count == 0);
    REQUIRE(schema.columns.empty());
}
