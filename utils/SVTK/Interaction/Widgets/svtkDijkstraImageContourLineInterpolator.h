/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDijkstraImageContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDijkstraImageContourLineInterpolator
 * @brief   Contour interpolator for placing points on an image.
 *
 *
 * svtkDijkstraImageContourLineInterpolator interpolates and places
 * contour points on images. The class interpolates nodes by
 * computing a graph laying on the image data. By graph, we mean
 * that the line interpolating the two end points traverses along
 * pixels so as to form a shortest path. A Dijkstra algorithm is
 * used to compute the path.
 *
 * The class is meant to be used in conjunction with
 * svtkImageActorPointPlacer. One reason for this coupling is a
 * performance issue: both classes need to perform a cell pick, and
 * coupling avoids multiple cell picks (cell picks are slow).  Another
 * issue is that the interpolator may need to set the image input to
 * its svtkDijkstraImageGeodesicPath ivar.
 *
 * @sa
 * svtkContourWidget svtkContourLineInterpolator svtkDijkstraImageGeodesicPath
 */

#ifndef svtkDijkstraImageContourLineInterpolator_h
#define svtkDijkstraImageContourLineInterpolator_h

#include "svtkContourLineInterpolator.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkDijkstraImageGeodesicPath;
class svtkImageData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkDijkstraImageContourLineInterpolator
  : public svtkContourLineInterpolator
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkDijkstraImageContourLineInterpolator, svtkContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  static svtkDijkstraImageContourLineInterpolator* New();

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance svtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine(svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override;

  //@{
  /**
   * Set the image data for the svtkDijkstraImageGeodesicPath.
   * If not set, the interpolator uses the image data input to the image actor.
   * The image actor is obtained from the expected svtkImageActorPointPlacer.
   */
  virtual void SetCostImage(svtkImageData*);
  svtkGetObjectMacro(CostImage, svtkImageData);
  //@}

  //@{
  /**
   * access to the internal dijkstra path
   */
  svtkGetObjectMacro(DijkstraImageGeodesicPath, svtkDijkstraImageGeodesicPath);
  //@}

protected:
  svtkDijkstraImageContourLineInterpolator();
  ~svtkDijkstraImageContourLineInterpolator() override;

  svtkImageData* CostImage;
  svtkDijkstraImageGeodesicPath* DijkstraImageGeodesicPath;

private:
  svtkDijkstraImageContourLineInterpolator(const svtkDijkstraImageContourLineInterpolator&) = delete;
  void operator=(const svtkDijkstraImageContourLineInterpolator&) = delete;
};

#endif
