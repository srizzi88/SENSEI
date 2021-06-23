/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractArray.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkExtractArray
 * @brief   Given a svtkArrayData object containing one-or-more
 * svtkArray instances, produces a svtkArrayData containing just one svtkArray,
 * identified by index.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkExtractArray_h
#define svtkExtractArray_h

#include "svtkArrayDataAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkExtractArray : public svtkArrayDataAlgorithm
{
public:
  static svtkExtractArray* New();
  svtkTypeMacro(svtkExtractArray, svtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Controls which array will be extracted.
   */
  svtkGetMacro(Index, svtkIdType);
  svtkSetMacro(Index, svtkIdType);
  //@}

protected:
  svtkExtractArray();
  ~svtkExtractArray() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkExtractArray(const svtkExtractArray&) = delete;
  void operator=(const svtkExtractArray&) = delete;

  svtkIdType Index;
};

#endif
