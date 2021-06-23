/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPDataSetWriter
 * @brief   Write any type of PSVTK XML file.
 *
 * svtkXMLPDataSetWriter is a wrapper around the PSVTK XML file format
 * writers.  Given an input svtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * svtkXMLPImageDataWriter svtkXMLPStructuredGridWriter
 * svtkXMLPRectilinearGridWriter svtkXMLPPolyDataWriter
 * svtkXMLPUnstructuredGridWriter
 */

#ifndef svtkXMLPDataSetWriter_h
#define svtkXMLPDataSetWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPDataWriter.h"

class SVTKIOPARALLELXML_EXPORT svtkXMLPDataSetWriter : public svtkXMLPDataWriter
{
public:
  svtkTypeMacro(svtkXMLPDataSetWriter, svtkXMLPDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkXMLPDataSetWriter* New();

  /**
   * Get/Set the writer's input.
   */
  svtkDataSet* GetInput();

protected:
  svtkXMLPDataSetWriter();
  ~svtkXMLPDataSetWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Override writing method from superclass.
  int WriteInternal() override;

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName() override;
  const char* GetDefaultFileExtension() override;
  svtkXMLWriter* CreatePieceWriter(int index) override;

private:
  svtkXMLPDataSetWriter(const svtkXMLPDataSetWriter&) = delete;
  void operator=(const svtkXMLPDataSetWriter&) = delete;
};

#endif
