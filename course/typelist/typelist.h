#pragma once

#include "typelist-helpers.h"

#include <cstddef>
#include <iostream>

namespace ct::tl {

// typelist
template <typename...>
struct TypeList {};

// contains
template <typename Type, typename List>
struct contains_impl;

template <typename Type, template <typename...> typename List, typename... Types>
struct contains_impl<Type, List<Types...>> {
  static constexpr bool result = (std::is_same_v<Type, Types> || ...);
};

template <typename Type, typename List>
constexpr bool contains = contains_impl<Type, List>::result;

// flip_all
template <typename List>
struct flip_all_impl;

template <template <typename...> typename List, typename... Types>
struct flip_all_impl<List<Types...>> {
  using type = List<flip<Types>...>;
};

template <typename List>
using flip_all = flip_all_impl<List>::type;

// index_of_unique
template <typename Type, typename List>
struct index_of_unique_impl {
private:
  using indexed = enumerate<List>;
  using found = map_find<indexed, Type>;

public:
  static constexpr std::size_t result = get_index<found>::value;
};

template <typename Type, typename List>
inline constexpr std::size_t index_of_unique = index_of_unique_impl<Type, List>::result;

// flatten
template <typename List>
struct flatten_impl;

template <typename List>
using flatten = flatten_impl<List>::type;

template <typename Type>
struct flatten_inner {
  using type = TypeList<Type>;
};

template <template <typename...> typename List>
struct flatten_inner<List<>> {
  using type = TypeList<>;
};

template <template <typename...> typename List, typename... Types>
struct flatten_inner<List<Types...>> {
  using type = concatenate<TypeList<>, typename flatten_inner<Types>::type...>;
};

template <typename Type>
struct flatten_impl {
  using type = flatten_inner<Type>::type;
};

template <template <typename...> typename List, typename... Types>
struct flatten_impl<List<Types...>> {
  using type = convert<typename flatten_inner<List<Types...>>::type, List>;
};

// merge_sort
template <template <typename, typename> typename Cmp, typename List>
struct merge_sort_impl;

template <template <typename, typename> typename Cmp, template <typename...> typename List>
struct merge_sort_impl<Cmp, List<>> {
  using type = List<>;
};

template <template <typename, typename> typename Cmp, template <typename...> typename List, typename Type>
struct merge_sort_impl<Cmp, List<Type>> {
  using type = List<Type>;
};

template <template <typename, typename> typename Cmp, typename List>
using merge_sort = merge_sort_impl<Cmp, List>::type;

template <template <typename, typename> typename Cmp, typename List1, typename List2>
struct merge_impl;

template <template <typename, typename> typename Cmp, typename List1, typename List2>
using merge = merge_impl<Cmp, List1, List2>::type;

template <bool B, template <typename, typename> typename Cmp, typename L1, typename L2>
struct conditional_merge_impl;

template <
    template <typename, typename> typename Cmp,
    template <typename...> typename List,
    typename Head1,
    typename... Tail1,
    typename Head2,
    typename... Tail2>
struct conditional_merge_impl<true, Cmp, List<Head1, Tail1...>, List<Head2, Tail2...>> {
  using type = push_front<Head1, merge<Cmp, List<Tail1...>, List<Head2, Tail2...>>>;
};

template <
    template <typename, typename> typename Cmp,
    template <typename...> typename List,
    typename Head1,
    typename... Tail1,
    typename Head2,
    typename... Tail2>
struct conditional_merge_impl<false, Cmp, List<Head1, Tail1...>, List<Head2, Tail2...>> {
  using type = push_front<Head2, merge<Cmp, List<Head1, Tail1...>, List<Tail2...>>>;
};

template <template <typename, typename> typename Cmp, bool B, typename L1, typename L2>
using conditional_merge = conditional_merge_impl<B, Cmp, L1, L2>::type;

template <
    template <typename, typename> typename Cmp,
    template <typename...> typename List,
    typename Head1,
    typename... Tail1,
    typename Head2,
    typename... Tail2>
struct merge_impl<Cmp, List<Head1, Tail1...>, List<Head2, Tail2...>> {
  using type = conditional_merge<Cmp, Cmp<Head1, Head2>::value, List<Head1, Tail1...>, List<Head2, Tail2...>>;
};

template <template <typename, typename> typename Cmp, template <typename...> typename List, typename... Types>
struct merge_impl<Cmp, List<Types...>, List<>> {
  using type = List<Types...>;
};

template <template <typename, typename> typename Cmp, template <typename...> typename List, typename... Types>
struct merge_impl<Cmp, List<>, List<Types...>> {
  using type = List<Types...>;
};

template <template <typename, typename> typename Cmp, template <typename...> typename List>
struct merge_impl<Cmp, List<>, List<>> {
  using type = List<>;
};

template <template <typename, typename> typename Cmp, template <typename...> typename List, typename... Types>
struct merge_sort_impl<Cmp, List<Types...>> {
private:
  static constexpr std::size_t middle = sizeof...(Types) / 2;
  using list = List<Types...>;
  using indexed = enumerate<list>;

  template <typename Element>
  using is_first_half = std::bool_constant<(get_index<Element>::value < middle)>;

  template <typename Element>
  using is_second_half = std::bool_constant<(get_index<Element>::value >= middle)>;

  using first_indexed = filter<is_first_half, indexed>;
  using second_indexed = filter<is_second_half, indexed>;
  using first = denumerate<first_indexed>;
  using second = denumerate<second_indexed>;

public:
  using type = merge<Cmp, merge_sort<Cmp, first>, merge_sort<Cmp, second>>;
};

} // namespace ct::tl
