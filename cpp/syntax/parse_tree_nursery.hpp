#pragma once

#include "parse_tree.hpp"

#include "canopy/tree_nursery.hpp"

namespace silva {

#define SILVA_EXPECT_PARSE(name, cond, fmt_str, ...) \
  SILVA_EXPECT(cond,                                 \
               MINOR,                                \
               "[{}] {}: " fmt_str,                  \
               fragment_location_by(),               \
               lexicon.name_id_wrap(name) __VA_OPT__(, ) __VA_ARGS__);

#define SILVA_EXPECT_PARSE_FWD(name, expr) \
  SILVA_EXPECT_FWD(expr, "[{}] {}", fragment_location_by(), lexicon.name_id_wrap(name))

#define SILVA_EXPECT_PARSE_FRAGMENT_CODEPOINT(name, codepoint)                              \
  {                                                                                         \
    const auto cp = SILVA_EXPECT_PARSE_FWD(name, fp->get_unique_codepoint(fragment_index)); \
    SILVA_EXPECT_PARSE(name, cp == codepoint, "expected {}, got {}", codepoint, cp);        \
    fragment_index += 1;                                                                    \
  }

#define SILVA_EXPECT_PARSE_FRAGMENT_CATEGORY(name, frag_cat)                      \
  SILVA_EXPECT_PARSE(name,                                                        \
                     num_fragments_left() >= 1 &&                                 \
                         fragment_category_by() == fragment_category_t::frag_cat, \
                     "expected category {}, got {}",                              \
                     fragment_category_t::frag_cat,                               \
                     fragment_category_by());                                     \
  fragment_index += 1;

  struct parse_tree_nursery_state_t : public tree_nursery_state_t {
    index_t fragment_index = 0;
    index_t token_index    = 0;
  };

  struct parse_tree_nursery_t
    : public tree_nursery_t<parse_tree_node_t, parse_tree_nursery_state_t, parse_tree_nursery_t> {
    syntax_farm_ptr_t sfp;
    fragmentization_ptr_t fp;
    fragment_span_t fs;

    tokenization_t tokenization;

    parse_tree_nursery_t(fragment_span_t);

    index_t fragment_index = 0;

    struct token_stake_t {
      parse_tree_nursery_t* nursery = nullptr;
      name_id_t token_cat           = name_id_none;
      index_t orig_fragment_index   = 0;

      token_stake_t() = default;
      token_stake_t(parse_tree_nursery_t* nursery,
                    const name_id_t token_cat,
                    const index_t orig_fragment_index);

      token_stake_t(stake_t&&);
      token_stake_t& operator=(stake_t&&);

      token_stake_t(const stake_t&)            = delete;
      token_stake_t& operator=(const stake_t&) = delete;

      void add_token(const token_t&);

      token_t commit();
      void clear();
      ~token_stake_t();
    };
    [[nodiscard]] token_stake_t token_stake(this auto& self, const token_id_t token_cat);

    expected_t<token_t> literal_fragmented_token(fragmented_token_t);

    void add_token(const token_t&);

    void on_get_state(parse_tree_nursery_state_t&) const;
    void on_set_state(const parse_tree_nursery_state_t&);

    void on_stake_ctor(parse_tree_node_t& proto_node) const;
    void on_stake_create_node(parse_tree_node_t& proto_node, name_id_t) const;
    void on_stake_add_proto_node(parse_tree_node_t& proto_node,
                                 const parse_tree_node_t& other) const;
    void on_stake_commit_pre(parse_tree_node_t& proto_node) const;
    void on_stake_commit_owning_to_proto(parse_tree_node_t& proto_node) const;

    expected_t<void> init(name_id_t, const lexicon_t&);
    expected_t<parse_tree_ptr_t> finish() &&;

    index_t num_fragments_left() const;
    const fragment_t* fragment_by(index_t idx_offset = 0) const;

    // Returns U'\0' if the given codepoint does not have a unique codepoint.
    unicode::codepoint_t fragment_unique_codepoint_or_zero_by(index_t idx_offset = 0) const;

    fragment_category_t fragment_category_by(index_t idx_offset = 0) const;
    fragment_location_t fragment_location_by(index_t idx_offset = 0) const;
    fragment_location_t fragment_location_at(index_t idx) const;
  };
}

// IMPLEMENTATION

namespace silva {

  inline parse_tree_nursery_t::token_stake_t::token_stake_t(parse_tree_nursery_t* nursery,
                                                            const name_id_t token_cat,
                                                            const index_t orig_fragment_index)
    : nursery(nursery), token_cat(token_cat), orig_fragment_index(orig_fragment_index)
  {
  }

  inline void parse_tree_nursery_t::token_stake_t::add_token(const token_t& token)
  {
    SILVA_ASSERT(token.frag_idx_begin >= orig_fragment_index);
    SILVA_ASSERT(token.frag_idx_end == nursery->fragment_index);
  }

  inline token_t parse_tree_nursery_t::token_stake_t::commit()
  {
    const fragment_span_t fs{nursery->fp, orig_fragment_index, nursery->fragment_index};
    token_t retval{
        .token_id       = nursery->sfp->token_id(fs),
        .category       = token_cat,
        .frag_idx_begin = fs.begin,
        .frag_idx_end   = fs.end,
    };
    nursery = nullptr;
    return retval;
  }

  inline void parse_tree_nursery_t::token_stake_t::clear()
  {
    if (nursery != nullptr) {
      nursery->fragment_index = orig_fragment_index;
    }
    nursery = nullptr;
  }

  inline parse_tree_nursery_t::token_stake_t::~token_stake_t()
  {
    clear();
  }

  [[nodiscard]] parse_tree_nursery_t::token_stake_t
  parse_tree_nursery_t::token_stake(this auto& self, const name_id_t token_cat)
  {
    return token_stake_t{&self, token_cat, self.fragment_index};
  }

  inline expected_t<token_t>
  parse_tree_nursery_t::literal_fragmented_token(const fragmented_token_t ft)
  {
    auto ts                     = token_stake(name_id_literal);
    const index_t n             = ft.items.size();
    const index_t orig_frag_idx = fragment_index;
    SILVA_EXPECT(num_fragments_left() >= n,
                 MINOR,
                 "[{}] not enough fragments left when expecting {}",
                 fragment_location_at(orig_frag_idx),
                 sfp->token_id_wrap(ft.token_id));
    for (index_t i = 0; i < n; ++i) {
      auto maybe_curr_cp = fp->get_unique_codepoint(fragment_index);
      if (!maybe_curr_cp.has_value()) {
        maybe_curr_cp.error().clear();
        SILVA_EXPECT(false,
                     MINOR,
                     "[{}] expected {}",
                     fragment_location_at(orig_frag_idx),
                     sfp->token_id_wrap(ft.token_id));
      }
      const unicode::codepoint_t curr_cp = *maybe_curr_cp;
      SILVA_EXPECT(ft.items[i].codepoint == curr_cp,
                   MINOR,
                   "[{}] expected {}",
                   fragment_location_at(orig_frag_idx),
                   sfp->token_id_wrap(ft.token_id));
      fragment_index += 1;
    }
    return ts.commit();
  }

  inline void parse_tree_nursery_t::add_token(const token_t& token)
  {
    tokenization.tokens.push_back(token);
  }
}
