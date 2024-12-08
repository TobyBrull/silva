#include "fern.hpp"

#include "parse_root.hpp"
#include "parse_tree_nursery.hpp"

#include "canopy/convert.hpp"

namespace silva {
  const parse_root_t* fern_parse_root()
  {
    static const parse_root_t retval = [&] {
      auto tokenization = SILVA_TRY_ASSERT(tokenize(const_ptr_unowned(&fern_seed_source_code)));
      auto fern_seed_pt = SILVA_TRY_ASSERT(seed_parse(to_unique_ptr(std::move(tokenization))));
      auto retval = SILVA_TRY_ASSERT(parse_root_t::create(to_unique_ptr(std::move(fern_seed_pt))));
      return std::move(retval);
    }();
    return &retval;
  }

  namespace impl {
    struct fern_nursery_t : public parse_tree_nursery_t {
      optional_t<token_id_t> tt_brkt_open  = lookup_token("[");
      optional_t<token_id_t> tt_brkt_close = lookup_token("]");
      optional_t<token_id_t> tt_semi_colon = lookup_token(";");
      optional_t<token_id_t> tt_colon      = lookup_token(":");
      optional_t<token_id_t> tt_none       = lookup_token("none");
      optional_t<token_id_t> tt_true       = lookup_token("true");
      optional_t<token_id_t> tt_false      = lookup_token("false");

      fern_nursery_t(const_ptr_t<tokenization_t> tokenization)
        : parse_tree_nursery_t(std::move(tokenization), fern_parse_root())
      {
      }

      expected_t<parse_tree_sub_t> label()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};

        SILVA_EXPECT(num_tokens_left() >= 2 && token_id(1) == tt_colon);

        SILVA_EXPECT_FMT(token_data()->category == token_category_t::STRING ||
                             token_data()->category == token_category_t::IDENTIFIER,
                         "Expected identifier or string for label");
        gg_rule.set_rule_index(std::to_underlying(fern_rule_t::LABEL));
        token_index += 2;

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> item()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};

        if (auto result = fern(); result) {
          gg_rule.sub += result.value();
          gg_rule.set_rule_index(std::to_underlying(fern_rule_t::ITEM_0));
        }
        else {
          SILVA_EXPECT_FMT(token_id() == tt_none || token_id() == tt_true ||
                               token_id() == tt_false ||
                               token_data()->category == token_category_t::STRING ||
                               token_data()->category == token_category_t::NUMBER,
                           "");
          gg_rule.set_rule_index(std::to_underlying(fern_rule_t::ITEM_1));
          token_index += 1;
        }

        return gg_rule.release();
      }

      expected_t<parse_tree_sub_t> labeled_item()
      {
        parse_tree_guard_for_rule_t gg_rule{&retval, &token_index};
        gg_rule.set_rule_index(std::to_underlying(fern_rule_t::LABELED_ITEM));

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
        gg_rule.set_rule_index(std::to_underlying(fern_rule_t::FERN));

        SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_brkt_open,
                         "Expected fern, but didn't find '['");
        token_index += 1;

        while (num_tokens_left() >= 1 && token_id() != tt_brkt_close) {
          gg_rule.sub += SILVA_TRY(labeled_item());
        }

        SILVA_EXPECT_FMT(num_tokens_left() >= 1 && token_id() == tt_brkt_close,
                         "Expected fern not ending with '['");
        token_index += 1;

        return gg_rule.release();
      }
    };
  }

  expected_t<parse_tree_t> fern_parse(const_ptr_t<tokenization_t> tokenization)
  {
    impl::fern_nursery_t fern_nursery(std::move(tokenization));
    const parse_tree_sub_t sub = SILVA_TRY(fern_nursery.fern());
    SILVA_ASSERT(sub.num_children == 1);
    SILVA_ASSERT(sub.num_children_total == fern_nursery.retval.nodes.size());
    SILVA_EXPECT_FMT(fern_nursery.token_index == tokenization->tokens.size(),
                     "Tokens left after parsing fern.");
    return {std::move(fern_nursery.retval)};
  }

  // Fern parse_tree output functions /////////////////////////////////////////////////////////////

  std::string
  fern_to_string(const parse_tree_t* pt, const index_t start_node, const bool with_semicolon)
  {
    SILVA_ASSERT(pt->nodes[start_node].rule_index == std::to_underlying(fern_rule_t::FERN));
    std::string retval;
    int depth{0};
    const auto retval_newline = [&retval, &depth]() {
      retval += '\n';
      for (int i = 0; i < depth * 2; ++i) {
        retval += ' ';
      }
    };
    const auto result = pt->visit_subtree(
        [&](const std::span<const parse_tree_visit_t> stack,
            const parse_tree_event_t event) -> expected_t<void> {
          SILVA_ASSERT(!stack.empty());
          const parse_tree_t::node_t& node = pt->nodes[stack.back().node_index];
          if (node.rule_index == std::to_underlying(fern_rule_t::FERN)) {
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
          else if (node.rule_index == std::to_underlying(fern_rule_t::LABELED_ITEM)) {
            if (is_on_entry(event)) {
              retval_newline();
            }
            if (is_on_exit(event) && with_semicolon) {
              retval += ';';
            }
          }
          else if (node.rule_index == std::to_underlying(fern_rule_t::LABEL)) {
            retval += pt->tokenization->token_data(node.token_index)->str;
            retval += " : ";
          }
          else if (node.rule_index == std::to_underlying(fern_rule_t::ITEM_1)) {
            retval += pt->tokenization->token_data(node.token_index)->str;
          }
          return {};
        },
        start_node);
    SILVA_ASSERT(result);
    return retval;
  }

  std::string fern_to_graphviz(const parse_tree_t* pt, const index_t start_node)
  {
    SILVA_ASSERT(pt->nodes[start_node].rule_index == std::to_underlying(fern_rule_t::FERN));
    std::string retval    = "digraph Fern {\n";
    std::string curr_path = "/";
    std::optional<std::string_view> last_label_str;
    const auto result = pt->visit_subtree(
        [&](const std::span<const parse_tree_visit_t> stack,
            const parse_tree_event_t event) -> expected_t<void> {
          SILVA_ASSERT(!stack.empty());
          const parse_tree_t::node_t& node = pt->nodes[stack.back().node_index];
          if (node.rule_index == std::to_underlying(fern_rule_t::LABELED_ITEM)) {
            if (is_on_entry(event)) {
              std::string prev_path = curr_path;
              curr_path += fmt::format("{}/", stack.back().child_index);
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
          else if (node.rule_index == std::to_underlying(fern_rule_t::LABEL)) {
            last_label_str = pt->tokenization->token_data(node.token_index)->str;
          }
          else if (node.rule_index == std::to_underlying(fern_rule_t::ITEM_1)) {
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
          return {};
        },
        start_node);
    SILVA_ASSERT(result);
    retval += "}";
    return retval;
  }

  // Object-oriented interface /////////////////////////////////////////////////////////////////////

  item_t::item_t() : value(std::nullopt) {}

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

    std::string operator()(std::nullopt_t) { return "none"; }
    std::string operator()(const bool arg) { return arg ? "true" : "false"; }
    std::string operator()(const std::string& arg) { return "\"" + arg + "\""; }
    std::string operator()(const double arg) { return fmt::format("{}", arg); }
    std::string operator()(const std::unique_ptr<fern_t>& arg)
    {
      return arg->to_str_fern(indent + 2);
    }
  };

  std::string fern_t::to_str_fern(const int indent) const
  {
    std::string retval;
    std::string ind(indent + 2, ' ');
    std::string_view ind_v = ind;

    const index_t n = items.size();

    if (items.empty()) {
      return "[]";
    }
    else {
      std::vector<std::optional<std::string_view>> used_labels;
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

  void fern_t_to_str_graphviz_impl_fern(std::string& retval,
                                        const fern_t& fern,
                                        const std::string_view prefix);

  void fern_t_to_str_graphviz_impl_item(std::string& retval,
                                        const std::optional<std::string>& label,
                                        const item_t& item,
                                        const std::string_view parent_name,
                                        const std::string_view item_name)
  {
    retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_name, item_name);
    std::string label_str = label.has_value() ? fmt::format("\\n[\\\"{}\\\"]", label.value()) : "";
    struct visitor {
      std::string& retval;
      const std::string_view label_str;
      const item_t& item;
      std::string_view item_name;

      void operator()(std::nullopt_t) const
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
        retval +=
            fmt::format("  \"{}\" [label=\"{}{}\\n{}\"]\n", item_name, item_name, label_str, value);
      }
      void operator()(const std::string& value) const
      {
        retval += fmt::format("  \"{}\" [label=\"{}{}\\n\\\"{}\\\"\"]\n",
                              item_name,
                              item_name,
                              label_str,
                              value);
      }
      void operator()(const std::unique_ptr<fern_t>& value) const
      {
        fern_t_to_str_graphviz_impl_fern(retval, *value, item_name);
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

  void fern_t_to_str_graphviz_impl_fern(std::string& retval,
                                        const fern_t& fern,
                                        const std::string_view prefix)
  {
    const index_t n = fern.items.size();
    std::vector<std::optional<std::string>> labels(n);
    for (const auto& [k, v]: fern.labels) {
      labels[v] = k;
    }
    for (index_t i = 0; i < n; ++i) {
      fern_t_to_str_graphviz_impl_item(retval,
                                       labels[i],
                                       fern.items[i],
                                       prefix,
                                       fmt::format("{}{}/", prefix, i));
    }
  }

  std::string fern_t::to_str_graphviz() const
  {
    std::string retval;
    retval += "digraph Fern {\n";
    fern_t_to_str_graphviz_impl_fern(retval, *this, "/");
    retval += "}";
    return retval;
  }

  labeled_item_t parse_tree_to_item(const parse_tree_t* pt, const index_t start_node)
  {
    const parse_tree_t::node_t& labeled_item = pt->nodes[start_node];
    SILVA_ASSERT(labeled_item.rule_index == std::to_underlying(fern_rule_t::LABELED_ITEM));
    labeled_item_t retval;
    const auto result = pt->visit_children(
        [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
          const parse_tree_t::node_t& node = pt->nodes[node_index];
          if (labeled_item.num_children == 2 && child_index == 0) {
            SILVA_ASSERT(node.rule_index == std::to_underlying(fern_rule_t::LABEL));
            retval.label = pt->tokenization->token_data(node.token_index)->as_string();
          }
          else {
            if (node.rule_index == std::to_underlying(fern_rule_t::ITEM_0)) {
              retval.item.value = std::make_unique<fern_t>(fern_create(pt, node_index + 1));
            }
            else if (node.rule_index == std::to_underlying(fern_rule_t::ITEM_1)) {
              const auto* token_data = pt->tokenization->token_data(node.token_index);
              if (token_data->str == "none") {
                retval.item.value = std::nullopt;
              }
              else if (token_data->str == "true") {
                retval.item.value = true;
              }
              else if (token_data->str == "false") {
                retval.item.value = false;
              }
              else if (token_data->category == token_category_t::STRING) {
                retval.item.value = token_data->as_string();
              }
              else if (token_data->category == token_category_t::NUMBER) {
                retval.item.value = token_data->as_double();
              }
            }
          }
          return true;
        },
        start_node);
    SILVA_ASSERT(result);
    return retval;
  }

  fern_t fern_create(const parse_tree_t* pt, const index_t start_node)
  {
    SILVA_ASSERT(pt->nodes[start_node].rule_index == std::to_underlying(fern_rule_t::FERN));
    fern_t retval;
    const auto result = pt->visit_children(
        [&](const index_t node_index, const index_t child_index) -> expected_t<bool> {
          labeled_item_t li  = parse_tree_to_item(pt, node_index);
          const size_t index = retval.items.size();
          retval.items.push_back(std::move(li.item));
          if (li.label.has_value()) {
            retval.labels.emplace(std::move(li.label.value()), index);
          }
          return true;
        },
        start_node);
    SILVA_ASSERT(result);
    return retval;
  }
}
