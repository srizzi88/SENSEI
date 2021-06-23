/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveIsolatedVertices.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkRemoveIsolatedVertices
 * @brief   remove vertices of a svtkGraph with
 *    degree zero.
 *
 *
 */

#ifndef svtkRemoveIsolatedVertices_h
#define svtkRemoveIsolatedVertices_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkDataSet;

class SVTKINFOVISCORE_EXPORT svtkRemoveIsolatedVertices : public svtkGraphAlgorithm
{
public:
  static svtkRemoveIsolatedVertices* New();
  svtkTypeMacro(svtkRemoveIsolatedVertices, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkRemoveIsolatedVertices();
  ~svtkRemoveIsolatedVertices() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkRemoveIsolatedVertices(const svtkRemoveIsolatedVertices&) = delete;
  void operator=(const svtkRemoveIsolatedVertices&) = delete;
};

#endif
