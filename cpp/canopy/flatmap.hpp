#pragma once

#include "canopy/types.hpp"

namespace silva {
  template<typename Key, typename Value>
  struct flatmap_t {
    vector_t<Key> keys;
    vector_t<Value> values;
  };
}
