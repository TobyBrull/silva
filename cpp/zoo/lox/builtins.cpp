#include "builtins.hpp"

#include <chrono>

namespace silva::lox {
  constexpr string_view_t builtins_lox_str = R"'(
    fun clock() {}
    fun getc() {}
    fun chr(ascii_code) {}
    fun exit(exit_code) {}
    fun print_error(text) {}
  )'";

  expected_t<hash_map_t<token_id_t, object_ref_t>>
  make_builtins(syntax_ward_ptr_t swp, const parser_t& parser, object_pool_t& object_pool)
  {
    const tokenization_ptr_t builtin_tt =
        SILVA_EXPECT_FWD(tokenize(swp, "builtins.lox", builtins_lox_str));
    const auto pts_builtin = SILVA_EXPECT_FWD(parser(builtin_tt, swp->name_id_of("Lox")))->span();

    struct builtin_decl_t {
      token_id_t name = token_id_none;
      silva::function_t<expected_t<object_ref_t>(object_pool_t&, span_t<const object_ref_t>)> impl;
    };
    vector_t<builtin_decl_t> builtin_decls{
        builtin_decl_t{
            .name = swp->token_id("clock").value(),
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
            .name = swp->token_id("getc").value(),
            .impl = [](object_pool_t& object_pool,
                       const span_t<const object_ref_t> params) -> expected_t<object_ref_t> {
              SILVA_EXPECT(params.size() == 0, RUNTIME);
              const char retval_char = getchar();
              return object_pool.make(double(retval_char));
            },
        },
        builtin_decl_t{
            .name = swp->token_id("chr").value(),
            .impl = [swp = swp, ti_ascii_code = swp->token_id("ascii_code").value()](
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
            .name = swp->token_id("exit").value(),
            .impl = [swp = swp, ti_exit_code = swp->token_id("exit_code").value()](
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
            .name = swp->token_id("print_error").value(),
            .impl = [swp = swp, ti_text = swp->token_id("text").value()](
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

    auto [it, end] = pts_builtin.children_range();
    for (const builtin_decl_t& builtin_decl: builtin_decls) {
      SILVA_EXPECT(it != end, ASSERT);
      const auto pts_function =
          pts_builtin.sub_tree_span_at(it.pos).sub_tree_span_at(1).sub_tree_span_at(1);
      const token_id_t lox_name = pts_function.tp->tokens[pts_function[0].token_begin];
      SILVA_EXPECT(lox_name == builtin_decl.name,
                   ASSERT,
                   "expected function '{}', but found '{}'",
                   swp->token_id_wrap(lox_name),
                   swp->token_id_wrap(builtin_decl.name));
      function_builtin_t fb{{pts_function}, builtin_decl.impl};
      // fb.closure           = scopes.root();
      ++it;
      retval[lox_name] = object_pool.make(std::move(fb));
    }

    return retval;
  }
}
