/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCountFaces.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCountFaces
 * @brief   Add a cell data array containing the number of faces
 * per cell.
 *
 *
 * This filter adds a cell data array containing the number of faces per cell.
 */

#ifndef svtkCountFaces_h
#define svtkCountFaces_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkCountFaces : public svtkPassInputTypeAlgorithm
{
public:
  static svtkCountFaces* New();
  svtkTypeMacro(svtkCountFaces, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the new output array containing the face counts.
   */
  svtkSetStringMacro(OutputArrayName);
  svtkGetStringMacro(OutputArrayName);
  //@}

protected:
  svtkCountFaces();
  ~svtkCountFaces() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  char* OutputArrayName;

private:
  svtkCountFaces(const svtkCountFaces&) = delete;
  void operator=(const svtkCountFaces&) = delete;
};

#endif // svtkCountFaces_h
