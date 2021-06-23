/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAccumulate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageAccumulate
 * @brief   Generalized histograms up to 3 dimensions.
 *
 * svtkImageAccumulate - This filter divides component space into
 * discrete bins.  It then counts the number of pixels associated
 * with each bin.
 * The dimensionality of the output depends on how many components the
 * input pixels have. An input images with N components per pixels will
 * result in an N-dimensional histogram, where N can be 1, 2, or 3.
 * The input can be any type, but the output is always int.
 * Some statistics are computed on the pixel values at the same time.
 * The SetStencil and ReverseStencil functions allow the statistics to be
 * computed on an arbitrary portion of the input data.
 * See the documentation for svtkImageStencilData for more information.
 *
 * This filter also supports ignoring pixels with value equal to 0. Using this
 * option with svtkImageMask may result in results being slightly off since 0
 * could be a valid value from your input.
 *
 */

#ifndef svtkImageAccumulate_h
#define svtkImageAccumulate_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingStatisticsModule.h" // For export macro

class svtkImageStencilData;

class SVTKIMAGINGSTATISTICS_EXPORT svtkImageAccumulate : public svtkImageAlgorithm
{
public:
  static svtkImageAccumulate* New();
  svtkTypeMacro(svtkImageAccumulate, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get - The component spacing is the dimension of each bin.
   * This ends up being the spacing of the output "image".
   * If the number of input scalar components are less than three,
   * then some of these spacing values are ignored.
   * For a 1D histogram with 10 bins spanning the values 1000 to 2000,
   * this spacing should be set to 100, 0, 0.
   * Initial value is (1.0,1.0,1.0).
   */
  svtkSetVector3Macro(ComponentSpacing, double);
  svtkGetVector3Macro(ComponentSpacing, double);
  //@}

  //@{
  /**
   * Set/Get - The component origin is the location of bin (0, 0, 0).
   * Note that if the Component extent does not include the value (0,0,0),
   * then this origin bin will not actually be in the output.
   * The origin of the output ends up being the same as the componenet origin.
   * For a 1D histogram with 10 bins spanning the values 1000 to 2000,
   * this origin should be set to 1000, 0, 0.
   * Initial value is (0.0,0.0,0.0).
   */
  svtkSetVector3Macro(ComponentOrigin, double);
  svtkGetVector3Macro(ComponentOrigin, double);
  //@}

  //@{
  /**
   * Set/Get - The component extent sets the number/extent of the bins.
   * For a 1D histogram with 10 bins spanning the values 1000 to 2000,
   * this extent should be set to 0, 9, 0, 0, 0, 0.
   * The extent specifies inclusive min/max values.
   * This implies that the top extent should be set to the number of bins - 1.
   * Initial value is (0,255,0,0,0,0)
   */
  void SetComponentExtent(int extent[6]);
  void SetComponentExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetComponentExtent(int extent[6]);
  int* GetComponentExtent() SVTK_SIZEHINT(6) { return this->ComponentExtent; }
  //@}

  //@{
  /**
   * Use a stencil to specify which voxels to accumulate.
   * Backcompatible methods.
   * It set and get the stencil on input port 1.
   * Initial value is nullptr.
   */
  void SetStencilData(svtkImageStencilData* stencil);
  svtkImageStencilData* GetStencil();
  //@}

  //@{
  /**
   * Reverse the stencil. Initial value is false.
   */
  svtkSetClampMacro(ReverseStencil, svtkTypeBool, 0, 1);
  svtkBooleanMacro(ReverseStencil, svtkTypeBool);
  svtkGetMacro(ReverseStencil, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the statistics information for the data.
   * The values only make sense after the execution of the filter.
   * Initial values are 0.
   */
  svtkGetVector3Macro(Min, double);
  svtkGetVector3Macro(Max, double);
  svtkGetVector3Macro(Mean, double);
  svtkGetVector3Macro(StandardDeviation, double);
  svtkGetMacro(VoxelCount, svtkIdType);
  //@}

  //@{
  /**
   * Should the data with value 0 be ignored? Initial value is false.
   */
  svtkSetClampMacro(IgnoreZero, svtkTypeBool, 0, 1);
  svtkGetMacro(IgnoreZero, svtkTypeBool);
  svtkBooleanMacro(IgnoreZero, svtkTypeBool);
  //@}

protected:
  svtkImageAccumulate();
  ~svtkImageAccumulate() override;

  double ComponentSpacing[3];
  double ComponentOrigin[3];
  int ComponentExtent[6];

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkTypeBool IgnoreZero;
  double Min[3];
  double Max[3];
  double Mean[3];
  double StandardDeviation[3];
  svtkIdType VoxelCount;

  svtkTypeBool ReverseStencil;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkImageAccumulate(const svtkImageAccumulate&) = delete;
  void operator=(const svtkImageAccumulate&) = delete;
};

#endif
