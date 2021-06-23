/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBlueObeliskData.h"
#include "svtkChemistryConfigure.h"

#include <fstream>

int GenerateBlueObeliskHeader(int, char*[])
{
  svtksys::ifstream xml(SVTK_BODR_DATA_PATH_BUILD "/elements.xml");
  if (!xml)
  {
    std::cerr << "Error opening file " SVTK_BODR_DATA_PATH_BUILD "/elements.xml.\n";
    return EXIT_FAILURE;
  }

  std::cout << "// SVTK/Domains/Chemistry/Testing/Cxx/GenerateBlueObeliskHeader.cxx\n";
  if (!svtkBlueObeliskData::GenerateHeaderFromXML(xml, std::cout))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
