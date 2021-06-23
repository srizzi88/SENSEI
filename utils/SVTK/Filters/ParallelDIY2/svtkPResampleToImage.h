/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPResampleToImage
 * @brief   sample dataset on a uniform grid in parallel
 *
 * svtkPResampleToImage is a parallel filter that resamples the input dataset on
 * a uniform grid. It internally uses svtkProbeFilter to do the probing.
 * @sa
 * svtkResampleToImage svtkProbeFilter
 */

#ifndef svtkPResampleToImage_h
#define svtkPResampleToImage_h

#include "svtkFiltersParallelDIY2Module.h" // For export macro
#include "svtkResampleToImage.h"

class svtkDataSet;
class svtkImageData;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELDIY2_EXPORT svtkPResampleToImage : public svtkResampleToImage
{
public:
  svtkTypeMacro(svtkPResampleToImage, svtkResampleToImage);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPResampleToImage* New();

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPResampleToImage();
  ~svtkPResampleToImage() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMultiProcessController* Controller;

private:
  svtkPResampleToImage(const svtkPResampleToImage&) = delete;
  void operator=(const svtkPResampleToImage&) = delete;
};

#endif
