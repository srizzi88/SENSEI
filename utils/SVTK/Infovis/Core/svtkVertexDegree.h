/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVertexDegree.h

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
 * @class   svtkVertexDegree
 * @brief   Adds an attribute array with the degree of each vertex
 *
 *
 * Adds an attribute array with the degree of each vertex. By default the name
 * of the array will be "VertexDegree", but that can be changed by calling
 * SetOutputArrayName("foo");
 */

#ifndef svtkVertexDegree_h
#define svtkVertexDegree_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkVertexDegree : public svtkGraphAlgorithm
{
public:
  static svtkVertexDegree* New();

  svtkTypeMacro(svtkVertexDegree, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the output array name. If no output array name is
   * set then the name 'VertexDegree' is used.
   */
  svtkSetStringMacro(OutputArrayName);
  //@}

protected:
  svtkVertexDegree();
  ~svtkVertexDegree() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  char* OutputArrayName;

  svtkVertexDegree(const svtkVertexDegree&) = delete;
  void operator=(const svtkVertexDegree&) = delete;
};

#endif
