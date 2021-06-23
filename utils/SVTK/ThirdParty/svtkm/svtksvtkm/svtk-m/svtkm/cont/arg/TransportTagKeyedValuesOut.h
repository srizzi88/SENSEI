//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagKeyedValuesOut_h
#define svtk_m_cont_arg_TransportTagKeyedValuesOut_h

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input values in a reduce by key.
///
/// \c TransportTagKeyedValuesOut is a tag used with the \c Transport class
/// to transport \c ArrayHandle objects for output values. The values are
/// rearranged and grouped based on the keys they are associated with.
///
struct TransportTagKeyedValuesOut
{
};

// Specialization of Transport class for TransportTagKeyedValuesOut is
// implemented in svtkm/worklet/Keys.h. That class is not accessible from here
// due to SVTK-m package dependencies.
}
}
}

#endif //svtk_m_cont_arg_TransportTagKeyedValuesOut_h
