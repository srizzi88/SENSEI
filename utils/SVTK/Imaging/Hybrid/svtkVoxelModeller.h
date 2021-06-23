/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVoxelModeller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVoxelModeller
 * @brief   convert an arbitrary dataset to a voxel representation
 *
 * svtkVoxelModeller is a filter that converts an arbitrary data set to a
 * structured point (i.e., voxel) representation. It is very similar to
 * svtkImplicitModeller, except that it doesn't record distance; instead it
 * records occupancy. By default it supports a compact output of 0/1
 * SVTK_BIT. Other svtk scalar types can be specified. The Foreground and
 * Background values of the output can also be specified.
 * NOTE: Not all svtk filters/readers/writers support the SVTK_BIT
 * scalar type. You may want to use SVTK_CHAR as an alternative.
 * @sa
 * svtkImplicitModeller
 */

#ifndef svtkVoxelModeller_h
#define svtkVoxelModeller_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkVoxelModeller : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkVoxelModeller, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct an instance of svtkVoxelModeller with its sample dimensions
   * set to (50,50,50), and so that the model bounds are
   * automatically computed from its input. The maximum distance is set to
   * examine the whole grid. This could be made much faster, and probably
   * will be in the future.
   */
  static svtkVoxelModeller* New();

  /**
   * Compute the ModelBounds based on the input geometry.
   */
  double ComputeModelBounds(double origin[3], double ar[3]);

  //@{
  /**
   * Set the i-j-k dimensions on which to sample the distance function.
   * Default is (50, 50, 50)
   */
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  svtkGetVectorMacro(SampleDimensions, int, 3);
  //@}

  //@{
  /**
   * Specify distance away from surface of input geometry to sample. Smaller
   * values make large increases in performance. Default is 1.0.
   */
  svtkSetClampMacro(MaximumDistance, double, 0.0, 1.0);
  svtkGetMacro(MaximumDistance, double);
  //@}

  //@{
  /**
   * Specify the position in space to perform the voxelization.
   * Default is (0, 0, 0, 0, 0, 0)
   */
  void SetModelBounds(const double bounds[6]);
  void SetModelBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  svtkGetVectorMacro(ModelBounds, double, 6);
  //@}

  //@{
  /**
   * Control the scalar type of the output image. The default is
   * SVTK_BIT.
   * NOTE: Not all filters/readers/writers support the SVTK_BIT
   * scalar type. You may want to use SVTK_CHAR as an alternative.
   */
  svtkSetMacro(ScalarType, int);
  void SetScalarTypeToFloat() { this->SetScalarType(SVTK_FLOAT); }
  void SetScalarTypeToDouble() { this->SetScalarType(SVTK_DOUBLE); }
  void SetScalarTypeToInt() { this->SetScalarType(SVTK_INT); }
  void SetScalarTypeToUnsignedInt() { this->SetScalarType(SVTK_UNSIGNED_INT); }
  void SetScalarTypeToLong() { this->SetScalarType(SVTK_LONG); }
  void SetScalarTypeToUnsignedLong() { this->SetScalarType(SVTK_UNSIGNED_LONG); }
  void SetScalarTypeToShort() { this->SetScalarType(SVTK_SHORT); }
  void SetScalarTypeToUnsignedShort() { this->SetScalarType(SVTK_UNSIGNED_SHORT); }
  void SetScalarTypeToUnsignedChar() { this->SetScalarType(SVTK_UNSIGNED_CHAR); }
  void SetScalarTypeToChar() { this->SetScalarType(SVTK_CHAR); }
  void SetScalarTypeToBit() { this->SetScalarType(SVTK_BIT); }
  svtkGetMacro(ScalarType, int);
  //@}

  //@{
  /**
   * Set the Foreground/Background values of the output. The
   * Foreground value is set when a voxel is occupied. The Background
   * value is set when a voxel is not occupied.
   * The default ForegroundValue is 1. The default BackgroundValue is
   * 0.
   */
  svtkSetMacro(ForegroundValue, double);
  svtkGetMacro(ForegroundValue, double);
  svtkSetMacro(BackgroundValue, double);
  svtkGetMacro(BackgroundValue, double);
  //@}

protected:
  svtkVoxelModeller();
  ~svtkVoxelModeller() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // see svtkAlgorithm for details
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int SampleDimensions[3];
  double MaximumDistance;
  double ModelBounds[6];
  double ForegroundValue;
  double BackgroundValue;
  int ScalarType;

private:
  svtkVoxelModeller(const svtkVoxelModeller&) = delete;
  void operator=(const svtkVoxelModeller&) = delete;
};

#endif
