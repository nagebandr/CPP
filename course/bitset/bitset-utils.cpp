#include "bitset-utils.h"

#include <algorithm>
#include <bit>

namespace ct::details {
namespace {
Word n_ones_left(std::size_t n) {
  if (n == 0) {
    return ZEROES;
  }
  return ONES << (WORD_SIZE - n);
}

Word apply_word(Word left, Word right, BinaryOp op) {
  switch (op) {
  case BinaryOp::And:
    return left & right;
  case BinaryOp::Or:
    return left | right;
  case BinaryOp::Xor:
    return left ^ right;
  }
  return left;
}

bool neutral(BinaryOp op) {
  return op == BinaryOp::And;
}

Word construct_word(ConstWordPointer data, std::size_t offset, std::size_t size, bool neutral_bit) {
  const std::size_t offset_mod = offset % WORD_SIZE;
  if (offset_mod + size <= WORD_SIZE) {
    if (neutral_bit) {
      return data[offset / WORD_SIZE] << offset_mod | ~n_ones_left(size);
    }
    return data[offset / WORD_SIZE] << offset_mod & n_ones_left(size);
  }

  const std::size_t first_size = WORD_SIZE - offset_mod;
  const std::size_t second_size = size - first_size;
  return construct_word(data, offset, first_size, false) +
         (construct_word(data, offset + first_size, second_size, neutral_bit) >> first_size);
}

Word rotate_word(ConstWordPointer data, std::size_t offset, std::size_t size, bool neutral_bit, std::size_t rotation) {
  return std::rotr(construct_word(data, offset, size, neutral_bit), static_cast<int>(rotation));
}

void apply_chunk(
    WordPointer data1,
    std::size_t offset1,
    ConstWordPointer data2,
    std::size_t offset2,
    std::size_t size,
    BinaryOp op
) {
  if (const std::size_t offset_mod = offset1 % WORD_SIZE; offset_mod + size <= WORD_SIZE) {
    data1[offset1 / WORD_SIZE] =
        apply_word(data1[offset1 / WORD_SIZE], rotate_word(data2, offset2, size, neutral(op), offset_mod), op);
    return;
  }

  const std::size_t first_size = WORD_SIZE - (offset1 % WORD_SIZE);
  const std::size_t second_size = size - first_size;
  apply_chunk(data1, offset1, data2, offset2, first_size, op);
  apply_chunk(data1, offset1 + first_size, data2, offset2 + first_size, second_size, op);
}
} // namespace

std::size_t word_count(std::size_t offset, std::size_t size) {
  if (size == 0) {
    return 0;
  }
  return ((offset + size - 1) / WORD_SIZE) - (offset / WORD_SIZE) + 1;
}

WordPointer allocate_memory(std::size_t count) {
  return static_cast<WordPointer>(operator new(count * sizeof(Word)));
}

void deallocate_memory(WordPointer data) {
  operator delete(data);
}

void apply(
    WordPointer data1,
    std::size_t offset1,
    ConstWordPointer data2,
    std::size_t offset2,
    std::size_t size,
    BinaryOp op
) {
  for (std::size_t i = 0; i < size; i += WORD_SIZE) {
    apply_chunk(data1, offset1 + i, data2, offset2 + i, std::min(WORD_SIZE, size - i), op);
  }
}

void assign(WordPointer data, std::size_t offset, std::size_t size, ConstWordPointer word) {
  apply(data, offset, word, 0, size, BinaryOp::And);
  apply(data, offset, word, 0, size, BinaryOp::Or);
}

void assign(WordPointer data1, std::size_t offset1, ConstWordPointer data2, std::size_t offset2, std::size_t size) {
  for (std::size_t i = 0; i < size; i += WORD_SIZE) {
    const Word cur_word = construct_word(data2, offset2 + i, std::min(WORD_SIZE, size - i), false);
    assign(data1, offset1 + i, std::min(WORD_SIZE, size - i), &cur_word);
  }
}

void transform(WordPointer data, std::size_t offset, std::size_t size, Word (*func)(Word)) {
  for (std::size_t i = 0; i < size; i += WORD_SIZE) {
    Word res = func(construct_word(data, offset + i, std::min(WORD_SIZE, size - i), false));
    assign(data, offset + i, std::min(WORD_SIZE, size - i), &res);
  }
}

std::size_t count(ConstWordPointer data, std::size_t offset, std::size_t size, bool ones, bool short_circuit) {
  std::size_t res = 0;
  for (std::size_t i = 0; i < size; i += WORD_SIZE) {
    const Word cur_word = construct_word(data, offset + i, std::min(WORD_SIZE, size - i), !ones);
    if (ones) {
      res += std::popcount(cur_word);
    } else {
      res += WORD_SIZE - std::popcount(cur_word);
    }
    if (short_circuit && res > 0) {
      return res;
    }
  }
  return res;
}

bool equal(ConstWordPointer data1, std::size_t offset1, ConstWordPointer data2, std::size_t offset2, std::size_t size) {
  for (std::size_t i = 0; i < size; i += WORD_SIZE) {
    const Word word1 = construct_word(data1, offset1 + i, std::min(WORD_SIZE, size - i), false);
    const Word word2 = construct_word(data2, offset2 + i, std::min(WORD_SIZE, size - i), false);
    if (word1 != word2) {
      return false;
    }
  }
  return true;
}
} // namespace ct::details
