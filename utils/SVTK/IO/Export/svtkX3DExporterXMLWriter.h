/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporterXMLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkX3DExporterXMLWriter
 * @brief   X3D Exporter XML Writer
 *
 * svtkX3DExporterXMLWriter
 */

#ifndef svtkX3DExporterXMLWriter_h
#define svtkX3DExporterXMLWriter_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkX3DExporterWriter.h"
#include <string> // for std::string

class svtkX3DExporterXMLNodeInfoStack;

class SVTKIOEXPORT_EXPORT svtkX3DExporterXMLWriter : public svtkX3DExporterWriter
{

public:
  static svtkX3DExporterXMLWriter* New();
  svtkTypeMacro(svtkX3DExporterXMLWriter, svtkX3DExporterWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void CloseFile() override;
  int OpenFile(const char* file) override;
  void Flush() override;

  int OpenStream() override;

  void StartDocument() override;
  void EndDocument() override;

  // Elements
  void StartNode(int elementID) override;
  void EndNode() override;

  // Attributes
  // SFString / MFString
  void SetField(int attributeID, const char*, bool mfstring = true) override;
  // SFInt32
  void SetField(int attributeID, int) override;
  // SFFloat
  void SetField(int attributeID, float) override;
  // SFDouble
  void SetField(int attributeID, double) override;
  // SFBool
  void SetField(int attributeID, bool) override;

  // For MFxxx attributes
  void SetField(int attributeID, int type, const double* a) override;
  void SetField(int attributeID, int type, svtkDataArray* a) override;
  void SetField(int attributeID, const double* values, size_t size) override;
  // MFInt32, SFIMAGE
  void SetField(int attributeID, const int* values, size_t size, bool image = false) override;

protected:
  svtkX3DExporterXMLWriter();
  ~svtkX3DExporterXMLWriter() override;

private:
  const char* GetNewline() { return "\n"; }
  void AddDepth();
  void SubDepth();

  std::string ActTab;
  int Depth;
  ostream* OutputStream;
  svtkX3DExporterXMLNodeInfoStack* InfoStack;

  svtkX3DExporterXMLWriter(const svtkX3DExporterXMLWriter&) = delete;
  void operator=(const svtkX3DExporterXMLWriter&) = delete;
};

#endif
