#pragma once

#include "bitset-reference.h"
#include "bitset-traits.h"

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace ct {
template <bool IsConst>
class BasicBitSetIterator {
  using WordPointer = std::conditional_t<IsConst, details::ConstWordPointer, details::WordPointer>;
  using ConstIterator = details::ConstIterator;

  static constexpr std::size_t WORD_SIZE = details::WORD_SIZE;
  WordPointer data_;
  std::size_t bit_;

  std::size_t bit() const {
    return bit_;
  }

  WordPointer data() const {
    return data_;
  }

  BasicBitSetIterator(WordPointer data, std::size_t offset)
      : data_(data)
      , bit_(offset) {}

  friend class BitSet;
  template <bool>
  friend class BasicBitSetIterator;
  template <bool>
  friend class BasicBitSetView;

public:
  using value_type = details::Value;
  using pointer = void;
  using reference = std::conditional_t<IsConst, details::ConstReference, details::Reference>;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;

  BasicBitSetIterator() = default;

  reference operator*() const {
    return {data() + (bit() / WORD_SIZE), WORD_SIZE - 1 - (bit() % WORD_SIZE)};
  }

  pointer operator->() const = delete;

  operator ConstIterator() const {
    return {data(), bit()};
  }

  reference operator[](difference_type diff) const {
    return {data() + ((bit() + diff) / WORD_SIZE), WORD_SIZE - 1 - ((bit() + diff) % WORD_SIZE)};
  }

  BasicBitSetIterator& operator++() {
    ++bit_;
    return *this;
  }

  BasicBitSetIterator operator++(int) {
    BasicBitSetIterator temp = *this;
    ++*this;
    return temp;
  }

  BasicBitSetIterator& operator--() {
    --bit_;
    return *this;
  }

  BasicBitSetIterator operator--(int) {
    BasicBitSetIterator temp = *this;
    --*this;
    return temp;
  }

  BasicBitSetIterator& operator+=(difference_type diff) {
    bit_ += diff;
    return *this;
  }

  BasicBitSetIterator& operator-=(difference_type diff) {
    return *this += -diff;
  }

  friend difference_type operator-(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return static_cast<difference_type>(left.bit()) - static_cast<difference_type>(right.bit());
  }

  friend BasicBitSetIterator operator+(const BasicBitSetIterator& it, difference_type diff) {
    return BasicBitSetIterator(it.data(), it.bit() + diff);
  }

  friend BasicBitSetIterator operator+(difference_type diff, const BasicBitSetIterator& it) {
    return it + diff;
  }

  friend BasicBitSetIterator operator-(const BasicBitSetIterator& it, difference_type diff) {
    return it + -diff;
  }

  friend bool operator==(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right == 0;
  }

  friend bool operator!=(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right != 0;
  }

  friend bool operator<(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right < 0;
  }

  friend bool operator<=(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right <= 0;
  }

  friend bool operator>(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right > 0;
  }

  friend bool operator>=(const BasicBitSetIterator& left, const BasicBitSetIterator& right) {
    return left - right >= 0;
  }

  friend void swap(BasicBitSetIterator& left, BasicBitSetIterator& right) noexcept {
    using std::swap;
    swap(left.data_, right.data_);
    swap(left.bit_, right.bit_);
  }
};
} // namespace ct
