#include "env_context.hpp"

#include "assert.hpp"

extern char** environ;

namespace silva {
  namespace impl {
    void try_parse_variable(hash_map_t<string_or_view_t, string_or_view_t>& retval,
                            const string_view_t arg_str)
    {
      auto const pos = arg_str.find('=');
      if (pos != std::string::npos) {
        SILVA_ASSERT(arg_str.size() >= pos + 1);
        const string_view_t name       = arg_str.substr(0, pos);
        const string_view_t value      = arg_str.substr(pos + 1);
        retval[string_or_view_t{name}] = string_or_view_t{value};
      }
    }
  }

  void env_context_fill_environ(env_context_t* var_context)
  {
    for (char** p_entry = environ; *p_entry != nullptr; ++p_entry) {
      impl::try_parse_variable(var_context->variables, *p_entry);
    }
  }

  void env_context_fill_cmdline(env_context_t* var_context, const int argc, char* argv[])
  {
    for (int i = 1; i < argc; ++i) {
      const string_view_t arg_str{argv[i]};
      if (arg_str.starts_with("--")) {
        impl::try_parse_variable(var_context->variables, arg_str.substr(2));
      }
    }
  }

  expected_t<string_view_t> env_context_get(const string_view_t name)
  {
    auto curr = env_context_t::get();
    while (!curr.is_nullptr()) {
      const auto it = curr->variables.find(string_or_view_t{name});
      if (it != curr->variables.end()) {
        return it->second.as_string_view();
      }
      else {
        curr = curr->get_parent();
      }
    }
    SILVA_EXPECT(false, MINOR, "Could not find '{}' in var_context", name);
  }
}
