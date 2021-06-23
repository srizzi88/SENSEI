/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageStencilAlgorithm
 * @brief   producer of svtkImageStencilData
 *
 * svtkImageStencilAlgorithm is a superclass for filters that generate
 * the special svtkImageStencilData type.  This data type is a special
 * representation of a binary image that can be used as a mask by
 * several imaging filters.
 * @sa
 * svtkImageStencilData svtkImageStencilSource
 */

#ifndef svtkImageStencilAlgorithm_h
#define svtkImageStencilAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkImageStencilData;

class SVTKIMAGINGCORE_EXPORT svtkImageStencilAlgorithm : public svtkAlgorithm
{
public:
  static svtkImageStencilAlgorithm* New();
  svtkTypeMacro(svtkImageStencilAlgorithm, svtkAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get or set the output for this source.
   */
  void SetOutput(svtkImageStencilData* output);
  svtkImageStencilData* GetOutput();
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkImageStencilAlgorithm();
  ~svtkImageStencilAlgorithm() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  svtkImageStencilData* AllocateOutputData(svtkDataObject* out, int* updateExt);

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkImageStencilAlgorithm(const svtkImageStencilAlgorithm&) = delete;
  void operator=(const svtkImageStencilAlgorithm&) = delete;
};

#endif
