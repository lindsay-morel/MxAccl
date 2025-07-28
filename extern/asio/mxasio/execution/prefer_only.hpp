//
// execution/prefer_only.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXECUTION_PREFER_ONLY_HPP
#define MXASIO_EXECUTION_PREFER_ONLY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/type_traits.hpp"
#include "mxasio/is_applicable_property.hpp"
#include "mxasio/prefer.hpp"
#include "mxasio/query.hpp"
#include "mxasio/traits/static_query.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

#if defined(GENERATING_DOCUMENTATION)

namespace execution {

/// A property adapter that is used with the polymorphic executor wrapper
/// to mark properties as preferable, but not requirable.
template <typename Property>
struct prefer_only
{
  /// The prefer_only adapter applies to the same types as the nested property.
  template <typename T>
  static constexpr bool is_applicable_property_v =
    is_applicable_property<T, Property>::value;

  /// The context_t property cannot be required.
  static constexpr bool is_requirable = false;

  /// The context_t property can be preferred, it the underlying property can
  /// be preferred.
  /**
   * @c true if @c Property::is_preferable is @c true, otherwise @c false.
   */
  static constexpr bool is_preferable = automatically_determined;

  /// The type returned by queries against an @c any_executor.
  typedef typename Property::polymorphic_query_result_type
    polymorphic_query_result_type;
};

} // namespace execution

#else // defined(GENERATING_DOCUMENTATION)

namespace execution {
namespace detail {

template <typename InnerProperty, typename = void>
struct prefer_only_is_preferable
{
  static constexpr bool is_preferable = false;
};

template <typename InnerProperty>
struct prefer_only_is_preferable<InnerProperty,
    enable_if_t<
      InnerProperty::is_preferable
    >
  >
{
  static constexpr bool is_preferable = true;
};

template <typename InnerProperty, typename = void>
struct prefer_only_polymorphic_query_result_type
{
};

template <typename InnerProperty>
struct prefer_only_polymorphic_query_result_type<InnerProperty,
    void_t<
      typename InnerProperty::polymorphic_query_result_type
    >
  >
{
  typedef typename InnerProperty::polymorphic_query_result_type
    polymorphic_query_result_type;
};

template <typename InnerProperty, typename = void>
struct prefer_only_property
{
  InnerProperty property;

  prefer_only_property(const InnerProperty& p)
    : property(p)
  {
  }
};

#if defined(MXASIO_HAS_WORKING_EXPRESSION_SFINAE)

template <typename InnerProperty>
struct prefer_only_property<InnerProperty,
    void_t<
      decltype(mxasio::declval<const InnerProperty>().value())
    >
  >
{
  InnerProperty property;

  prefer_only_property(const InnerProperty& p)
    : property(p)
  {
  }

  constexpr auto value() const
    noexcept(noexcept(mxasio::declval<const InnerProperty>().value()))
    -> decltype(mxasio::declval<const InnerProperty>().value())
  {
    return property.value();
  }
};

#else // defined(MXASIO_HAS_WORKING_EXPRESSION_SFINAE)

struct prefer_only_memfns_base
{
  void value();
};

template <typename T>
struct prefer_only_memfns_derived
  : T, prefer_only_memfns_base
{
};

template <typename T, T>
struct prefer_only_memfns_check
{
};

template <typename>
char (&prefer_only_value_memfn_helper(...))[2];

template <typename T>
char prefer_only_value_memfn_helper(
    prefer_only_memfns_check<
      void (prefer_only_memfns_base::*)(),
      &prefer_only_memfns_derived<T>::value>*);

template <typename InnerProperty>
struct prefer_only_property<InnerProperty,
    enable_if_t<
      sizeof(prefer_only_value_memfn_helper<InnerProperty>(0)) != 1
        && !is_same<typename InnerProperty::polymorphic_query_result_type,
          void>::value
    >
  >
{
  InnerProperty property;

  prefer_only_property(const InnerProperty& p)
    : property(p)
  {
  }

  constexpr typename InnerProperty::polymorphic_query_result_type
  value() const
  {
    return property.value();
  }
};

#endif // defined(MXASIO_HAS_WORKING_EXPRESSION_SFINAE)

} // namespace detail

template <typename InnerProperty>
struct prefer_only :
  detail::prefer_only_is_preferable<InnerProperty>,
  detail::prefer_only_polymorphic_query_result_type<InnerProperty>,
  detail::prefer_only_property<InnerProperty>
{
  static constexpr bool is_requirable = false;

  constexpr prefer_only(const InnerProperty& p)
    : detail::prefer_only_property<InnerProperty>(p)
  {
  }

#if defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
  template <typename T>
  static constexpr
  typename traits::static_query<T, InnerProperty>::result_type
  static_query()
    noexcept(traits::static_query<T, InnerProperty>::is_noexcept)
  {
    return traits::static_query<T, InnerProperty>::value();
  }

  template <typename E, typename T = decltype(prefer_only::static_query<E>())>
  static constexpr const T static_query_v
    = prefer_only::static_query<E>();
#endif // defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

  template <typename Executor, typename Property>
  friend constexpr
  prefer_result_t<const Executor&, const InnerProperty&>
  prefer(const Executor& ex, const prefer_only<Property>& p,
      enable_if_t<
        is_same<Property, InnerProperty>::value
      >* = 0,
      enable_if_t<
        can_prefer<const Executor&, const InnerProperty&>::value
      >* = 0)
#if !defined(MXASIO_MSVC) \
  && !defined(__clang__) // Clang crashes if noexcept is used here.
    noexcept(is_nothrow_prefer<const Executor&, const InnerProperty&>::value)
#endif // !defined(MXASIO_MSVC)
       //   && !defined(__clang__)
  {
    return mxasio::prefer(ex, p.property);
  }

  template <typename Executor, typename Property>
  friend constexpr
  query_result_t<const Executor&, const InnerProperty&>
  query(const Executor& ex, const prefer_only<Property>& p,
      enable_if_t<
        is_same<Property, InnerProperty>::value
      >* = 0,
      enable_if_t<
        can_query<const Executor&, const InnerProperty&>::value
      >* = 0)
#if !defined(MXASIO_MSVC) \
  && !defined(__clang__) // Clang crashes if noexcept is used here.
    noexcept(is_nothrow_query<const Executor&, const InnerProperty&>::value)
#endif // !defined(MXASIO_MSVC)
       //   && !defined(__clang__)
  {
    return mxasio::query(ex, p.property);
  }
};

#if defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  && defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)
template <typename InnerProperty> template <typename E, typename T>
const T prefer_only<InnerProperty>::static_query_v;
#endif // defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   && defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

} // namespace execution

template <typename T, typename InnerProperty>
struct is_applicable_property<T, execution::prefer_only<InnerProperty>>
  : is_applicable_property<T, InnerProperty>
{
};

namespace traits {

#if !defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT) \
  || !defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

template <typename T, typename InnerProperty>
struct static_query<T, execution::prefer_only<InnerProperty>> :
  static_query<T, const InnerProperty&>
{
};

#endif // !defined(MXASIO_HAS_DEDUCED_STATIC_QUERY_TRAIT)
       //   || !defined(MXASIO_HAS_SFINAE_VARIABLE_TEMPLATES)

#if !defined(MXASIO_HAS_DEDUCED_PREFER_FREE_TRAIT)

template <typename T, typename InnerProperty>
struct prefer_free_default<T, execution::prefer_only<InnerProperty>,
    enable_if_t<
      can_prefer<const T&, const InnerProperty&>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_prefer<const T&, const InnerProperty&>::value;

  typedef prefer_result_t<const T&, const InnerProperty&> result_type;
};

#endif // !defined(MXASIO_HAS_DEDUCED_PREFER_FREE_TRAIT)

#if !defined(MXASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

template <typename T, typename InnerProperty>
struct query_free<T, execution::prefer_only<InnerProperty>,
    enable_if_t<
      can_query<const T&, const InnerProperty&>::value
    >
  >
{
  static constexpr bool is_valid = true;
  static constexpr bool is_noexcept =
    is_nothrow_query<const T&, const InnerProperty&>::value;

  typedef query_result_t<const T&, const InnerProperty&> result_type;
};

#endif // !defined(MXASIO_HAS_DEDUCED_QUERY_FREE_TRAIT)

} // namespace traits

#endif // defined(GENERATING_DOCUMENTATION)

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_EXECUTION_PREFER_ONLY_HPP
