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
