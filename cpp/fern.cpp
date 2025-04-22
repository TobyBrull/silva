#include "fern.hpp"

#include "canopy/convert.hpp"
#include "canopy/error.hpp"
#include "canopy/expected.hpp"
#include "syntax/parse_tree_nursery.hpp"
#include "syntax/seed_engine.hpp"
#include "syntax/syntax_ward.hpp"

namespace silva {
  using enum token_category_t;
  using enum error_level_t;

  unique_ptr_t<seed_engine_t> fern_seed_engine(syntax_ward_ptr_t swp)
  {
    auto retval = std::make_unique<seed_engine_t>(std::move(swp));
    SILVA_EXPECT_ASSERT(retval->add_complete_file("fern.seed", fern_seed));
    return retval;
  }

  namespace impl {
    struct fern_parse_tree_nursery_t : public parse_tree_nursery_t {
      token_id_t tt_brkt_open  = *swp->token_id("[");
      token_id_t tt_brkt_close = *swp->token_id("]");
      token_id_t tt_colon      = *swp->token_id(":");
      token_id_t tt_none       = *swp->token_id("none");
      token_id_t tt_true       = *swp->token_id("true");
      token_id_t tt_false      = *swp->token_id("false");

      name_id_t fni_fern     = swp->name_id_of("Fern");
      name_id_t fni_lbl_item = swp->name_id_of(fni_fern, "LabeledItem");
      name_id_t fni_label    = swp->name_id_of(fni_fern, "Label");
      name_id_t fni_value    = swp->name_id_of(fni_fern, "Value");

      fern_parse_tree_nursery_t(tokenization_ptr_t tp) : parse_tree_nursery_t(tp) {}

      expected_t<parse_tree_node_t> value()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_value);
        SILVA_EXPECT_PARSE(fni_value,
                           num_tokens_left() >= 1 &&
                               (token_id_by() == tt_none || token_id_by() == tt_true ||
                                token_id_by() == tt_false || token_data_by()->category == STRING ||
                                token_data_by()->category == NUMBER),
                           "unexpected {}",
                           swp->token_id_wrap(token_id_by()));
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> label()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_label);
        SILVA_EXPECT_PARSE(
            fni_label,
            num_tokens_left() >= 1 &&
                (token_data_by()->category == STRING || token_data_by()->category == IDENTIFIER),
            "expected string or identifier, found {}",
            swp->token_id_wrap(token_id_by()));
        token_index += 1;
        return ss_rule.commit();
      }

      expected_t<parse_tree_node_t> labeled_item()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_lbl_item);

        if (num_tokens_left() >= 2 && token_id_by(1) == tt_colon) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_lbl_item, label()));
          token_index += 1;
        }

        error_nursery_t error_nursery;

        if (auto result = fern(); result) {
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        else {
          error_nursery.add_child_error(std::move(result).error());
        }

        if (auto result = value(); result) {
          ss_rule.add_proto_node(*result);
          return ss_rule.commit();
        }
        else {
          error_nursery.add_child_error(std::move(result).error());
        }

        return std::unexpected(std::move(error_nursery)
                                   .finish(MINOR,
                                           "[{}] {}",
                                           token_position_at(ss_rule.orig_state.token_index),
                                           swp->name_id_wrap(fni_lbl_item)));
      }

      expected_t<parse_tree_node_t> fern()
      {
        auto ss_rule = stake();
        ss_rule.create_node(fni_fern);
        SILVA_EXPECT_PARSE(fni_fern,
                           num_tokens_left() >= 1 && token_id_by() == tt_brkt_open,
                           "expected {}",
                           swp->token_id_wrap(tt_brkt_open));
        token_index += 1;
        while (num_tokens_left() >= 1 && token_id_by() != tt_brkt_close) {
          ss_rule.add_proto_node(SILVA_EXPECT_PARSE_FWD(fni_fern, labeled_item()));
        }
        SILVA_EXPECT_PARSE(fni_fern,
                           num_tokens_left() >= 1 && token_id_by() == tt_brkt_close,
                           "expected {}",
                           swp->token_id_wrap(tt_brkt_close));
        token_index += 1;
        return ss_rule.commit();
      }
    };
  }

  expected_t<parse_tree_ptr_t> fern_parse(tokenization_ptr_t tp)
  {
    const index_t n = tp->tokens.size();
    impl::fern_parse_tree_nursery_t nursery(tp);
    const parse_tree_node_t ptn = SILVA_EXPECT_FWD(nursery.fern());
    SILVA_EXPECT(ptn.num_children == 1, ASSERT);
    SILVA_EXPECT(ptn.subtree_size == nursery.tree.size(), ASSERT);
    SILVA_EXPECT(nursery.token_index == n, MAJOR, "Tokens left after parsing fern.");
    return tp->swp->add(std::move(nursery).finish());
  }

  // Fern parse_tree output functions /////////////////////////////////////////////////////////////

  expected_t<string_t> fern_to_string(const parse_tree_t* pt, const index_t start_node)
  {
    syntax_ward_ptr_t swp        = pt->tp->swp;
    const name_id_t fni_fern     = swp->name_id_of("Fern");
    const name_id_t fni_lbl_item = swp->name_id_of(fni_fern, "LabeledItem");
    const name_id_t fni_label    = swp->name_id_of(fni_fern, "Label");
    const name_id_t fni_value    = swp->name_id_of(fni_fern, "Value");

    SILVA_EXPECT(pt->nodes[start_node].rule_name == fni_fern, ASSERT);
    string_t retval;
    int depth{0};
    const auto retval_newline = [&retval, &depth]() {
      retval += '\n';
      for (int i = 0; i < depth * 2; ++i) {
        retval += ' ';
      }
    };
    auto result = pt->span()
                      .sub_tree_span_at(start_node)
                      .visit_subtree([&](const span_t<const tree_branch_t> path,
                                         const tree_event_t event) -> expected_t<bool> {
                        SILVA_EXPECT(!path.empty(), ASSERT);
                        const auto& node = pt->nodes[path.back().node_index];
                        if (node.rule_name == fni_fern) {
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
                        else if (node.rule_name == fni_lbl_item) {
                          if (is_on_entry(event)) {
                            retval_newline();
                          }
                        }
                        else if (node.rule_name == fni_label) {
                          retval += pt->tp->token_info_get(node.token_begin)->str;
                          retval += " : ";
                        }
                        else if (node.rule_name == fni_value) {
                          retval += pt->tp->token_info_get(node.token_begin)->str;
                        }
                        return true;
                      });
    SILVA_EXPECT_FWD(std::move(result));
    return retval;
  }

  expected_t<string_t> fern_to_graphviz(const parse_tree_t* pt, const index_t start_node)
  {
    syntax_ward_ptr_t swp        = pt->tp->swp;
    const name_id_t fni_fern     = swp->name_id_of("Fern");
    const name_id_t fni_lbl_item = swp->name_id_of(fni_fern, "LabeledItem");
    const name_id_t fni_label    = swp->name_id_of(fni_fern, "Label");
    const name_id_t fni_value    = swp->name_id_of(fni_fern, "Value");

    SILVA_EXPECT(pt->nodes[start_node].rule_name == fni_fern, ASSERT);
    string_t retval    = "digraph Fern {\n";
    string_t curr_path = "/";
    optional_t<string_view_t> last_label_str;
    auto result =
        pt->span()
            .sub_tree_span_at(start_node)
            .visit_subtree([&](const span_t<const tree_branch_t> path,
                               const tree_event_t event) -> expected_t<bool> {
              SILVA_EXPECT(!path.empty(), ASSERT);
              const auto& node = pt->nodes[path.back().node_index];
              if (node.rule_name == fni_lbl_item) {
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
              else if (node.rule_name == fni_label) {
                last_label_str = pt->tp->token_info_get(node.token_begin)->str;
              }
              else if (node.rule_name == fni_value) {
                if (last_label_str.has_value()) {
                  retval +=
                      fmt::format("  \"{}\" [label=\"{}\\n[{}]\\n{}\"]\n",
                                  curr_path,
                                  curr_path,
                                  string_escaped(last_label_str.value()),
                                  string_escaped(pt->tp->token_info_get(node.token_begin)->str));
                }
                else {
                  retval +=
                      fmt::format("  \"{}\" [label=\"{}\\n{}\"]\n",
                                  curr_path,
                                  curr_path,
                                  string_escaped(pt->tp->token_info_get(node.token_begin)->str));
                }
              }
              return true;
            });
    SILVA_EXPECT_FWD(std::move(result));
    retval += "}";
    return retval;
  }

  // Object-oriented interface /////////////////////////////////////////////////////////////////////

  fern_item_t::fern_item_t() : value(none) {}

  void fern_t::push_back(fern_labeled_item_t&& li)
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
    string_t operator()(const string_t& arg) { return fmt::format("'{}'", arg); }
    string_t operator()(const double arg) { return fmt::format("{}", arg); }
    string_t operator()(const unique_ptr_t<fern_t>& arg) { return arg->to_string(indent + 2); }
  };

  string_t fern_t::to_string(const int indent) const
  {
    const index_t n = items.size();
    string_t retval;
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
        retval += fmt::format("\n{:{}}", "", indent + 2);
        if (used_labels[i].has_value()) {
          retval += fmt::format("'{}' : ", used_labels[i].value());
        }
        retval += std::visit(to_str_visitor{indent}, items[i].value);
      }
      retval += fmt::format("\n{:{}}]", "", indent);
    }
    return retval;
  }

  namespace impl {
    void
    fern_t__to_graphviz__fern(string_t& retval, const fern_t& fern, const string_view_t prefix);

    void fern_t__to_graphviz__item(string_t& retval,
                                   const optional_t<string_t>& label,
                                   const fern_item_t& item,
                                   const string_view_t parent_name,
                                   const string_view_t item_name)
    {
      retval += fmt::format("  \"{}\" -> \"{}\"\n", parent_name, item_name);
      string_t label_str = label.has_value() ? fmt::format("\\n['{}']", label.value()) : "";
      struct visitor {
        string_t& retval;
        const string_view_t label_str;
        const fern_item_t& item;
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
          retval += fmt::format("  \"{}\" [label=\"{}{}\\n'{}'\"]\n",
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
      syntax_ward_ptr_t swp          = parse_tree->tp->swp;

      token_id_t tt_none  = *swp->token_id("none");
      token_id_t tt_true  = *swp->token_id("true");
      token_id_t tt_false = *swp->token_id("false");

      name_id_t fni_fern     = swp->name_id_of("Fern");
      name_id_t fni_lbl_item = swp->name_id_of(fni_fern, "LabeledItem");
      name_id_t fni_label    = swp->name_id_of(fni_fern, "Label");
      name_id_t fni_value    = swp->name_id_of(fni_fern, "Value");

      expected_t<fern_labeled_item_t> labeled_item(const parse_tree_span_t pts)
      {
        const auto& labeled_item = pts[0];
        SILVA_EXPECT(labeled_item.rule_name == fni_lbl_item, MINOR);
        fern_labeled_item_t retval;
        for (const auto [child_node_index, child_index]: pts.children_range()) {
          const auto& node = pts[child_node_index];
          if (labeled_item.num_children == 2 && child_index == 0) {
            SILVA_EXPECT(node.rule_name == fni_label, MINOR);
            retval.label = string_t{SILVA_EXPECT_FWD(
                parse_tree->tp->token_info_get(node.token_begin)->string_as_plain_contained(),
                MAJOR)};
          }
          else if (node.rule_name == fni_fern) {
            fern_t sub_fern   = SILVA_EXPECT_FWD(fern(pts.sub_tree_span_at(child_node_index)));
            retval.item.value = std::make_unique<fern_t>(std::move(sub_fern));
          }
          else if (node.rule_name == fni_value) {
            SILVA_EXPECT(node.num_children == 0, MINOR, "Value node must have zero children");
            const token_id_t token_id = parse_tree->tp->tokens[node.token_begin];
            const auto* token_data    = parse_tree->tp->token_info_get(node.token_begin);
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
              retval.item.value =
                  string_t{SILVA_EXPECT_FWD(token_data->string_as_plain_contained(), MAJOR)};
            }
            else if (token_data->category == NUMBER) {
              retval.item.value = SILVA_EXPECT_FWD(token_data->number_as_double(), MAJOR);
            }
            else {
              SILVA_EXPECT(false, MINOR, "Unknown item '{}'", token_data->str);
            }
          }
        }
        return retval;
      }

      expected_t<fern_t> fern(const parse_tree_span_t pts)
      {
        SILVA_EXPECT(pts[0].rule_name == fni_fern, MINOR);
        fern_t retval;
        for (const auto [child_node_index, child_index]: pts.children_range()) {
          fern_labeled_item_t li =
              SILVA_EXPECT_FWD(labeled_item(pts.sub_tree_span_at(child_node_index)));
          retval.push_back(std::move(li));
        }
        return retval;
      }
    };
  }

  expected_t<fern_t> fern_create(const parse_tree_t* parse_tree, const index_t start_node)
  {
    impl::fern_nursery_t fern_nursery{.parse_tree = parse_tree};
    return fern_nursery.fern(parse_tree->span());
  }
}
