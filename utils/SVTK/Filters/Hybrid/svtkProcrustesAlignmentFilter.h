/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcrustesAlignmentFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProcrustesAlignmentFilter
 * @brief   aligns a set of pointsets together
 *
 *
 * svtkProcrustesAlignmentFilter is a filter that takes a set of pointsets
 * (any object derived from svtkPointSet) and aligns them in a least-squares
 * sense to their mutual mean. The algorithm is iterated until convergence,
 * as the mean must be recomputed after each alignment.
 *
 * svtkProcrustesAlignmentFilter requires a svtkMultiBlock input consisting
 * of svtkPointSets as first level children.
 *
 * The default (in svtkLandmarkTransform) is for a similarity alignment.
 * For a rigid-body alignment (to build a 'size-and-shape' model) use:
 *
 *    GetLandmarkTransform()->SetModeToRigidBody().
 *
 * Affine alignments are not normally used but are left in for completeness:
 *
 *    GetLandmarkTransform()->SetModeToAffine().
 *
 * svtkProcrustesAlignmentFilter is an implementation of:
 *
 *    J.C. Gower (1975)
 *    Generalized Procrustes Analysis. Psychometrika, 40:33-51.
 *
 * @warning
 * All of the input pointsets must have the same number of points.
 *
 * @par Thanks:
 * Tim Hutton and Rasmus Paulsen who developed and contributed this class
 *
 * @sa
 * svtkLandmarkTransform
 */

#ifndef svtkProcrustesAlignmentFilter_h
#define svtkProcrustesAlignmentFilter_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkLandmarkTransform;
class svtkPointSet;
class svtkPoints;

class SVTKFILTERSHYBRID_EXPORT svtkProcrustesAlignmentFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkProcrustesAlignmentFilter, svtkMultiBlockDataSetAlgorithm);

  /**
   * Prints information about the state of the filter.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates with similarity transform.
   */
  static svtkProcrustesAlignmentFilter* New();

  //@{
  /**
   * Get the internal landmark transform. Use it to constrain the number of
   * degrees of freedom of the alignment (i.e. rigid body, similarity, etc.).
   * The default is a similarity alignment.
   */
  svtkGetObjectMacro(LandmarkTransform, svtkLandmarkTransform);
  //@}

  //@{
  /**
   * Get the estimated mean point cloud
   */
  svtkGetObjectMacro(MeanPoints, svtkPoints);
  //@}

  //@{
  /**
   * When on, the initial alignment is to the centroid
   * of the cohort curves.  When off, the alignment is to the
   * centroid of the first input.  Default is off for
   * backward compatibility.
   */
  svtkSetMacro(StartFromCentroid, bool);
  svtkGetMacro(StartFromCentroid, bool);
  svtkBooleanMacro(StartFromCentroid, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. If the desired precision is
   * DEFAULT_PRECISION and any of the inputs are double precision, then the
   * mean points will be double precision. Otherwise, if the desired
   * precision is DEFAULT_PRECISION and all the inputs are single precision,
   * then the mean points will be single precision.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkProcrustesAlignmentFilter();
  ~svtkProcrustesAlignmentFilter() override;

  /**
   * Usual data generation method.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkLandmarkTransform* LandmarkTransform;

  bool StartFromCentroid;

  svtkPoints* MeanPoints;
  int OutputPointsPrecision;

private:
  svtkProcrustesAlignmentFilter(const svtkProcrustesAlignmentFilter&) = delete;
  void operator=(const svtkProcrustesAlignmentFilter&) = delete;
};

#endif
