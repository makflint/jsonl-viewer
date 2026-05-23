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

TEST_CASE("analyze_raw_schema distinguishes null from missing") {
    std::vector<nlohmann::json> entries = {
        nlohmann::json::parse(R"({"nr_kw":"A","typ":"gruntowa"})"),
        nlohmann::json::parse(R"({"nr_kw":"B","typ":null})"),
        nlohmann::json::parse(R"({"nr_kw":"C"})")  // typ missing
    };

    auto schema = analyze_raw_schema(entries);

    auto& typ = schema.columns[1];
    REQUIRE(typ.name == "typ");
    REQUIRE(typ.stats.present_count == 2);  // missing in record 3
    REQUIRE(typ.stats.null_count == 1);
}

TEST_CASE("analyze_raw_schema captures numeric min and max") {
    std::vector<nlohmann::json> entries = {
        nlohmann::json::parse(R"({"pow":634.0})"),
        nlohmann::json::parse(R"({"pow":3097.0})"),
        nlohmann::json::parse(R"({"pow":5.0})"),
        nlohmann::json::parse(R"({"pow":null})")
    };

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.columns[0].stats.numeric_min.has_value());
    REQUIRE(*schema.columns[0].stats.numeric_min == 5.0);
    REQUIRE(*schema.columns[0].stats.numeric_max == 3097.0);
}

TEST_CASE("analyze_raw_schema collects top string values sorted by frequency") {
    std::vector<nlohmann::json> entries;
    for (int i = 0; i < 5; ++i) entries.push_back(nlohmann::json::parse(R"({"typ":"gruntowa"})"));
    for (int i = 0; i < 2; ++i) entries.push_back(nlohmann::json::parse(R"({"typ":"lokalowa"})"));
    entries.push_back(nlohmann::json::parse(R"({"typ":"inny"})"));

    auto schema = analyze_raw_schema(entries);

    const auto& top = schema.columns[0].stats.top_values;
    REQUIRE(top.size() == 3);
    REQUIRE(top[0].first == "gruntowa");
    REQUIRE(top[0].second == 5);
    REQUIRE(top[1].first == "lokalowa");
    REQUIRE(top[1].second == 2);
    REQUIRE(top[2].first == "inny");
    REQUIRE(top[2].second == 1);
}

TEST_CASE("analyze_raw_schema captures array length stats and kind") {
    std::vector<nlohmann::json> entries = {
        nlohmann::json::parse(R"({"dzial_3":[1,2,3]})"),
        nlohmann::json::parse(R"({"dzial_3":[]})"),
        nlohmann::json::parse(R"({"dzial_3":[1,2,3,4,5]})")
    };

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.columns[0].kind == "array_summary");
    REQUIRE(schema.columns[0].stats.array_max_length.has_value());
    REQUIRE(*schema.columns[0].stats.array_max_length == 5.0);
    REQUIRE(*schema.columns[0].stats.array_avg_length == Catch::Approx(8.0 / 3.0));
}

TEST_CASE("analyze_raw_schema recurses into nested objects") {
    std::vector<nlohmann::json> entries = {
        nlohmann::json::parse(R"({"dzial_1o":{"powierzchnia_m2":634.0,"typ_nieruchomosci":"gruntowa"}})"),
        nlohmann::json::parse(R"({"dzial_1o":{"powierzchnia_m2":383.0,"typ_nieruchomosci":"gruntowa"}})")
    };

    auto schema = analyze_raw_schema(entries);

    REQUIRE(schema.columns.size() == 1);
    REQUIRE(schema.columns[0].name == "dzial_1o");
    REQUIRE(schema.columns[0].kind == "object");
    REQUIRE(schema.columns[0].children.size() == 2);
    REQUIRE(schema.columns[0].children[0].name == "powierzchnia_m2");
    REQUIRE(schema.columns[0].children[0].path == "dzial_1o.powierzchnia_m2");
    REQUIRE(schema.columns[0].children[0].kind == "leaf");
    REQUIRE(schema.columns[0].children[0].stats.present_count == 2);
    REQUIRE(schema.columns[0].children[1].name == "typ_nieruchomosci");
    REQUIRE(schema.columns[0].children[1].path == "dzial_1o.typ_nieruchomosci");
}
