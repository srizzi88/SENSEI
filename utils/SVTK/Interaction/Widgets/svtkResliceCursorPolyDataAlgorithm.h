/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorPolyDataAlgorithm
 * @brief   generates a 2D reslice cursor polydata
 *
 * svtkResliceCursorPolyDataAlgorithm is a class that generates a 2D
 * reslice cursor svtkPolyData, suitable for rendering within a
 * svtkResliceCursorActor. The class takes as input the reslice plane
 * normal index (an index into the normal plane maintained by the reslice
 * cursor object) and generates the polydata represeting the other two
 * reslice axes suitable for rendering on a slice through this plane.
 * The cursor consists of two intersection axes lines that meet at the
 * cursor focus. These lines may have a user defined thickness. They
 * need not be orthogonal to each other.
 * @sa
 * svtkResliceCursorActor svtkResliceCursor svtkResliceCursorWidget
 */

#ifndef svtkResliceCursorPolyDataAlgorithm_h
#define svtkResliceCursorPolyDataAlgorithm_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCutter;
class svtkResliceCursor;
class svtkPlane;
class svtkBox;
class svtkClipPolyData;
class svtkLinearExtrusionFilter;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorPolyDataAlgorithm : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkResliceCursorPolyDataAlgorithm, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkResliceCursorPolyDataAlgorithm* New();

  //@{
  /**
   * Which of the 3 axes defines the reslice plane normal ?
   */
  svtkSetMacro(ReslicePlaneNormal, int);
  svtkGetMacro(ReslicePlaneNormal, int);
  //@}

  enum
  {
    XAxis = 0,
    YAxis,
    ZAxis
  };

  /**
   * Set the planes that correspond to the reslice axes.
   */
  void SetReslicePlaneNormalToXAxis() { this->SetReslicePlaneNormal(XAxis); }
  void SetReslicePlaneNormalToYAxis() { this->SetReslicePlaneNormal(YAxis); }
  void SetReslicePlaneNormalToZAxis() { this->SetReslicePlaneNormal(ZAxis); }

  //@{
  /**
   * Set the Reslice cursor from which to generate the polydata representation
   */
  virtual void SetResliceCursor(svtkResliceCursor*);
  svtkGetObjectMacro(ResliceCursor, svtkResliceCursor);
  //@}

  //@{
  /**
   * Set/Get the slice bounds, ie the slice of this view on which to display
   * the reslice cursor.
   */
  svtkSetVector6Macro(SliceBounds, double);
  svtkGetVector6Macro(SliceBounds, double);
  //@}

  //@{
  /**
   * Get either one of the axes that this object produces. Depending on
   * the mode, one renders either the centerline axes or both the
   * centerline axes and the slab
   */
  virtual svtkPolyData* GetCenterlineAxis1();
  virtual svtkPolyData* GetCenterlineAxis2();
  virtual svtkPolyData* GetThickSlabAxis1();
  virtual svtkPolyData* GetThickSlabAxis2();
  //@}

  //@{
  /**
   * Get the index of the axes and the planes that they represent
   */
  virtual int GetAxis1();
  virtual int GetAxis2();
  virtual int GetPlaneAxis1();
  virtual int GetPlaneAxis2();
  //@}

  /**
   * Convenience method that, given one plane, returns the other plane
   * that this class represents.
   */
  int GetOtherPlaneForAxis(int p);

  /**
   * Get the MTime. Check the MTime of the internal ResliceCursor as well, if
   * one has been set
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkResliceCursorPolyDataAlgorithm();
  ~svtkResliceCursorPolyDataAlgorithm() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void GetSlabPolyData(int axis, int planeAxis, svtkPolyData* pd);

  virtual void CutAndClip(svtkPolyData* in, svtkPolyData* out);

  // Build the reslice slab axis
  void BuildResliceSlabAxisTopology();

  int ReslicePlaneNormal;
  svtkResliceCursor* ResliceCursor;
  svtkCutter* Cutter;
  svtkPlane* SlicePlane;
  svtkBox* Box;
  svtkClipPolyData* ClipWithBox;
  double SliceBounds[6];
  bool Extrude;
  svtkLinearExtrusionFilter* ExtrusionFilter1;
  svtkLinearExtrusionFilter* ExtrusionFilter2;
  svtkPolyData* ThickAxes[2];

private:
  svtkResliceCursorPolyDataAlgorithm(const svtkResliceCursorPolyDataAlgorithm&) = delete;
  void operator=(const svtkResliceCursorPolyDataAlgorithm&) = delete;
};

#endif
