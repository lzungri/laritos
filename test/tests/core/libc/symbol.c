/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>

#include <stdint.h>
#include <string.h>
#include <test/test.h>

static char *symbol_with_text = "HELLO";

static int function(void) {
    return 12345;
}

T(symbol_get_returns_the_address_of_the_given_symbol_name) {
    char **text = symbol_get("symbol_with_text");
    tassert(text == &symbol_with_text);
    tassert(strncmp(*text, symbol_with_text, strlen(symbol_with_text)) == 0);
TEND

T(symbol_get_returns_the_address_of_the_given_function) {
    // This is a little risky if symbol_get doesn't work propertly, we'll do it anyway...
    int (*func)(void) = symbol_get("function");
    tassert(func == function);
    tassert(func() == function());
TEND

T(symbol_get_returns_null_on_non_existent_symbol) {
    tassert(symbol_get("XXX") == NULL);
TEND

T(symbol_get_at_returns_symbol_name_at_the_given_address) {
    char symbol[32] = { 0 };
    tassert(symbol_get_name_at(&symbol_with_text, symbol, sizeof(symbol) - 1) >= 0);
    tassert(strncmp(symbol, "symbol_with_text", sizeof(symbol)) == 0);
TEND

T(symbol_get_at_on_a_fuzzy_addr_returns_symbol_name_located_at_closest_lowest_addr) {
    char symbol[32] = { 0 };
    tassert(symbol_get_name_at(((char *) &symbol_with_text) + 1, symbol, sizeof(symbol) - 1) >= 0);
    tassert(strncmp(symbol, "symbol_with_text", sizeof(symbol)) == 0);
TEND

T(symbol_get_at_on_a_fuzzy_addr_returns_function_located_at_closest_lowest_addr) {
    char symbol[32] = { 0 };
    tassert(symbol_get_name_at(((char *) function) + 1, symbol, sizeof(symbol) - 1) >= 0);
    tassert(strncmp(symbol, "function", sizeof(symbol)) == 0);
TEND
