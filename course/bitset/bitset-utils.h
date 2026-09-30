#pragma once

#include "bitset-traits.h"

#include <cstddef>

namespace ct::details {
enum class BinaryOp {
  And,
  Or,
  Xor,
};

std::size_t word_count(std::size_t offset, std::size_t size);

WordPointer allocate_memory(std::size_t count);

void deallocate_memory(WordPointer data);

void apply(
    WordPointer data1,
    std::size_t offset1,
    ConstWordPointer data2,
    std::size_t offset2,
    std::size_t size,
    BinaryOp op
);

void assign(WordPointer data, std::size_t offset, std::size_t size, ConstWordPointer word);

void assign(WordPointer data1, std::size_t offset1, ConstWordPointer data2, std::size_t offset2, std::size_t size);

void transform(WordPointer data, std::size_t offset, std::size_t size, Word (*func)(Word));

std::size_t count(ConstWordPointer data, std::size_t offset, std::size_t size, bool ones, bool short_circuit);

bool equal(ConstWordPointer data1, std::size_t offset1, ConstWordPointer data2, std::size_t offset2, std::size_t size);
} // namespace ct::details
