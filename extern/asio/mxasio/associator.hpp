//
// associator.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_ASSOCIATOR_HPP
#define MXASIO_ASSOCIATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

/// Used to generically specialise associators for a type.
template <template <typename, typename> class Associator,
    typename T, typename DefaultCandidate>
struct associator
{
};

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_ASSOCIATOR_HPP
