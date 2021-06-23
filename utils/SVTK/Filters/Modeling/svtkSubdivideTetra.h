/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSubdivideTetra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSubdivideTetra
 * @brief   subdivide one tetrahedron into twelve for every tetra
 *
 * This filter subdivides tetrahedra in an unstructured grid into twelve tetrahedra.
 */

#ifndef svtkSubdivideTetra_h
#define svtkSubdivideTetra_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkSubdivideTetra : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkSubdivideTetra* New();
  svtkTypeMacro(svtkSubdivideTetra, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkSubdivideTetra();
  ~svtkSubdivideTetra() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSubdivideTetra(const svtkSubdivideTetra&) = delete;
  void operator=(const svtkSubdivideTetra&) = delete;
};

#endif
