#include "builtins.hpp"

#include <chrono>

namespace silva::lox {
  expected_t<hash_map_t<token_id_t, object_ref_t>>
  make_builtins(syntax_farm_ptr_t sfp, const parser_t& parser, object_pool_t& object_pool)
  {
    struct builtin_decl_t {
      token_id_t name = token_id_none;
      silva::function_t<expected_t<object_ref_t>(object_pool_t&, span_t<const object_ref_t>)> impl;
    };
    array_t<builtin_decl_t> builtin_decls{
        builtin_decl_t{
            .name = sfp->token_id("clock"),
            .impl = [](object_pool_t& object_pool,
                       const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 0, RUNTIME);
              const auto now      = std::chrono::system_clock::now();
              const auto duration = now.time_since_epoch();
              const double retval =
                  std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
              return object_pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = sfp->token_id("getc"),
            .impl = [](object_pool_t& object_pool,
                       const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 0, RUNTIME);
              const char retval_char = getchar();
              return object_pool.make(double(retval_char));
            },
        },
        builtin_decl_t{
            .name = sfp->token_id("chr"),
            .impl = [sfp = sfp, ti_ascii_code = sfp->token_id("ascii_code")](
                        object_pool_t& object_pool,
                        const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 1, RUNTIME);
              const object_ref_t& val = params.front();
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<const double>(val->data);
              string_t retval{(char)double_val};
              return object_pool.make(retval);
            },
        },
        builtin_decl_t{
            .name = sfp->token_id("exit"),
            .impl = [sfp = sfp, ti_exit_code = sfp->token_id("exit_code")](
                        object_pool_t& object_pool,
                        const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 1, RUNTIME);
              const object_ref_t& val = params.front();
              SILVA_ASSERT(val->holds_double());
              const double double_val = std::get<const double>(val->data);
              std::exit((int)double_val);
            },
        },
        builtin_decl_t{
            .name = sfp->token_id("print_error"),
            .impl = [sfp = sfp, ti_text = sfp->token_id("text")](
                        object_pool_t& object_pool,
                        const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 1, RUNTIME);
              const object_ref_t& val = params.front();
              SILVA_ASSERT(val->holds_string());
              const auto& text = std::get<const string_t>(val->data);
              fmt::println("ERROR: {}", text);
              return object_pool.make(none);
            },
        },
    };

    hash_map_t<token_id_t, object_ref_t> retval;

    const string_t builtins_lox_str = R"'(
    fun clock() {}
    fun getc() {}
    fun chr(ascii_code) {}
    fun exit(exit_code) {}
    fun print_error(text) {}
)'";

    const auto fp          = SILVA_EXPECT_FWD(fragmentize(sfp, "builtins.lox", builtins_lox_str));
    const auto pts_builtin = SILVA_EXPECT_FWD(parser(fp, sfp->name_id_of("Lox")))->span();

    auto [it, end] = pts_builtin.children_range();
    for (const builtin_decl_t& builtin_decl: builtin_decls) {
      SILVA_EXPECT(it != end, ASSERT);
      const auto pts_function =
          pts_builtin.sub_tree_span_at(it.pos).sub_tree_span_at(1).sub_tree_span_at(1);
      const token_id_t lox_name = pts_function.tp->tokens[pts_function[0].token_begin];
      SILVA_EXPECT(lox_name == builtin_decl.name,
                   ASSERT,
                   "expected function '{}', but found '{}'",
                   sfp->token_id_wrap(lox_name),
                   sfp->token_id_wrap(builtin_decl.name));
      function_builtin_t fb{{pts_function}, builtin_decl.impl};
      // fb.closure           = scopes.root();
      ++it;
      retval[lox_name] = object_pool.make(std::move(fb));
    }

    return retval;
  }
}
