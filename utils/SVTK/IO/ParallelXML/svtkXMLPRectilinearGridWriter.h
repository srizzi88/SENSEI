/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPRectilinearGridWriter
 * @brief   Write PSVTK XML RectilinearGrid files.
 *
 * svtkXMLPRectilinearGridWriter writes the PSVTK XML RectilinearGrid
 * file format.  One rectilinear grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtr".
 * This writer uses svtkXMLRectilinearGridWriter to write the
 * individual piece files.
 *
 * @sa
 * svtkXMLRectilinearGridWriter
 */

#ifndef svtkXMLPRectilinearGridWriter_h
#define svtkXMLPRectilinearGridWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPStructuredDataWriter.h"

class svtkRectilinearGrid;

class SVTKIOPARALLELXML_EXPORT svtkXMLPRectilinearGridWriter : public svtkXMLPStructuredDataWriter
{
public:
  static svtkXMLPRectilinearGridWriter* New();
  svtkTypeMacro(svtkXMLPRectilinearGridWriter, svtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkRectilinearGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPRectilinearGridWriter();
  ~svtkXMLPRectilinearGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  const char* GetDataSetName() override;
  svtkXMLStructuredDataWriter* CreateStructuredPieceWriter() override;
  void WritePData(svtkIndent indent) override;

private:
  svtkXMLPRectilinearGridWriter(const svtkXMLPRectilinearGridWriter&) = delete;
  void operator=(const svtkXMLPRectilinearGridWriter&) = delete;
};

#endif
