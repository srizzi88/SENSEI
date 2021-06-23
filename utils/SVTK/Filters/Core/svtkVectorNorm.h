/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVectorNorm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVectorNorm
 * @brief   generate scalars from Euclidean norm of vectors
 *
 * svtkVectorNorm is a filter that generates scalar values by computing
 * Euclidean norm of vector triplets. Scalars can be normalized
 * 0<=s<=1 if desired.
 *
 * Note that this filter operates on point or cell attribute data, or
 * both.  By default, the filter operates on both point and cell data
 * if vector point and cell data, respectively, are available from the
 * input. Alternatively, you can choose to generate scalar norm values
 * for just cell or point data.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef svtkVectorNorm_h
#define svtkVectorNorm_h

#define SVTK_ATTRIBUTE_MODE_DEFAULT 0
#define SVTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define SVTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkVectorNorm : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkVectorNorm, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with normalize flag off.
   */
  static svtkVectorNorm* New();

  // Specify whether to normalize scalar values. If the data is normalized,
  // then it will fall in the range [0,1].
  svtkSetMacro(Normalize, svtkTypeBool);
  svtkGetMacro(Normalize, svtkTypeBool);
  svtkBooleanMacro(Normalize, svtkTypeBool);

  //@{
  /**
   * Control how the filter works to generate scalar data from the
   * input vector data. By default, (AttributeModeToDefault) the
   * filter will generate the scalar norm for point and cell data (if
   * vector data present in the input). Alternatively, you can
   * explicitly set the filter to generate point data
   * (AttributeModeToUsePointData) or cell data
   * (AttributeModeToUseCellData).
   */
  svtkSetMacro(AttributeMode, int);
  svtkGetMacro(AttributeMode, int);
  void SetAttributeModeToDefault() { this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_DEFAULT); }
  void SetAttributeModeToUsePointData()
  {
    this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_USE_POINT_DATA);
  }
  void SetAttributeModeToUseCellData() { this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_USE_CELL_DATA); }
  const char* GetAttributeModeAsString();
  //@}

protected:
  svtkVectorNorm();
  ~svtkVectorNorm() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool Normalize; // normalize 0<=n<=1 if true.
  int AttributeMode;     // control whether to use point or cell data, or both

private:
  svtkVectorNorm(const svtkVectorNorm&) = delete;
  void operator=(const svtkVectorNorm&) = delete;
};

#endif
