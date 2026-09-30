#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace ct {
template <bool>
class BasicBitSetReference;
template <bool>
class BasicBitSetIterator;
template <bool>
class BasicBitSetView;

namespace details {
using Word = uint64_t;
using ConstWord = const Word;
using WordPointer = Word*;
using ConstWordPointer = const Word*;
using Reference = BasicBitSetReference<false>;
using ConstReference = BasicBitSetReference<true>;
using Iterator = BasicBitSetIterator<false>;
using ConstIterator = BasicBitSetIterator<true>;
using View = BasicBitSetView<false>;
using ConstView = BasicBitSetView<true>;
using Value = bool;

inline constexpr std::size_t WORD_SIZE = sizeof(Word) * 8;
inline constexpr Word ONES = std::numeric_limits<Word>::max();
inline constexpr Word ZEROES = 0;
inline constexpr Word BIT = 1;
inline constexpr std::size_t NPOS = std::numeric_limits<std::size_t>::max();
} // namespace details
} // namespace ct
