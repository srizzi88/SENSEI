/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveDuplicatePolys.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRemoveDuplicatePolys
 * @brief   remove duplicate/degenerate polygons
 *
 * svtkRemoveDuplicatePolys is a filter that removes duplicate or degenerate
 * polygons. Assumes the input grid does not contain duplicate points. You
 * may want to run svtkCleanPolyData first to assert it. If duplicated
 * polygons are found they are removed in the output.
 *
 * @sa
 * svtkCleanPolyData
 */

#ifndef svtkRemoveDuplicatePolys_h
#define svtkRemoveDuplicatePolys_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSCORE_EXPORT svtkRemoveDuplicatePolys : public svtkPolyDataAlgorithm
{
public:
  static svtkRemoveDuplicatePolys* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkRemoveDuplicatePolys, svtkPolyDataAlgorithm);

protected:
  svtkRemoveDuplicatePolys();
  ~svtkRemoveDuplicatePolys() override;

  // Usual data generation method.
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkRemoveDuplicatePolys(const svtkRemoveDuplicatePolys&) = delete;
  void operator=(const svtkRemoveDuplicatePolys&) = delete;
};

#endif
