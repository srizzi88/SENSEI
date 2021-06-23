/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPTextureMapToSphere.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPTextureMapToSphere
 * @brief   generate texture coordinates by mapping points to sphere
 *
 * svtkPTextureMapToSphere inherits from svtkTextureMapToSphere to handle multi-processing
 * environment.
 *
 * @sa
 * svtkTextureMapToPlane svtkTextureMapToCylinder
 * svtkTransformTexture svtkThresholdTextureCoords
 * svtkTextureMapToSphere
 */

#ifndef svtkPTextureMapToSphere_h
#define svtkPTextureMapToSphere_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkTextureMapToSphere.h"

class svtkDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPTextureMapToSphere : public svtkTextureMapToSphere
{
public:
  svtkTypeMacro(svtkPTextureMapToSphere, svtkTextureMapToSphere);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPTextureMapToSphere* New();

protected:
  svtkPTextureMapToSphere();
  ~svtkPTextureMapToSphere() override = default;

  virtual void ComputeCenter(svtkDataSet* dataSet) override;

  svtkMultiProcessController* Controller;

private:
  svtkPTextureMapToSphere(const svtkPTextureMapToSphere&) = delete;
  void operator=(const svtkPTextureMapToSphere&) = delete;
};

#endif
