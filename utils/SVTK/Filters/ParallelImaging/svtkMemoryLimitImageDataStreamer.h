/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMemoryLimitImageDataStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMemoryLimitImageDataStreamer
 * @brief   Initiates streaming on image data.
 *
 * To satisfy a request, this filter calls update on its input
 * many times with smaller update extents.  All processing up stream
 * streams smaller pieces.
 */

#ifndef svtkMemoryLimitImageDataStreamer_h
#define svtkMemoryLimitImageDataStreamer_h

#include "svtkFiltersParallelImagingModule.h" // For export macro
#include "svtkImageDataStreamer.h"

class SVTKFILTERSPARALLELIMAGING_EXPORT svtkMemoryLimitImageDataStreamer : public svtkImageDataStreamer
{
public:
  static svtkMemoryLimitImageDataStreamer* New();
  svtkTypeMacro(svtkMemoryLimitImageDataStreamer, svtkImageDataStreamer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the memory limit in kibibytes (1024 bytes).
   */
  svtkSetMacro(MemoryLimit, unsigned long);
  svtkGetMacro(MemoryLimit, unsigned long);
  //@}

  // See the svtkAlgorithm for a description of what these do
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkMemoryLimitImageDataStreamer();
  ~svtkMemoryLimitImageDataStreamer() override {}

  unsigned long MemoryLimit;

private:
  svtkMemoryLimitImageDataStreamer(const svtkMemoryLimitImageDataStreamer&) = delete;
  void operator=(const svtkMemoryLimitImageDataStreamer&) = delete;
};

#endif
