#pragma once

#include "canopy/types.hpp"

namespace silva {
  template<typename Key, typename Value>
  struct flatmap_t {
    vector_t<Key> keys;
    vector_t<Value> values;

    struct const_iterator_t {
      using difference_type   = std::ptrdiff_t;
      using value_type        = pair_t<const Key&, const Value&>;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::random_access_iterator_tag;

      typename vector_t<Key>::const_iterator keys_iter;
      typename vector_t<Value>::const_iterator values_iter;

      pair_t<const Key&, const Value&> operator*() const { return {*keys_iter, *values_iter}; }

      pair_t<const Key&, const Value&> operator->() const { return {*keys_iter, *values_iter}; }
      bool operator==(const const_iterator_t& other) const
      {
        return keys_iter == other.keys_iter && values_iter == other.values_iter;
      }

      bool operator!=(const const_iterator_t& other) const { return !(*this == other); }

      const_iterator_t& operator++()
      {
        ++keys_iter;
        ++values_iter;
        return *this;
      }
      const_iterator_t operator++(int)
      {
        const_iterator_t temp = *this;
        ++(*this);
        return temp;
      }
      const_iterator_t& operator+=(std::ptrdiff_t n)
      {
        keys_iter += n;
        values_iter += n;
        return *this;
      }

      const_iterator_t& operator--()
      {
        --keys_iter;
        --values_iter;
        return *this;
      }
      const_iterator_t operator--(int)
      {
        const_iterator_t temp = *this;
        --(*this);
        return temp;
      }
      const_iterator_t& operator-=(std::ptrdiff_t n)
      {
        keys_iter -= n;
        values_iter -= n;
        return *this;
      }
    };

    const_iterator_t begin() const { return {keys.begin(), values.begin()}; }
    const_iterator_t end() const { return {keys.end(), values.end()}; }
  };
}
