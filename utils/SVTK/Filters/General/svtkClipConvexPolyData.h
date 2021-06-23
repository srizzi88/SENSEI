/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipConvexPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClipConvexPolyData
 * @brief   clip any dataset with user-specified implicit function or input scalar data
 *
 * svtkClipConvexPolyData is a filter that clips a convex polydata with a set
 * of planes. Its main usage is for clipping a bounding volume with frustum
 * planes (used later one in volume rendering).
 */

#ifndef svtkClipConvexPolyData_h
#define svtkClipConvexPolyData_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPlaneCollection;
class svtkPlane;
class svtkClipConvexPolyDataInternals;

class SVTKFILTERSGENERAL_EXPORT svtkClipConvexPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkClipConvexPolyData* New();
  svtkTypeMacro(svtkClipConvexPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set all the planes at once using a svtkPlanes implicit function.
   * This also sets the D value.
   */
  void SetPlanes(svtkPlaneCollection* planes);
  svtkGetObjectMacro(Planes, svtkPlaneCollection);
  //@}

  /**
   * Redefines this method, as this filter depends on time of its components
   * (planes)
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkClipConvexPolyData();
  ~svtkClipConvexPolyData() override;

  // The method that does it all...
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Clip the input with a given plane `p'.
   * tolerance ?
   */
  void ClipWithPlane(svtkPlane* p, double tolerance);

  /**
   * Tells if clipping the input by plane `p' creates some degeneracies.
   */
  bool HasDegeneracies(svtkPlane* p);

  /**
   * Delete calculation data.
   */
  void ClearInternals();

  /**
   * ?
   */
  void ClearNewVertices();

  /**
   * ?
   */
  void RemoveEmptyPolygons();

  svtkPlaneCollection* Planes;
  svtkClipConvexPolyDataInternals* Internal;

private:
  svtkClipConvexPolyData(const svtkClipConvexPolyData&) = delete;
  void operator=(const svtkClipConvexPolyData&) = delete;
};

#endif
