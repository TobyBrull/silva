#include "fern.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"

#include "canopy/convert.hpp"

namespace silva {
  using enum token_category_t;
  using enum fern_rule_t;

  const parse_root_t* fern_parse_root()
  {
    static const parse_root_t retval =
        SILVA_TRY_ASSERT(parse_root_t::create(const_ptr_unowned(&fern_seed_source_code)));
    return &retval;
  }

  namespace impl {
    struct fern_parse_tree_nursery_t : public parse_tree_nursery_t {
      optional_t<token_id_t> tt_brkt_open  = retval.tokenization->lookup_token("[");
      optional_t<token_id_t> tt_brkt_close = retval.tokenization->lookup_token("]");
      optional_t<token_id_t> tt_semi_colon = retval.tokenization->lookup_token(";");
      optional_t<token_id_t> tt_colon      = retval.tokenization->lookup_token(":");
      optional_t<token_id_t> tt_none       = retval.tokenization->lookup_token("none");
      optional_t<token_id_t> tt_true       = retval.tokenization->lookup_token("true");
      optional_t<token_id_t> tt_false      = retval.tokenization->lookup_token("false");

      fern_parse_tree_nursery_t(const_ptr_t<tokenization_t> tokenization)
        : parse_tree_nursery_t(std::move(tokenization), const_ptr_unowned(fern_parse_root()))
      {
      }

      expected_t<parse_tree_sub_t> label()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        SILVA_EXPECT(num_tokens_left() >= 2 && token_id(1) == tt_colon);
        SILVA_EXPECT_FMT(token_data()->category == STRING || token_data()->category == IDENTIFIER,
                         "Expected identifier or string for label");
        gg_rule.set_rule_index(to_int(LABEL));
        token_index += 2;
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> item()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        if (auto result = fern(); result) {
          gg_rule.sub += result.value();
          gg_rule.set_rule_index(to_int(ITEM_0));
        }
        else {
          SILVA_EXPECT_FMT(token_id() == tt_none || token_id() == tt_true ||
                               token_id() == tt_false || token_data()->category == STRING ||
                               token_data()->category == NUMBER,
                           "");
          gg_rule.set_rule_index(to_int(ITEM_1));
          token_index += 1;
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> labeled_item()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(LABELED_ITEM));
        SILVA_EXPECT_FMT(num_tokens_left() >= 1, "No tokens left when trying to parse an item.");
        if (num_tokens_left() >= 2 && token_id(1) == tt_colon) {
          gg_rule.sub += SILVA_TRY(label());
        }
        gg_rule.sub += SILVA_TRY(item());
        if (num_tokens_left() >= 1 && token_id() == tt_semi_colon) {
          token_index += 1;
        }
        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> fern()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(to_int(FERN));
        SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_brkt_open,
                         "Expected fern, but didn't find '['");
        token_index += 1;
        while (num_tokens_left() >= 1 && token_id() != tt_brkt_close) {
          gg_rule.sub += SILVA_TRY(labeled_item());
        }
        SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_brkt_close,
                         "Expected end of fern, but didn't find '['");
        token_index += 1;
        return gg_rule.release();
      }
    };
  }

  expected_t<parse_tree_t> fern_parse(const_ptr_t<tokenization_t> tokenization)
  {
    const index_t n = tokenization->tokens.size();
    impl::fern_parse_tree_nursery_t nursery(std::move(tokenization));
    const parse_tree_sub_t sub = SILVA_TRY(nursery.fern());
    SILVA_ASSERT(sub.num_children == 1);
    SILVA_ASSERT(sub.num_children_total == nursery.retval.nodes.size());
    SILVA_EXPECT_FMT(nursery.token_index == n, "Tokens left after parsing fern.");
    return {std::move(nursery.retval)};
  }

  // Fern parse_tree output functions /////////////////////////////////////////////////////////////

  string_t
  fern_to_string(const parse_tree_t* pt, const index_t start_node, const bool with_semicolon)
  {
    SILVA_ASSERT(pt->root.get() == fern_parse_root());
    SILVA_ASSERT(pt->nodes[start_node].rule_index == to_int(FERN));
    string_t retval;
    int depth{0};
    const auto retval_newline = [&retval, &depth]() {
      retval += '\n';
      for (int i = 0; i < depth * 2; ++i) {
        retval += ' ';
      }
    };
    const auto result = pt->visit_subtree(
        [&](const span_t<const parse_tree_t::visit_state_t> path,
            const parse_tree_event_t event) -> expected_t<bool> {
          SILVA_ASSERT(!path.empty());
          const parse_tree_t::node_t& node = pt->nodes[path.back().node_index];
          if (node.rule_index == to_int(FERN)) {
            if (is_on_entry(event)) {
              retval += '[';
              depth += 1;
            }
            if (is_on_exit(event)) {
              depth -= 1;
              if (0 < node.num_children) {
                retval_newline();
              }
              retval += ']';
            }
          }
          else if (node.rule_index == to_int(LABELED_ITEM)) {
            if (is_on_entry(event)) {
              retval_newline();
            }
            if (is_on_exit(event) && with_semicolon) {
              retval += ';';
            }
          }
          else if (node.rule_index == to_int(LABEL)) {
            retval += pt->tokenization->token_data(node.token_index)->str;
            retval += " : ";
          }
          else if (node.rule_index == to_int(ITEM_1)) {
            retval += pt->tokenization->token_data(node.token_index)->str;
          }
          return true;
        },
        start_node);
    SILVA_ASSERT(result);
    return retval;
  }

  string_t fern_to_graphviz(const parse_tree_t* pt, const index_t start_node)
  {
    SILVA_ASSERT(pt->root.get() == fern_parse_root());
    SILVA_ASSERT(pt->nodes[start_node].rule_index == to_int(FERN));
    string_t retval    = "digraph Fern {\n";
    string_t curr_path = "/";
    optional_t<string_view_t> last_label_str;
    const auto result = pt->visit_subtree(
        [&](const span_t<const parse_tree_t::visit_state_t> path,
            const parse_tree_event_t event) -> expected_t<bool> {
          SILVA_ASSERT(!path.empty());
          const parse_tree_t::node_t& node = pt->nodes[path.back().node_index];
          if (node.rule_index == to_int(LABELED_ITEM)) {
            if (is_on_entry(event)) {
              string_t prev_path = curr_path;
              curr_path += fmt::format("{}/", path.back().child_index);
              retval += fmt::format("  \"{}\" -> \"{}\"\n", prev_path, curr_path);
            }
            if (is_on_exit(event)) {
              curr_path.pop_back();
              while (curr_path.back() != '/') {
                curr_path.pop_back();
              }
              last_label_str = none;
            }
          }
          else if (node.rule_index == to_int(LABEL)) {
            last_label_str = pt->tokenization->token_data(node.token_index)->str;
          }
          else if (node.rule_index == to_int(ITEM_1)) {
            if (last_label_str.has_value()) {
              retval +=
                  fmt::format("  \"{}\" [label=\"{}\\n[{}]\\n{}\"]\n",
                              curr_path,
                              curr_path,
                              string_escaped(last_label_str.value()),
                              string_escaped(pt->tokenization->token_data(node.token_index)->str));
            }
            else {
              retval +=
                  fmt::format("  \"{}\" [label=\"{}\\n{}\"]\n",
                              curr_path,
                              curr_path,
                              string_escaped(pt->tokenization->token_data(node.token_index)->str));
            }
          }
          return true;
        },
        start_node);
    SILVA_ASSERT(result);
    retval += "}";
    return retval;
  }

  // Object-oriented interface /////////////////////////////////////////////////////////////////////

  item_t::item_t() : value(none) {}

  void fern_t::push_back(labeled_item_t&& li)
  {
    const index_t index = items.size();
    items.push_back(std::move(li.item));
    if (li.label.has_value()) {
      labels.emplace(std::move(li.label.value()), index);
    }
  }

  struct to_str_visitor {
    int indent = 0;

    string_t operator()(none_t) { return "none"; }
    string_t operator()(const bool arg) { return arg ? "true" : "false"; }
    string_t operator()(const string_t& arg) { return fmt::format("\"{}\"", arg); }
    string_t operator()(const double arg) { return fmt::format("{}", arg); }
    string_t operator()(const unique_ptr_t<fern_t>& arg) { return arg->to_string(indent + 2); }
  };

  string_t fern_t::to_string(const int indent) const
  {
    const index_t n = items.size();
    string_t retval;
    string_t ind(indent + 2, ' ');
    string_view_t ind_v = ind;
    if (items.empty()) {
      return "[]";
    }
    else {
      vector_t<optional_t<string_view_t>> used_labels;
      used_labels.resize(n);
      for (const auto& [k, v]: labels) {
        used_labels[v] = k;
      }
      retval += '[';
      for (index_t i = 0; i < n; ++i) {
        retval += '\n';
        retval += ind;
        if (used_labels[i].has_value()) {
          retval += '"';
          retval += used_labels[i].value();
          retval += "\" : ";
        }
        retval += std::visit(to_str_visitor{indent}, items[i].value);
        retval += ";";
      }
      retval += '\n';
      retval += ind_v.substr(0, indent);
      retval += "]";
    }
    return retval;
  }

  namespace impl {
    void
    fern_t__to_graphviz__fern(string_t& retval, const fern_t& fern, const string_view_t prefix);

    void fern_t__to_graphviz__item(string_t& retval,
                                   const optional_t<string_t>& label,
                                   const item_t& item,
                                   const string_view_t parent_name,
                                   const string_view_t item_name)
    {
      retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_name, item_name);
      string_t label_str = label.has_value() ? fmt::format("\\n[\\\"{}\\\"]", label.value()) : "";
      struct visitor {
        string_t& retval;
        const string_view_t label_str;
        const item_t& item;
        string_view_t item_name;

        void operator()(none_t) const
        {
          retval +=
              fmt::format("  \"{}\" [label=\"{}{}\\nnone\"]\n", item_name, item_name, label_str);
        }
        void operator()(const bool value) const
        {
          retval += fmt::format("  \"{}\" [label=\"{}\\n{}{}\"]\n",
                                item_name,
                                item_name,
                                label_str,
                                value ? "true" : "false");
        }
        void operator()(const double value) const
        {
          retval += fmt::format("  \"{}\" [label=\"{}{}\\n{}\"]\n",
                                item_name,
                                item_name,
                                label_str,
                                value);
        }
        void operator()(const string_t& value) const
        {
          retval += fmt::format("  \"{}\" [label=\"{}{}\\n\\\"{}\\\"\"]\n",
                                item_name,
                                item_name,
                                label_str,
                                value);
        }
        void operator()(const unique_ptr_t<fern_t>& value) const
        {
          fern_t__to_graphviz__fern(retval, *value, item_name);
        }
      };
      std::visit(
          visitor{
              .retval    = retval,
              .label_str = label_str,
              .item      = item,
              .item_name = item_name,
          },
          item.value);
    }

    void fern_t__to_graphviz__fern(string_t& retval, const fern_t& fern, const string_view_t prefix)
    {
      const index_t n = fern.items.size();
      vector_t<optional_t<string_t>> labels(n);
      for (const auto& [k, v]: fern.labels) {
        labels[v] = k;
      }
      for (index_t i = 0; i < n; ++i) {
        fern_t__to_graphviz__item(retval,
                                  labels[i],
                                  fern.items[i],
                                  prefix,
                                  fmt::format("{}{}/", prefix, i));
      }
    }
  }

  string_t fern_t::to_graphviz() const
  {
    string_t retval;
    retval += "digraph Fern {\n";
    impl::fern_t__to_graphviz__fern(retval, *this, "/");
    retval += "}";
    return retval;
  }

  namespace impl {
    struct fern_nursery_t {
      const parse_tree_t* parse_tree = nullptr;

      optional_t<token_id_t> tt_none  = parse_tree->tokenization->lookup_token("none");
      optional_t<token_id_t> tt_true  = parse_tree->tokenization->lookup_token("true");
      optional_t<token_id_t> tt_false = parse_tree->tokenization->lookup_token("false");

      labeled_item_t item(const index_t start_node)
      {
        const parse_tree_t::node_t& labeled_item = parse_tree->nodes[start_node];
        SILVA_ASSERT(labeled_item.rule_index == to_int(LABELED_ITEM));
        labeled_item_t retval;
        const auto result = parse_tree->visit_children(
            [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
              const parse_tree_t::node_t& node = parse_tree->nodes[child_node_index];
              if (labeled_item.num_children == 2 && child_index == 0) {
                SILVA_ASSERT(node.rule_index == to_int(LABEL));
                retval.label = parse_tree->tokenization->token_data(node.token_index)->as_string();
              }
              else {
                if (node.rule_index == to_int(ITEM_0)) {
                  retval.item.value =
                      std::make_unique<fern_t>(fern_create(parse_tree, child_node_index + 1));
                }
                else if (node.rule_index == to_int(ITEM_1)) {
                  const token_id_t token_id = parse_tree->tokenization->tokens[node.token_index];
                  const auto* token_data = parse_tree->tokenization->token_data(node.token_index);
                  if (token_id == tt_none) {
                    retval.item.value = none;
                  }
                  else if (token_id == tt_true) {
                    retval.item.value = true;
                  }
                  else if (token_id == tt_false) {
                    retval.item.value = false;
                  }
                  else if (token_data->category == STRING) {
                    retval.item.value = token_data->as_string();
                  }
                  else if (token_data->category == NUMBER) {
                    retval.item.value = token_data->as_double();
                  }
                  else {
                    SILVA_ASSERT_FMT(false, "Unknown item '{}'", token_data->str);
                  }
                }
              }
              return true;
            },
            start_node);
        SILVA_ASSERT(result);
        return retval;
      }

      fern_t fern(const index_t start_node)
      {
        SILVA_ASSERT(parse_tree->nodes[start_node].rule_index == to_int(FERN));
        fern_t retval;
        const auto result = parse_tree->visit_children(
            [&](const index_t child_node_index, const index_t child_index) -> expected_t<bool> {
              labeled_item_t li = item(child_node_index);
              retval.push_back(std::move(li));
              return true;
            },
            start_node);
        SILVA_ASSERT(result);
        return retval;
      }
    };
  }

  fern_t fern_create(const parse_tree_t* parse_tree, const index_t start_node)
  {
    impl::fern_nursery_t fern_nursery{.parse_tree = parse_tree};
    return fern_nursery.fern(start_node);
  }
}
