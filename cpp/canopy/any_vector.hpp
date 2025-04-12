#pragma once

#include "bit.hpp"
#include "hash.hpp"
#include "iterator_facade.hpp"
#include "memory.hpp"
#include "misc.hpp"

#include <type_traits>
#include <utility>

namespace silva {

  class any_vector_index_t {
    template<typename... Fs>
    friend struct any_vector_t;
    index_t byte_index = 0;
    explicit any_vector_index_t(index_t byte_index) : byte_index(byte_index) {}

   public:
    any_vector_index_t() = default;

    friend auto operator<=>(const any_vector_index_t&, const any_vector_index_t&) = default;
    friend hash_value_t hash_impl(const any_vector_index_t& x);
  };

  // Each item in an any_vector_t consists of 8 byte aligned
  //  * 4 + 4 bytes: total size of the item (including these two fields)
  //    and the type_data_t index of the contained type.
  //  * in-place constructed payload.

  template<typename... Fs>
  class any_vector_t {
    index_t _chunk_byte_capacity = 0;

    vector_t<tuple_t<decltype(Fs{}.template any_function<int>())...>> _type_funcs;
    hashmap_t<std::type_index, index_t> _type_to_index;
    template<typename T>
    index_t type_index_of();

    byte_t* _buffer        = nullptr;
    index_t _byte_size     = 0;
    index_t _byte_capacity = 0;
    index_t _size          = 0;

   public:
    constexpr static inline index_t alignment = 8;

    static_assert((customization_point_c<Fs> && ...));
    static_assert((std::same_as<move_ctor_t, Fs> || ...));
    static_assert((std::same_as<dtor_t, Fs> || ...));

    any_vector_t(index_t chunk_byte_capacity = 4 * 1'024);

    any_vector_t(any_vector_t&&);
    any_vector_t& operator=(any_vector_t&&);

    any_vector_t(const any_vector_t&)            = delete;
    any_vector_t& operator=(const any_vector_t&) = delete;

    ~any_vector_t();

    index_t size() const;
    index_t byte_size() const;
    index_t byte_capacity() const;

    void clear();

    void resize_down_to(any_vector_index_t);

    void reserve_bytes(index_t);

    template<typename T>
    any_vector_index_t push_back(T&&);

    template<typename T, typename... Args>
    any_vector_index_t emplace_back(Args&&...);

    template<typename F, typename... Args>
    auto apply(this auto&&, any_vector_index_t, F, Args&&...);

    struct index_iter_t : public iterator_facade_t {
      const any_vector_t* any_vector = nullptr;
      any_vector_index_t avi;

      any_vector_index_t dereference() const;
      void increment();
      friend auto operator<=>(const index_iter_t&, const index_iter_t&) = default;
    };
    auto index_range() const;
    index_iter_t index_iter_at(any_vector_index_t) const;
    index_iter_t index_iter_begin() const;
    index_iter_t index_iter_end() const;
  };
}

// IMPLEMENTATION

namespace silva {
  inline hash_value_t hash_impl(const any_vector_index_t& x)
  {
    return hash(x.byte_index);
  }

  template<typename... Fs>
  template<typename T>
  index_t any_vector_t<Fs...>::type_index_of()
  {
    static_assert(alignof(T) <= alignment);
    const std::type_index ti{typeid(T)};
    const auto [it, inserted] = _type_to_index.emplace(ti, _type_funcs.size());
    if (inserted) {
      _type_funcs.push_back(std::make_tuple(Fs{}.template any_function<T>()...));
    }
    return it->second;
  }

  template<typename... Fs>
  any_vector_t<Fs...>::any_vector_t(const index_t chunk_byte_capacity)
    : _chunk_byte_capacity(chunk_byte_capacity)
  {
  }

  template<typename... Fs>
  any_vector_t<Fs...>::any_vector_t(any_vector_t&& other)
    : _chunk_byte_capacity(other._chunk_byte_capacity)
    , _type_funcs(std::move(other._type_funcs))
    , _type_to_index(std::move(other._type_to_index))
    , _buffer(std::exchange(other._buffer, nullptr))
    , _byte_size(std::exchange(other._byte_size, 0))
    , _byte_capacity(std::exchange(other._byte_capacity, 0))
    , _size(std::exchange(other._size, 0))
  {
  }

  template<typename... Fs>
  any_vector_t<Fs...>& any_vector_t<Fs...>::operator=(any_vector_t&& other)
  {
    if (this != &other) {
      clear();
      _chunk_byte_capacity = other._chunk_byte_capacity;
      _type_funcs          = std::move(other._type_funcs);
      _type_to_index       = std::move(other._type_to_index);
      _buffer              = std::exchange(other._buffer, nullptr);
      _byte_size           = std::exchange(other._byte_size, 0);
      _byte_capacity       = std::exchange(other._byte_capacity, 0);
      _size                = std::exchange(other._size, 0);
    }
    return *this;
  }

  template<typename... Fs>
  any_vector_t<Fs...>::~any_vector_t()
  {
    clear();
  }

  template<typename... Fs>
  index_t any_vector_t<Fs...>::size() const
  {
    return _size;
  }

  template<typename... Fs>
  index_t any_vector_t<Fs...>::byte_size() const
  {
    return _byte_size;
  }

  template<typename... Fs>
  index_t any_vector_t<Fs...>::byte_capacity() const
  {
    return _byte_capacity;
  }

  template<typename... Fs>
  void any_vector_t<Fs...>::clear()
  {
    resize_down_to(any_vector_index_t{0});
    delete[] _buffer;
    _buffer        = nullptr;
    _byte_size     = 0;
    _byte_capacity = 0;
    _size          = 0;
  }

  template<typename... Fs>
  void any_vector_t<Fs...>::resize_down_to(const any_vector_index_t avi)
  {
    const auto end = index_iter_end();
    index_t count  = 0;
    for (auto it = index_iter_at(avi); it != end; ++it) {
      apply(*it, dtor);
      count += 1;
    }
    _byte_size = avi.byte_index;
    _size -= count;
  }

  template<typename... Fs>
  void any_vector_t<Fs...>::reserve_bytes(const index_t min_bytes)
  {
    const index_t new_byte_capacity =
        std::max(_byte_capacity, chunked_size(min_bytes, _chunk_byte_capacity));
    if (new_byte_capacity > _byte_capacity) {
      byte_t* new_buffer = new byte_t[new_byte_capacity];
      for (const auto avi: index_range()) {
        const uint32_t curr_size       = bit_cast_ptr<uint32_t>(_buffer + avi.byte_index);
        const uint32_t curr_type_index = bit_cast_ptr<uint32_t>(_buffer + avi.byte_index + 4);
        bit_write_at<index_t>(new_buffer + avi.byte_index, curr_size);
        bit_write_at<index_t>(new_buffer + avi.byte_index + 4, curr_type_index);
        apply(avi, move_ctor, new_buffer + avi.byte_index + 8);
      }
      const index_t old_size      = _size;
      const index_t old_byte_size = _byte_size;
      clear();
      _buffer        = new_buffer;
      _byte_size     = old_byte_size;
      _byte_capacity = new_byte_capacity;
      _size          = old_size;
    }
  }

  template<typename... Fs>
  template<typename T>
  any_vector_index_t any_vector_t<Fs...>::push_back(T&& x)
  {
    return emplace_back<std::remove_cvref_t<T>>(std::forward<T>(x));
  }

  template<typename... Fs>
  template<typename T, typename... Args>
  any_vector_index_t any_vector_t<Fs...>::emplace_back(Args&&... args)
  {
    const index_t type_index             = type_index_of<T>();
    constexpr index_t required_byte_size = 4 + 4 + chunked_size(sizeof(T), alignment);
    static_assert(required_byte_size % alignment == 0);
    reserve_bytes(_byte_size + required_byte_size);
    bit_write_at<index_t>(_buffer + _byte_size, required_byte_size);
    bit_write_at<index_t>(_buffer + _byte_size + 4, type_index);
    std::construct_at<T>(reinterpret_cast<T*>(_buffer + _byte_size + 8),
                         std::forward<Args>(args)...);
    const any_vector_index_t retval{_byte_size};
    _byte_size += required_byte_size;
    _size += 1;
    return retval;
  }

  template<typename... Fs>
  template<typename F, typename... Args>
  auto any_vector_t<Fs...>::apply(this auto&& self, const any_vector_index_t avi, F, Args&&... args)
  {
    static_assert((std::same_as<Fs, F> || ...));
    constexpr std::size_t F_index = []<std::size_t... Is>(std::index_sequence<Is...>) {
      return ((std::same_as<Fs, F> ? Is : 0) + ...);
    }(std::index_sequence_for<Fs...>{});
    const uint32_t curr_size       = bit_cast_ptr<uint32_t>(self._buffer + avi.byte_index);
    const uint32_t curr_type_index = bit_cast_ptr<uint32_t>(self._buffer + avi.byte_index + 4);
    const auto& type_func          = self._type_funcs[curr_type_index];
    return std::get<F_index>(type_func)(self._buffer + avi.byte_index + 8,
                                        std::forward<Args>(args)...);
  }

  template<typename... Fs>
  any_vector_index_t any_vector_t<Fs...>::index_iter_t::dereference() const
  {
    return avi;
  }

  template<typename... Fs>
  void any_vector_t<Fs...>::index_iter_t::increment()
  {
    const uint32_t curr_size = bit_cast_ptr<uint32_t>(any_vector->_buffer + avi.byte_index);
    avi.byte_index += curr_size;
  }

  template<typename... Fs>
  auto any_vector_t<Fs...>::index_range() const
  {
    return std::ranges::subrange<index_iter_t, index_iter_t>(index_iter_begin(), index_iter_end());
  }

  template<typename... Fs>
  any_vector_t<Fs...>::index_iter_t
  any_vector_t<Fs...>::index_iter_at(const any_vector_index_t avi) const
  {
    return index_iter_t{
        .any_vector = this,
        .avi        = avi,
    };
  }

  template<typename... Fs>
  any_vector_t<Fs...>::index_iter_t any_vector_t<Fs...>::index_iter_begin() const
  {
    return index_iter_t{
        .any_vector = this,
        .avi        = any_vector_index_t{0},
    };
  }

  template<typename... Fs>
  any_vector_t<Fs...>::index_iter_t any_vector_t<Fs...>::index_iter_end() const
  {
    return index_iter_t{
        .any_vector = this,
        .avi        = any_vector_index_t{_byte_size},
    };
  }
}
