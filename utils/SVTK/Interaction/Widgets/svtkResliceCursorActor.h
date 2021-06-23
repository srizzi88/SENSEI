/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorActor
 * @brief   Represent a reslice cursor
 *
 * A reslice cursor consists of a pair of lines (cross hairs), thin or thick,
 * that may be interactively manipulated for thin/thick reformats through the
 * data.
 * @sa
 * svtkResliceCursor svtkResliceCursorPolyDataAlgorithm svtkResliceCursorWidget
 * svtkResliceCursorRepresentation svtkResliceCursorLineRepresentation
 */

#ifndef svtkResliceCursorActor_h
#define svtkResliceCursorActor_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkProp3D.h"

class svtkResliceCursor;
class svtkResliceCursorPolyDataAlgorithm;
class svtkPolyDataMapper;
class svtkActor;
class svtkProperty;
class svtkBoundingBox;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorActor : public svtkProp3D
{

public:
  //@{
  /**
   * Standard SVTK methods
   */
  static svtkResliceCursorActor* New();
  svtkTypeMacro(svtkResliceCursorActor, svtkProp3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the cursor algorithm. The cursor must be set on the algorithm
   */
  svtkGetObjectMacro(CursorAlgorithm, svtkResliceCursorPolyDataAlgorithm);
  //@}

  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  svtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double* GetBounds() override;

  /**
   * Get the actors mtime plus consider its algorithm.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Get property of the internal actor.
   */
  svtkProperty* GetCenterlineProperty(int i);
  svtkProperty* GetThickSlabProperty(int i);
  //@}

  /**
   * Get the centerline actor along a particular axis
   */
  svtkActor* GetCenterlineActor(int axis);

  /**
   * Set the user matrix on all the internal actors.
   */
  virtual void SetUserMatrix(svtkMatrix4x4* matrix);

protected:
  svtkResliceCursorActor();
  ~svtkResliceCursorActor() override;

  void UpdateViewProps(svtkViewport* v = nullptr);
  void UpdateHoleSize(svtkViewport* v);

  svtkResliceCursorPolyDataAlgorithm* CursorAlgorithm;
  svtkPolyDataMapper* CursorCenterlineMapper[3];
  svtkActor* CursorCenterlineActor[3];
  svtkPolyDataMapper* CursorThickSlabMapper[3];
  svtkActor* CursorThickSlabActor[3];
  svtkProperty* CenterlineProperty[3];
  svtkProperty* ThickSlabProperty[3];

private:
  svtkResliceCursorActor(const svtkResliceCursorActor&) = delete;
  void operator=(const svtkResliceCursorActor&) = delete;
};

#endif
