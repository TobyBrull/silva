#pragma once

#include "canopy/byte_sink.hpp"
#include "canopy/sprite.hpp"

namespace silva {
  template<typename T>
  class object_ref_t;

  template<typename T>
  class object_pool_t : public menhir_t {
    struct object_data_t {
      index_t ref_count = 0;
      optional_t<T> obj;
      index_t next_free = -1;
    };
    vector_t<object_data_t> object_datas;
    index_t next_free = -1;

    void free(index_t);
    friend class object_ref_t<T>;

   public:
    template<typename... Args>
    object_ref_t<T> make(Args&&...);

    template<typename F>
    void for_each_object(F f)
    {
      for (auto& x: object_datas) {
        if (x.obj.has_value()) {
          f(x.obj.value());
        }
      }
    }

    string_t to_string() const;
  };
  template<typename T>
  using object_pool_ptr_t = ptr_t<object_pool_t<T>>;

  template<typename T>
  class object_ref_t {
    object_pool_ptr_t<T> pool;
    index_t idx = -1;

    friend class object_pool_t<T>;
    object_ref_t(object_pool_ptr_t<T>, index_t);

   public:
    object_ref_t() = default;
    ~object_ref_t();

    object_ref_t(object_ref_t&&);
    object_ref_t(const object_ref_t&);
    object_ref_t& operator=(object_ref_t&&);
    object_ref_t& operator=(const object_ref_t&);

    bool is_nullptr() const;
    void clear();

    T* operator->() const;
    T& operator*() const;

    friend void pretty_write_impl(byte_sink_t* stream, const object_ref_t& x)
    {
      return pretty_write_impl(stream, *x);
    }
    friend std::ostream& operator<<(std::ostream& os, const object_ref_t& x) { return os << *x; }
  };
}

// IMPLEMENTATION

namespace silva {

  // object_pool_t

  template<typename T>
  void object_pool_t<T>::free(const index_t idx)
  {
    SILVA_ASSERT(object_datas[idx].ref_count == 0);
    object_datas[idx].obj.reset();
    object_datas[idx].next_free = next_free;
    next_free                   = idx;
  }

  template<typename T>
  template<typename... Args>
  object_ref_t<T> object_pool_t<T>::make(Args&&... args)
  {
    index_t idx{};
    if (next_free == -1) {
      idx = object_datas.size();
      object_datas.emplace_back();
      object_datas.back().obj.emplace(std::forward<Args>(args)...);
    }
    else {
      idx       = next_free;
      next_free = object_datas[idx].next_free;
      object_datas[idx].obj.emplace(std::forward<Args>(args)...);
    }
    return object_ref_t<T>{ptr(), idx};
  }

  template<typename T>
  string_t object_pool_t<T>::to_string() const
  {
    string_t retval;
    retval += fmt::format("object_pool_t with {} objects\n", object_datas.size());
    for (const auto& od: object_datas) {
      if (od.obj.has_value()) {
        retval += fmt::format("  - {} {}\n", od.ref_count, od.obj.value_or(T{}));
      }
      else {
        retval += fmt::format("  - tombstone\n");
      }
    }
    return retval;
  }

  // object_ref_t

  template<typename T>
  object_ref_t<T>::object_ref_t(object_pool_ptr_t<T> arg_pool, const index_t idx)
    : pool(std::move(arg_pool)), idx(idx)
  {
    pool->object_datas[idx].ref_count += 1;
  }

  template<typename T>
  object_ref_t<T>::~object_ref_t()
  {
    clear();
  }

  template<typename T>
  object_ref_t<T>::object_ref_t(object_ref_t&& other)
    : pool(std::move(other.pool)), idx(std::exchange(other.idx, -1))
  {
  }
  template<typename T>
  object_ref_t<T>::object_ref_t(const object_ref_t& other) : pool(other.pool), idx(other.idx)
  {
    if (!pool.is_nullptr()) {
      pool->object_datas[idx].ref_count += 1;
    }
  }
  template<typename T>
  object_ref_t<T>& object_ref_t<T>::operator=(object_ref_t&& other)
  {
    if (this != &other) {
      clear();
      pool = std::move(other.pool);
      idx  = std::exchange(other.idx, -1);
    }
    return *this;
  }
  template<typename T>
  object_ref_t<T>& object_ref_t<T>::operator=(const object_ref_t& other)
  {
    if (this != &other) {
      clear();
      pool = other.pool;
      idx  = other.idx;
      if (!pool.is_nullptr()) {
        pool->object_datas[idx].ref_count += 1;
      }
    }
    return *this;
  }

  template<typename T>
  bool object_ref_t<T>::is_nullptr() const
  {
    return pool.is_nullptr();
  }
  template<typename T>
  void object_ref_t<T>::clear()
  {
    if (!pool.is_nullptr()) {
      pool->object_datas[idx].ref_count -= 1;
      if (pool->object_datas[idx].ref_count == 0) {
        pool->free(idx);
      }
      pool.clear();
      idx = -1;
    }
  }

  template<typename T>
  T* object_ref_t<T>::operator->() const
  {
    SILVA_ASSERT(idx >= 0);
    return &(pool->object_datas[idx].obj.value());
  }
  template<typename T>
  T& object_ref_t<T>::operator*() const
  {
    SILVA_ASSERT(idx >= 0);
    return pool->object_datas[idx].obj.value();
  }
}
