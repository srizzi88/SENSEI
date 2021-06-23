//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagKeysIn_h
#define svtk_m_cont_arg_TransportTagKeysIn_h

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for keys in a reduce by key.
///
/// \c TransportTagKeysIn is a tag used with the \c Transport class to
/// transport svtkm::worklet::Keys objects for the input domain of a
/// reduce by keys worklet.
///
struct TransportTagKeysIn
{
};

// Specialization of Transport class for TransportTagKeysIn is implemented in
// svtkm/worklet/Keys.h. That class is not accessible from here due to SVTK-m
// package dependencies.
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagKeysIn_h
