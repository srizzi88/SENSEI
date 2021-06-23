/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataToPointSet.h

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
 * @class   svtkImageDataToPointSet
 * @brief   Converts a svtkImageData to a svtkPointSet
 *
 *
 * svtkImageDataToPointSet takes a svtkImageData as an image and outputs an
 * equivalent svtkStructuredGrid (which is a subclass of svtkPointSet).
 *
 * @par Thanks:
 * This class was developed by Kenneth Moreland (kmorel@sandia.gov) from
 * Sandia National Laboratories.
 */

#ifndef svtkImageDataToPointSet_h
#define svtkImageDataToPointSet_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkImageData;
class svtkStructuredData;

class SVTKFILTERSGENERAL_EXPORT svtkImageDataToPointSet : public svtkStructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkImageDataToPointSet, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkImageDataToPointSet* New();

protected:
  svtkImageDataToPointSet();
  ~svtkImageDataToPointSet() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkImageDataToPointSet(const svtkImageDataToPointSet&) = delete;
  void operator=(const svtkImageDataToPointSet&) = delete;
};

#endif // svtkImageDataToPointSet_h
