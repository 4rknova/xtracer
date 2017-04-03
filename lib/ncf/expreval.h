#ifndef NCF_EXPREVAL_H_INCLUDED
#define NCF_EXPREVAL_H_INCLUDED

namespace ncf {

enum STATUS {
      STATUS_OK
    , STATUS_ERR_PARENTHESIS
    , STATUS_ERR_SYNTAX
    , STATUS_ERR_DIV_BY_ZERO
};

struct result_t
{
    STATUS status;
    double number;

    result_t(STATUS err, double num = 0);
};

result_t eval(char *expression);

} /* namespace ncf */

#endif /* NCF_EXPREVAL_H_INCLUDED */
