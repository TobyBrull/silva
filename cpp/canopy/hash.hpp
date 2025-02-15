#pragma once

#include "types.hpp"

namespace silva {
  using hash_value_t = std::size_t;

  struct hash_combiner_t {
    hash_value_t value = 0;
    void combine(hash_value_t);
  };

  template<typename T>
    requires std::is_integral_v<T> || std::is_enum_v<T>
  hash_value_t hash_impl(T x);

  template<typename T>
    requires std::is_same_v<T, string_t> || std::is_same_v<T, string_view_t> ||
      (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>)
  hash_value_t hash_impl(const T& x);

  template<typename T, typename U>
  hash_value_t hash_impl(const pair_t<T, U>&);

  template<typename... Ts>
  hash_value_t hash_impl(const tuple_t<Ts...>&);

  template<typename... Ts>
  hash_value_t hash_impl(const variant_t<Ts...>&);

  struct hash_t {
    template<typename T>
    constexpr auto operator()(const T& x) const;
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
  constexpr auto hash_t::operator()(const T& x) const
  {
    using silva::hash_impl;
    return hash_impl(x);
  }

  template<typename T>
    requires std::is_integral_v<T> || std::is_enum_v<T>
  hash_value_t hash_impl(T x)
  {
    return static_cast<hash_value_t>(x);
  }

  template<typename T>
    requires std::is_same_v<T, string_t> || std::is_same_v<T, string_view_t> ||
      (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>)
  hash_value_t hash_impl(const T& x)
  {
    return std::hash<string_view_t>{}(x);
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
    (hc.combine(std::get<Is>(x)), ...);
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
}
