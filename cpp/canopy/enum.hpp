#pragma once

#include "hash.hpp"

#include <rfl/enums.hpp>

namespace silva {
  template<typename Enum>
    requires std::is_enum_v<Enum>
  const hash_map_t<Enum, string_t>& enum_hashmap_to_string();

  template<typename Enum>
    requires std::is_enum_v<Enum>
  const hash_map_t<string_t, Enum>& enum_hashmap_from_string();
}

// IMPLEMENTATION

namespace silva {
  template<typename Enum>
    requires std::is_enum_v<Enum>
  const hash_map_t<Enum, string_t>& enum_hashmap_to_string()
  {
    const static auto singleton = [] {
      hash_map_t<Enum, string_t> retval;
      const auto cats = rfl::get_enumerators<Enum>();
      cats.apply([&](const auto& field) { retval[field.value()] = field.name(); });
      return retval;
    }();
    return singleton;
  }

  template<typename Enum>
    requires std::is_enum_v<Enum>
  const hash_map_t<string_t, Enum>& enum_hashmap_from_string()
  {
    const static auto singleton = [] {
      hash_map_t<string_t, Enum> retval;
      const auto cats = rfl::get_enumerators<Enum>();
      cats.apply([&](const auto& field) { retval[string_t{field.name()}] = field.value(); });
      return retval;
    }();
    return singleton;
  }
}
