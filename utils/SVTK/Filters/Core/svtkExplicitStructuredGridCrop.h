/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridCrop.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExplicitStructuredGridCrop
 * @brief   Filter which extracts a piece of explicit structured
 * grid changing its extents.
 */

#ifndef svtkExplicitStructuredGridCrop_h
#define svtkExplicitStructuredGridCrop_h

#include "svtkExplicitStructuredGridAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkExplicitStructuredGridCrop
  : public svtkExplicitStructuredGridAlgorithm
{
public:
  static svtkExplicitStructuredGridCrop* New();
  svtkTypeMacro(svtkExplicitStructuredGridCrop, svtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The whole extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6], svtkInformation* outInfo = nullptr);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() { return this->OutputWholeExtent; }
  //@}

  void ResetOutputWholeExtent();

protected:
  svtkExplicitStructuredGridCrop();
  ~svtkExplicitStructuredGridCrop() override = default;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Initialized;
  int OutputWholeExtent[6];

private:
  svtkExplicitStructuredGridCrop(const svtkExplicitStructuredGridCrop&) = delete;
  void operator=(const svtkExplicitStructuredGridCrop&) = delete;
};

#endif
