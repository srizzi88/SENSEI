/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineCornerSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOutlineCornerSource
 * @brief   create wireframe outline corners around bounding box
 *
 * svtkOutlineCornerSource creates wireframe outline corners around a user-specified
 * bounding box.
 */

#ifndef svtkOutlineCornerSource_h
#define svtkOutlineCornerSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkOutlineSource.h"

class SVTKFILTERSSOURCES_EXPORT svtkOutlineCornerSource : public svtkOutlineSource
{
public:
  svtkTypeMacro(svtkOutlineCornerSource, svtkOutlineSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct outline corner source with default corner factor = 0.2
   */
  static svtkOutlineCornerSource* New();

  //@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  svtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  svtkGetMacro(CornerFactor, double);
  //@}

protected:
  svtkOutlineCornerSource();
  ~svtkOutlineCornerSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double CornerFactor;

private:
  svtkOutlineCornerSource(const svtkOutlineCornerSource&) = delete;
  void operator=(const svtkOutlineCornerSource&) = delete;
};

#endif
