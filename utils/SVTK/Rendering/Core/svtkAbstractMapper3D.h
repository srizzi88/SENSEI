/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractMapper3D
 * @brief   abstract class specifies interface to map 3D data
 *
 * svtkAbstractMapper3D is an abstract class to specify interface between 3D
 * data and graphics primitives or software rendering techniques. Subclasses
 * of svtkAbstractMapper3D can be used for rendering geometry or rendering
 * volumetric data.
 *
 * This class also defines an API to support hardware clipping planes (at most
 * six planes can be defined). It also provides geometric data about the input
 * data it maps, such as the bounding box and center.
 *
 * @sa
 * svtkAbstractMapper svtkMapper svtkPolyDataMapper svtkVolumeMapper
 */

#ifndef svtkAbstractMapper3D_h
#define svtkAbstractMapper3D_h

#include "svtkAbstractMapper.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkWindow;
class svtkDataSet;
class svtkMatrix4x4;

class SVTKRENDERINGCORE_EXPORT svtkAbstractMapper3D : public svtkAbstractMapper
{
public:
  svtkTypeMacro(svtkAbstractMapper3D, svtkAbstractMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * Update this->Bounds as a side effect.
   */
  virtual double* GetBounds() SVTK_SIZEHINT(6) = 0;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  virtual void GetBounds(double bounds[6]);

  //@{
  /**
   * Return the Center of this mapper's data.
   */
  double* GetCenter() SVTK_SIZEHINT(3);
  void GetCenter(double center[3])
  {
    double* rc = this->GetCenter();
    center[0] = rc[0];
    center[1] = rc[1];
    center[2] = rc[2];
  }
  //@}

  /**
   * Return the diagonal length of this mappers bounding box.
   */
  double GetLength();

  /**
   * Is this a ray cast mapper? A subclass would return 1 if the
   * ray caster is needed to generate an image from this mapper.
   */
  virtual svtkTypeBool IsARayCastMapper() { return 0; }

  /**
   * Is this a "render into image" mapper? A subclass would return 1 if the
   * mapper produces an image by rendering into a software image buffer.
   */
  virtual svtkTypeBool IsARenderIntoImageMapper() { return 0; }

  /**
   * Get the ith clipping plane as a homogeneous plane equation.
   * Use GetNumberOfClippingPlanes to get the number of planes.
   */
  void GetClippingPlaneInDataCoords(svtkMatrix4x4* propMatrix, int i, double planeEquation[4]);

protected:
  svtkAbstractMapper3D();
  ~svtkAbstractMapper3D() override {}

  double Bounds[6];
  double Center[3];

private:
  svtkAbstractMapper3D(const svtkAbstractMapper3D&) = delete;
  void operator=(const svtkAbstractMapper3D&) = delete;
};

#endif
