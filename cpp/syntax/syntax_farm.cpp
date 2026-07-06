#include "syntax_farm.hpp"

#include "canopy/string_convert.hpp"
#include "parse_tree.hpp"

namespace silva {
  bool token_t::is_language() const
  {
    return token_id == token_id_language;
  }

  expected_t<string_view_t> token_info_t::string_as_plain_contained() const
  {
    SILVA_EXPECT(str.size() >= 2, MINOR);
    SILVA_EXPECT((str.front() == '\'' && str.back() == '\'') ||
                     (str.front() == '"' && str.back() == '"'),
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
    SILVA_EXPECT((str.front() == '\'' && str.back() == '\'') ||
                     (str.front() == '"' && str.back() == '"'),
                 MINOR);
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

  syntax_farm_t::syntax_farm_t()
  {
    {
      token_infos.emplace_back();
      token_lookup[""] = token_id_t{};
    }
    {
      SILVA_ASSERT(token_infos.size() == token_id_language.val);
      token_infos.emplace_back(token_info_t{.str = "language"});
      token_lookup["language"] = token_id_language;
    }
    {
      SILVA_ASSERT(token_infos.size() == token_id_literal.val);
      token_infos.emplace_back(token_info_t{.str = "literal"});
      token_lookup["literal"] = token_id_literal;
    }
    {
      const name_info_t fni{0, 0};
      name_infos.emplace_back(fni);
      name_lookup.emplace(fni, 0);
    }
    {
      SILVA_ASSERT(name_infos.size() == name_id_literal.val);
      const name_info_t fni{0, token_id_literal};
      name_infos.emplace_back(fni);
      name_lookup.emplace(fni, name_id_literal);
    }
  }

  syntax_farm_t::~syntax_farm_t() = default;

  const token_info_t& syntax_farm_t::get(const token_id_t ti) const
  {
    return token_infos[ti.val];
  }
  const name_info_t& syntax_farm_t::get(const name_id_t ni) const
  {
    return name_infos[ni.val];
  }

  token_id_t syntax_farm_t::token_id(const string_view_t token_str)
  {
    const auto it = token_lookup.find(string_t{token_str});
    if (it != token_lookup.end()) {
      return it->second;
    }
    else {
      const token_id_t new_token_id(token_infos.size());
      token_infos.push_back(token_info_t{string_t{token_str}});
      token_lookup.emplace(token_str, new_token_id);
      return new_token_id;
    }
  }

  token_id_t syntax_farm_t::token_id(const fragment_span_t fs)
  {
    const index_t beg_byte_offset = fs.fp->get_fragment_byte_offset(fs.begin);
    const index_t end_byte_offset = fs.fp->get_fragment_byte_offset(fs.end);
    return token_id(fs.fp->source_code.substr(beg_byte_offset, end_byte_offset - beg_byte_offset));
  }

  expected_t<token_id_t> syntax_farm_t::token_id_in_string(const token_id_t ti)
  {
    const auto& token_info = get(ti);
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

  bool syntax_farm_t::name_id_is_parent(const name_id_t parent_name, name_id_t child_name) const
  {
    while (true) {
      if (child_name == parent_name) {
        return true;
      }
      if (!child_name.is_valid()) {
        return false;
      }
      child_name = get(child_name).parent_name;
    }
  }

  name_id_t syntax_farm_t::name_id_lca(const name_id_t lhs, const name_id_t rhs) const
  {
    // TODO: O(1) time, O(n) memory ?
    const auto ni_path = [this](name_id_t x) {
      array_t<name_id_t> retval;
      while (true) {
        retval.push_back(x);
        if (!x.is_valid()) {
          break;
        }
        x = get(x).parent_name;
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

  token_id_wrap_t syntax_farm_t::token_id_wrap(const token_id_t token_id)
  {
    return token_id_wrap_t{
        .sfp      = ptr(),
        .token_id = token_id,
    };
  }

  lexicon_t::lexicon_t(syntax_farm_ptr_t sfp) : sfp(std::move(sfp)) {}

  lexicon_t::~lexicon_t() = default;

  name_id_wrap_t lexicon_t::name_id_wrap(const name_id_t name_id) const
  {
    return name_id_wrap_t{
        .lp      = ptr(),
        .name_id = name_id,
    };
  }

  string_t lexicon_t::name_id_str(const name_id_t name_id) const
  {
    if (!name_id.is_valid()) {
      return "";
    }
    const name_info_t& ni = sfp->get(name_id);
    return name_id_str(ni.parent_name) + sfp->get(name_sep).str + sfp->get(ni.base_name).str;
  }

  expected_t<name_id_t> lexicon_t::name_id_definition(const name_id_t scope_name,
                                                      span_t<const token_t> ts) const
  {
    name_id_t retval = scope_name;
    SILVA_EXPECT(!ts.empty(), MINOR);
    index_t idx = 0;
    if (ts.front().token_id == name_sep) {
      retval = name_id_t{};
      idx += 1;
    }
    while (idx < ts.size()) {
      const token_id_t base = ts[idx].token_id;
      SILVA_EXPECT(base != name_sep, MINOR);
      retval = sfp->name_id(retval, base);
      idx += 1;
      if (idx < ts.size()) {
        SILVA_EXPECT(ts[idx].token_id == name_sep, MINOR);
        idx += 1;
      }
    }
    return retval;
  }

  void pretty_write_impl(const token_id_wrap_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->format("token[ {} ]", x.sfp->get(x.token_id).str);
  }

  void pretty_write_impl(const name_id_wrap_t& x, byte_sink_t* byte_sink)
  {
    byte_sink->write_str(x.lp->name_id_str(x.name_id));
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
