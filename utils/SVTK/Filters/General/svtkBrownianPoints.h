/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBrownianPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBrownianPoints
 * @brief   assign random vector to points
 *
 * svtkBrownianPoints is a filter object that assigns a random vector (i.e.,
 * magnitude and direction) to each point. The minimum and maximum speed
 * values can be controlled by the user.
 *
 * @sa
 * svtkRandomAttributeGenerator
 */

#ifndef svtkBrownianPoints_h
#define svtkBrownianPoints_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkBrownianPoints : public svtkDataSetAlgorithm
{
public:
  /**
   * Create instance with minimum speed 0.0, maximum speed 1.0.
   */
  static svtkBrownianPoints* New();

  svtkTypeMacro(svtkBrownianPoints, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the minimum speed value.
   */
  svtkSetClampMacro(MinimumSpeed, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MinimumSpeed, double);
  //@}

  //@{
  /**
   * Set the maximum speed value.
   */
  svtkSetClampMacro(MaximumSpeed, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumSpeed, double);
  //@}

protected:
  svtkBrownianPoints();
  ~svtkBrownianPoints() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double MinimumSpeed;
  double MaximumSpeed;

private:
  svtkBrownianPoints(const svtkBrownianPoints&) = delete;
  void operator=(const svtkBrownianPoints&) = delete;
};

#endif
