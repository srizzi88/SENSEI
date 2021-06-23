/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSpatialAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSpatialAlgorithm
 * @brief   Filters that operate on pixel neighborhoods.
 *
 * svtkImageSpatialAlgorithm is a super class for filters that operate on an
 * input neighborhood for each output pixel. It handles even sized
 * neighborhoods, but their can be a half pixel shift associated with
 * processing.  This superclass has some logic for handling boundaries.  It
 * can split regions into boundary and non-boundary pieces and call different
 * execute methods.
 */

#ifndef svtkImageSpatialAlgorithm_h
#define svtkImageSpatialAlgorithm_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageSpatialAlgorithm : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageSpatialAlgorithm* New();
  svtkTypeMacro(svtkImageSpatialAlgorithm, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the Kernel size.
   */
  svtkGetVector3Macro(KernelSize, int);
  //@}

  //@{
  /**
   * Get the Kernel middle.
   */
  svtkGetVector3Macro(KernelMiddle, int);
  //@}

protected:
  svtkImageSpatialAlgorithm();
  ~svtkImageSpatialAlgorithm() override {}

  int KernelSize[3];
  int KernelMiddle[3];  // Index of kernel origin
  int HandleBoundaries; // Output shrinks if boundaries aren't handled

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ComputeOutputWholeExtent(int extent[6], int handleBoundaries);
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void InternalRequestUpdateExtent(int* extent, int* inExtent, int* wholeExtent);

private:
  svtkImageSpatialAlgorithm(const svtkImageSpatialAlgorithm&) = delete;
  void operator=(const svtkImageSpatialAlgorithm&) = delete;
};

#endif
