#pragma once

#include "canopy/assert.hpp"
#include "canopy/expected.hpp"

namespace silva {

  // An index in the "token_infos" vector of "syntax_farm_t". Equality of two tokens is then
  // equivalent to the equality of their token_info_index_t.
  using token_id_t = index_t;

  // Index in "name_infos".
  using name_id_t = index_t;

  constexpr inline token_id_t token_id_none     = 0;
  constexpr inline token_id_t token_id_language = 1;
  constexpr inline name_id_t name_id_root       = 0;

  struct token_info_t {
    string_t str;

    expected_t<string_view_t> string_as_plain_contained() const;
    expected_t<string_t> contained_string() const;
    expected_t<double> number_as_double() const;

    friend auto operator<=>(const token_info_t&, const token_info_t&) = default;
  };

  struct name_info_t {
    name_id_t parent_name = name_id_root;
    token_id_t base_name  = token_id_none;

    friend auto operator<=>(const name_info_t&, const name_info_t&) = default;
    friend hash_value_t hash_impl(const name_info_t& x);
  };

  struct token_id_wrap_t;
  struct name_id_wrap_t;

  struct fragmentization_t;
  struct tokenization_t;
  struct parse_tree_t;
  using fragmentization_ptr_t = ptr_t<const fragmentization_t>;
  using tokenization_ptr_t    = ptr_t<const tokenization_t>;
  using parse_tree_ptr_t      = ptr_t<const parse_tree_t>;
  struct fragment_span_t;
  struct token_span_t;
  struct parse_tree_span_t;

  struct lexicon_t;

  struct syntax_farm_t : public menhir_t {
    array_t<token_info_t> token_infos;
    hash_map_t<string_t, token_id_t> token_lookup;

    array_t<name_info_t> name_infos;
    hash_map_t<name_info_t, name_id_t> name_lookup;

    hash_map_t<std::type_index, unique_ptr_t<const lexicon_t>> lexicons;

    array_t<unique_ptr_t<const fragmentization_t>> fragmentizations;
    array_t<unique_ptr_t<const tokenization_t>> tokenizations;
    array_t<unique_ptr_t<const parse_tree_t>> parse_trees;

    syntax_farm_t();
    ~syntax_farm_t();

    token_id_t token_id(string_view_t);

    expected_t<token_id_t> token_id_in_string(token_id_t);

    name_id_t name_id(name_id_t parent_name, token_id_t base_name);
    name_id_t name_id_span(name_id_t parent_name, span_t<const token_id_t>);
    bool name_id_is_parent(name_id_t parent_name, name_id_t child_name) const;

    name_id_t name_id_lca(name_id_t, name_id_t) const;

    template<typename... Ts>
    name_id_t name_id_of(Ts&&... xs);
    template<typename... Ts>
    name_id_t name_id_of(name_id_t parent_name, Ts&&... xs);

    token_id_wrap_t token_id_wrap(token_id_t);

    template<typename LexiconType>
    const LexiconType& get_lexicon();

    fragmentization_ptr_t add(unique_ptr_t<const fragmentization_t>);
    tokenization_ptr_t add(unique_ptr_t<const tokenization_t>);
    parse_tree_ptr_t add(unique_ptr_t<const parse_tree_t>);
  };
  using syntax_farm_ptr_t = ptr_t<syntax_farm_t>;

  struct lexicon_t : public menhir_t {
    syntax_farm_ptr_t sfp;
    token_id_t language_name = token_id_none;

    token_id_t here_name = sfp->token_id("⊙");
    token_id_t name_sep  = sfp->token_id(".");

    lexicon_t(syntax_farm_ptr_t);

    virtual ~lexicon_t();

    name_id_wrap_t name_id_wrap(name_id_t) const;

    string_t name_id_str(name_id_t) const;

    expected_t<name_id_t> name_id(const name_id_t scope_name, const token_span_t&) const;
  };
  using lexicon_ptr_t = ptr_t<const lexicon_t>;

  struct token_id_wrap_t {
    syntax_farm_ptr_t sfp;
    token_id_t token_id = token_id_none;

    friend void pretty_write_impl(const token_id_wrap_t&, byte_sink_t*);
  };

  struct name_id_wrap_t {
    lexicon_ptr_t lp;
    name_id_t name_id = name_id_root;

    friend void pretty_write_impl(const name_id_wrap_t&, byte_sink_t*);
  };
}

// IMPLEMENTATION

namespace silva {
  template<typename LexiconType>
  const LexiconType& syntax_farm_t::get_lexicon()
  {
    static_assert(std::derived_from<LexiconType, lexicon_t>);
    const std::type_index type_idx = typeid(LexiconType);
    const auto it                  = lexicons.find(type_idx);
    if (it != lexicons.end()) {
      return dynamic_cast<const LexiconType&>(*it->second);
    }
    else {
      std::unique_ptr<LexiconType> ll(new LexiconType(ptr()));
      const LexiconType* lp = ll.get();
      auto [it, inserted]   = lexicons.emplace(type_idx, std::move(ll));
      SILVA_ASSERT(inserted);
      return *lp;
    }
  }

  template<typename... Ts>
  name_id_t syntax_farm_t::name_id_of(Ts&&... xs)
  {
    array_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)))), ...);
    return name_id_span(name_id_root, vec);
  }

  template<typename... Ts>
  name_id_t syntax_farm_t::name_id_of(name_id_t parent_name, Ts&&... xs)
  {
    array_t<token_id_t> vec;
    ((vec.push_back(token_id(std::forward<Ts>(xs)))), ...);
    return name_id_span(parent_name, vec);
  }
}
