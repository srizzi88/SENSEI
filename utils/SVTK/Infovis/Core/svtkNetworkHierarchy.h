/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetworkHierarchy.h

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
 * @class   svtkNetworkHierarchy
 * @brief   Filter that takes a graph and makes a
 * tree out of the network ip addresses in that graph.
 *
 *
 * Use SetInputArrayToProcess(0, ...) to set the array to that has
 * the network ip addresses.
 * Currently this array must be a svtkStringArray.
 */

#ifndef svtkNetworkHierarchy_h
#define svtkNetworkHierarchy_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTreeAlgorithm.h"

class svtkStdString;

class SVTKINFOVISCORE_EXPORT svtkNetworkHierarchy : public svtkTreeAlgorithm
{
public:
  static svtkNetworkHierarchy* New();
  svtkTypeMacro(svtkNetworkHierarchy, svtkTreeAlgorithm);

  //@{
  /**
   * Used to store the ip array name
   */
  svtkGetStringMacro(IPArrayName);
  svtkSetStringMacro(IPArrayName);
  //@}

  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkNetworkHierarchy();
  ~svtkNetworkHierarchy() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;
  int FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;

private:
  svtkNetworkHierarchy(const svtkNetworkHierarchy&) = delete;
  void operator=(const svtkNetworkHierarchy&) = delete;

  // Internal helper functions
  unsigned int ITON(const svtkStdString& ip);
  void GetSubnets(unsigned int packedIP, int* subnets);

  char* IPArrayName;
};

#endif
