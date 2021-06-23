/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCenterOfMass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCenterOfMass
 * @brief   Find the center of mass of a set of points.
 *
 * svtkCenterOfMass finds the "center of mass" of a svtkPointSet (svtkPolyData
 * or svtkUnstructuredGrid). Optionally, the user can specify to use the scalars
 * as weights in the computation. If this option, UseScalarsAsWeights, is off,
 * each point contributes equally in the calculation.
 *
 * You must ensure Update() has been called before GetCenter will produce a valid
 * value.
 */

#ifndef svtkCenterOfMass_h
#define svtkCenterOfMass_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkPoints;
class svtkDataArray;

class SVTKFILTERSCORE_EXPORT svtkCenterOfMass : public svtkPointSetAlgorithm
{
public:
  static svtkCenterOfMass* New();
  svtkTypeMacro(svtkCenterOfMass, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of the center of mass computation.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set a flag to determine if the points are weighted.
   */
  svtkSetMacro(UseScalarsAsWeights, bool);
  svtkGetMacro(UseScalarsAsWeights, bool);
  //@}

  /**
   * This function is called by RequestData. It exists so that
   * other classes may use this computation without constructing
   * a svtkCenterOfMass object.  The scalars can be set to nullptr
   * if all points are to be weighted equally.  If scalars are
   * used, it is the caller's responsibility to ensure that the
   * number of scalars matches the number of points, and that
   * the sum of the scalars is a positive value.
   */
  static void ComputeCenterOfMass(svtkPoints* input, svtkDataArray* scalars, double center[3]);

protected:
  svtkCenterOfMass();

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkCenterOfMass(const svtkCenterOfMass&) = delete;
  void operator=(const svtkCenterOfMass&) = delete;

  bool UseScalarsAsWeights;
  double Center[3];
};

#endif
