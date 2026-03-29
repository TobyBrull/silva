#include "syntax_farm.hpp"

#include "canopy/string_convert.hpp"
#include "name_id_style.hpp"
#include "parse_tree.hpp"

namespace silva {

  expected_t<string_view_t> token_info_t::string_as_plain_contained() const
  {
    SILVA_EXPECT(str.size() >= 2, MINOR);
    SILVA_EXPECT(str.front() == '\'' && str.back() == '\'',
                 MINOR,
                 "token must start and end with quotation marks, but got [{}]",
                 str);
    for (index_t i = 1; i < str.size() - 2; ++i) {
      SILVA_EXPECT(str[i] != '\\', MINOR);
    }
    return string_view_t{str}.substr(1, str.size() - 2);
  }

  expected_t<string_t> token_info_t::contained_string() const
  {
    SILVA_EXPECT(str.size() >= 2, MINOR);
    SILVA_EXPECT(str.front() == '\'' && str.back() == '\'', MINOR);
    string_t retval;
    retval.reserve(str.size() - 2);
    index_t i = 1;
    while (i < str.size() - 1) {
      if (str[i] == '\\') {
        SILVA_EXPECT(i + 1 < str.size() - 1, MINOR);
        retval.push_back(str[i + 1]);
        i += 2;
      }
      else {
        retval.push_back(str[i]);
        i += 1;
      }
    }
    return {std::move(retval)};
  }

  expected_t<double> token_info_t::number_as_double() const
  {
    return convert_to<double>(str);
  }

  hash_value_t hash_impl(const name_info_t& x)
  {
    return hash(tuple_t<name_id_t, token_id_t>{x.parent_name, x.base_name});
  }

  struct syntax_farm_t::impl_t {
    syntax_farm_ptr_t sfp;
    name_id_style_t default_nis{sfp};

    impl_t(syntax_farm_ptr_t sfp) : sfp(sfp) {}
  };

  syntax_farm_t::syntax_farm_t()
  {
    token_infos.emplace_back();
    token_lookup[""] = token_id_none;

    token_infos.emplace_back(token_info_t{.str = "language"});
    token_lookup["language"] = token_id_language;

    const name_info_t fni{0, 0};
    name_infos.emplace_back(fni);
    name_lookup.emplace(fni, 0);
    impl = std::make_unique<impl_t>(ptr());
  }

  syntax_farm_t::~syntax_farm_t() = default;

  token_id_t syntax_farm_t::token_id(const string_view_t token_str)
  {
    const auto it = token_lookup.find(string_t{token_str});
    if (it != token_lookup.end()) {
      return it->second;
    }
    else {
      const token_id_t new_token_id = token_infos.size();
      token_infos.push_back(token_info_t{string_t{token_str}});
      token_lookup.emplace(token_str, new_token_id);
      return new_token_id;
    }
  }

  expected_t<token_id_t> syntax_farm_t::token_id_in_string(const token_id_t ti)
  {
    const auto& token_info = token_infos[ti];
    const string_t str     = SILVA_EXPECT_FWD(token_info.contained_string(),
                                          "{} not a string containing a token",
                                          token_id_wrap(ti));
    return token_id(str);
  }

  name_id_t syntax_farm_t::name_id(const name_id_t parent_name, const token_id_t base_name)
  {
    const name_info_t fni{parent_name, base_name};
    const auto [it, inserted] = name_lookup.emplace(fni, name_infos.size());
    if (inserted) {
      name_infos.push_back(fni);
    }
    return it->second;
  }

  name_id_t syntax_farm_t::name_id_span(const name_id_t parent_name,
                                        const span_t<const token_id_t> token_ids)
  {
    name_id_t retval = parent_name;
    for (const token_id_t token_id: token_ids) {
      retval = name_id(retval, token_id);
    }
    return retval;
  }

  bool syntax_farm_t::name_id_is_parent(const name_id_t parent_name, token_id_t child_name) const
  {
    while (true) {
      if (child_name == parent_name) {
        return true;
      }
      if (child_name == name_id_root) {
        return false;
      }
      child_name = name_infos[child_name].parent_name;
    }
  }

  name_id_t syntax_farm_t::name_id_lca(const name_id_t lhs, const name_id_t rhs) const
  {
    // TODO: O(1) time, O(n) memory ?
    const auto ni_path = [this](name_id_t x) {
      array_t<name_id_t> retval;
      while (true) {
        retval.push_back(x);
        if (x == name_id_root) {
          break;
        }
        x = name_infos[x].parent_name;
      }
      std::ranges::reverse(retval);
      return retval;
    };
    const auto lhs_path = ni_path(lhs);
    const auto rhs_path = ni_path(rhs);
    const index_t n     = std::min(lhs_path.size(), rhs_path.size());
    index_t common      = 0;
    while (common + 1 < n && lhs_path[common + 1] == rhs_path[common + 1]) {
      common += 1;
    }
    SILVA_ASSERT(lhs_path[common] == rhs_path[common]);
    return lhs_path[common];
  }

  const name_id_style_t& syntax_farm_t::default_name_id_style() const
  {
    return impl->default_nis;
  }

  token_id_wrap_t syntax_farm_t::token_id_wrap(const token_id_t token_id)
  {
    return token_id_wrap_t{
        .sfp      = ptr(),
        .token_id = token_id,
    };
  }
  name_id_wrap_t syntax_farm_t::name_id_wrap(const name_id_t name_id)
  {
    return name_id_wrap_t{
        .sfp     = ptr(),
        .name_id = name_id,
    };
  }

  void pretty_write_impl(const token_id_wrap_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->format("token[ {} ]", x.sfp->token_infos[x.token_id].str);
  }

  void pretty_write_impl(const name_id_wrap_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(x.sfp->default_name_id_style().absolute(x.name_id));
  }

  fragmentization_ptr_t syntax_farm_t::add(unique_ptr_t<const fragmentization_t> x)
  {
    fragmentizations.push_back(std::move(x));
    return fragmentizations.back()->ptr();
  }
  tokenization_ptr_t syntax_farm_t::add(unique_ptr_t<const tokenization_t> x)
  {
    tokenizations.push_back(std::move(x));
    return tokenizations.back()->ptr();
  }
  parse_tree_ptr_t syntax_farm_t::add(unique_ptr_t<const parse_tree_t> x)
  {
    parse_trees.push_back(std::move(x));
    return parse_trees.back()->ptr();
  }
}
