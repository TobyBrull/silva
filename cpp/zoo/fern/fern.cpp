#include "fern.hpp"

#include "canopy/error.hpp"
#include "canopy/expected.hpp"
#include "canopy/string_convert.hpp"

#include "syntax/parse_tree_nursery.hpp"
#include "syntax/syntax_farm.hpp"

namespace silva::fern {

  expected_t<string_t> to_string(const parse_tree_t* pt, const index_t start_node)
  {
    syntax_farm_ptr_t sfp       = pt->tp->sfp;
    const name_id_t ni_fern     = sfp->name_id_of("Fern");
    const name_id_t ni_lbl_item = sfp->name_id_of(ni_fern, "LabeledItem");
    const name_id_t ni_label    = sfp->name_id_of(ni_fern, "Label");
    const name_id_t ni_value    = sfp->name_id_of(ni_fern, "Value");

    SILVA_EXPECT(pt->nodes[start_node].rule_name == ni_fern, ASSERT);
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
                        if (node.rule_name == ni_fern) {
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
                        else if (node.rule_name == ni_lbl_item) {
                          if (is_on_entry(event)) {
                            retval_newline();
                          }
                        }
                        else if (node.rule_name == ni_label) {
                          retval += pt->tp->token_info_get(node.token_begin)->str;
                          retval += " : ";
                        }
                        else if (node.rule_name == ni_value) {
                          retval += pt->tp->token_info_get(node.token_begin)->str;
                        }
                        return true;
                      });
    SILVA_EXPECT_FWD(std::move(result));
    return retval;
  }

  expected_t<string_t> to_graphviz(const parse_tree_t* pt, const index_t start_node)
  {
    syntax_farm_ptr_t sfp       = pt->tp->sfp;
    const name_id_t ni_fern     = sfp->name_id_of("Fern");
    const name_id_t ni_lbl_item = sfp->name_id_of(ni_fern, "LabeledItem");
    const name_id_t ni_label    = sfp->name_id_of(ni_fern, "Label");
    const name_id_t ni_value    = sfp->name_id_of(ni_fern, "Value");

    SILVA_EXPECT(pt->nodes[start_node].rule_name == ni_fern, ASSERT);
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
              if (node.rule_name == ni_lbl_item) {
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
              else if (node.rule_name == ni_label) {
                last_label_str = pt->tp->token_info_get(node.token_begin)->str;
              }
              else if (node.rule_name == ni_value) {
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
      array_t<optional_t<string_view_t>> used_labels;
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
      array_t<optional_t<string_t>> labels(n);
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
      syntax_farm_ptr_t sfp          = parse_tree->tp->sfp;

      token_id_t ti_none  = sfp->token_id("none");
      token_id_t ti_true  = sfp->token_id("true");
      token_id_t ti_false = sfp->token_id("false");

      token_id_t ti_number = sfp->token_id("number");
      token_id_t ti_string = sfp->token_id("string");

      name_id_t ni_fern     = sfp->name_id_of("Fern");
      name_id_t ni_lbl_item = sfp->name_id_of(ni_fern, "LabeledItem");
      name_id_t ni_label    = sfp->name_id_of(ni_fern, "Label");
      name_id_t ni_value    = sfp->name_id_of(ni_fern, "Value");

      expected_t<fern_labeled_item_t> labeled_item(const parse_tree_span_t pts)
      {
        const auto& labeled_item = pts[0];
        SILVA_EXPECT(labeled_item.rule_name == ni_lbl_item, MINOR);
        fern_labeled_item_t retval;
        for (const auto [child_node_index, child_index]: pts.children_range()) {
          const auto& node = pts[child_node_index];
          if (labeled_item.num_children == 2 && child_index == 0) {
            SILVA_EXPECT(node.rule_name == ni_label, MINOR);
            retval.label = string_t{SILVA_EXPECT_FWD(
                parse_tree->tp->token_info_get(node.token_begin)->string_as_plain_contained(),
                MAJOR)};
          }
          else if (node.rule_name == ni_fern) {
            fern_t sub_fern   = SILVA_EXPECT_FWD(fern(pts.sub_tree_span_at(child_node_index)));
            retval.item.value = std::make_unique<fern_t>(std::move(sub_fern));
          }
          else if (node.rule_name == ni_value) {
            SILVA_EXPECT(node.num_children == 0, MINOR, "Value node must have zero children");
            const token_id_t token_id  = parse_tree->tp->tokens[node.token_begin];
            const token_id_t token_cat = parse_tree->tp->categories[node.token_begin];
            const auto* token_data     = parse_tree->tp->token_info_get(node.token_begin);
            if (token_id == ti_none) {
              retval.item.value = none;
            }
            else if (token_id == ti_true) {
              retval.item.value = true;
            }
            else if (token_id == ti_false) {
              retval.item.value = false;
            }
            else if (token_cat == ti_string) {
              retval.item.value =
                  string_t{SILVA_EXPECT_FWD(token_data->string_as_plain_contained(), MAJOR)};
            }
            else if (token_cat == ti_number) {
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
        SILVA_EXPECT(pts[0].rule_name == ni_fern, MINOR);
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

  expected_t<fern_t> create(const parse_tree_t* parse_tree, const index_t start_node)
  {
    impl::fern_nursery_t fern_nursery{.parse_tree = parse_tree};
    return fern_nursery.fern(parse_tree->span());
  }
}
