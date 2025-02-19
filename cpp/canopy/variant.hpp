#pragma once

#include "assert.hpp"

#include <variant>

namespace silva {
  template<typename... Ts>
  using variant_t = std::variant<Ts...>;

  template<typename T>
  struct is_variant_t : std::false_type {};

  template<typename... Ts>
  struct is_variant_t<variant_t<Ts...>> : std::true_type {};

  template<typename VarLhs, typename VarRhs>
  struct variant_join_impl_t;

  template<typename VarLhs, typename VarRhs>
    requires(is_variant_t<VarLhs>::value && is_variant_t<VarRhs>::value)
  using variant_join_t = typename variant_join_impl_t<VarLhs, VarRhs>::type;

  template<typename T, typename Var>
    requires is_variant_t<Var>::value
  struct variant_has_alternative_t;

  template<typename... Ts>
  struct variant_holds_t {
    template<typename Var>
      requires is_variant_t<Var>::value
    constexpr bool operator()(const Var& x) const;
  };

  // Returns if "var" contains one of the the alternatives specified
  // in variant "TestVar".
  template<typename TestVar, typename... Ts>
    requires is_variant_t<TestVar>::value
  constexpr bool variant_holds(const variant_t<Ts...>&);

  template<typename GetVar, typename... Ts>
    requires is_variant_t<GetVar>::value
  constexpr GetVar variant_get(const variant_t<Ts...>&);
}

// IMPLEMENTATION

namespace silva {
  template<typename... VarLhses, typename... VarRhses>
  struct variant_join_impl_t<variant_t<VarLhses...>, variant_t<VarRhses...>> {
    using type = variant_t<VarLhses..., VarRhses...>;
  };

  template<typename T, typename... Vars>
  struct variant_has_alternative_t<T, variant_t<Vars...>> {
    constexpr static bool value = (std::same_as<T, Vars> || ...);
  };

  template<typename... Ts>
  template<typename Var>
    requires is_variant_t<Var>::value
  constexpr bool variant_holds_t<Ts...>::operator()(const Var& x) const
  {
    return (std::holds_alternative<Ts>(x) || ...);
  }

  namespace impl {
    template<typename TestVar, typename ValueVar>
    struct variant_holds_impl_t;

    template<typename... TestVars, typename... ValueVars>
    struct variant_holds_impl_t<variant_t<TestVars...>, variant_t<ValueVars...>> {
      constexpr bool operator()(const variant_t<ValueVars...>& value_var) const
      {
        return (std::holds_alternative<TestVars>(value_var) || ...);
      }
    };
  }

  template<typename TestVar, typename... Ts>
    requires is_variant_t<TestVar>::value
  constexpr bool variant_holds(const variant_t<Ts...>& orig_var)
  {
    return impl::variant_holds_impl_t<TestVar, variant_t<Ts...>>{}(orig_var);
  }

  template<typename GetVar, typename... Ts>
    requires is_variant_t<GetVar>::value
  constexpr GetVar variant_get(const variant_t<Ts...>& orig_var)
  {
    return std::visit(
        []<typename T>(const T& arg) -> GetVar {
          if constexpr (variant_has_alternative_t<T, GetVar>::value) {
            return arg;
          }
          else {
            throw std::bad_variant_access{};
          }
        },
        orig_var);
  }
}
