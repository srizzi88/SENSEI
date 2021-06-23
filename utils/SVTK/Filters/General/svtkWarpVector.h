/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpVector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWarpVector
 * @brief   deform geometry with vector data
 *
 * svtkWarpVector is a filter that modifies point coordinates by moving
 * points along vector times the scale factor. Useful for showing flow
 * profiles or mechanical deformation.
 *
 * The filter passes both its point data and cell data to its output.
 */

#ifndef svtkWarpVector_h
#define svtkWarpVector_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkWarpVector : public svtkPointSetAlgorithm
{
public:
  static svtkWarpVector* New();
  svtkTypeMacro(svtkWarpVector, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify value to scale displacement.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkWarpVector();
  ~svtkWarpVector() override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double ScaleFactor;

private:
  svtkWarpVector(const svtkWarpVector&) = delete;
  void operator=(const svtkWarpVector&) = delete;
};

#endif
