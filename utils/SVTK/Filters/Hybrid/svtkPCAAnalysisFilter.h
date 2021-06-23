/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCAAnalysisFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPCAAnalysisFilter
 * @brief   Performs principal component analysis of a set of aligned pointsets
 *
 *
 * svtkPCAAnalysisFilter is a filter that takes as input a set of aligned
 * pointsets (any object derived from svtkPointSet) and performs
 * a principal component analysis of the coordinates.
 * This can be used to visualise the major or minor modes of variation
 * seen in a set of similar biological objects with corresponding
 * landmarks.
 * svtkPCAAnalysisFilter is designed to work with the output from
 * the svtkProcrustesAnalysisFilter
 * svtkPCAAnalysisFilter requires a svtkMultiBlock input consisting
 * of svtkPointSets as first level children.
 *
 * svtkPCAAnalysisFilter is an implementation of (for example):
 *
 * T. Cootes et al. : Active Shape Models - their training and application.
 * Computer Vision and Image Understanding, 61(1):38-59, 1995.
 *
 * The material can also be found in Tim Cootes' ever-changing online report
 * published at his website:
 * http://www.isbe.man.ac.uk/~bim/
 *
 * @warning
 * All of the input pointsets must have the same number of points.
 *
 * @par Thanks:
 * Rasmus Paulsen and Tim Hutton who developed and contributed this class
 *
 * @sa
 * svtkProcrustesAlignmentFilter
 */

#ifndef svtkPCAAnalysisFilter_h
#define svtkPCAAnalysisFilter_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkFloatArray;
class svtkPointSet;

class SVTKFILTERSHYBRID_EXPORT svtkPCAAnalysisFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkPCAAnalysisFilter, svtkMultiBlockDataSetAlgorithm);

  /**
   * Prints information about the state of the filter.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates with similarity transform.
   */
  static svtkPCAAnalysisFilter* New();

  //@{
  /**
   * Get the vector of eigenvalues sorted in descending order
   */
  svtkGetObjectMacro(Evals, svtkFloatArray);
  //@}

  /**
   * Fills the shape with:

   * mean + b[0] * sqrt(eigenvalue[0]) * eigenvector[0]
   * + b[1] * sqrt(eigenvalue[1]) * eigenvector[1]
   * ...
   * + b[sizeb-1] * sqrt(eigenvalue[bsize-1]) * eigenvector[bsize-1]

   * here b are the parameters expressed in standard deviations
   * bsize is the number of parameters in the b vector
   * This function assumes that shape is already allocated
   * with the right size, it just moves the points.
   */
  void GetParameterisedShape(svtkFloatArray* b, svtkPointSet* shape);

  /**
   * Return the bsize parameters b that best model the given shape
   * (in standard deviations).
   * That is that the given shape will be approximated by:

   * shape ~ mean + b[0] * sqrt(eigenvalue[0]) * eigenvector[0]
   * + b[1] * sqrt(eigenvalue[1]) * eigenvector[1]
   * ...
   * + b[bsize-1] * sqrt(eigenvalue[bsize-1]) * eigenvector[bsize-1]
   */
  void GetShapeParameters(svtkPointSet* shape, svtkFloatArray* b, int bsize);

  /**
   * Retrieve how many modes are necessary to model the given proportion of the variation.
   * proportion should be between 0 and 1
   */
  int GetModesRequiredFor(double proportion);

protected:
  svtkPCAAnalysisFilter();
  ~svtkPCAAnalysisFilter() override;

  /**
   * Usual data generation method.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPCAAnalysisFilter(const svtkPCAAnalysisFilter&) = delete;
  void operator=(const svtkPCAAnalysisFilter&) = delete;

  // Eigenvalues
  svtkFloatArray* Evals;

  // Matrix where each column is an eigenvector
  double** evecMat2;

  // The mean shape in a vector
  double* meanshape;
};

#endif
