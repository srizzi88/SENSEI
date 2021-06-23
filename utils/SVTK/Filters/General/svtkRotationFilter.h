/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRotationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRotationFilter
 * @brief   Duplicates a data set by rotation about an axis
 *
 * The svtkRotationFilter duplicates a data set by rotation about one of the
 * 3 axis of the dataset's reference.
 * Since it converts data sets into unstructured grids, it is not efficient
 * for structured data sets.
 *
 * @par Thanks:
 * Theophane Foggia of The Swiss National Supercomputing Centre (CSCS)
 * for creating and contributing this filter
 */

#ifndef svtkRotationFilter_h
#define svtkRotationFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkRotationFilter : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkRotationFilter* New();
  svtkTypeMacro(svtkRotationFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum RotationAxis
  {
    USE_X = 0,
    USE_Y = 1,
    USE_Z = 2
  };

  //@{
  /**
   * Set the axis of rotation to use. It is set by default to Z.
   */
  svtkSetClampMacro(Axis, int, 0, 2);
  svtkGetMacro(Axis, int);
  void SetAxisToX() { this->SetAxis(USE_X); }
  void SetAxisToY() { this->SetAxis(USE_Y); }
  void SetAxisToZ() { this->SetAxis(USE_Z); }
  //@}

  //@{
  /**
   * Set the rotation angle to use.
   */
  svtkSetMacro(Angle, double);
  svtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Set the rotation center coordinates.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set the number of copies to create. The source will be rotated N times
   * and a new polydata copy of the original created at each angular position
   * All copies will be appended to form a single output
   */
  svtkSetMacro(NumberOfCopies, int);
  svtkGetMacro(NumberOfCopies, int);
  //@}

  //@{
  /**
   * If on (the default), copy the input geometry to the output. If off,
   * the output will only contain the rotation.
   */
  svtkSetMacro(CopyInput, svtkTypeBool);
  svtkGetMacro(CopyInput, svtkTypeBool);
  svtkBooleanMacro(CopyInput, svtkTypeBool);
  //@}

protected:
  svtkRotationFilter();
  ~svtkRotationFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int Axis;
  double Angle;
  double Center[3];
  int NumberOfCopies;
  svtkTypeBool CopyInput;

private:
  svtkRotationFilter(const svtkRotationFilter&) = delete;
  void operator=(const svtkRotationFilter&) = delete;
};

#endif
