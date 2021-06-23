/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPUnstructuredDataWriter
 * @brief   Superclass for PSVTK XML unstructured data writers.
 *
 * svtkXMLPUnstructuredDataWriter provides PSVTK XML writing
 * functionality that is common among all the parallel unstructured
 * data formats.
 */

#ifndef svtkXMLPUnstructuredDataWriter_h
#define svtkXMLPUnstructuredDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPDataWriter.h"

class svtkPointSet;
class svtkXMLUnstructuredDataWriter;

class SVTKIOPARALLELXML_EXPORT svtkXMLPUnstructuredDataWriter : public svtkXMLPDataWriter
{
public:
  svtkTypeMacro(svtkXMLPUnstructuredDataWriter, svtkXMLPDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPUnstructuredDataWriter();
  ~svtkXMLPUnstructuredDataWriter() override;

  svtkPointSet* GetInputAsPointSet();
  virtual svtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter() = 0;
  svtkXMLWriter* CreatePieceWriter(int index) override;
  void WritePData(svtkIndent indent) override;

private:
  svtkXMLPUnstructuredDataWriter(const svtkXMLPUnstructuredDataWriter&) = delete;
  void operator=(const svtkXMLPUnstructuredDataWriter&) = delete;
};

#endif
