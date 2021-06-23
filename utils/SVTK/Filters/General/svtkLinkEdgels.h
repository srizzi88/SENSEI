/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinkEdgels.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinkEdgels
 * @brief   links edgels together to form digital curves.
 *
 * svtkLinkEdgels links edgels into digital curves which are then stored
 * as polylines. The algorithm works one pixel at a time only looking at
 * its immediate neighbors. There is a GradientThreshold that can be set
 * that eliminates any pixels with a smaller gradient value. This can
 * be used as the lower threshold of a two value edgel thresholding.
 *
 * For the remaining edgels, links are first tried for the four
 * connected neighbors.  A successful neighbor will satisfy three
 * tests. First both edgels must be above the gradient
 * threshold. Second, the difference between the orientation between
 * the two edgels (Alpha) and each edgels orientation (Phi) must be
 * less than LinkThreshold. Third, the difference between the two
 * edgels Phi values must be less than PhiThreshold.
 * The most successful link is selected. The measure is simply the
 * sum of the three angle differences (actually stored as the sum of
 * the cosines). If none of the four connect neighbors succeeds, then
 * the eight connect neighbors are examined using the same method.
 *
 * This filter requires gradient information so you will need to use
 * a svtkImageGradient at some point prior to this filter.  Typically
 * a svtkNonMaximumSuppression filter is also used. svtkThresholdEdgels
 * can be used to complete the two value edgel thresholding as used
 * in a Canny edge detector. The svtkSubpixelPositionEdgels filter
 * can also be used after this filter to adjust the edgel locations.
 *
 * @sa
 * svtkImageData svtkImageGradient svtkImageNonMaximumSuppression
 */

#ifndef svtkLinkEdgels_h
#define svtkLinkEdgels_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkDataArray;
class svtkDoubleArray;
class svtkPoints;

class SVTKFILTERSGENERAL_EXPORT svtkLinkEdgels : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkLinkEdgels, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct instance of svtkLinkEdgels with GradientThreshold set to
   * 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
   */
  static svtkLinkEdgels* New();

  //@{
  /**
   * Set/Get the threshold for Phi vs. Alpha link thresholding.
   */
  svtkSetMacro(LinkThreshold, double);
  svtkGetMacro(LinkThreshold, double);
  //@}

  //@{
  /**
   * Set/get the threshold for Phi vs. Phi link thresholding.
   */
  svtkSetMacro(PhiThreshold, double);
  svtkGetMacro(PhiThreshold, double);
  //@}

  //@{
  /**
   * Set/Get the threshold for image gradient thresholding.
   */
  svtkSetMacro(GradientThreshold, double);
  svtkGetMacro(GradientThreshold, double);
  //@}

protected:
  svtkLinkEdgels();
  ~svtkLinkEdgels() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void LinkEdgels(int xdim, int ydim, double* image, svtkDataArray* inVectors,
    svtkCellArray* newLines, svtkPoints* newPts, svtkDoubleArray* outScalars,
    svtkDoubleArray* outVectors, int z);
  double GradientThreshold;
  double PhiThreshold;
  double LinkThreshold;

private:
  svtkLinkEdgels(const svtkLinkEdgels&) = delete;
  void operator=(const svtkLinkEdgels&) = delete;
};

#endif
