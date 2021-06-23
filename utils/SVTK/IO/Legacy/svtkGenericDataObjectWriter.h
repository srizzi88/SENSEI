/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericDataObjectWriter
 * @brief   writes any type of svtk data object to file
 *
 * svtkGenericDataObjectWriter is a concrete class that writes data objects
 * to disk. The input to this object is any subclass of svtkDataObject.
 */

#ifndef svtkGenericDataObjectWriter_h
#define svtkGenericDataObjectWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class SVTKIOLEGACY_EXPORT svtkGenericDataObjectWriter : public svtkDataWriter
{
public:
  static svtkGenericDataObjectWriter* New();
  svtkTypeMacro(svtkGenericDataObjectWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkGenericDataObjectWriter();
  ~svtkGenericDataObjectWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGenericDataObjectWriter(const svtkGenericDataObjectWriter&) = delete;
  void operator=(const svtkGenericDataObjectWriter&) = delete;
};

#endif
