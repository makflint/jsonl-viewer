#include "../third_party/catch_amalgamated.hpp"
#include "../src/parser.hpp"

TEST_CASE("Parse single JSONL line extracts type") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "user");
}
