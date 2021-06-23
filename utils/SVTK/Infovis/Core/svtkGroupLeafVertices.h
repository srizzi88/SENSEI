/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGroupLeafVertices.h

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
 * @class   svtkGroupLeafVertices
 * @brief   Filter that expands a tree, categorizing leaf vertices
 *
 *
 * Use SetInputArrayToProcess(0, ...) to set the array to group on.
 * Currently this array must be a svtkStringArray.
 */

#ifndef svtkGroupLeafVertices_h
#define svtkGroupLeafVertices_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkGroupLeafVertices : public svtkTreeAlgorithm
{
public:
  static svtkGroupLeafVertices* New();
  svtkTypeMacro(svtkGroupLeafVertices, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name of the domain that non-leaf vertices will be assigned to.
   * If the input graph already contains vertices in this domain:
   * - If the ids for this domain are numeric, starts assignment with max id
   * - If the ids for this domain are strings, starts assignment with "group X"
   * where "X" is the max id.
   * Default is "group_vertex".
   */
  svtkSetStringMacro(GroupDomain);
  svtkGetStringMacro(GroupDomain);
  //@}

protected:
  svtkGroupLeafVertices();
  ~svtkGroupLeafVertices() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* GroupDomain;

private:
  svtkGroupLeafVertices(const svtkGroupLeafVertices&) = delete;
  void operator=(const svtkGroupLeafVertices&) = delete;
};

#endif
