/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToAMR.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageToAMR
 * @brief   filter to convert any svtkImageData to a
 * svtkOverlappingAMR.
 *
 * svtkImageToAMR is a simple filter that converts any svtkImageData to a
 * svtkOverlappingAMR dataset. The input svtkImageData is treated as the highest
 * refinement available for the highest level. The lower refinements and the
 * number of blocks is controlled properties specified on the filter.
 */

#ifndef svtkImageToAMR_h
#define svtkImageToAMR_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkOverlappingAMRAlgorithm.h"

class SVTKFILTERSAMR_EXPORT svtkImageToAMR : public svtkOverlappingAMRAlgorithm
{
public:
  static svtkImageToAMR* New();
  svtkTypeMacro(svtkImageToAMR, svtkOverlappingAMRAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the maximum number of levels in the generated Overlapping-AMR.
   */
  svtkSetClampMacro(NumberOfLevels, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfLevels, int);
  //@}

  //@{
  /**
   * Set the refinement ratio for levels. This refinement ratio is used for all
   * levels.
   */
  svtkSetClampMacro(RefinementRatio, int, 2, SVTK_INT_MAX);
  svtkGetMacro(RefinementRatio, int);
  //@}

  //@{
  /**
   * Set the maximum number of blocks in the output
   */
  svtkSetClampMacro(MaximumNumberOfBlocks, int, 1, SVTK_INT_MAX);
  svtkGetMacro(MaximumNumberOfBlocks, int);
  //@}

protected:
  svtkImageToAMR();
  ~svtkImageToAMR() override;

  /**
   * Fill the input port information objects for this algorithm.  This
   * is invoked by the first call to GetInputPortInformation for each
   * port so subclasses can specify what they can handle.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int NumberOfLevels;
  int MaximumNumberOfBlocks;
  int RefinementRatio;

private:
  svtkImageToAMR(const svtkImageToAMR&) = delete;
  void operator=(const svtkImageToAMR&) = delete;
};

#endif
