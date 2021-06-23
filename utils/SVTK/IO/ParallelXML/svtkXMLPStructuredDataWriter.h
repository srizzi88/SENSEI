/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPStructuredDataWriter
 * @brief   Superclass for PSVTK XML structured data writers.
 *
 * svtkXMLPStructuredDataWriter provides PSVTK XML writing functionality
 * that is common among all the parallel structured data formats.
 */

#ifndef svtkXMLPStructuredDataWriter_h
#define svtkXMLPStructuredDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPDataWriter.h"

#include <map>    // for keeping track of extents
#include <vector> // for keeping track of extents

class svtkXMLStructuredDataWriter;

class SVTKIOPARALLELXML_EXPORT svtkXMLPStructuredDataWriter : public svtkXMLPDataWriter
{
public:
  svtkTypeMacro(svtkXMLPStructuredDataWriter, svtkXMLPDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPStructuredDataWriter();
  ~svtkXMLPStructuredDataWriter() override;

  virtual svtkXMLStructuredDataWriter* CreateStructuredPieceWriter() = 0;
  void WritePrimaryElementAttributes(ostream& os, svtkIndent indent) override;
  void WritePPieceAttributes(int index) override;
  svtkXMLWriter* CreatePieceWriter(int index) override;

  int WriteInternal() override;

  void PrepareSummaryFile() override;
  int WritePiece(int index) override;

private:
  svtkXMLPStructuredDataWriter(const svtkXMLPStructuredDataWriter&) = delete;
  void operator=(const svtkXMLPStructuredDataWriter&) = delete;

  typedef std::map<int, std::vector<int> > ExtentsType;
  ExtentsType Extents;
};

#endif
