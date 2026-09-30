#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <numeric>

namespace ct {
template <typename T>
class Matrix {
private:
  T* data_{nullptr};
  std::size_t rows_{0};
  std::size_t cols_{0};

  template <typename S>
  class BasicColIterator {
  public:
    using value_type = T;
    using pointer = S*;
    using reference = S&;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    BasicColIterator() = default;

    reference operator*() const {
      return data()[index()];
    }

    pointer operator->() const {
      return &data()[index()];
    }

    operator BasicColIterator<const T>() const {
      return {data(), cols(), index()};
    }

    reference operator[](const difference_type diff) const {
      return data()[index() + (diff * cols())];
    }

    BasicColIterator& operator++() {
      index_ += cols();
      return *this;
    }

    BasicColIterator operator++(int) {
      BasicColIterator temp = *this;
      ++(*this);
      return temp;
    }

    BasicColIterator& operator--() {
      index_ -= cols();
      return *this;
    }

    BasicColIterator operator--(int) {
      BasicColIterator temp = *this;
      --(*this);
      return temp;
    }

    BasicColIterator& operator+=(const difference_type diff) {
      index_ += diff * cols();
      return *this;
    }

    BasicColIterator& operator-=(const difference_type diff) {
      return *this += -diff;
    }

    friend difference_type operator-(const BasicColIterator& left, const BasicColIterator& right) {
      return (static_cast<difference_type>(left.index()) - static_cast<difference_type>(right.index())) /
             static_cast<difference_type>(left.cols());
    }

    friend BasicColIterator operator+(const BasicColIterator& it, const difference_type diff) {
      return BasicColIterator(it.data(), it.cols(), it.index() + (diff * it.cols()));
    }

    friend BasicColIterator operator+(const difference_type diff, const BasicColIterator& it) {
      return it + diff;
    }

    friend BasicColIterator operator-(const BasicColIterator& it, const difference_type diff) {
      return it + -diff;
    }

    friend bool operator==(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right == 0;
    }

    friend bool operator!=(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right != 0;
    }

    friend bool operator<(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right < 0;
    }

    friend bool operator<=(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right <= 0;
    }

    friend bool operator>(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right > 0;
    }

    friend bool operator>=(const BasicColIterator& left, const BasicColIterator& right) {
      return left - right >= 0;
    }

    friend void swap(BasicColIterator& left, BasicColIterator& right) noexcept {
      using std::swap;
      swap(left.cols_, right.cols_);
      swap(left.index_, right.index_);
      swap(left.data_, right.data_);
    }

  private:
    std::size_t cols() const {
      return cols_;
    }

    std::size_t index() const {
      return index_;
    }

    pointer data() const {
      return data_;
    }

    BasicColIterator(const pointer data, const std::size_t cols, const std::size_t index)
        : cols_(cols)
        , index_(index)
        , data_(data) {}

    std::size_t cols_;
    std::size_t index_;
    pointer data_;
    friend class Matrix;
  };

  template <typename>
  class BasicView;

  template <typename>
  class MutableBasicView;

public:
  using ValueType = T;

  using Reference = T&;
  using ConstReference = const T&;

  using Pointer = T*;
  using ConstPointer = const T*;

  using Iterator = Pointer;
  using ConstIterator = ConstPointer;

  using RowIterator = Pointer;
  using ConstRowIterator = ConstPointer;

  using ColIterator = BasicColIterator<T>;
  using ConstColIterator = BasicColIterator<const T>;

  using RowView = MutableBasicView<RowIterator>;
  using ConstRowView = BasicView<ConstRowIterator>;

  using ColView = MutableBasicView<ColIterator>;
  using ConstColView = BasicView<ConstColIterator>;

private:
  Pointer allocate_memory(const std::size_t size) {
    return static_cast<Pointer>(new T[size]);
  }

  template <typename Iterator>
  class BasicView {
  private:
    Iterator begin_;
    Iterator end_;

    BasicView(Iterator begin, Iterator end)
        : begin_(begin)
        , end_(end) {}

    friend class Matrix;

  public:
    BasicView() = default;
    BasicView(const BasicView&) = default;
    BasicView& operator=(const BasicView&) = default;

    std::iterator_traits<Iterator>::reference operator[](size_t idx) const {
      return *(begin_ + idx);
    }

    Iterator begin() const {
      return begin_;
    }

    Iterator end() const {
      return end_;
    }
  };

  template <typename Iterator>
  class MutableBasicView : public BasicView<Iterator> {
  private:
    friend class Matrix;

    MutableBasicView(Iterator begin, Iterator end)
        : BasicView<Iterator>(begin, end) {}

  public:
    using BasicView<Iterator>::begin;
    using BasicView<Iterator>::end;

    MutableBasicView operator*=(ConstReference scalar) const {
      std::transform(begin(), end(), begin(), [scalar](const T& value) {
        return value * scalar;
      });
      return *this;
    }
  };

public:
  Matrix() = default;

  Matrix(const std::size_t rows, const std::size_t cols)
      : rows_(rows)
      , cols_(cols) {
    if (rows == 0 || cols == 0) {
      rows_ = 0;
      cols_ = 0;
    }
    if (!empty()) {
      data_ = allocate_memory(size());
      std::fill_n(data(), size(), T{});
    }
  }

  template <std::size_t ROWS, std::size_t COLS>
  Matrix(const T (&init)[ROWS][COLS])
      : data_(allocate_memory(ROWS * COLS))
      , rows_(ROWS)
      , cols_(COLS) {
    std::copy_n(&init[0][0], size(), data());
  }

  Matrix(const Matrix& other)
      : rows_(other.rows_)
      , cols_(other.cols_) {
    if (!empty()) {
      data_ = allocate_memory(size());
      std::copy_n(other.data(), size(), data());
    }
  }

  Matrix& operator=(const Matrix& other) {
    if (this == &other) {
      return *this;
    }
    Matrix temp(other);
    swap(*this, temp);
    return *this;
  }

  ~Matrix() {
    delete[] data_;
  }

  Iterator begin() {
    return data();
  }

  ConstIterator begin() const {
    return data();
  }

  Iterator end() {
    return data() + size();
  }

  ConstIterator end() const {
    return data() + size();
  }

  RowIterator row_begin(std::size_t row) {
    assert(row < rows());
    return data() + (row * cols());
  }

  ConstRowIterator row_begin(std::size_t row) const {
    assert(row < rows());
    return data() + (row * cols());
  }

  RowIterator row_end(std::size_t row) {
    assert(row < rows());
    return data() + ((row + 1) * cols());
  }

  ConstRowIterator row_end(std::size_t row) const {
    assert(row < rows());
    return data() + ((row + 1) * cols());
  }

  ColIterator col_begin(std::size_t col) {
    assert(col < cols());
    return {data(), cols(), col};
  }

  ConstColIterator col_begin(std::size_t col) const {
    assert(col < cols());
    return {data(), cols(), col};
  }

  ColIterator col_end(const std::size_t col) {
    assert(col < cols());
    return {data(), cols(), col + size()};
  }

  ConstColIterator col_end(const std::size_t col) const {
    assert(col < cols());
    return {data(), cols(), col + size()};
  }

  RowView row(const std::size_t row) {
    assert(row < rows());
    return RowView(row_begin(row), row_end(row));
  }

  ConstRowView row(const std::size_t row) const {
    assert(row < rows());
    return ConstRowView(row_begin(row), row_end(row));
  }

  ColView col(const std::size_t col) {
    assert(col < cols());
    return ColView(col_begin(col), col_end(col));
  }

  ConstColView col(const std::size_t col) const {
    assert(col < cols());
    return ConstColView(col_begin(col), col_end(col));
  }

  std::size_t rows() const {
    return rows_;
  }

  std::size_t cols() const {
    return cols_;
  }

  std::size_t size() const {
    return rows() * cols();
  }

  bool empty() const {
    return size() == 0;
  }

  Reference operator()(const std::size_t row, const std::size_t col) {
    assert(row < rows() && col < cols());
    return data()[(row * cols()) + col];
  }

  ConstReference operator()(const std::size_t row, const std::size_t col) const {
    assert(row < rows() && col < cols());
    return data()[(row * cols()) + col];
  }

  Reference operator[](const std::size_t row, const std::size_t col) {
    return (*this)(row, col);
  }

  ConstReference operator[](const std::size_t row, const std::size_t col) const {
    return (*this)(row, col);
  }

  Pointer data() {
    return data_;
  }

  ConstPointer data() const {
    return data_;
  }

  friend bool operator==(const Matrix& left, const Matrix& right) {
    if (left.rows() != right.rows() || left.cols() != right.cols()) {
      return false;
    }
    return std::equal(left.begin(), left.end(), right.begin());
  }

  friend bool operator!=(const Matrix& left, const Matrix& right) {
    return !(left == right);
  }

  Matrix& operator+=(const Matrix& other) {
    assert(rows() == other.rows() && cols() == other.cols());
    std::transform(begin(), end(), other.begin(), begin(), std::plus<T>());
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    assert(rows() == other.rows() && cols() == other.cols());
    std::transform(begin(), end(), other.begin(), begin(), std::minus<T>());
    return *this;
  }

  Matrix& operator*=(const Matrix& other) {
    Matrix temp = (*this) * other;
    swap(*this, temp);
    return *this;
  }

  Matrix& operator*=(ConstReference scalar) {
    RowView(begin(), end()) *= scalar;
    return *this;
  }

  friend Matrix operator+(const Matrix& left, const Matrix& right) {
    Matrix result(left);
    result += right;
    return result;
  }

  friend Matrix operator-(const Matrix& left, const Matrix& right) {
    Matrix result(left);
    result -= right;
    return result;
  }

  friend Matrix operator*(const Matrix& left, const Matrix& right) {
    assert(left.cols() == right.rows());
    Matrix result(left.rows(), right.cols());
    for (std::size_t i = 0; i < result.rows(); ++i) {
      for (std::size_t j = 0; j < result.cols(); ++j) {
        result(i, j) = std::inner_product(left.row_begin(i), left.row_end(i), right.col_begin(j), T{});
      }
    }
    return result;
  }

  friend Matrix operator*(const Matrix& left, ConstReference right) {
    Matrix result(left);
    result *= right;
    return result;
  }

  friend Matrix operator*(ConstReference left, const Matrix& right) {
    return right * left;
  }

  friend void swap(Matrix& left, Matrix& right) noexcept {
    using std::swap;
    swap(left.data_, right.data_);
    swap(left.rows_, right.rows_);
    swap(left.cols_, right.cols_);
  }
};

} // namespace ct
