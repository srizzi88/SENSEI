/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorPicker
 * @brief   ray-cast cell picker for the reslice cursor
 *
 * This class is used by the svtkResliceCursorWidget to pick reslice axes
 * drawn by a svtkResliceCursorActor. The class returns the axes picked if
 * any, whether one has picked the center. It takes as input an instance
 * of svtkResliceCursorPolyDataAlgorithm. This is all done internally by
 * svtkResliceCursorWidget and as such users are not expected to use this
 * class directly, unless they are overriding the behaviour of
 * svtkResliceCursorWidget.
 * @sa
 * svtkResliceCursor svtkResliceCursorWidget
 */

#ifndef svtkResliceCursorPicker_h
#define svtkResliceCursorPicker_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPicker.h"

class svtkPolyData;
class svtkGenericCell;
class svtkResliceCursorPolyDataAlgorithm;
class svtkMatrix4x4;
class svtkPlane;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorPicker : public svtkPicker
{
public:
  static svtkResliceCursorPicker* New();
  svtkTypeMacro(svtkResliceCursorPicker, svtkPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values are the (x,y) pixel coordinates for the pick, and
   * the third value is z=0. The return value will be non-zero if
   * something was successfully picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer) override;

  //@{
  /**
   * Get the picked axis
   */
  svtkGetMacro(PickedAxis1, int);
  svtkGetMacro(PickedAxis2, int);
  svtkGetMacro(PickedCenter, int);
  //@}

  //@{
  /**
   * Set the reslice cursor algorithm. One must be set
   */
  virtual void SetResliceCursorAlgorithm(svtkResliceCursorPolyDataAlgorithm*);
  svtkGetObjectMacro(ResliceCursorAlgorithm, svtkResliceCursorPolyDataAlgorithm);
  //@}

  virtual void SetTransformMatrix(svtkMatrix4x4*);

  /**
   * Overloaded pick method that returns the picked coordinates of the current
   * resliced plane in world coordinates when given a display position
   */
  void Pick(double displayPos[2], double world[3], svtkRenderer* ren);

protected:
  svtkResliceCursorPicker();
  ~svtkResliceCursorPicker() override;

  virtual int IntersectPolyDataWithLine(double p1[3], double p2[3], svtkPolyData*, double tol);
  virtual int IntersectPointWithLine(double p1[3], double p2[3], double X[3], double tol);

  void TransformPlane();
  void TransformPoint(double pIn[4], double pOut[4]);
  void InverseTransformPoint(double pIn[4], double pOut[4]);

private:
  svtkGenericCell* Cell; // used to accelerate picking
  svtkResliceCursorPolyDataAlgorithm* ResliceCursorAlgorithm;

  int PickedAxis1;
  int PickedAxis2;
  int PickedCenter;
  svtkMatrix4x4* TransformMatrix;
  svtkPlane* Plane;

private:
  svtkResliceCursorPicker(const svtkResliceCursorPicker&) = delete;
  void operator=(const svtkResliceCursorPicker&) = delete;
};

#endif
