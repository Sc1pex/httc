#include <catch2/catch_test_macros.hpp>
#include <httc/status.hpp>

TEST_CASE("StatusCode constexpr", "[status]") {
    constexpr auto ok = httc::StatusCode::OK;
    static_assert(ok.code == 200);

    constexpr auto valid = httc::StatusCode::is_valid(200);
    static_assert(valid);

    constexpr auto invalid = httc::StatusCode::is_valid(99);
    static_assert(!invalid);

    constexpr auto from_int = httc::StatusCode::from_int(404);
    static_assert(from_int.has_value());
    static_assert(from_int->code == 404);
}
