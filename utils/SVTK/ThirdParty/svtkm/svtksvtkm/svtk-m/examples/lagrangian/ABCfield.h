//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Math.h>
class ABCfield
{

public:
  void calculateVelocity(double* location, double t, double* velocity)
  {
    double ep = 0.25;
    double period = 1.0;

    double sinval = ep * svtkm::Sin(period * t);

    velocity[0] = svtkm::Sin(location[2] + sinval) + svtkm::Cos(location[1] + sinval);
    velocity[1] = svtkm::Sin(location[0] + sinval) + svtkm::Cos(location[2] + sinval);
    velocity[2] = svtkm::Sin(location[1] + sinval) + svtkm::Cos(location[0] + sinval);
  }
};
