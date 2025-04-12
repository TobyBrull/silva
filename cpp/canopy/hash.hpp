#pragma once

#include "customization_point.hpp"
#include "variant.hpp"

#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace silva {
  using hash_value_t = std::size_t;

  struct hash_combiner_t {
    hash_value_t value = 0;
    void combine(hash_value_t);
  };

  hash_value_t hash_impl(const string_t&);
  hash_value_t hash_impl(const string_view_t&);

  template<typename T>
    requires std::is_fundamental_v<T> || std::is_enum_v<T>
  hash_value_t hash_impl(T x);

  template<typename T>
    requires std::is_pointer_v<T>
  hash_value_t hash_impl(T x);

  template<typename T, typename U>
  hash_value_t hash_impl(const pair_t<T, U>&);

  template<typename... Ts>
  hash_value_t hash_impl(const tuple_t<Ts...>&);

  template<typename... Ts>
  hash_value_t hash_impl(const variant_t<Ts...>&);

  template<typename... Ts>
  hash_value_t hash_impl(const std::type_index&);

  struct hash_t : public customization_point_t<hash_value_t(const void*)> {
    template<typename T>
    constexpr hash_value_t operator()(const T& x) const;
  };
  inline constexpr hash_t hash;

  template<typename K, typename V>
  using hashmap_t = std::unordered_map<K, V, hash_t>;

  template<typename T>
  using hashset_t = std::unordered_set<T, hash_t>;
}

// IMPLEMENTATION

namespace silva {
  inline void hash_combiner_t::combine(const hash_value_t x)
  {
    value ^= x + 0x9e3779b9 + (value << 6) + (value >> 2);
  }

  template<typename T>
  constexpr hash_value_t hash_t::operator()(const T& x) const
  {
    using silva::hash_impl;
    return hash_impl(x);
  }

  inline hash_value_t hash_impl(const string_t& x)
  {
    return std::hash<string_view_t>{}(x);
  }

  inline hash_value_t hash_impl(const string_view_t& x)
  {
    return std::hash<string_view_t>{}(x);
  }

  template<typename T>
    requires std::is_fundamental_v<T> || std::is_enum_v<T>
  hash_value_t hash_impl(T x)
  {
    return std::hash<T>{}(x);
  }

  template<typename T>
    requires std::is_pointer_v<T>
  hash_value_t hash_impl(T x)
  {
    if constexpr (std::same_as<std::remove_cv_t<std::remove_pointer_t<T>>, char>) {
      return std::hash<string_view_t>{}(string_view_t{x});
    }
    else {
      return hash_value_t(x);
    }
  }

  template<typename T, typename U>
  hash_value_t hash_impl(const pair_t<T, U>& x)
  {
    hash_combiner_t hc;
    hc.combine(hash(x.first));
    hc.combine(hash(x.second));
    return hc.value;
  }

  template<typename Tuple, std::size_t... Is>
  hash_value_t hash_impl_indexed(const Tuple& x, std::index_sequence<Is...>)
  {
    hash_combiner_t hc;
    (hc.combine(hash(std::get<Is>(x))), ...);
    return hc.value;
  }

  template<typename... Ts>
  hash_value_t hash_impl(const tuple_t<Ts...>& x)
  {
    return hash_impl_indexed(x, std::index_sequence_for<Ts...>{});
  }

  template<typename... Ts>
  hash_value_t hash_impl(const variant_t<Ts...>& x)
  {
    hash_combiner_t hc{.value = x.index()};
    hc.combine(std::visit([](const auto& xx) { return hash(xx); }, x));
    return hc.value;
  }

  template<typename... Ts>
  hash_value_t hash_impl(const std::type_index& x)
  {
    return x.hash_code();
  }
}
