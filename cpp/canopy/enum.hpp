#pragma once

#include "hash.hpp"

#include <rfl/enums.hpp>

namespace silva {
  template<typename Enum>
    requires std::is_enum_v<Enum>
  hashmap_t<Enum, string_t> enum_hashmap_to_string();

  template<typename Enum>
    requires std::is_enum_v<Enum>
  hashmap_t<string_t, Enum> enum_hashmap_from_string();
}

// IMPLEMENTATION

namespace silva {
  template<typename Enum>
    requires std::is_enum_v<Enum>
  hashmap_t<Enum, string_t> enum_hashmap_to_string()
  {
    hashmap_t<Enum, string_t> retval;
    const auto cats = rfl::get_enumerators<Enum>();
    cats.apply([&](const auto& field) { retval[field.value()] = field.name(); });
    return retval;
  }

  template<typename Enum>
    requires std::is_enum_v<Enum>
  hashmap_t<string_t, Enum> enum_hashmap_from_string()
  {
    hashmap_t<string_t, Enum> retval;
    const auto cats = rfl::get_enumerators<Enum>();
    cats.apply([&](const auto& field) { retval[field.name()] = field.value(); });
    return retval;
  }
}
