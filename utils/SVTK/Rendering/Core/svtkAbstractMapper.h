/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractMapper
 * @brief   abstract class specifies interface to map data
 *
 * svtkAbstractMapper is an abstract class to specify interface between data and
 * graphics primitives or software rendering techniques. Subclasses of
 * svtkAbstractMapper can be used for rendering 2D data, geometry, or volumetric
 * data.
 *
 * @sa
 * svtkAbstractMapper3D svtkMapper svtkPolyDataMapper svtkVolumeMapper
 */

#ifndef svtkAbstractMapper_h
#define svtkAbstractMapper_h

#include "svtkAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro

#define SVTK_SCALAR_MODE_DEFAULT 0
#define SVTK_SCALAR_MODE_USE_POINT_DATA 1
#define SVTK_SCALAR_MODE_USE_CELL_DATA 2
#define SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA 3
#define SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA 4
#define SVTK_SCALAR_MODE_USE_FIELD_DATA 5

#define SVTK_GET_ARRAY_BY_ID 0
#define SVTK_GET_ARRAY_BY_NAME 1

class svtkAbstractArray;
class svtkDataSet;
class svtkPlane;
class svtkPlaneCollection;
class svtkPlanes;
class svtkTimerLog;
class svtkWindow;

class SVTKRENDERINGCORE_EXPORT svtkAbstractMapper : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkAbstractMapper, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Override Modifiedtime as we have added Clipping planes
   */
  svtkMTimeType GetMTime() override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) {}

  //@{
  /**
   * Get the time required to draw the geometry last time it was rendered
   */
  svtkGetMacro(TimeToDraw, double);
  //@}

  //@{
  /**
   * Specify clipping planes to be applied when the data is mapped
   * (at most 6 clipping planes can be specified).
   */
  void AddClippingPlane(svtkPlane* plane);
  void RemoveClippingPlane(svtkPlane* plane);
  void RemoveAllClippingPlanes();
  //@}

  //@{
  /**
   * Get/Set the svtkPlaneCollection which specifies the
   * clipping planes.
   */
  virtual void SetClippingPlanes(svtkPlaneCollection*);
  svtkGetObjectMacro(ClippingPlanes, svtkPlaneCollection);
  //@}

  /**
   * An alternative way to set clipping planes: use up to six planes found
   * in the supplied instance of the implicit function svtkPlanes.
   */
  void SetClippingPlanes(svtkPlanes* planes);

  /**
   * Make a shallow copy of this mapper.
   */
  virtual void ShallowCopy(svtkAbstractMapper* m);

  /**
   * Internal helper function for getting the active scalars. The scalar
   * mode indicates where the scalars come from.  The cellFlag is a
   * return value that is set when the scalars actually are cell scalars.
   * (0 for point scalars, 1 for cell scalars, 2 for field scalars)
   * The arrayAccessMode is used to indicate how to retrieve the scalars from
   * field data, per id or per name (if the scalarMode indicates that).
   */
  static svtkDataArray* GetScalars(svtkDataSet* input, int scalarMode, int arrayAccessMode,
    int arrayId, const char* arrayName, int& cellFlag);

  /**
   * Internal helper function for getting the active scalars as an
   * abstract array. The scalar mode indicates where the scalars come
   * from.  The cellFlag is a return value that is set when the
   * scalars actually are cell scalars.  (0 for point scalars, 1 for
   * cell scalars, 2 for field scalars) The arrayAccessMode is used to
   * indicate how to retrieve the scalars from field data, per id or
   * per name (if the scalarMode indicates that).
   */
  static svtkAbstractArray* GetAbstractScalars(svtkDataSet* input, int scalarMode,
    int arrayAccessMode, int arrayId, const char* arrayName, int& cellFlag);

  /**
   * Get the number of clipping planes.
   */
  int GetNumberOfClippingPlanes();

protected:
  svtkAbstractMapper();
  ~svtkAbstractMapper() override;

  svtkTimerLog* Timer;
  double TimeToDraw;
  svtkWindow* LastWindow; // Window used for the previous render
  svtkPlaneCollection* ClippingPlanes;

private:
  svtkAbstractMapper(const svtkAbstractMapper&) = delete;
  void operator=(const svtkAbstractMapper&) = delete;
};

#endif
