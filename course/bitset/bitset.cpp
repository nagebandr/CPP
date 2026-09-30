#include "bitset.h"

#include "bitset-utils.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>

namespace ct {
namespace {
constexpr std::size_t WORD_SIZE = details::WORD_SIZE;
constexpr BitSet::Word ZEROES = details::ZEROES;
constexpr BitSet::Word ONES = details::ONES;
} // namespace

BitSet::BitSet() = default;

BitSet::BitSet(std::size_t size, std::size_t offset)
    : offset_(offset)
    , size_(size) {
  if (size_ == 0) {
    return;
  }

  data_ = details::allocate_memory(word_count());
}

BitSet::BitSet(std::size_t size, bool value)
    : BitSet(size, static_cast<std::size_t>(0)) {
  if (size_ == 0) {
    return;
  }

  std::fill_n(data(), word_count(), value ? ONES : ZEROES);
}

BitSet::BitSet(const BitSet& other)
    : BitSet(other.size(), other.offset()) {
  if (other.empty()) {
    return;
  }

  std::copy_n(other.data(), word_count(), data());
}

BitSet::BitSet(const std::string_view str)
    : BitSet(str.size(), static_cast<std::size_t>(0)) {
  if (str.empty()) {
    return;
  }
  std::fill_n(data(), word_count(), ZEROES);

  Iterator it(data(), offset());
  for (const char c : str) {
    *it++ = c == '1';
  }
}

BitSet::BitSet(const ConstView& other)
    : BitSet(other.begin(), other.end()) {}

BitSet::BitSet(ConstIterator first, ConstIterator last)
    : BitSet(first < last ? last.bit() - first.bit() : 0, first < last ? first.bit() % WORD_SIZE : 0) {
  if (first >= last) {
    return;
  }

  std::copy_n(first.data() + (first.bit() / WORD_SIZE), word_count(), data());
}

BitSet& BitSet::operator=(const BitSet& other) & {
  if (this == &other) {
    return *this;
  }
  BitSet copy = other;
  swap(copy);
  return *this;
}

BitSet& BitSet::operator=(const std::string_view str) & {
  BitSet copy(str);
  swap(copy);
  return *this;
}

BitSet& BitSet::operator=(const ConstView& other) & {
  BitSet copy(other);
  swap(copy);
  return *this;
}

BitSet::~BitSet() {
  details::deallocate_memory(data());
}

void BitSet::swap(BitSet& other) noexcept {
  using std::swap;
  swap(data_, other.data_);
  swap(offset_, other.offset_);
  swap(size_, other.size_);
}

std::size_t BitSet::size() const {
  return size_;
}

bool BitSet::empty() const {
  return size() == 0;
}

BitSet::Reference BitSet::operator[](std::size_t index) {
  return View(*this)[index];
}

BitSet::ConstReference BitSet::operator[](std::size_t index) const {
  return ConstView(*this)[index];
}

BitSet::Iterator BitSet::begin() {
  return View(*this).begin();
}

BitSet::ConstIterator BitSet::begin() const {
  return ConstView(*this).begin();
}

BitSet::Iterator BitSet::end() {
  return View(*this).end();
}

BitSet::ConstIterator BitSet::end() const {
  return ConstView(*this).end();
}

BitSet& BitSet::operator&=(const ConstView& other) & {
  View(*this) &= other;
  return *this;
}

BitSet& BitSet::operator|=(const ConstView& other) & {
  View(*this) |= other;
  return *this;
}

BitSet& BitSet::operator^=(const ConstView& other) & {
  View(*this) ^= other;
  return *this;
}

BitSet& BitSet::operator<<=(std::size_t count) & {
  if (count == 0) {
    return *this;
  }

  BitSet tmp = ConstView(*this) << count;
  swap(tmp);
  return *this;
}

BitSet& BitSet::operator>>=(std::size_t count) & {
  if (count == 0) {
    return *this;
  }

  BitSet tmp = ConstView(*this) >> count;
  swap(tmp);
  return *this;
}

BitSet& BitSet::flip() {
  View(*this).flip();
  return *this;
}

BitSet& BitSet::set() {
  View(*this).set();
  return *this;
}

BitSet& BitSet::reset() {
  View(*this).reset();
  return *this;
}

std::size_t BitSet::count() const {
  return ConstView(*this).count();
}

bool BitSet::any() const {
  return ConstView(*this).any();
}

bool BitSet::all() const {
  return ConstView(*this).all();
}

BitSet::operator View() {
  return {data(), offset(), size()};
}

BitSet::operator ConstView() const {
  return {data(), offset(), size()};
}

BitSet::View BitSet::subview(std::size_t offset, std::size_t count) {
  return View(*this).subview(offset, count);
}

BitSet::ConstView BitSet::subview(std::size_t offset, std::size_t count) const {
  return ConstView(*this).subview(offset, count);
}

details::WordPointer BitSet::data() const {
  return data_;
}

std::size_t BitSet::offset() const {
  return offset_;
}

std::size_t BitSet::word_count() const {
  return details::word_count(offset(), size());
}

BitSet operator&(const BitSet& left, const BitSet& right) {
  return BitSet::ConstView(left) & BitSet::ConstView(right);
}

BitSet operator|(const BitSet& left, const BitSet& right) {
  return BitSet::ConstView(left) | BitSet::ConstView(right);
}

BitSet operator^(const BitSet& left, const BitSet& right) {
  return BitSet::ConstView(left) ^ BitSet::ConstView(right);
}

BitSet operator~(const BitSet& bs) {
  BitSet result(bs);
  result.flip();
  return result;
}

BitSet operator<<(const BitSet& bs, std::size_t count) {
  return BitSet::ConstView(bs) << count;
}

BitSet operator>>(const BitSet& bs, std::size_t count) {
  return BitSet::ConstView(bs) >> count;
}

bool operator==(const BitSet& left, const BitSet& right) {
  return BitSet::ConstView(left) == BitSet::ConstView(right);
}

bool operator!=(const BitSet& left, const BitSet& right) {
  return !(left == right);
}

void swap(BitSet& left, BitSet& right) noexcept {
  left.swap(right);
}

std::string to_string(const BitSet& bs) {
  return to_string(BitSet::ConstView(bs));
}

std::ostream& operator<<(std::ostream& out, const BitSet& bs) {
  out << BitSet::ConstView(bs);
  return out;
}

BitSet operator&(const BitSet::ConstView& left, const BitSet::ConstView& right) {
  BitSet result(left);
  result &= right;
  return result;
}

BitSet operator|(const BitSet::ConstView& left, const BitSet::ConstView& right) {
  BitSet result(left);
  result |= right;
  return result;
}

BitSet operator^(const BitSet::ConstView& left, const BitSet::ConstView& right) {
  BitSet result(left);
  result ^= right;
  return result;
}

BitSet operator~(const BitSet::ConstView& view) {
  BitSet result(view);
  result.flip();
  return result;
}

BitSet operator<<(const BitSet::ConstView& view, std::size_t count) {
  if (count == 0) {
    return BitSet(view);
  }

  BitSet result(view.size() + count, false);
  BitSet::View result_view = result;
  details::assign(result_view.data(), result_view.offset(), view.data(), view.offset(), view.size());
  return result;
}

BitSet operator>>(const BitSet::ConstView& view, std::size_t count) {
  if (count >= view.size()) {
    return BitSet();
  }

  return BitSet(view.subview(0, view.size() - count));
}
} // namespace ct
