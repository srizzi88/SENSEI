/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLPDataWriter
 * @brief   Write data in a parallel XML format.
 *
 * svtkXMLPDataWriter is the superclass for all XML parallel data set
 * writers.  It provides functionality needed for writing parallel
 * formats, such as the selection of which writer writes the summary
 * file and what range of pieces are assigned to each serial writer.
 */

#ifndef svtkXMLPDataWriter_h
#define svtkXMLPDataWriter_h

#include "svtkIOParallelXMLModule.h" // For export macro
#include "svtkXMLPDataObjectWriter.h"

class svtkCallbackCommand;
class svtkMultiProcessController;

class SVTKIOPARALLELXML_EXPORT svtkXMLPDataWriter : public svtkXMLPDataObjectWriter
{
public:
  svtkTypeMacro(svtkXMLPDataWriter, svtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkXMLPDataWriter();
  ~svtkXMLPDataWriter() override;

  virtual svtkXMLWriter* CreatePieceWriter(int index) = 0;

  void WritePData(svtkIndent indent) override;

  int WritePieceInternal() override;

  int WritePiece(int index) override;

  void WritePrimaryElementAttributes(ostream& os, svtkIndent indent) override;

private:
  svtkXMLPDataWriter(const svtkXMLPDataWriter&) = delete;
  void operator=(const svtkXMLPDataWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

#endif
