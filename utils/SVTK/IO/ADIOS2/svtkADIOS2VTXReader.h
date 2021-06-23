/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkADIOS2VTXReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * svtkADIOS2VTXReader.h  public facing class
 *                     enables reading adios2 bp files using the
 *                     SVTK ADIOS2 Readers (VTX) developed
 *                     at Oak Ridge National Laboratory
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef svtkADIOS2VTXReader_h
#define svtkADIOS2VTXReader_h

#include <memory> // std::unique_ptr

#include "svtkIOADIOS2Module.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

// forward declaring to keep it private
namespace vtx
{
class VTXSchemaManager;
}

class svtkIndent;
class svtkInformation;
class svtkInformationvector;

class SVTKIOADIOS2_EXPORT svtkADIOS2VTXReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkADIOS2VTXReader* New();
  svtkTypeMacro(svtkADIOS2VTXReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent index) override;

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

protected:
  svtkADIOS2VTXReader();
  ~svtkADIOS2VTXReader() = default;

  svtkADIOS2VTXReader(const svtkADIOS2VTXReader&) = delete;
  void operator=(const svtkADIOS2VTXReader&) = delete;

  int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector);
  int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector);
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector);

private:
  char* FileName;
  std::unique_ptr<vtx::VTXSchemaManager> SchemaManager;
};

#endif /* svtkADIOS2VTXReader_h */
