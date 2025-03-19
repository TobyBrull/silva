#pragma once

#include "canopy/types.hpp"

#include <concepts>

namespace silva {
  template<typename Iter>
  concept iterator_facade_wrappable_c = requires(Iter i, Iter j, index_t n) {
    { i.dereference() };
    { i.increment() };
    { i.decrement() };
    { i.advance(n) };
    { i.distance_to(j) };
  };

  struct iterator_facade_t {
    template<typename Self>
    decltype(auto) operator*(this Self&& self)
    {
      return std::forward<Self>(self).dereference();
    }

    // Prefix
    template<typename Self>
    decltype(auto) operator++(this Self&& self)
    {
      self.increment();
      return std::forward<Self>(self);
    }
    template<typename Self>
    decltype(auto) operator--(this Self&& self)
    {
      self.decrement();
      return std::forward<Self>(self);
    }

    // Postfix
    template<typename Self>
    auto operator++(this Self&& self, int)
    {
      auto retval = self;
      std::forward<Self>(self).increment();
      return retval;
    }
    template<typename Self>
    auto operator--(this Self&& self, int)
    {
      auto retval = self;
      std::forward<Self>(self).decrement();
      return retval;
    }

    template<typename Self>
    decltype(auto) operator+=(this Self&& self, const index_t n)
    {
      self.advance(n);
      return std::forward<Self>(self);
    }
    template<typename Self>
    decltype(auto) operator-=(this Self&& self, const index_t n)
    {
      self.advance(-n);
      return std::forward<Self>(self);
    }

    template<typename Self>
    auto operator+(this Self&& self, const index_t n)
    {
      auto retval = std::forward<Self>(self);
      retval.advance(n);
      return retval;
    }
    template<typename Self>
    auto operator-(this Self&& self, const index_t n)
    {
      auto retval = std::forward<Self>(self);
      retval.advance(-n);
      return retval;
    }

    template<typename Left, typename Right>
      requires std::derived_from<std::remove_cvref_t<Left>, iterator_facade_t> &&
        std::derived_from<std::remove_cvref_t<Right>, iterator_facade_t>
    friend index_t operator-(Left&& left, Right&& right)
    {
      return std::forward<Right>(right).distance_to(std::forward<Left>(left));
    }

    friend auto operator<=>(const iterator_facade_t&, const iterator_facade_t&) = default;
  };
}
