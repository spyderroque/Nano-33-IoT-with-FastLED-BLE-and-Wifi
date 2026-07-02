#pragma once

// Parse an integer query-string parameter from an HTTP request line.
//
// Looks for "<key>=" where the key is immediately preceded by '?' or '&', so a
// short key never matches inside a longer one (e.g. asking for "r" must not
// pick up the "r" inside "br="). The value runs until the first '&', ' ' or
// '\r' and is parsed with atoi().
//
// Returns the parsed integer, or -1 if the key is absent or its value is empty.
//
// This function is deliberately free of any Arduino dependency so it can be
// unit-tested on the host (see test/test_param_parser).
int parseParam(const char *req, const char *key);
