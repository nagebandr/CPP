#pragma once

#include "bitset-iterator.h"
#include "bitset-reference.h"
#include "bitset-traits.h"
#include "bitset-utils.h"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <type_traits>

namespace ct {
class BitSet;

template <bool IsConst>
class BasicBitSetView {
public:
  using Value = details::Value;
  using Reference = std::conditional_t<IsConst, details::ConstReference, details::Reference>;
  using ConstReference = details::ConstReference;
  using Iterator = std::conditional_t<IsConst, details::ConstIterator, details::Iterator>;
  using ConstIterator = details::ConstIterator;
  using View = std::conditional_t<IsConst, details::ConstView, details::View>;
  using ConstView = details::ConstView;
  using Word = details::Word;

  BasicBitSetView() = default;

  BasicBitSetView(const BasicBitSetView& other) = default;

  BasicBitSetView(Iterator first, Iterator last) {
    if (last < first) {
      last = first;
    }
    data_ = first.data();
    offset_ = first.bit();
    size_ = last.bit() - first.bit();
  }

  BasicBitSetView& operator=(const BasicBitSetView& other) & = default;

  Reference operator[](std::size_t index) const {
    const std::size_t bit = offset() + index;
    return {data() + (bit / WORD_SIZE), WORD_SIZE - 1 - (bit % WORD_SIZE)};
  }

  Iterator begin() const {
    return {data(), offset()};
  }

  Iterator end() const {
    return {data(), offset() + size()};
  }

  bool all() const {
    return details::count(data(), offset(), size(), false, true) == 0;
  }

  bool any() const {
    return details::count(data(), offset(), size(), true, true) > 0;
  }

  std::size_t count() const {
    return details::count(data(), offset(), size(), true, false);
  }

  std::size_t size() const {
    return size_;
  }

  bool empty() const {
    return size() == 0;
  }

  BasicBitSetView subview(std::size_t os = 0, std::size_t cnt = NPOS) const {
    if (os > size()) {
      return {data(), offset() + size(), 0};
    }
    return {data(), offset() + os, std::min(cnt, size() - os)};
  }

  void swap(BasicBitSetView& other) noexcept {
    using std::swap;
    swap(data_, other.data_);
    swap(offset_, other.offset_);
    swap(size_, other.size_);
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView set() const {
    details::transform(data(), offset(), size(), [](Word) {
      return ONES;
    });
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView reset() const {
    details::transform(data(), offset(), size(), [](Word) {
      return ZEROES;
    });
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView flip() const {
    details::transform(data(), offset(), size(), [](Word word) {
      return ~word;
    });
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView operator&=(const ConstView& other) const {
    details::apply(data(), offset(), other.data(), other.offset(), size(), details::BinaryOp::And);
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView operator|=(const ConstView& other) const {
    details::apply(data(), offset(), other.data(), other.offset(), size(), details::BinaryOp::Or);
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetView operator^=(const ConstView& other) const {
    details::apply(data(), offset(), other.data(), other.offset(), size(), details::BinaryOp::Xor);
    return *this;
  }

  operator ConstView() const {
    return {data(), offset(), size()};
  }

  friend void swap(BasicBitSetView& left, BasicBitSetView& right) noexcept {
    left.swap(right);
  }

  friend std::string to_string(const BasicBitSetView& view) {
    std::string result;
    result.reserve(view.size());
    for (const auto& bit : view) {
      result.push_back(bit ? '1' : '0');
    }
    return result;
  }

  friend std::ostream& operator<<(std::ostream& out, const BasicBitSetView& view) {
    for (const auto& bit : view) {
      out << (bit ? '1' : '0');
    }
    return out;
  }

  friend BitSet operator&(const ConstView& left, const ConstView& right);

  friend BitSet operator|(const ConstView& left, const ConstView& right);

  friend BitSet operator^(const ConstView& left, const ConstView& right);

  friend BitSet operator~(const ConstView& view);

  friend BitSet operator<<(const ConstView& view, std::size_t count);

  friend BitSet operator>>(const ConstView& view, std::size_t count);

private:
  using WordPointer = std::conditional_t<IsConst, details::ConstWordPointer, details::WordPointer>;

  static constexpr std::size_t WORD_SIZE = details::WORD_SIZE;
  static constexpr std::size_t NPOS = details::NPOS;
  static constexpr Word ONES = details::ONES;
  static constexpr Word ZEROES = details::ZEROES;

  WordPointer data_{nullptr};
  std::size_t offset_{0};
  std::size_t size_{0};

  BasicBitSetView(WordPointer data, std::size_t offset, std::size_t size)
      : data_(data)
      , offset_(offset)
      , size_(size) {}

  std::size_t offset() const {
    return offset_;
  }

  WordPointer data() const {
    return data_;
  }

  std::size_t words() const {
    return details::word_count(offset(), size());
  }

  bool equals(const ConstView& other) const {
    return details::equal(data(), offset(), other.data(), other.offset(), size());
  }

  friend class BitSet;
  template <bool>
  friend class BasicBitSetView;

  friend bool operator==(const ConstView& left, const ConstView& right);
};

inline bool operator==(const BasicBitSetView<true>& left, const BasicBitSetView<true>& right) {
  if (left.size() != right.size()) {
    return false;
  }
  return left.equals(right);
}

inline bool operator!=(const BasicBitSetView<true>& left, const BasicBitSetView<true>& right) {
  return !(left == right);
}
} // namespace ct
