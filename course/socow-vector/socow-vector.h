#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace ct {
template <typename T, std::size_t SMALL_SIZE>
class SocowVector {
public:
  static_assert(SMALL_SIZE > 0, "SMALL_SIZE must be positive");
  static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");
  static_assert(std::is_nothrow_move_constructible_v<T>, "T must have a non-throwing move constructor");
  static_assert(std::is_copy_assignable_v<T>, "T must have a copy assignment operator");
  static_assert(std::is_nothrow_move_assignable_v<T>, "T must have a non-throwing move assignment operator");
  static_assert(std::is_nothrow_swappable_v<T>, "T must have a non-throwing swap");

  using ValueType = T;
  using Reference = T&;
  using ConstReference = const T&;
  using Pointer = T*;
  using ConstPointer = const T*;
  using Iterator = Pointer;
  using ConstIterator = ConstPointer;

  // O(1) noexcept
  SocowVector() noexcept {}

  // O(SMALL_SIZE) strong
  SocowVector(const SocowVector& other) {
    if (other.is_small()) {
      std::uninitialized_copy(other.begin(), other.end(), small_data());
      is_small_ = true;
    } else {
      big_data_ = other.big_data();
      is_small_ = false;
      big_data()->share();
    }
    size_ = other.size();
  }

  // O(N) strong
  SocowVector& operator=(const SocowVector& other) {
    if (this == &other) {
      return *this;
    }
    SocowVector tmp(other);
    force_clear();
    swap(tmp);
    return *this;
  }

  // O(SMALL_SIZE) noexcept
  SocowVector(SocowVector&& other) noexcept {
    if (other.is_small()) {
      is_small_ = true;
      size_ = other.size();
      std::uninitialized_move(other.begin(), other.end(), begin());
      other.clear();
    } else {
      is_small_ = false;
      big_data_ = other.big_data();
      size_ = other.size();
      other.is_small_ = true;
      other.size_ = 0;
    }
  }

  // O(N) noexcept
  SocowVector& operator=(SocowVector&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    force_clear();
    swap(other);
    return *this;
  }

  // O(SMALL_SIZE) noexcept
  void swap(SocowVector& other) noexcept {
    if (this == &other) {
      return;
    }
    using std::swap;
    if (is_small()) {
      if (other.is_small()) {
        if (size() <= other.size()) {
          std::swap_ranges(begin(), end(), other.begin());
          std::uninitialized_move(other.begin() + size(), other.end(), begin() + size());
          std::destroy(other.begin() + size(), other.end());
          swap(size_, other.size_);
        } else {
          other.swap(*this);
        }
      } else {
        Buffer* tmp = other.big_data();
        other.is_small_ = true;
        std::uninitialized_move(begin(), end(), other.small_data());
        std::destroy(begin(), end());
        is_small_ = false;
        swap(size_, other.size_);
        big_data_ = tmp;
      }
    } else {
      if (other.is_small()) {
        other.swap(*this);
      } else {
        swap(big_data_, other.big_data_);
        swap(size_, other.size_);
      }
    }
  }

  // O(1) noexcept
  std::size_t size() const noexcept {
    return size_;
  }

  // O(1) noexcept
  std::size_t capacity() const noexcept {
    return is_small() ? SMALL_SIZE : big_data()->capacity();
  }

  // O(1) noexcept
  bool empty() const noexcept {
    return size() == 0;
  }

  // O(N) strong
  Reference operator[](std::size_t index) {
    return data()[index];
  }

  // O(1) noexcept
  ConstReference operator[](std::size_t index) const noexcept {
    return c_data()[index];
  }

  // O(N) strong
  Reference front() {
    return (*this)[0];
  }

  // O(1) noexcept
  ConstReference front() const noexcept {
    return (*this)[0];
  }

  // O(N) strong
  Reference back() {
    return (*this)[size() - 1];
  }

  // O(1) noexcept
  ConstReference back() const noexcept {
    return (*this)[size() - 1];
  }

  // O(N) strong
  Pointer data() {
    if (is_small()) {
      return small_data();
    }
    unshare(size(), capacity());
    return big_data()->data();
  }

  // O(1) noexcept
  ConstPointer data() const noexcept {
    return c_data();
  }

  // O(N) strong
  Iterator begin() {
    return data();
  }

  // O(1) noexcept
  ConstIterator begin() const noexcept {
    return c_data();
  }

  // O(N) strong
  Iterator end() {
    return data() + size();
  }

  // O(1) noexcept
  ConstIterator end() const noexcept {
    return c_data() + size();
  }

  // O(N) strong
  void push_back(ConstReference value) {
    push_back_impl(value);
  }

  // O(N) strong
  void push_back(ValueType&& value) {
    push_back_impl(std::move(value));
  }

  // O(N) strong
  Iterator insert(const ConstIterator pos, ConstReference value) {
    return insert_impl(pos, value);
  }

  // O(N) strong
  Iterator insert(const ConstIterator pos, ValueType&& value) {
    return insert_impl(pos, std::move(value));
  }

  // O(N) strong
  void pop_back() {
    std::size_t new_size = size() - 1;
    if (is_single()) {
      back().~ValueType();
      size_ = new_size;
      return;
    }
    std::size_t new_capacity = new_size <= SMALL_SIZE ? SMALL_SIZE : capacity();
    unshare(new_size, new_capacity);
  }

  // O(N) strong
  Iterator erase(ConstIterator pos) {
    return erase(pos, pos + 1);
  }

  // O(N) strong
  Iterator erase(ConstIterator first, ConstIterator last) {
    auto index = static_cast<std::size_t>(first - c_data());
    auto count = static_cast<std::size_t>(last - first);
    if (count == 0) {
      return begin() + index;
    }
    std::size_t old_size = size();
    std::size_t new_size = old_size - count;
    if (!is_single()) {
      std::size_t new_capacity = new_size <= SMALL_SIZE ? SMALL_SIZE : capacity();
      SocowVector tmp(new_capacity);
      tmp.mass_push_back(c_data(), c_data() + index);
      tmp.mass_push_back(c_data() + index + count, c_data() + old_size);
      *this = std::move(tmp);
      return begin() + index;
    }
    for (std::size_t i = index; i + count < old_size; ++i) {
      std::iter_swap(begin() + i, begin() + i + count);
    }
    std::destroy(begin() + new_size, begin() + old_size);
    size_ = new_size;
    return begin() + index;
  }

  // O(N) strong
  void reserve(std::size_t new_capacity) {
    if (is_single()) {
      if (new_capacity <= capacity()) {
        return;
      }
      migrate(new_capacity);
      return;
    }
    if (new_capacity < size()) {
      return;
    }
    unshare(size(), new_capacity);
  }

  // O(N) strong
  void shrink_to_fit() {
    if (is_small() || size() == capacity()) {
      return;
    }
    if (is_single()) {
      migrate(size());
      return;
    }
    unshare(size(), size());
  }

  // O(N) noexcept
  void clear() noexcept {
    if (is_single()) {
      std::destroy(begin(), end());
    } else {
      big_data()->unshare();
      is_small_ = true;
    }
    size_ = 0;
  }

  // O(N) noexcept
  ~SocowVector() noexcept {
    force_clear();
  }

private:
  class Buffer {
  public:
    void share() noexcept {
      ++ref_count_;
    }

    void unshare() noexcept {
      --ref_count_;
    }

    std::size_t ref_count() const noexcept {
      return ref_count_;
    }

    std::size_t capacity() const noexcept {
      return capacity_;
    }

    Pointer data() noexcept {
      return data_;
    }

  private:
    std::size_t ref_count_{1};
    std::size_t capacity_{0};
    ValueType data_[0];
    friend class SocowVector;
  };

  bool is_small() const noexcept {
    return is_small_;
  }

  bool is_single() const noexcept {
    return is_small() || big_data()->ref_count() == 1;
  }

  Pointer small_data() noexcept {
    return &small_data_[0];
  }

  ConstPointer small_data() const noexcept {
    return &small_data_[0];
  }

  Buffer* big_data() const noexcept {
    return big_data_;
  }

  ConstPointer c_data() const noexcept {
    return is_small() ? small_data() : big_data()->data();
  }

  explicit SocowVector(std::size_t capacity) {
    if (capacity <= SMALL_SIZE) {
      return;
    }
    std::size_t alignment = alignof(Buffer);
    std::size_t bytes = sizeof(Buffer) + (capacity * sizeof(ValueType));
    std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
    auto* mem = operator new(aligned_bytes, std::align_val_t{alignment});
    big_data_ = static_cast<Buffer*>(mem);
    big_data_->capacity_ = capacity;
    big_data_->ref_count_ = 1;
    is_small_ = false;
  }

  template <typename It>
  void mass_push_back(It first, It last) {
    for (; first != last; ++first) {
      if constexpr (std::is_same_v<It, ConstIterator>) {
        push_back(*first);
      } else {
        push_back(std::move(*first));
      }
    }
  }

  static void dealloc(Buffer* buff) noexcept {
    operator delete(reinterpret_cast<std::byte*>(buff), std::align_val_t{alignof(Buffer)});
  }

  void force_clear() noexcept {
    clear();
    if (!is_small()) {
      dealloc(big_data());
      is_small_ = true;
    }
  }

  void unshare(std::size_t new_size, std::size_t new_capacity) {
    if (is_single()) {
      return;
    }
    SocowVector tmp(new_capacity);
    tmp.mass_push_back(c_data(), c_data() + new_size);
    *this = std::move(tmp);
  }

  void migrate(std::size_t new_capacity) {
    SocowVector tmp(new_capacity);
    tmp.mass_push_back(begin(), end());
    *this = std::move(tmp);
  }

  template <typename Ref>
  void push_back_impl(Ref&& value) {
    std::size_t old_size = size();
    if (old_size < capacity() && is_single()) {
      new (begin() + old_size) ValueType(std::forward<Ref>(value));
      size_ = old_size + 1;
      return;
    }
    std::size_t new_capacity = old_size < capacity() ? capacity() : capacity() * 2;
    SocowVector tmp(new_capacity);
    if (!is_single()) {
      tmp.mass_push_back(c_data(), c_data() + old_size);
      new (tmp.data() + old_size) ValueType(std::forward<Ref>(value));
    } else {
      new (tmp.data() + old_size) ValueType(std::forward<Ref>(value));
      tmp.mass_push_back(begin(), end());
    }
    tmp.size_ = old_size + 1;
    *this = std::move(tmp);
  }

  template <typename Ref>
  Iterator insert_impl(const ConstIterator pos, Ref&& value) {
    auto index = static_cast<std::size_t>(pos - c_data());
    push_back(std::forward<Ref>(value));
    std::rotate(begin() + index, end() - 1, end());
    return begin() + index;
  }

  union {
    ValueType small_data_[SMALL_SIZE];
    Buffer* big_data_;
  };

  bool is_small_{true};
  std::size_t size_{0};
};
} // namespace ct
