/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTerrainContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTerrainContourLineInterpolator
 * @brief   Contour interpolator for DEM data.
 *
 *
 * svtkTerrainContourLineInterpolator interpolates nodes on height field data.
 * The class is meant to be used in conjunciton with a svtkContourWidget,
 * enabling you to draw paths on terrain data. The class internally uses a
 * svtkProjectedTerrainPath. Users can set kind of interpolation
 * desired between two node points by setting the modes of the this filter.
 * For instance:
 *
 * \code
 * contourRepresentation->SetLineInterpolator(interpolator);
 * interpolator->SetImageData( demDataFile );
 * interpolator->GetProjector()->SetProjectionModeToHug();
 * interpolator->SetHeightOffset(25.0);
 * \endcode
 *
 * You are required to set the ImageData to this class as the height-field
 * image.
 *
 * @sa
 * svtkTerrainDataPointPlacer svtkProjectedTerrainPath
 */

#ifndef svtkTerrainContourLineInterpolator_h
#define svtkTerrainContourLineInterpolator_h

#include "svtkContourLineInterpolator.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkImageData;
class svtkProjectedTerrainPath;

class SVTKINTERACTIONWIDGETS_EXPORT svtkTerrainContourLineInterpolator
  : public svtkContourLineInterpolator
{
public:
  /**
   * Instantiate this class.
   */
  static svtkTerrainContourLineInterpolator* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkTerrainContourLineInterpolator, svtkContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Interpolate to create lines between contour nodes idx1 and idx2.
   * Depending on the projection mode, the interpolated line may either
   * hug the terrain, just connect the two points with a straight line or
   * a non-occluded interpolation.
   * Used internally by svtkContourRepresentation.
   */
  int InterpolateLine(svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override;

  /**
   * The interpolator is given a chance to update the node.
   * Used internally by svtkContourRepresentation
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node),
    int svtkNotUsed(idx)) override;

  //@{
  /**
   * Set the height field data. The height field data is a 2D image. The
   * scalars in the image represent the height field. This must be set.
   */
  virtual void SetImageData(svtkImageData*);
  svtkGetObjectMacro(ImageData, svtkImageData);
  //@}

  //@{
  /**
   * Get the svtkProjectedTerrainPath operator used to project the terrain
   * onto the data. This operator has several modes, See the documentation
   * of svtkProjectedTerrainPath. The default mode is to hug the terrain
   * data at 0 height offset.
   */
  svtkGetObjectMacro(Projector, svtkProjectedTerrainPath);
  //@}

protected:
  svtkTerrainContourLineInterpolator();
  ~svtkTerrainContourLineInterpolator() override;

  svtkImageData* ImageData; // height field data
  svtkProjectedTerrainPath* Projector;

private:
  svtkTerrainContourLineInterpolator(const svtkTerrainContourLineInterpolator&) = delete;
  void operator=(const svtkTerrainContourLineInterpolator&) = delete;
};

#endif
