/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 */

#ifndef svtkPPolyDataNormals_h
#define svtkPPolyDataNormals_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataNormals.h"

class SVTKFILTERSPARALLEL_EXPORT svtkPPolyDataNormals : public svtkPolyDataNormals
{
public:
  svtkTypeMacro(svtkPPolyDataNormals, svtkPolyDataNormals);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPPolyDataNormals* New();

  //@{
  /**
   * To get piece invariance, this filter has to request an
   * extra ghost level.  By default piece invariance is on.
   */
  svtkSetMacro(PieceInvariant, svtkTypeBool);
  svtkGetMacro(PieceInvariant, svtkTypeBool);
  svtkBooleanMacro(PieceInvariant, svtkTypeBool);
  //@}

protected:
  svtkPPolyDataNormals();
  ~svtkPPolyDataNormals() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PieceInvariant;

private:
  svtkPPolyDataNormals(const svtkPPolyDataNormals&) = delete;
  void operator=(const svtkPPolyDataNormals&) = delete;
};

#endif
