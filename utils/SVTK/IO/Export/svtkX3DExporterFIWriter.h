/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporterFIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkX3DExporterFIWriter
 *
 */

#ifndef svtkX3DExporterFIWriter_h
#define svtkX3DExporterFIWriter_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkX3DExporterWriter.h"

class svtkX3DExporterFIByteWriter;
class svtkX3DExporterFINodeInfoStack;
class svtkZLibDataCompressor;

class SVTKIOEXPORT_EXPORT svtkX3DExporterFIWriter : public svtkX3DExporterWriter
{
public:
  static svtkX3DExporterFIWriter* New();
  svtkTypeMacro(svtkX3DExporterFIWriter, svtkX3DExporterWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void CloseFile() override;
  int OpenFile(const char* file) override;
  int OpenStream() override;

  // void Write(const char* str);

  void Flush() override;

  void StartDocument() override;
  void EndDocument() override;

  // Elements
  void StartNode(int elementID) override;
  void EndNode() override;

  // Attributes
  // SFString / MFString
  // void SetField(int attributeID, const std::string &value);
  void SetField(int attributeID, const char*, bool mfstring = false) override;
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

  // MFInt32
  void SetField(int attributeID, int type, svtkCellArray* a);
  void SetField(int attributeID, const int* values, size_t size, bool image = false) override;

  //@{
  /**
   * Use fastest instead of best compression
   */
  svtkSetClampMacro(Fastest, svtkTypeBool, 0, 1);
  svtkBooleanMacro(Fastest, svtkTypeBool);
  svtkGetMacro(Fastest, svtkTypeBool);
  //@}

protected:
  svtkX3DExporterFIWriter();
  ~svtkX3DExporterFIWriter() override;

private:
  void StartAttribute(int attributeID, bool literal, bool addToTable = false);
  void EndAttribute();

  void CheckNode(bool callerIsAttribute = true);
  bool IsLineFeedEncodingOn;

  // int Depth;
  svtkX3DExporterFIByteWriter* Writer;
  svtkX3DExporterFINodeInfoStack* InfoStack;
  svtkZLibDataCompressor* Compressor;

  svtkTypeBool Fastest;

  svtkX3DExporterFIWriter(const svtkX3DExporterFIWriter&) = delete;
  void operator=(const svtkX3DExporterFIWriter&) = delete;
};

#endif
