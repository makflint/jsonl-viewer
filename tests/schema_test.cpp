#include "../third_party/catch_amalgamated.hpp"
#include "../src/schema.hpp"

TEST_CASE("analyze_raw_schema returns empty schema for empty input") {
    std::vector<nlohmann::json> entries;

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.record_count == 0);
    REQUIRE(schema.columns.empty());
}

TEST_CASE("analyze_raw_schema captures top-level string field") {
    std::vector<nlohmann::json> entries = { nlohmann::json::parse(R"({"nr_kw":"KA1K/00000001/9"})") };

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.record_count == 1);
    REQUIRE(schema.columns.size() == 1);
    REQUIRE(schema.columns[0].name == "nr_kw");
    REQUIRE(schema.columns[0].path == "nr_kw");
    REQUIRE(schema.columns[0].kind == "leaf");
    REQUIRE(schema.columns[0].stats.present_count == 1);
    REQUIRE(schema.columns[0].stats.type_counts["string"] == 1);
}
