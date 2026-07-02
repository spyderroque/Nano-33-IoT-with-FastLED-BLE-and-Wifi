// Host unit tests for parseParam() — run with:  pio test -e native
//
// parseParam() extracts an integer query-string value from an HTTP request
// line. These tests pin down its contract and guard against the regression
// where a short key ("r") was matched inside a longer one ("br").

#include <unity.h>

#include "param_parser.h"

void setUp(void) {}
void tearDown(void) {}

// A typical single-parameter request line.
void test_single_param(void) {
    TEST_ASSERT_EQUAL_INT(128, parseParam("GET /set?w=128 HTTP/1.1", "w"));
}

// Regression: "br" must not be picked up when asking for "r".
// Before the fix, parseParam(req, "r") matched the "r=" inside "br=" and
// returned the brightness value, silently corrupting the red channel.
void test_short_key_not_matched_inside_long_key(void) {
    const char *req = "GET /set?br=200 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(200, parseParam(req, "br"));
    TEST_ASSERT_EQUAL_INT(-1,  parseParam(req, "r"));
}

// Both keys present — each resolves to its own value regardless of order.
void test_multiple_params(void) {
    const char *a = "GET /set?r=50&br=200 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(50,  parseParam(a, "r"));
    TEST_ASSERT_EQUAL_INT(200, parseParam(a, "br"));

    const char *b = "GET /set?br=200&r=50 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(50,  parseParam(b, "r"));
    TEST_ASSERT_EQUAL_INT(200, parseParam(b, "br"));
}

// The colour channels use keys r / g / bl.
void test_color_keys(void) {
    const char *req = "GET /set?r=10&g=20&bl=30 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(10, parseParam(req, "r"));
    TEST_ASSERT_EQUAL_INT(20, parseParam(req, "g"));
    TEST_ASSERT_EQUAL_INT(30, parseParam(req, "bl"));
}

// A missing key yields -1.
void test_missing_key(void) {
    TEST_ASSERT_EQUAL_INT(-1, parseParam("GET /set?w=5 HTTP/1.1", "g"));
}

// A present key with an empty value yields -1 (treated as "not set").
void test_empty_value(void) {
    const char *req = "GET /set?w=&r=5 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(-1, parseParam(req, "w"));
    TEST_ASSERT_EQUAL_INT(5,  parseParam(req, "r"));
}

// A value may be terminated by '&' as well as by a trailing space.
void test_value_terminated_by_ampersand(void) {
    const char *req = "GET /set?w=128&br=64 HTTP/1.1";
    TEST_ASSERT_EQUAL_INT(128, parseParam(req, "w"));
    TEST_ASSERT_EQUAL_INT(64,  parseParam(req, "br"));
}

// Boundary values 0 and 255 round-trip correctly.
void test_boundary_values(void) {
    TEST_ASSERT_EQUAL_INT(0,   parseParam("GET /set?w=0 HTTP/1.1", "w"));
    TEST_ASSERT_EQUAL_INT(255, parseParam("GET /set?w=255 HTTP/1.1", "w"));
}

// The key must be delimited by '?' or '&'; a bare substring in the path
// (here "w=" inside "/setw=9") must not be treated as a parameter.
void test_requires_delimiter(void) {
    TEST_ASSERT_EQUAL_INT(-1, parseParam("GET /setw=9 HTTP/1.1", "w"));
}

// A NULL request or key is handled gracefully.
void test_null_inputs(void) {
    TEST_ASSERT_EQUAL_INT(-1, parseParam(NULL, "w"));
    TEST_ASSERT_EQUAL_INT(-1, parseParam("GET /set?w=1 HTTP/1.1", NULL));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_single_param);
    RUN_TEST(test_short_key_not_matched_inside_long_key);
    RUN_TEST(test_multiple_params);
    RUN_TEST(test_color_keys);
    RUN_TEST(test_missing_key);
    RUN_TEST(test_empty_value);
    RUN_TEST(test_value_terminated_by_ampersand);
    RUN_TEST(test_boundary_values);
    RUN_TEST(test_requires_delimiter);
    RUN_TEST(test_null_inputs);
    return UNITY_END();
}
