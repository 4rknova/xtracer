#include <cstdlib>
#include "expreval.h"

namespace ncf {

result_t parse_summands(char *&expr, size_t pcount);
result_t parse_factors(char *&expr, size_t pcount);
result_t parse_atom(char *&expr, size_t pcount);

result_t::result_t(STATUS err, double num)
    : status(err)
    , number(num)
{}

result_t parse_atom(char *&expr, size_t pcount)
{
    while(*expr == ' ') ++expr;     // Skip spaces
    bool negative = false;
    if (*expr == '-') { negative = true; ++expr; }
    if (*expr == '+') {                  ++expr; }
    if (*expr == '(') {
        ++expr;
        ++pcount;
        result_t res = parse_summands(expr, pcount);
        if (res.status != STATUS_OK) return res;

        if (*expr != ')') return result_t(STATUS_ERR_PARENTHESIS);

        ++expr;
        --pcount;
        return result_t(STATUS_OK, negative ? -res.number : res.number);
    }

    /* Read the number from the string, advance
    ** the pointer and return the result.
    */
    char* end_ptr;
    double num = strtod(expr, &end_ptr);

    // Check for invalid characters
    if (end_ptr == expr) return result_t(STATUS_ERR_SYNTAX);

    expr = end_ptr;
    return result_t(STATUS_OK, negative ? -num : num);
}

result_t parse_factors(char *&expr, size_t pcount)
{
    result_t res1 = parse_atom(expr, pcount);
    if (res1.status != STATUS_OK) return res1;

    for(;;) {
        while (*expr == ' ') ++expr;
        char  op  = *expr;

        if(op != '/' && op != '*') return res1;
        ++expr;
        result_t res2 = parse_atom(expr, pcount);
        if (res2.status != STATUS_OK) return res2;

        if (op == '/') {
            if (res2.number == 0) return result_t(STATUS_ERR_DIV_BY_ZERO);
            res1.number /= res2.number;
        }
        else res1.number *= res2.number;
    }

    return result_t(STATUS_ERR_SYNTAX);
}

result_t parse_summands(char *&expr, size_t pcount)
{
    result_t res1 = parse_factors(expr, pcount);
    if (res1.status != STATUS_OK) return res1;

    for(;;) {
        while (*expr == ' ') ++expr;
        char op = *expr;
        if(op != '-' && op != '+') return res1;
        ++expr;
        result_t res2 = parse_factors(expr, pcount);
        if (res2.status != STATUS_OK) return res2;

        if (op == '-') res1.number -= res2.number;
        else           res1.number += res2.number;
    }

    return result_t(STATUS_ERR_SYNTAX);
}

// Recursive Descent Parsing
result_t eval(char *expression)
{
	result_t res = parse_summands(expression, 0);
    if (res.status != STATUS_OK) return result_t(res.status,0);
    if (*expression != '\0') return result_t(STATUS_ERR_SYNTAX, 0);
    return res;
}

} /* namespace ncf */
