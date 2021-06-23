/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPUnstructuredGridWriter
 * @brief   Write PSVTK XML UnstructuredGrid files.
 *
 * svtkXMLPUnstructuredGridWriter writes the PSVTK XML UnstructuredGrid
 * file format.  One unstructured grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtu".
 * This writer uses svtkXMLUnstructuredGridWriter to write the
 * individual piece files.
 *
 * @sa
 * svtkXMLUnstructuredGridWriter
 */

#ifndef svtkXMLPUnstructuredGridWriter_h
#define svtkXMLPUnstructuredGridWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPUnstructuredDataWriter.h"

class svtkUnstructuredGridBase;

class SVTKIOPARALLELXML_EXPORT svtkXMLPUnstructuredGridWriter : public svtkXMLPUnstructuredDataWriter
{
public:
  static svtkXMLPUnstructuredGridWriter* New();
  svtkTypeMacro(svtkXMLPUnstructuredGridWriter, svtkXMLPUnstructuredDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  svtkUnstructuredGridBase* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  svtkXMLPUnstructuredGridWriter();
  ~svtkXMLPUnstructuredGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  const char* GetDataSetName() override;
  svtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter() override;

private:
  svtkXMLPUnstructuredGridWriter(const svtkXMLPUnstructuredGridWriter&) = delete;
  void operator=(const svtkXMLPUnstructuredGridWriter&) = delete;
};

#endif
