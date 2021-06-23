/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCountVertices.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCountVertices
 * @brief   Add a cell data array containing the number of
 * vertices per cell.
 *
 *
 * This filter adds a cell data array containing the number of vertices per
 * cell.
 */

#ifndef svtkCountVertices_h
#define svtkCountVertices_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkCountVertices : public svtkPassInputTypeAlgorithm
{
public:
  static svtkCountVertices* New();
  svtkTypeMacro(svtkCountVertices, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the new output array containing the vertex counts.
   */
  svtkSetStringMacro(OutputArrayName);
  svtkGetStringMacro(OutputArrayName);
  //@}

protected:
  svtkCountVertices();
  ~svtkCountVertices() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  char* OutputArrayName;

private:
  svtkCountVertices(const svtkCountVertices&) = delete;
  void operator=(const svtkCountVertices&) = delete;
};

#endif // svtkCountVertices_h
