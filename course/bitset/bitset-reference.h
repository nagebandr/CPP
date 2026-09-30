#pragma once

#include "bitset-traits.h"

#include <cstddef>
#include <type_traits>

namespace ct {
template <bool>
class BasicBitSetIterator;
template <bool>
class BasicBitSetView;

template <bool IsConst>
class BasicBitSetReference {
public:
  operator bool() const {
    return (*word_ & BIT << bit_) != 0;
  }

  template <bool B = IsConst>
    requires (!B)
  BasicBitSetReference& operator=(bool value) {
    if (value) {
      *word_ |= BIT << bit_;
    } else {
      *word_ &= ~(BIT << bit_);
    }
    return *this;
  }

  template <bool B = IsConst>
    requires (!B)
  void flip() {
    *word_ ^= BIT << bit_;
  }

  operator BasicBitSetReference<true>() const {
    return {word_, bit_};
  }

private:
  using Word = details::Word;
  using WordPointer = std::conditional_t<IsConst, details::ConstWordPointer, details::WordPointer>;

  static constexpr Word BIT = details::BIT;

  WordPointer word_;
  std::size_t bit_;

  BasicBitSetReference(WordPointer word, std::size_t bit)
      : word_(word)
      , bit_(bit) {}

  template <bool>
  friend class BasicBitSetReference;
  template <bool>
  friend class BasicBitSetIterator;
  template <bool>
  friend class BasicBitSetView;
};
} // namespace ct
