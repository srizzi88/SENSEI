/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkmPointTransform
 * @brief transform points via svtkm PointTransform filter
 *
 * svtkmPointTransform is a filter to transform point coordinates. For now it
 * does not support transforming associated point normals and vectors, as well
 * as cell normals and vectors with the point coordinates.
 */

#ifndef svtkmPointTransform_h
#define svtkmPointTransform_h

#include "svtkAcceleratorsSVTKmModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkHomogeneousTransform;

class SVTKACCELERATORSSVTKM_EXPORT svtkmPointTransform : public svtkPointSetAlgorithm
{
public:
  svtkTypeMacro(svtkmPointTransform, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmPointTransform* New();

  //@{
  /**
   * Specify the transform object used to transform the points
   */
  void SetTransform(svtkHomogeneousTransform* tf);
  svtkGetObjectMacro(Transform, svtkHomogeneousTransform);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkmPointTransform();
  ~svtkmPointTransform() override;
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkHomogeneousTransform* Transform;

private:
  svtkmPointTransform(const svtkmPointTransform&) = delete;
  void operator=(const svtkmPointTransform&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkmPointTransform.h
