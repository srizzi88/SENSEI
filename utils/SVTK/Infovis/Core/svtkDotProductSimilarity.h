/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDotProductSimilarity.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkDotProductSimilarity
 * @brief   compute dot-product similarity metrics.
 *
 *
 * Treats matrices as collections of vectors and computes dot-product similarity
 * metrics between vectors.
 *
 * The results are returned as an edge-table that lists the index of each vector
 * and their computed similarity.  The output edge-table is typically used with
 * svtkTableToGraph to create a similarity graph.
 *
 * This filter can be used with one or two input matrices.  If you provide a single
 * matrix as input, every vector in the matrix is compared with every other vector. If
 * you provide two matrices, every vector in the first matrix is compared with every
 * vector in the second matrix.
 *
 * Note that this filter *only* computes the dot-product between each pair of vectors;
 * if you want to compute the cosine of the angles between vectors, you will need to
 * normalize the inputs yourself.
 *
 * Inputs:
 *   Input port 0: (required) A svtkDenseArray<double> with two dimensions (a matrix).
 *   Input port 1: (optional) A svtkDenseArray<double> with two dimensions (a matrix).
 *
 * Outputs:
 *   Output port 0: A svtkTable containing "source", "target", and "similarity" columns.
 *
 * @warning
 * Note that the complexity of this filter is quadratic!  It also requires dense arrays
 * as input, in the future it should be generalized to accept sparse arrays.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkDotProductSimilarity_h
#define svtkDotProductSimilarity_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkDotProductSimilarity : public svtkTableAlgorithm
{
public:
  static svtkDotProductSimilarity* New();
  svtkTypeMacro(svtkDotProductSimilarity, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Controls whether to compute similarities for row-vectors or column-vectors.
   * 0 = rows, 1 = columns.
   */
  svtkGetMacro(VectorDimension, svtkIdType);
  svtkSetMacro(VectorDimension, svtkIdType);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the upper diagonal of the similarity matrix.  Default: true.
   */
  svtkGetMacro(UpperDiagonal, int);
  svtkSetMacro(UpperDiagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the diagonal of the similarity matrix.  Default: false.
   */
  svtkGetMacro(Diagonal, int);
  svtkSetMacro(Diagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for a single input matrix, controls whether the
   * results will include the lower diagonal of the similarity matrix.  Default: false.
   */
  svtkGetMacro(LowerDiagonal, int);
  svtkSetMacro(LowerDiagonal, int);
  //@}

  //@{
  /**
   * When computing similarities for two input matrices, controls whether the results
   * will include comparisons from the first matrix to the second matrix.
   */
  svtkGetMacro(FirstSecond, int);
  svtkSetMacro(FirstSecond, int);
  //@}

  //@{
  /**
   * When computing similarities for two input matrices, controls whether the results
   * will include comparisons from the second matrix to the first matrix.
   */
  svtkGetMacro(SecondFirst, int);
  svtkSetMacro(SecondFirst, int);
  //@}

  //@{
  /**
   * Specifies a minimum threshold that a similarity must exceed to be included in
   * the output.
   */
  svtkGetMacro(MinimumThreshold, double);
  svtkSetMacro(MinimumThreshold, double);
  //@}

  //@{
  /**
   * Specifies a minimum number of edges to include for each vector.
   */
  svtkGetMacro(MinimumCount, svtkIdType);
  svtkSetMacro(MinimumCount, svtkIdType);
  //@}

  //@{
  /**
   * Specifies a maximum number of edges to include for each vector.
   */
  svtkGetMacro(MaximumCount, svtkIdType);
  svtkSetMacro(MaximumCount, svtkIdType);
  //@}

protected:
  svtkDotProductSimilarity();
  ~svtkDotProductSimilarity() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkDotProductSimilarity(const svtkDotProductSimilarity&) = delete;
  void operator=(const svtkDotProductSimilarity&) = delete;

  svtkIdType VectorDimension;
  double MinimumThreshold;
  svtkIdType MinimumCount;
  svtkIdType MaximumCount;

  int UpperDiagonal;
  int Diagonal;
  int LowerDiagonal;
  int FirstSecond;
  int SecondFirst;
};

#endif
