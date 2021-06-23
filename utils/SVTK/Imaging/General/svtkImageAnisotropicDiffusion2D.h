/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAnisotropicDiffusion2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageAnisotropicDiffusion2D
 * @brief   edge preserving smoothing.
 *
 *
 * svtkImageAnisotropicDiffusion2D diffuses a 2d image iteratively.
 * The neighborhood of the diffusion is determined by the instance
 * flags. If "Edges" is on the 4 edge connected voxels
 * are included, and if "Corners" is on, the 4 corner connected voxels
 * are included.  "DiffusionFactor" determines how far a pixel value
 * moves toward its neighbors, and is insensitive to the number of
 * neighbors chosen.  The diffusion is anisotropic because it only occurs
 * when a gradient measure is below "GradientThreshold".  Two gradient measures
 * exist and are toggled by the "GradientMagnitudeThreshold" flag.
 * When "GradientMagnitudeThreshold" is on, the magnitude of the gradient,
 * computed by central differences, above "DiffusionThreshold"
 * a voxel is not modified.  The alternative measure examines each
 * neighbor independently.  The gradient between the voxel and the neighbor
 * must be below the "DiffusionThreshold" for diffusion to occur with
 * THAT neighbor.
 *
 * @sa
 * svtkImageAnisotropicDiffusion3D
 */

#ifndef svtkImageAnisotropicDiffusion2D_h
#define svtkImageAnisotropicDiffusion2D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro
class SVTKIMAGINGGENERAL_EXPORT svtkImageAnisotropicDiffusion2D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageAnisotropicDiffusion2D* New();
  svtkTypeMacro(svtkImageAnisotropicDiffusion2D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method sets the number of iterations which also affects the
   * input neighborhood needed to compute one output pixel.  Each iteration
   * requires an extra pixel layer on the neighborhood.  This is only relevant
   * when you are trying to stream or are requesting a sub extent of the "wholeExtent".
   */
  void SetNumberOfIterations(int num);

  //@{
  /**
   * Get the number of iterations.
   */
  svtkGetMacro(NumberOfIterations, int);
  //@}

  //@{
  /**
   * Set/Get the difference threshold that stops diffusion.
   * when the difference between two pixel is greater than this threshold,
   * the pixels are not diffused.  This causes diffusion to avoid sharp edges.
   * If the GradientMagnitudeThreshold is set, then gradient magnitude is used
   * for comparison instead of pixel differences.
   */
  svtkSetMacro(DiffusionThreshold, double);
  svtkGetMacro(DiffusionThreshold, double);
  //@}

  //@{
  /**
   * The diffusion factor specifies how much neighboring pixels effect each other.
   * No diffusion occurs with a factor of 0, and a diffusion factor of 1 causes
   * the pixel to become the average of all its neighbors.
   */
  svtkSetMacro(DiffusionFactor, double);
  svtkGetMacro(DiffusionFactor, double);
  //@}

  //@{
  /**
   * Choose neighbors to diffuse (6 faces, 12 edges, 8 corners).
   */
  svtkSetMacro(Faces, svtkTypeBool);
  svtkGetMacro(Faces, svtkTypeBool);
  svtkBooleanMacro(Faces, svtkTypeBool);
  svtkSetMacro(Edges, svtkTypeBool);
  svtkGetMacro(Edges, svtkTypeBool);
  svtkBooleanMacro(Edges, svtkTypeBool);
  svtkSetMacro(Corners, svtkTypeBool);
  svtkGetMacro(Corners, svtkTypeBool);
  svtkBooleanMacro(Corners, svtkTypeBool);
  //@}

  //@{
  /**
   * Switch between gradient magnitude threshold and pixel gradient threshold.
   */
  svtkSetMacro(GradientMagnitudeThreshold, svtkTypeBool);
  svtkGetMacro(GradientMagnitudeThreshold, svtkTypeBool);
  svtkBooleanMacro(GradientMagnitudeThreshold, svtkTypeBool);
  //@}

protected:
  svtkImageAnisotropicDiffusion2D();
  ~svtkImageAnisotropicDiffusion2D() override {}

  int NumberOfIterations;
  double DiffusionThreshold;
  double DiffusionFactor;
  // to determine which neighbors to diffuse
  svtkTypeBool Faces;
  svtkTypeBool Edges;
  svtkTypeBool Corners;
  // What threshold to use
  svtkTypeBool GradientMagnitudeThreshold;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  void Iterate(
    svtkImageData* in, svtkImageData* out, double ar0, double ar1, int* coreExtent, int count);

private:
  svtkImageAnisotropicDiffusion2D(const svtkImageAnisotropicDiffusion2D&) = delete;
  void operator=(const svtkImageAnisotropicDiffusion2D&) = delete;
};

#endif
