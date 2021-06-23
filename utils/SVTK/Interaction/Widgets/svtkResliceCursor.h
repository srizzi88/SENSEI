/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursor
 * @brief   Geometry for a reslice cursor
 *
 * This class represents a reslice cursor. It consists of two cross
 * sectional hairs, with an optional thickness. The crosshairs
 * hairs may have a hole in the center. These may be translated or rotated
 * independent of each other in the view. The result is used to reslice
 * the data along these cross sections. This allows the user to perform
 * multi-planar thin or thick reformat of the data on an image view, rather
 * than a 3D view.
 * @sa
 * svtkResliceCursorWidget svtkResliceCursor svtkResliceCursorPolyDataAlgorithm
 * svtkResliceCursorRepresentation svtkResliceCursorThickLineRepresentation
 * svtkResliceCursorActor svtkImagePlaneWidget
 */

#ifndef svtkResliceCursor_h
#define svtkResliceCursor_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class svtkImageData;
class svtkPolyData;
class svtkPlane;
class svtkPlaneCollection;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursor : public svtkObject
{
public:
  svtkTypeMacro(svtkResliceCursor, svtkObject);

  static svtkResliceCursor* New();

  //@{
  /**
   * Set the image (3D) that we are slicing
   */
  virtual void SetImage(svtkImageData*);
  svtkGetObjectMacro(Image, svtkImageData);
  //@}

  //@{
  /**
   * Set/Get the cente of the reslice cursor.
   */
  virtual void SetCenter(double, double, double);
  virtual void SetCenter(double center[3]);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the thickness of the cursor
   */
  svtkSetVector3Macro(Thickness, double);
  svtkGetVector3Macro(Thickness, double);
  //@}

  //@{
  /**
   * Enable disable thick mode. Default is to enable it.
   */
  svtkSetMacro(ThickMode, svtkTypeBool);
  svtkGetMacro(ThickMode, svtkTypeBool);
  svtkBooleanMacro(ThickMode, svtkTypeBool);
  //@}

  /**
   * Get the 3D PolyData representation
   */
  virtual svtkPolyData* GetPolyData();

  /**
   * Get the slab and centerline polydata along an axis
   */
  virtual svtkPolyData* GetCenterlineAxisPolyData(int axis);

  /**
   * Printself method.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the planes that represent normals along the X, Y and Z. The argument
   * passed to this method must be an integer in the range 0-2 (corresponding
   * to the X, Y and Z axes.
   */
  virtual svtkPlane* GetPlane(int n);

  /**
   * Build the polydata
   */
  virtual void Update();

  //@{
  /**
   * Get the computed axes directions
   */
  svtkGetVector3Macro(XAxis, double);
  svtkGetVector3Macro(YAxis, double);
  svtkGetVector3Macro(ZAxis, double);
  svtkSetVector3Macro(XAxis, double);
  svtkSetVector3Macro(YAxis, double);
  svtkSetVector3Macro(ZAxis, double);
  virtual double* GetAxis(int i);
  //@}

  //@{
  /**
   * Show a hole in the center of the cursor, so its easy to see the pixels
   * within the hole. ON by default
   */
  svtkSetMacro(Hole, int);
  svtkGetMacro(Hole, int);
  //@}

  //@{
  /**
   * Set the width of the hole in mm
   */
  svtkSetMacro(HoleWidth, double);
  svtkGetMacro(HoleWidth, double);
  //@}

  //@{
  /**
   * Set the width of the hole in pixels. If set, this will override the
   * hole with in mm.
   */
  svtkSetMacro(HoleWidthInPixels, double);
  svtkGetMacro(HoleWidthInPixels, double);
  //@}

  /**
   * Get the MTime. Check the MTime of the internal planes as well.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Reset the cursor to the default position, ie with the axes, normal
   * to each other and axis aligned and with the cursor pointed at the
   * center of the image.
   */
  virtual void Reset();

protected:
  svtkResliceCursor();
  ~svtkResliceCursor() override;

  virtual void BuildCursorGeometry();
  virtual void BuildPolyData();
  virtual void BuildCursorTopology();
  virtual void BuildCursorTopologyWithHole();
  virtual void BuildCursorTopologyWithoutHole();
  virtual void BuildCursorGeometryWithoutHole();
  virtual void BuildCursorGeometryWithHole();
  virtual void ComputeAxes();

  svtkTypeBool ThickMode;
  int Hole;
  double HoleWidth;
  double HoleWidthInPixels;
  double Thickness[3];
  double Center[3];
  double XAxis[3];
  double YAxis[3];
  double ZAxis[3];
  svtkImageData* Image;
  svtkPolyData* PolyData;

  svtkPolyData* CenterlineAxis[3];

  svtkPlaneCollection* ReslicePlanes;
  svtkTimeStamp PolyDataBuildTime;

private:
  svtkResliceCursor(const svtkResliceCursor&) = delete;
  void operator=(const svtkResliceCursor&) = delete;
};

#endif
