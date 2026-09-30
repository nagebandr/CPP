#pragma once

#include "bitset-iterator.h"
#include "bitset-reference.h"
#include "bitset-traits.h"
#include "bitset-view.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>

namespace ct {
class BitSet {
public:
  using Value = details::Value;
  using Reference = details::Reference;
  using ConstReference = details::ConstReference;
  using Iterator = details::Iterator;
  using ConstIterator = details::ConstIterator;
  using View = details::View;
  using ConstView = details::ConstView;
  using Word = details::Word;

  static constexpr std::size_t NPOS = details::NPOS;

  BitSet();

  BitSet(std::size_t size, bool value);

  BitSet(const BitSet& other);

  explicit BitSet(std::string_view str);

  explicit BitSet(const ConstView& other);

  BitSet(ConstIterator first, ConstIterator last);

  BitSet& operator=(const BitSet& other) &;

  BitSet& operator=(std::string_view str) &;

  BitSet& operator=(const ConstView& other) &;

  ~BitSet();

  void swap(BitSet& other) noexcept;

  std::size_t size() const;

  bool empty() const;

  Reference operator[](std::size_t index);

  ConstReference operator[](std::size_t index) const;

  Iterator begin();

  ConstIterator begin() const;

  Iterator end();

  ConstIterator end() const;

  BitSet& operator&=(const ConstView& other) &;

  BitSet& operator|=(const ConstView& other) &;

  BitSet& operator^=(const ConstView& other) &;

  BitSet& operator<<=(std::size_t count) &;

  BitSet& operator>>=(std::size_t count) &;

  BitSet& flip();

  BitSet& set();

  BitSet& reset();

  std::size_t count() const;

  bool any() const;

  bool all() const;

  operator View();

  operator ConstView() const;

  View subview(std::size_t offset = 0, std::size_t count = NPOS);

  ConstView subview(std::size_t offset = 0, std::size_t count = NPOS) const;

private:
  BitSet(std::size_t size, std::size_t offset);

  details::WordPointer data() const;
  std::size_t offset() const;
  std::size_t word_count() const;

  details::WordPointer data_{nullptr};
  std::size_t offset_{0};
  std::size_t size_{0};
};

BitSet operator&(const BitSet& left, const BitSet& right);

BitSet operator|(const BitSet& left, const BitSet& right);

BitSet operator^(const BitSet& left, const BitSet& right);

BitSet operator~(const BitSet& bs);

BitSet operator<<(const BitSet& left, std::size_t count);

BitSet operator>>(const BitSet& left, std::size_t count);

bool operator==(const BitSet& left, const BitSet& right);

bool operator!=(const BitSet& left, const BitSet& right);

void swap(BitSet& left, BitSet& right) noexcept;

std::string to_string(const BitSet& bs);

std::ostream& operator<<(std::ostream& out, const BitSet& bs);
} // namespace ct
