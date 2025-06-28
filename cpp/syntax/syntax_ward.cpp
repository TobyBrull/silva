#include "syntax_ward.hpp"

#include "canopy/convert.hpp"
#include "name_id_style.hpp"
#include "parse_tree.hpp"

namespace silva {

  using enum token_category_t;

  namespace impl {
    constexpr char whitespace_chars[] = {' '};
    constexpr char identifier_chars[] = {'_'};
    constexpr char oplet_chars[]      = {
        // parentheses
        '[',
        ']',
        '(',
        ')',
        '{',
        '}',
        // other
        '~',
    };
    constexpr char operator_chars[] = {
        // punctuation
        ',',
        '.',
        ':',
        // comparison
        '<',
        '>',
        '=',
        // arithmetic
        '-',
        '+',
        '*',
        '/',
        '%',
        // logical
        '&',
        '|',
        // other
        '^',
        '@',
        '!',
        '?',
        ';',
        '$',
        '`',
    };
    constexpr char number_chars[] = {'.', '`', 'e'};

    template<index_t Size>
    constexpr bool is_one_of(const char c, const char (&char_array)[Size])
    {
      for (index_t i = 0; i < Size; ++i) {
        if (c == char_array[i]) {
          return true;
        }
      }
      return false;
    }

    template<typename Predicate>
    index_t find_token_length(const string_view_t rest, Predicate predicate)
    {
      SILVA_ASSERT(rest.size() >= 1);
      index_t index = 1;
      while (index < rest.size()) {
        const char x = rest[index];
        if (!predicate(x)) {
          break;
        }
        index += 1;
      }
      return index;
    }

    index_t find_whitespace_length(const string_view_t rest)
    {
      return find_token_length(rest, [](const char x) { return is_one_of(x, whitespace_chars); });
    }

    optional_t<index_t> find_string_length(const string_view_t rest)
    {
      SILVA_ASSERT(rest.size() >= 1);
      index_t index = 1;
      while (index < rest.size()) {
        if (rest[index] == '\'' && rest[index - 1] != '\\') {
          index += 1;
          break;
        }
        index += 1;
      }
      if (rest[index - 1] == '\'') {
        return index;
      }
      else {
        return {none};
      }
    }

    index_t find_comment_length(const string_view_t rest)
    {
      index_t index = 1;
      while (index < rest.size() && rest[index] != '\n') {
        index += 1;
      }
      return index;
    }

  }

  tuple_t<string_view_t, token_category_t> tokenize_one(const string_view_t text)
  {
    SILVA_ASSERT(!text.empty());
    const char c = text.front();
    if (impl::is_one_of(c, impl::whitespace_chars)) {
      const index_t len = impl::find_whitespace_length(text);
      return {text.substr(0, len), NONE};
    }
    else if (std::isalpha(c) || impl::is_one_of(c, impl::identifier_chars)) {
      const index_t len = impl::find_token_length(text, [](const char x) {
        return std::isalnum(x) || impl::is_one_of(x, impl::identifier_chars);
      });
      return {text.substr(0, len), IDENTIFIER};
    }
    else if (impl::is_one_of(c, impl::oplet_chars)) {
      return {text.substr(0, 1), OPERATOR};
    }
    else if (impl::is_one_of(c, impl::operator_chars)) {
      const index_t len = impl::find_token_length(text, [](const char x) {
        return impl::is_one_of(x, impl::operator_chars);
      });
      return {text.substr(0, len), OPERATOR};
    }
    else if (c == '\'') {
      const std::optional<index_t> maybe_length = impl::find_string_length(text);
      if (maybe_length.has_value()) {
        return {text.substr(0, maybe_length.value()), STRING};
      }
      else {
        return {text, NONE};
      }
    }
    else if (c == '#') {
      const index_t len = impl::find_comment_length(text);
      return {text.substr(0, len), NONE};
    }
    else if (std::isdigit(c)) {
      const index_t len = impl::find_token_length(text, [](const char x) {
        return std::isdigit(x) || impl::is_one_of(x, impl::number_chars);
      });
      return {text.substr(0, len), NUMBER};
    }
    else if (c == '\n') {
      return {text.substr(0, 1), NONE};
    }
    else {
      return {text.substr(0, 1), NONE};
    }
  }

  expected_t<string_view_t> token_info_t::string_as_plain_contained() const
  {
    SILVA_EXPECT(category == STRING, MAJOR);
    SILVA_EXPECT(str.size() >= 2, MINOR);
    SILVA_EXPECT(str.front() == '\'' && str.back() == '\'', MINOR);
    for (index_t i = 1; i < str.size() - 1; ++i) {
      SILVA_EXPECT(str[i] != '\\' && str[i] != '\'', MINOR);
    }
    return string_view_t{str}.substr(1, str.size() - 2);
  }

  expected_t<double> token_info_t::number_as_double() const
  {
    SILVA_EXPECT(category == NUMBER, MAJOR);
    return convert_to<double>(str);
  }

  hash_value_t hash_impl(const name_info_t& x)
  {
    return hash(tuple_t<name_id_t, token_id_t>{x.parent_name, x.base_name});
  }

  struct syntax_ward_t::impl_t {
    syntax_ward_ptr_t swp;
    name_id_style_t default_nis{swp};

    impl_t(syntax_ward_ptr_t swp) : swp(swp) {}
  };

  syntax_ward_t::syntax_ward_t()
  {
    token_infos.emplace_back();
    token_lookup[""] = token_id_none;
    const name_info_t fni{0, 0};
    name_infos.emplace_back(fni);
    name_lookup.emplace(fni, 0);
    impl = std::make_unique<impl_t>(ptr());
  }

  syntax_ward_t::~syntax_ward_t() = default;

  expected_t<token_id_t> syntax_ward_t::token_id(const string_view_t token_str)
  {
    const auto it = token_lookup.find(string_t{token_str});
    if (it != token_lookup.end()) {
      return it->second;
    }
    else {
      const auto [tokenized_str, token_cat] = tokenize_one(token_str);
      SILVA_EXPECT(tokenized_str.size() == token_str.size(), MINOR);
      const token_id_t new_token_id = token_infos.size();
      token_infos.push_back(token_info_t{token_cat, string_t{tokenized_str}});
      token_lookup.emplace(tokenized_str, new_token_id);
      return new_token_id;
    }
  }

  expected_t<token_id_t> syntax_ward_t::token_id_in_string(const token_id_t ti)
  {
    const auto& token_info = token_infos[ti];
    SILVA_EXPECT(token_info.category == STRING, MINOR, "{} not a string", token_id_wrap(ti));
    const string_t str{SILVA_EXPECT_FWD(token_info.string_as_plain_contained(),
                                        "{} not a string containing a token",
                                        token_id_wrap(ti))};
    return token_id(str);
  }

  name_id_t syntax_ward_t::name_id(const name_id_t parent_name, const token_id_t base_name)
  {
    const name_info_t fni{parent_name, base_name};
    const auto [it, inserted] = name_lookup.emplace(fni, name_infos.size());
    if (inserted) {
      name_infos.push_back(fni);
    }
    return it->second;
  }

  name_id_t syntax_ward_t::name_id_span(const name_id_t parent_name,
                                        const span_t<const token_id_t> token_ids)
  {
    name_id_t retval = parent_name;
    for (const token_id_t token_id: token_ids) {
      retval = name_id(retval, token_id);
    }
    return retval;
  }

  bool syntax_ward_t::name_id_is_parent(const name_id_t parent_name, token_id_t child_name) const
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

  name_id_t syntax_ward_t::name_id_lca(const name_id_t lhs, const name_id_t rhs) const
  {
    // TODO: O(1) time, O(n) memory ?
    const auto ni_path = [this](name_id_t x) {
      vector_t<name_id_t> retval;
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

  const name_id_style_t& syntax_ward_t::default_name_id_style() const
  {
    return impl->default_nis;
  }

  token_id_wrap_t syntax_ward_t::token_id_wrap(const token_id_t token_id)
  {
    return token_id_wrap_t{
        .swp      = ptr(),
        .token_id = token_id,
    };
  }
  name_id_wrap_t syntax_ward_t::name_id_wrap(const name_id_t name_id)
  {
    return name_id_wrap_t{
        .swp     = ptr(),
        .name_id = name_id,
    };
  }

  void stream_out_impl(stream_t* stream, const token_id_wrap_t& x)
  {
    stream->format("token[ {} ]", x.swp->token_infos[x.token_id].str);
  }

  void stream_out_impl(stream_t* stream, const name_id_wrap_t& x)
  {
    stream->write_str(x.swp->default_name_id_style().absolute(x.name_id));
  }

  tokenization_ptr_t syntax_ward_t::add(unique_ptr_t<const tokenization_t> x)
  {
    tokenizations.push_back(std::move(x));
    return tokenizations.back()->ptr();
  }

  parse_tree_ptr_t syntax_ward_t::add(unique_ptr_t<const parse_tree_t> x)
  {
    parse_trees.push_back(std::move(x));
    return parse_trees.back()->ptr();
  }
}
