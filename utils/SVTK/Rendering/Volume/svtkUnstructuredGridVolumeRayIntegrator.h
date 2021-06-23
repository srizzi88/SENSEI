/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeRayIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkUnstructuredGridVolumeRayIntegrator
 * @brief   a superclass for volume ray integration functions
 *
 *
 *
 * svtkUnstructuredGridVolumeRayIntegrator is a superclass for ray
 * integration functions that can be used within a
 * svtkUnstructuredGridVolumeRayCastMapper.
 *
 * @sa
 * svtkUnstructuredGridVolumeRayCastMapper
 * svtkUnstructuredGridVolumeRayCastFunction
 */

#ifndef svtkUnstructuredGridVolumeRayIntegrator_h
#define svtkUnstructuredGridVolumeRayIntegrator_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkVolume;
class svtkDoubleArray;
class svtkDataArray;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeRayIntegrator : public svtkObject
{
public:
  svtkTypeMacro(svtkUnstructuredGridVolumeRayIntegrator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set up the integrator with the given properties and scalars.
   */
  virtual void Initialize(svtkVolume* volume, svtkDataArray* scalars) = 0;

  /**
   * Given a set of intersections (defined by the three arrays), compute
   * the piecewise integration of the array in front to back order.
   * /c intersectionLengths holds the lengths of each piecewise segment.
   * /c nearIntersections and /c farIntersections hold the scalar values at
   * the front and back of each segment.  /c color should contain the RGBA
   * value of the volume in front of the segments passed in, and the result
   * will be placed back into /c color.
   */
  virtual void Integrate(svtkDoubleArray* intersectionLengths, svtkDataArray* nearIntersections,
    svtkDataArray* farIntersections, float color[4]) = 0;

protected:
  svtkUnstructuredGridVolumeRayIntegrator();
  ~svtkUnstructuredGridVolumeRayIntegrator() override;

private:
  svtkUnstructuredGridVolumeRayIntegrator(const svtkUnstructuredGridVolumeRayIntegrator&) = delete;
  void operator=(const svtkUnstructuredGridVolumeRayIntegrator&) = delete;
};

#endif
