#include <mutils/chibi/chibi.h>

using namespace MUtils;

namespace
{

// Add a search path to chibi
void sexp_add_path(sexp ctx, const char* str)
{
    const char* colon;
    if (str && *str)
    {
        colon = strchr(str, ':');
        if (colon)
        {
            sexp_add_path(ctx, colon + 1);
        }
        else
        {
            colon = str + strlen(str);
        }
        sexp_push(ctx, sexp_global(ctx, SEXP_G_MODULE_PATH), SEXP_VOID);
        sexp_car(sexp_global(ctx, SEXP_G_MODULE_PATH)) = sexp_c_string(ctx, str, colon - str);
        sexp_immutablep(sexp_global(ctx, SEXP_G_MODULE_PATH)) = 1;
    }
}

void setup_string_ports(Chibi& chibi)
{
    sexp_gc_var2(in, out);

    // intentionally no input
    in = sexp_open_input_string(chibi.ctx, sexp_c_string(chibi.ctx, "", -1));
    out = sexp_open_output_string(chibi.ctx);

    sexp_gc_preserve2(chibi.ctx, in, out);

    // do not close
    sexp_port_no_closep(in) = 1;
    sexp_port_no_closep(out) = 1;

    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_IN_SYMBOL), in);
    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_OUT_SYMBOL), out);
    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_ERR_SYMBOL), out);

    chibi.in = in;
    chibi.err = out;
    chibi.out = out;

    sexp_preserve_object(chibi.ctx, chibi.in);
    sexp_preserve_object(chibi.ctx, chibi.out);

    sexp_gc_release2(chibi.ctx);
}

void close_string_ports(Chibi& chibi)
{
    sexp_close_port(chibi.ctx, chibi.in);

    // err is the same as out for now, so don't close it 
    sexp_close_port(chibi.ctx, chibi.out);
    sexp_release_object(chibi.ctx, chibi.in);
    sexp_release_object(chibi.ctx, chibi.out);
}

} // Anon

namespace MUtils
{
std::string chibi_repl(Chibi& chibi, sexp env, const std::string& str)
{
    sexp_gc_var5(tmp, res, obj, out, out_str);
    sexp_gc_preserve5(chibi.ctx, tmp, res, obj, out, out_str);

    if (env == NULL)
    {
        env = sexp_context_env(chibi.ctx);
    }

    out = sexp_open_output_string(chibi.ctx);
    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_ERR_SYMBOL), out);
    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_OUT_SYMBOL), out);

    obj = sexp_eval_string(chibi.ctx, str.c_str(), -1, NULL); // (ctx, in);
    if (sexp_exceptionp(obj))
    {
        sexp_print_exception(chibi.ctx, obj, out);
    }
    else
    {
        sexp_context_top(chibi.ctx) = 0;
        if (!(sexp_idp(obj) || sexp_pairp(obj) || sexp_nullp(obj)))
            obj = sexp_make_lit(chibi.ctx, obj);
        tmp = sexp_env_bindings(env);
        res = sexp_eval(chibi.ctx, obj, env);
        sexp_warn_undefs(chibi.ctx, sexp_env_bindings(env), tmp, res);
        if (res && sexp_exceptionp(res))
        {
            sexp_print_exception(chibi.ctx, res, out);
            if (res != sexp_global(chibi.ctx, SEXP_G_OOS_ERROR))
            {
                sexp_stack_trace(chibi.ctx, out);
            }
        }
        else if (res != SEXP_VOID)
        {
            sexp_write(chibi.ctx, res, out);
        }
    }
    out_str = sexp_get_output_string(chibi.ctx, out);

    auto ret = sexp_string_data(out_str);
    sexp_gc_release5(chibi.ctx);

    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_ERR_SYMBOL), chibi.err);
    sexp_set_parameter(chibi.ctx, chibi.env, sexp_global(chibi.ctx, SEXP_G_CUR_OUT_SYMBOL), chibi.out);

    return ret;
}

bool chibi_init(Chibi& chibi, const std::string& modulePath)
{
    // Create an evaluation context
    chibi.ctx = sexp_make_eval_context(NULL, NULL, NULL, 0, 0);
    if (sexp_exceptionp(chibi.ctx))
    {
        return false;
    }

    // Add our app exe path to the base path for modules
    sexp_add_path(chibi.ctx, modulePath.c_str());

    // Load the init-7.scm standard environment
    sexp_load_standard_env(chibi.ctx, NULL, SEXP_SEVEN);

    // Remember the environment we created
    chibi.env = sexp_context_env(chibi.ctx);

    // We want to grab the port IO directly, so use strings not stderr, etc.
    setup_string_ports(chibi);

    return true;
}

bool chibi_destroy(Chibi& chibi)
{
    close_string_ports(chibi);
    if (sexp_destroy_context(chibi.ctx) == SEXP_FALSE)
    {
        return false;
    }
    return true;
}

} // MUtils
