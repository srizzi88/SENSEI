/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorDot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVectorDot
 * @brief   generate scalars from dot product of vectors and normals (e.g., show displacement plot)
 *
 * svtkVectorDot is a filter to generate point scalar values from a dataset.
 * The scalar value at a point is created by computing the dot product
 * between the normal and vector at each point. Combined with the appropriate
 * color map, this can show nodal lines/mode shapes of vibration, or a
 * displacement plot.
 *
 * Note that by default the resulting scalars are mapped into a specified
 * range. This requires an extra pass in the algorithm. This mapping pass can
 * be disabled (set MapScalars to off).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef svtkVectorDot_h
#define svtkVectorDot_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkVectorDot : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkVectorDot, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with scalar range (-1,1).
   */
  static svtkVectorDot* New();

  //@{
  /**
   * Enable/disable the mapping of scalars into a specified range. This will
   * significantly improve the performance of the algorithm but the resulting
   * scalar values will strictly be a function of the vector and normal
   * data. By default, MapScalars is enabled, and the output scalar
   * values will fall into the range ScalarRange.
   */
  svtkSetMacro(MapScalars, svtkTypeBool);
  svtkGetMacro(MapScalars, svtkTypeBool);
  svtkBooleanMacro(MapScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the range into which to map the scalars. This mapping only
   * occurs if MapScalars is enabled.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVectorMacro(ScalarRange, double, 2);
  //@}

  //@{
  /**
   * Return the actual range of the generated scalars (prior to mapping).
   * Note that the data is valid only after the filter executes.
   */
  svtkGetVectorMacro(ActualRange, double, 2);
  //@}

protected:
  svtkVectorDot();
  ~svtkVectorDot() override {}

  svtkTypeBool MapScalars;
  double ScalarRange[2];
  double ActualRange[2];

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkVectorDot(const svtkVectorDot&) = delete;
  void operator=(const svtkVectorDot&) = delete;
};

#endif
