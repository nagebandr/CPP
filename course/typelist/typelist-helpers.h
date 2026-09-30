#pragma once

#include <cstddef>
#include <list>
#include <utility>

namespace ct::tl {

// typelist forward declaration
template <typename...>
struct TypeList;

// flip
template <typename T>
struct flip_impl;

template <template <typename...> typename List, typename T1, typename T2>
struct flip_impl<List<T1, T2>> {
  using type = List<T2, T1>;
};

template <typename T>
using flip = flip_impl<T>::type;

// count
template <typename List>
struct count_impl;

template <template <typename...> typename List, typename... Types>
struct count_impl<List<Types...>> {
  static constexpr std::size_t value = sizeof...(Types);
};

template <typename List>
constexpr std::size_t count = count_impl<List>::value;

// push_front
template <typename T, typename List>
struct push_front_impl;

template <template <typename...> typename List, typename... Types, typename T>
struct push_front_impl<T, List<Types...>> {
  using type = List<T, Types...>;
};

template <typename T, typename List>
using push_front = push_front_impl<T, List>::type;

// convert
template <typename SourceList, template <typename...> typename TargetList>
struct convert_impl;

template <template <typename...> typename SourceList, typename... Types, template <typename...> typename TargetList>
struct convert_impl<SourceList<Types...>, TargetList> {
  using type = TargetList<Types...>;
};

template <typename SourceList, template <typename...> typename TargetList>
using convert = convert_impl<SourceList, TargetList>::type;

// conditional
template <bool B>
struct conditional_impl;

template <>
struct conditional_impl<true> {
  template <typename T, typename F>
  using type = T;
};

template <>
struct conditional_impl<false> {
  template <typename T, typename F>
  using type = F;
};

template <bool B, typename T, typename F>
using conditional = conditional_impl<B>::template type<T, F>;

// identity
template <typename Type>
struct identity {
  using type = Type;
};

// concatenate
template <
    template <typename...> typename List1,
    template <typename...> typename List2,
    typename... Types1,
    typename... Types2>
identity<List1<Types1..., Types2...>> operator+(identity<List1<Types1...>>, identity<List2<Types2...>>);

template <typename... Lists>
using concatenate = decltype((identity<Lists>{} + ... + identity<TypeList<>>{}))::type;

// filter
template <template <typename> typename Pred, typename List>
struct filter_impl;

template <template <typename> typename Pred, template <typename...> typename List, typename... Types>
struct filter_impl<Pred, List<Types...>> {
private:
  template <typename T>
  using single = conditional<Pred<T>::value, List<T>, List<>>;

public:
  using type = concatenate<single<Types>...>;
};

template <template <typename> typename Pred, typename List>
using filter = filter_impl<Pred, List>::type;

// enumerate
template <std::size_t VALUE>
struct index {
  static constexpr std::size_t value = VALUE;
};

template <typename List, typename Seq>
struct enumerate_impl;

template <template <typename...> typename List, typename... Types, std::size_t... Indices>
struct enumerate_impl<List<Types...>, std::index_sequence<Indices...>> {
  using type = List<TypeList<index<Indices>, Types>...>;
};

template <typename List>
using enumerate = enumerate_impl<List, std::make_index_sequence<count<List>>>::type;

// get_index
template <typename Element>
struct get_index;

template <template <typename...> typename List, std::size_t I, typename T>
struct get_index<List<index<I>, T>> {
  static constexpr std::size_t value = I;
};

// get_type
template <typename Element>
struct get_type;

template <template <typename...> typename List, std::size_t I, typename T>
struct get_type<List<index<I>, T>> {
  using type = T;
};

// denumerate
template <typename List>
struct denumerate_impl;

template <template <typename...> typename List, typename... Elements>
struct denumerate_impl<List<Elements...>> {
  using type = List<typename get_type<Elements>::type...>;
};

template <typename List>
using denumerate = denumerate_impl<List>::type;

// map_find
template <typename Map, typename Key>
struct map_find_impl;

template <
    template <typename...> typename List,
    template <typename...> typename Inner,
    typename... Elements,
    typename... Rest,
    typename Key>
struct map_find_impl<List<Inner<Elements...>, Rest...>, Key> {
private:
  template <typename I, typename... Other>
  static Inner<I, Key, Other...> f(Inner<I, Key, Other...>*);

  struct derived
      : Inner<Elements...>
      , Rest... {};

public:
  using type = decltype(f(static_cast<derived*>(nullptr)));
};

template <typename Map, typename Key>
using map_find = map_find_impl<Map, Key>::type;

} // namespace ct::tl
