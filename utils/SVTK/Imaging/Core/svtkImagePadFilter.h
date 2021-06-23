/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImagePadFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImagePadFilter
 * @brief   Super class for filters that fill in extra pixels.
 *
 * svtkImagePadFilter Changes the image extent of an image.  If the image
 * extent is larger than the input image extent, the extra pixels are
 * filled by an algorithm determined by the subclass.
 * The image extent of the output has to be specified.
 */

#ifndef svtkImagePadFilter_h
#define svtkImagePadFilter_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImagePadFilter : public svtkThreadedImageAlgorithm
{
public:
  static svtkImagePadFilter* New();
  svtkTypeMacro(svtkImagePadFilter, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The image extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6]);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY, int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int* GetOutputWholeExtent() SVTK_SIZEHINT(6) { return this->OutputWholeExtent; }
  //@}

  //@{
  /**
   * Set/Get the number of output scalar components.
   */
  svtkSetMacro(OutputNumberOfScalarComponents, int);
  svtkGetMacro(OutputNumberOfScalarComponents, int);
  //@}

protected:
  svtkImagePadFilter();
  ~svtkImagePadFilter() override {}

  int OutputWholeExtent[6];
  int OutputNumberOfScalarComponents;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wExt[6]);

private:
  svtkImagePadFilter(const svtkImagePadFilter&) = delete;
  void operator=(const svtkImagePadFilter&) = delete;
};

#endif
