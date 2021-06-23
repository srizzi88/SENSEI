/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectVisiblePoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSelectVisiblePoints
 * @brief   extract points that are visible (based on z-buffer calculation)
 *
 * svtkSelectVisiblePoints is a filter that selects points based on
 * whether they are visible or not. Visibility is determined by
 * accessing the z-buffer of a rendering window. (The position of each
 * input point is converted into display coordinates, and then the
 * z-value at that point is obtained. If within the user-specified
 * tolerance, the point is considered visible.)
 *
 * Points that are visible (or if the ivar SelectInvisible is on,
 * invisible points) are passed to the output. Associated data
 * attributes are passed to the output as well.
 *
 * This filter also allows you to specify a rectangular window in display
 * (pixel) coordinates in which the visible points must lie. This can be
 * used as a sort of local "brushing" operation to select just data within
 * a window.
 *
 *
 * @warning
 * You must carefully synchronize the execution of this filter. The
 * filter refers to a renderer, which is modified every time a render
 * occurs. Therefore, the filter is always out of date, and always
 * executes. You may have to perform two rendering passes, or if you
 * are using this filter in conjunction with svtkLabeledDataMapper,
 * things work out because 2D rendering occurs after the 3D rendering.
 */

#ifndef svtkSelectVisiblePoints_h
#define svtkSelectVisiblePoints_h

#include "svtkPolyDataAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkMatrix4x4;

class SVTKRENDERINGCORE_EXPORT svtkSelectVisiblePoints : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSelectVisiblePoints, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with no renderer; window selection turned off;
   * tolerance set to 0.01; and select invisible off.
   */
  static svtkSelectVisiblePoints* New();

  //@{
  /**
   * Specify the renderer in which the visibility computation is to be
   * performed.
   */
  void SetRenderer(svtkRenderer* ren)
  {
    if (this->Renderer != ren)
    {
      this->Renderer = ren;
      this->Modified();
    }
  }
  svtkRenderer* GetRenderer() { return this->Renderer; }
  //@}

  //@{
  /**
   * Set/Get the flag which enables selection in a rectangular display
   * region.
   */
  svtkSetMacro(SelectionWindow, svtkTypeBool);
  svtkGetMacro(SelectionWindow, svtkTypeBool);
  svtkBooleanMacro(SelectionWindow, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the selection window in display coordinates. You must specify
   * a rectangular region using (xmin,xmax,ymin,ymax).
   */
  svtkSetVector4Macro(Selection, int);
  svtkGetVectorMacro(Selection, int, 4);
  //@}

  //@{
  /**
   * Set/Get the flag which enables inverse selection; i.e., invisible points
   * are selected.
   */
  svtkSetMacro(SelectInvisible, svtkTypeBool);
  svtkGetMacro(SelectInvisible, svtkTypeBool);
  svtkBooleanMacro(SelectInvisible, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get a tolerance in normalized display coordinate system
   * to use to determine whether a point is visible. A
   * tolerance is usually required because the conversion from world space
   * to display space during rendering introduces numerical round-off.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set/Get a tolerance in world coordinate system
   * to use to determine whether a point is visible.
   * This allows determining visibility of small spheroid objects
   * (such as glyphs) with known size in world coordinates.
   * By default it is set to 0.
   */
  svtkSetClampMacro(ToleranceWorld, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(ToleranceWorld, double);
  //@}

  /**
   * Requires the renderer to be set. Populates the composite perspective transform
   * and returns a pointer to the Z-buffer (that must be deleted) if getZbuff is set.
   */
  float* Initialize(bool getZbuff);

  /**
   * Tests if a point x is being occluded or not against the Z-Buffer array passed in by
   * zPtr. Call Initialize before calling this method.
   */
  bool IsPointOccluded(const double x[3], const float* zPtr);

  /**
   * Return MTime also considering the renderer.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkSelectVisiblePoints();
  ~svtkSelectVisiblePoints() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkRenderer* Renderer;
  svtkMatrix4x4* CompositePerspectiveTransform;

  svtkTypeBool SelectionWindow;
  int Selection[4];
  int InternalSelection[4];
  svtkTypeBool SelectInvisible;
  double DirectionOfProjection[3];
  double Tolerance;
  double ToleranceWorld;

private:
  svtkSelectVisiblePoints(const svtkSelectVisiblePoints&) = delete;
  void operator=(const svtkSelectVisiblePoints&) = delete;
};

#endif
