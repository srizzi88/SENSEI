/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageEuclideanDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageEuclideanDistance
 * @brief   computes 3D Euclidean DT
 *
 * svtkImageEuclideanDistance implements the Euclidean DT using
 * Saito's algorithm. The distance map produced contains the square of the
 * Euclidean distance values.
 *
 * The algorithm has a o(n^(D+1)) complexity over nxnx...xn images in D
 * dimensions. It is very efficient on relatively small images. Cuisenaire's
 * algorithms should be used instead if n >> 500. These are not implemented
 * yet.
 *
 * For the special case of images where the slice-size is a multiple of
 * 2^N with a large N (typically for 256x256 slices), Saito's algorithm
 * encounters a lot of cache conflicts during the 3rd iteration which can
 * slow it very significantly. In that case, one should use
 * ::SetAlgorithmToSaitoCached() instead for better performance.
 *
 * References:
 *
 * T. Saito and J.I. Toriwaki. New algorithms for Euclidean distance
 * transformations of an n-dimensional digitised picture with applications.
 * Pattern Recognition, 27(11). pp. 1551--1565, 1994.
 *
 * O. Cuisenaire. Distance Transformation: fast algorithms and applications
 * to medical image processing. PhD Thesis, Universite catholique de Louvain,
 * October 1999. http://ltswww.epfl.ch/~cuisenai/papers/oc_thesis.pdf
 */

#ifndef svtkImageEuclideanDistance_h
#define svtkImageEuclideanDistance_h

#include "svtkImageDecomposeFilter.h"
#include "svtkImagingGeneralModule.h" // For export macro

#define SVTK_EDT_SAITO_CACHED 0
#define SVTK_EDT_SAITO 1

class SVTKIMAGINGGENERAL_EXPORT svtkImageEuclideanDistance : public svtkImageDecomposeFilter
{
public:
  static svtkImageEuclideanDistance* New();
  svtkTypeMacro(svtkImageEuclideanDistance, svtkImageDecomposeFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Used to set all non-zero voxels to MaximumDistance before starting
   * the distance transformation. Setting Initialize off keeps the current
   * value in the input image as starting point. This allows to superimpose
   * several distance maps.
   */
  svtkSetMacro(Initialize, svtkTypeBool);
  svtkGetMacro(Initialize, svtkTypeBool);
  svtkBooleanMacro(Initialize, svtkTypeBool);
  //@}

  //@{
  /**
   * Used to define whether Spacing should be used in the computation of the
   * distances
   */
  svtkSetMacro(ConsiderAnisotropy, svtkTypeBool);
  svtkGetMacro(ConsiderAnisotropy, svtkTypeBool);
  svtkBooleanMacro(ConsiderAnisotropy, svtkTypeBool);
  //@}

  //@{
  /**
   * Any distance bigger than this->MaximumDistance will not ne computed but
   * set to this->MaximumDistance instead.
   */
  svtkSetMacro(MaximumDistance, double);
  svtkGetMacro(MaximumDistance, double);
  //@}

  //@{
  /**
   * Selects a Euclidean DT algorithm.
   * 1. Saito
   * 2. Saito-cached
   * More algorithms will be added later on.
   */
  svtkSetMacro(Algorithm, int);
  svtkGetMacro(Algorithm, int);
  void SetAlgorithmToSaito() { this->SetAlgorithm(SVTK_EDT_SAITO); }
  void SetAlgorithmToSaitoCached() { this->SetAlgorithm(SVTK_EDT_SAITO_CACHED); }
  //@}

  int IterativeRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkImageEuclideanDistance();
  ~svtkImageEuclideanDistance() override {}

  double MaximumDistance;
  svtkTypeBool Initialize;
  svtkTypeBool ConsiderAnisotropy;
  int Algorithm;

  // Replaces "EnlargeOutputUpdateExtent"
  virtual void AllocateOutputScalars(svtkImageData* outData, int outExt[6], svtkInformation* outInfo);

  int IterativeRequestInformation(svtkInformation* in, svtkInformation* out) override;
  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;

private:
  svtkImageEuclideanDistance(const svtkImageEuclideanDistance&) = delete;
  void operator=(const svtkImageEuclideanDistance&) = delete;
};

#endif
