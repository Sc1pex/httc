#include <catch2/catch_test_macros.hpp>
#include <httc/percent_encoding.hpp>

TEST_CASE("Percent decode valid sequences") {
    SECTION("Basic percent decoding") {
        auto result = httc::percent_decode("hello%20world");
        REQUIRE(result.has_value());
        REQUIRE(*result == "hello world");
    }

    SECTION("Multiple percent sequences") {
        auto result = httc::percent_decode("hello%20world%21%40%23");
        REQUIRE(result.has_value());
        REQUIRE(*result == "hello world!@#");
    }

    SECTION("Uppercase hex digits") {
        auto result = httc::percent_decode("test%2A%2B%2C");
        REQUIRE(result.has_value());
        REQUIRE(*result == "test*+,");
    }

    SECTION("Lowercase hex digits") {
        auto result = httc::percent_decode("test%2a%2b%2c");
        REQUIRE(result.has_value());
        REQUIRE(*result == "test*+,");
    }

    SECTION("Mixed case hex digits") {
        auto result = httc::percent_decode("test%2A%2b%2C");
        REQUIRE(result.has_value());
        REQUIRE(*result == "test*+,");
    }

    SECTION("No percent encoding") {
        auto result = httc::percent_decode("hello world");
        REQUIRE(result.has_value());
        REQUIRE(*result == "hello world");
    }

    SECTION("Empty string") {
        auto result = httc::percent_decode("");
        REQUIRE(result.has_value());
        REQUIRE(*result == "");
    }

    SECTION("Only percent sequences") {
        auto result = httc::percent_decode("%48%65%6C%6C%6F");
        REQUIRE(result.has_value());
        REQUIRE(*result == "Hello");
    }

    SECTION("Special characters") {
        auto result = httc::percent_decode("name%3DJohn%26age%3D25");
        REQUIRE(result.has_value());
        REQUIRE(*result == "name=John&age=25");
    }

    SECTION("URL-safe characters") {
        auto result = httc::percent_decode("file%2Ename%2Etxt");
        REQUIRE(result.has_value());
        REQUIRE(*result == "file.name.txt");
    }

    SECTION("Unicode characters") {
        auto result = httc::percent_decode("%C3%A9%C3%A0%C3%A8");
        REQUIRE(result.has_value());
        REQUIRE(*result == "éàè");
    }
}

TEST_CASE("Percent decode invalid sequences") {
    SECTION("Incomplete percent sequence at end") {
        auto result = httc::percent_decode("hello%2");
        REQUIRE(!result.has_value());
    }

    SECTION("Incomplete percent sequence in middle") {
        auto result = httc::percent_decode("hello%2world");
        REQUIRE(!result.has_value());
    }

    SECTION("Invalid hex digit - first position") {
        auto result = httc::percent_decode("hello%G0world");
        REQUIRE(!result.has_value());
    }

    SECTION("Invalid hex digit - second position") {
        auto result = httc::percent_decode("hello%2Gworld");
        REQUIRE(!result.has_value());
    }

    SECTION("Invalid hex digits - both positions") {
        auto result = httc::percent_decode("hello%GGworld");
        REQUIRE(!result.has_value());
    }

    SECTION("Percent at end without digits") {
        auto result = httc::percent_decode("hello%");
        REQUIRE(!result.has_value());
    }

    SECTION("Multiple invalid sequences") {
        auto result = httc::percent_decode("test%ZZ%YY");
        REQUIRE(!result.has_value());
    }

    SECTION("Non-ASCII in hex position") {
        auto result = httc::percent_decode("test%2€");
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("Percent encode basic strings") {
    SECTION("String with spaces") {
        auto result = httc::percent_encode("hello world");
        REQUIRE(result == "hello%20world");
    }

    SECTION("String with special characters") {
        auto result = httc::percent_encode("name=John&age=25");
        REQUIRE(result == "name%3DJohn%26age%3D25");
    }

    SECTION("String with URL-unsafe characters") {
        auto result = httc::percent_encode("file name.txt");
        REQUIRE(result == "file%20name.txt");
    }

    SECTION("Empty string") {
        auto result = httc::percent_encode("");
        REQUIRE(result == "");
    }

    SECTION("String with no encoding needed") {
        auto result = httc::percent_encode("hello");
        REQUIRE(result == "hello");
    }

    SECTION("String with alphanumeric and safe chars") {
        auto result = httc::percent_encode("abc123-_.~");
        REQUIRE(result == "abc123-_.~");
    }

    SECTION("Reserved characters") {
        auto result = httc::percent_encode("!*'();:@&=+$,/?#[]");
        REQUIRE(result == "%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");
    }

    SECTION("Control characters") {
        auto result = httc::percent_encode("hello\x01\x02\x03world");
        REQUIRE(result == "hello%01%02%03world");
    }

    SECTION("High-bit characters") {
        auto result = httc::percent_encode("café");
        REQUIRE(result == "caf%C3%A9");
    }
}

TEST_CASE("Percent encode/decode roundtrip") {
    SECTION("Basic roundtrip") {
        std::string original = "hello world!@#$%^&*()";
        auto encoded = httc::percent_encode(original);
        auto decoded = httc::percent_decode(encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == original);
    }

    SECTION("Unicode roundtrip") {
        std::string original = "café naïve résumé";
        auto encoded = httc::percent_encode(original);
        auto decoded = httc::percent_decode(encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == original);
    }

    SECTION("Empty string roundtrip") {
        std::string original = "";
        auto encoded = httc::percent_encode(original);
        auto decoded = httc::percent_decode(encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == original);
    }

    SECTION("Complex string roundtrip") {
        std::string original = "path/to/file?param=value&other=data#fragment";
        auto encoded = httc::percent_encode(original);
        auto decoded = httc::percent_decode(encoded);
        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == original);
    }
}
