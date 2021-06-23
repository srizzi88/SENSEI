/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyLineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyLineSource
 * @brief   create a poly line from a list of input points
 *
 * svtkPolyLineSource is a source object that creates a poly line from
 * user-specified points. The output is a svtkPolyLine.
 */

#ifndef svtkPolyLineSource_h
#define svtkPolyLineSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyPointSource.h"

class svtkPoints;

class SVTKFILTERSSOURCES_EXPORT svtkPolyLineSource : public svtkPolyPointSource
{
public:
  static svtkPolyLineSource* New();
  svtkTypeMacro(svtkPolyLineSource, svtkPolyPointSource);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set whether to close the poly line by connecting the last and first points.
   */
  svtkSetMacro(Closed, svtkTypeBool);
  svtkGetMacro(Closed, svtkTypeBool);
  svtkBooleanMacro(Closed, svtkTypeBool);
  //@}

protected:
  svtkPolyLineSource();
  ~svtkPolyLineSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool Closed;

private:
  svtkPolyLineSource(const svtkPolyLineSource&) = delete;
  void operator=(const svtkPolyLineSource&) = delete;
};

#endif
