/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCacheFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCacheFilter
 * @brief   Caches multiple svtkImageData objects.
 *
 *
 * svtkImageCacheFilter keep a number of svtkImageDataObjects from previous
 * updates to satisfy future updates without needing to update the input.  It
 * does not change the data at all.  It just makes the pipeline more
 * efficient at the expense of using extra memory.
 */

#ifndef svtkImageCacheFilter_h
#define svtkImageCacheFilter_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingCoreModule.h" // For export macro

class svtkExecutive;

class SVTKIMAGINGCORE_EXPORT svtkImageCacheFilter : public svtkImageAlgorithm
{
public:
  static svtkImageCacheFilter* New();
  svtkTypeMacro(svtkImageCacheFilter, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * This is the maximum number of images that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  int GetCacheSize();
  //@}

protected:
  svtkImageCacheFilter();
  ~svtkImageCacheFilter() override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;
  void ExecuteData(svtkDataObject*) override;

private:
  svtkImageCacheFilter(const svtkImageCacheFilter&) = delete;
  void operator=(const svtkImageCacheFilter&) = delete;
};

#endif
