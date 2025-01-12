#include "shunting_yard.hpp"
#include "canopy/expected.hpp"
#include <variant>

namespace silva {

  bool shunting_yard_t::has_operator(const token_id_t token_id) const
  {
    return mapped_levels.contains(token_id);
  }

  optional_t<shunting_yard_t::level_index_t>&
  shunting_yard_t::slot_for(const token_id_t token_id, const level_type_t level_type)
  {
    const auto it = mapped_levels.find(token_id);
    SILVA_ASSERT(it != mapped_levels.end());
    mapped_levels_t& mlvl = it->second;
    using enum level_type_t;
    switch (level_type) {
      case NONE:
        SILVA_ASSERT(false);
      case BINARY_LEFT_TO_RIGHT:
        return mlvl.postfix_or_binary;
      case BINARY_RIGHT_TO_LEFT:
        return mlvl.postfix_or_binary;
      case PREFIX:
        return mlvl.prefix;
      case POSTFIX:
        return mlvl.postfix_or_binary;
    }
  }

  shunting_yard_t::level_index_t shunting_yard_t::add_level(const level_type_t type)
  {
    const level_index_t retval = levels.size();
    levels.emplace_back();
    levels.back().type = type;
    return retval;
  }

  void shunting_yard_t::add_operator(const level_index_t level_idx, const token_id_t token_id)
  {
    SILVA_ASSERT(level_idx < levels.size());
    level_t& lvl = levels[level_idx];
    mapped_levels[token_id];
    optional_t<level_index_t>& slot = slot_for(token_id, lvl.type);
    SILVA_ASSERT(slot.has_value() == false);
    slot = level_idx;
    lvl.token_ids.push_back(token_id);
  }

  expected_t<void> shunting_yard_run_t::push_back(Expression expr)
  {
    using enum shunting_yard_run_state_t;
    if (state == PRE_EXPR) {
      items.push_back(std::move(expr));
      state = POST_EXPR;
    }
    else if (state == POST_EXPR) {
      SILVA_EXPECT(false,
                   MINOR,
                   "Expression not allowed here. Expected postfix or binary operator");
    }
    else {
      SILVA_EXPECT(false, ASSERT);
    }
    return {};
  }

  expected_t<void> shunting_yard_run_t::push_back(const token_id_t token_id)
  {
    const auto* sy = shunting_yard;

    const auto it = shunting_yard->mapped_levels.find(token_id);
    SILVA_EXPECT(it != shunting_yard->mapped_levels.end(), MAJOR, "Unknown operator {}", token_id);
    const shunting_yard_t::mapped_levels_t& mlvl = it->second;

    using enum shunting_yard_run_state_t;
    using enum shunting_yard_t::level_type_t;
    if (state == PRE_EXPR) {
      if (mlvl.prefix.has_value()) {
        items.push_back(op_t{
            .token_id  = token_id,
            .is_prefix = true,
        });
      }
      else {
        SILVA_EXPECT(mlvl.prefix.has_value(),
                     MINOR,
                     "Expected prefix operator, but operator {} is not a prefix operator",
                     token_id);
      }
    }
    else if (state == POST_EXPR) {
      if (mlvl.postfix_or_binary.has_value()) {
        items.push_back(op_t{
            .token_id  = token_id,
            .is_prefix = false,
        });
        const auto level_type = shunting_yard->levels[mlvl.postfix_or_binary.value()].type;
        if (level_type == BINARY_LEFT_TO_RIGHT || level_type == BINARY_RIGHT_TO_LEFT) {
          state = PRE_EXPR;
        }
      }
      else {
        SILVA_EXPECT(false,
                     MINOR,
                     "Expected binary or postfix operator, but operator {} is neither",
                     token_id);
      }
    }
    else {
      SILVA_EXPECT(false, ASSERT);
    }
    return {};
  }

  expected_t<Expression> shunting_yard_run_t::finish()
  {
    for (shunting_yard_t::level_index_t level_idx = 0; level_idx < shunting_yard->levels.size();
         ++level_idx) {
      while (true) {
        const bool did_some = SILVA_EXPECT_FWD(apply_next(level_idx));
        if (!did_some) {
          break;
        }
      }
    }
    SILVA_EXPECT(items.size() == 1 && std::holds_alternative<Expression>(items.front()), ASSERT);
    return std::get<Expression>(items.front());
  }

  namespace impl {
    template<typename T>
    bool contains(const vector_t<T>& vec, const T& x)
    {
      return std::ranges::find(vec, x) != vec.end();
    }
  }

  expected_t<bool> shunting_yard_run_t::apply_next(const shunting_yard_t::level_index_t level_idx)
  {
    const auto& lvl            = shunting_yard->levels[level_idx];
    const auto is_match_prefix = [&lvl](const item_t& item) {
      const op_t* op = std::get_if<op_t>(&item);
      return op && op->is_prefix && impl::contains(lvl.token_ids, op->token_id);
    };
    const auto is_match_no_prefix = [&lvl](const item_t& item) {
      const op_t* op = std::get_if<op_t>(&item);
      return op && op->is_prefix == false && impl::contains(lvl.token_ids, op->token_id);
    };
    using enum shunting_yard_t::level_type_t;
    if (lvl.type == PREFIX) {
      for (index_t ip1 = items.size(); ip1 > 0; --ip1) {
        const index_t i = ip1 - 1;
        if (is_match_prefix(items[i])) {
          SILVA_EXPECT(ip1 < items.size() && std::holds_alternative<Expression>(items[ip1]),
                       MINOR,
                       "Prefix without following expression");
          const Expression& expr = std::get<Expression>(items[ip1]);
          auto new_expr          = callback(span_t<const Expression>(&expr, 1),
                                   std::get<op_t>(items[i]).token_id,
                                   level_idx);
          items.erase(items.begin() + i);
          items[i] = std::move(new_expr);
          return true;
        }
      }
      return false;
    }
    else if (lvl.type == POSTFIX) {
      for (index_t i = 0; i < items.size(); ++i) {
        if (is_match_no_prefix(items[i])) {
          SILVA_EXPECT(0 < i && std::holds_alternative<Expression>(items[i - 1]),
                       MINOR,
                       "Postfix without following expression");
          const Expression& expr = std::get<Expression>(items[i - 1]);
          auto new_expr          = callback(span_t<const Expression>(&expr, 1),
                                   std::get<op_t>(items[i]).token_id,
                                   level_idx);
          items.erase(items.begin() + i - 1);
          items[i - 1] = std::move(new_expr);
          return true;
        }
      }
      return false;
    }
    else if (lvl.type == BINARY_LEFT_TO_RIGHT) {
      for (index_t i = 0; i < items.size(); ++i) {
        if (is_match_no_prefix(items[i])) {
          SILVA_EXPECT(0 < i && i + 1 < items.size() &&
                           std::holds_alternative<Expression>(items[i - 1]) &&
                           std::holds_alternative<Expression>(items[i + 1]),
                       MINOR,
                       "Binary op without surrounding expressions");
          const index_t start_pos = i - 1;
          vector_t<Expression> exprs;
          const Expression& expr = std::get<Expression>(items[i - 1]);
          exprs.push_back(expr);
          while (i + 1 < items.size() && is_match_no_prefix(items[i]) &&
                 std::holds_alternative<Expression>(items[i + 1])) {
            exprs.push_back(std::get<Expression>(items[i + 1]));
            i += 2;
          }
          auto new_expr = callback(span_t<const Expression>(exprs.data(), exprs.size()),
                                   std::get<op_t>(items[start_pos + 1]).token_id,
                                   level_idx);
          items.erase(items.begin() + start_pos, items.begin() + i);
          items.insert(items.begin() + start_pos, std::move(new_expr));
          return true;
        }
      }
      return false;
    }
    else if (lvl.type == BINARY_RIGHT_TO_LEFT) {
      SILVA_EXPECT(false, FATAL, "Not implemented yet");
    }
    else {
      SILVA_EXPECT(false, ASSERT, "Unknown level");
    }
  }
}
