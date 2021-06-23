/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataObjectWriter
 * @brief   write svtk field data
 *
 * svtkDataObjectWriter is a source object that writes ASCII or binary
 * field data files in svtk format. Field data is a general form of data in
 * matrix form.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 *
 * @sa
 * svtkFieldData svtkFieldDataReader
 */

#ifndef svtkDataObjectWriter_h
#define svtkDataObjectWriter_h

#include "svtkDataWriter.h"     // Needs data because it calls methods on it
#include "svtkIOLegacyModule.h" // For export macro
#include "svtkStdString.h"      // For string used in api
#include "svtkWriter.h"

class SVTKIOLEGACY_EXPORT svtkDataObjectWriter : public svtkWriter
{
public:
  static svtkDataObjectWriter* New();
  svtkTypeMacro(svtkDataObjectWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods delegated to svtkDataWriter, see svtkDataWriter.
   */
  void SetFileName(const char* filename) { this->Writer->SetFileName(filename); }
  char* GetFileName() { return this->Writer->GetFileName(); }
  void SetHeader(const char* header) { this->Writer->SetHeader(header); }
  char* GetHeader() { return this->Writer->GetHeader(); }
  void SetFileType(int type) { this->Writer->SetFileType(type); }
  int GetFileType() { return this->Writer->GetFileType(); }
  void SetFileTypeToASCII() { this->Writer->SetFileType(SVTK_ASCII); }
  void SetFileTypeToBinary() { this->Writer->SetFileType(SVTK_BINARY); }
  void SetWriteToOutputString(int b) { this->Writer->SetWriteToOutputString(b); }
  void WriteToOutputStringOn() { this->Writer->WriteToOutputStringOn(); }
  void WriteToOutputStringOff() { this->Writer->WriteToOutputStringOff(); }
  int GetWriteToOutputString() { return this->Writer->GetWriteToOutputString(); }
  char* GetOutputString() { return this->Writer->GetOutputString(); }
  svtkStdString GetOutputStdString() { return this->Writer->GetOutputStdString(); }
  svtkIdType GetOutputStringLength() { return this->Writer->GetOutputStringLength(); }
  unsigned char* GetBinaryOutputString() { return this->Writer->GetBinaryOutputString(); }
  void SetFieldDataName(const char* fieldname) { this->Writer->SetFieldDataName(fieldname); }
  char* GetFieldDataName() { return this->Writer->GetFieldDataName(); }
  //@}

protected:
  svtkDataObjectWriter();
  ~svtkDataObjectWriter() override;

  void WriteData() override;
  svtkDataWriter* Writer;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDataObjectWriter(const svtkDataObjectWriter&) = delete;
  void operator=(const svtkDataObjectWriter&) = delete;
};

#endif
