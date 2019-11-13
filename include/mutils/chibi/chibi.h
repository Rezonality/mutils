#pragma once

#define  _MATH_DEFINES_DEFINED
#include <string>
#include "chibi/eval.h"
#include "chibi/gc_heap.h"

namespace MUtils
{

// This helper enables creation of a simple scheme repl.
// Calling chibi_repl will return the result of the string when run in the scheme interpreter
struct Chibi
{
    Chibi()
    {
        ctx = NULL;
        env = NULL;
        res = NULL;
        err = NULL;
        out = NULL;
        in = NULL;
    }
    sexp_gc_var6(ctx, env, res, err, out, in);
};

bool chibi_init(Chibi& chibi, const std::string& modulePath);
bool chibi_destroy(Chibi& chibi);
std::string chibi_repl(Chibi& chibi, sexp env, const std::string& str);

} // MUtils

