/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkRectilinearGridToPointSet
 * @brief   Converts a svtkRectilinearGrid to a svtkPointSet
 *
 *
 * svtkRectilinearGridToPointSet takes a svtkRectilinearGrid as an image and
 * outputs an equivalent svtkStructuredGrid (which is a subclass of
 * svtkPointSet).
 *
 * @par Thanks:
 * This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
 * Sandia National Laboratories.
 */

#ifndef svtkRectilinearGridToPointSet_h
#define svtkRectilinearGridToPointSet_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkRectilinearGrid;
class svtkStructuredData;

class SVTKFILTERSGENERAL_EXPORT svtkRectilinearGridToPointSet : public svtkStructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkRectilinearGridToPointSet, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkRectilinearGridToPointSet* New();

protected:
  svtkRectilinearGridToPointSet();
  ~svtkRectilinearGridToPointSet() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkRectilinearGridToPointSet(const svtkRectilinearGridToPointSet&) = delete;
  void operator=(const svtkRectilinearGridToPointSet&) = delete;

  int CopyStructure(svtkStructuredGrid* outData, svtkRectilinearGrid* inData);
};

#endif // svtkRectilinearGridToPointSet_h
