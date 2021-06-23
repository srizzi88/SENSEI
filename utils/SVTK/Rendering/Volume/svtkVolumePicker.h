/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumePicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumePicker
 * @brief   ray-cast picker enhanced for volumes
 *
 * svtkVolumePicker is a subclass of svtkCellPicker.  It has one
 * advantage over svtkCellPicker for volumes: it will be able to
 * correctly perform picking when CroppingPlanes are present.  This
 * isn't possible for svtkCellPicker since it doesn't link to
 * the VolumeRendering classes and hence cannot access information
 * about the CroppingPlanes.
 *
 * @sa
 * svtkPicker svtkPointPicker svtkCellPicker
 *
 * @par Thanks:
 * This class was contributed to SVTK by David Gobbi on behalf of Atamai Inc.
 */

#ifndef svtkVolumePicker_h
#define svtkVolumePicker_h

#include "svtkCellPicker.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class SVTKRENDERINGVOLUME_EXPORT svtkVolumePicker : public svtkCellPicker
{
public:
  static svtkVolumePicker* New();
  svtkTypeMacro(svtkVolumePicker, svtkCellPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set whether to pick the cropping planes of props that have them.
   * If this is set, then the pick will be done on the cropping planes
   * rather than on the data. The GetCroppingPlaneId() method will return
   * the index of the cropping plane of the volume that was picked.  This
   * setting is only relevant to the picking of volumes.
   */
  svtkSetMacro(PickCroppingPlanes, svtkTypeBool);
  svtkBooleanMacro(PickCroppingPlanes, svtkTypeBool);
  svtkGetMacro(PickCroppingPlanes, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the index of the cropping plane that the pick ray passed
   * through on its way to the prop. This will be set regardless
   * of whether PickCroppingPlanes is on.  The crop planes are ordered
   * as follows: xmin, xmax, ymin, ymax, zmin, zmax.  If the volume is
   * not cropped, the value will bet set to -1.
   */
  svtkGetMacro(CroppingPlaneId, int);
  //@}

protected:
  svtkVolumePicker();
  ~svtkVolumePicker() override;

  void ResetPickInfo() override;

  double IntersectVolumeWithLine(const double p1[3], const double p2[3], double t1, double t2,
    svtkProp3D* prop, svtkAbstractVolumeMapper* mapper) override;

  static int ClipLineWithCroppingRegion(const double bounds[6], const int extent[6], int flags,
    const double x1[3], const double x2[3], double t1, double t2, int& extentPlaneId,
    int& numSegments, double* t1List, double* t2List, double* s1List, int* planeIdList);

  svtkTypeBool PickCroppingPlanes;
  int CroppingPlaneId;

private:
  svtkVolumePicker(const svtkVolumePicker&) = delete;
  void operator=(const svtkVolumePicker&) = delete;
};

#endif
