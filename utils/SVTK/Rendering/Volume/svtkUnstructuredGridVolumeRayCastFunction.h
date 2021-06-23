/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeRayCastFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkUnstructuredGridVolumeRayCastFunction
 * @brief   a superclass for ray casting functions
 *
 *
 * svtkUnstructuredGridVolumeRayCastFunction is a superclass for ray casting functions that
 * can be used within a svtkUnstructuredGridVolumeRayCastMapper.
 *
 * @sa
 * svtkUnstructuredGridVolumeRayCastMapper svtkUnstructuredGridVolumeRayIntegrator
 */

#ifndef svtkUnstructuredGridVolumeRayCastFunction_h
#define svtkUnstructuredGridVolumeRayCastFunction_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkRenderer;
class svtkVolume;
class svtkUnstructuredGridVolumeRayCastIterator;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeRayCastFunction : public svtkObject
{
public:
  svtkTypeMacro(svtkUnstructuredGridVolumeRayCastFunction, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void Initialize(svtkRenderer* ren, svtkVolume* vol) = 0;

  virtual void Finalize() = 0;

  /**
   * Returns a new object that will iterate over all the intersections of a
   * ray with the cells of the input.  The calling code is responsible for
   * deleting the returned object.
   */
  SVTK_NEWINSTANCE
  virtual svtkUnstructuredGridVolumeRayCastIterator* NewIterator() = 0;

protected:
  svtkUnstructuredGridVolumeRayCastFunction() {}
  ~svtkUnstructuredGridVolumeRayCastFunction() override {}

private:
  svtkUnstructuredGridVolumeRayCastFunction(
    const svtkUnstructuredGridVolumeRayCastFunction&) = delete;
  void operator=(const svtkUnstructuredGridVolumeRayCastFunction&) = delete;
};

#endif
