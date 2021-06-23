/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporterWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkX3DExporterWriter
 * @brief   X3D Exporter Writer
 *
 * svtkX3DExporterWriter is the definition for
 * classes that implement a encoding for the
 * X3D exporter
 */

#ifndef svtkX3DExporterWriter_h
#define svtkX3DExporterWriter_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkObject.h"

// Forward declarations
class svtkDataArray;
class svtkUnsignedCharArray;
class svtkCellArray;

class SVTKIOEXPORT_EXPORT svtkX3DExporterWriter : public svtkObject
{
public:
  svtkTypeMacro(svtkX3DExporterWriter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Opens the file specified with file
   * returns 1 if successful otherwise 0
   */
  virtual int OpenFile(const char* file) = 0;

  /**
   * Init data support to be a stream instead of a file
   */
  virtual int OpenStream() = 0;

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  svtkSetMacro(WriteToOutputString, svtkTypeBool);
  svtkGetMacro(WriteToOutputString, svtkTypeBool);
  svtkBooleanMacro(WriteToOutputString, svtkTypeBool);
  //@}

  //@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  svtkGetMacro(OutputStringLength, svtkIdType);
  svtkGetStringMacro(OutputString);
  unsigned char* GetBinaryOutputString()
  {
    return reinterpret_cast<unsigned char*>(this->OutputString);
  }
  //@}

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char* RegisterAndGetOutputString();

  // Closes the file if open
  virtual void CloseFile() = 0;
  // Flush can be called optionally after some operations to
  // flush the buffer to the filestream. A writer not necessarily
  // implements this function
  virtual void Flush() {}

  /**
   * Starts a document and sets all necessary information,
   * i.e. the header of the implemented encoding
   */
  virtual void StartDocument() = 0;

  /**
   * Ends a document and sets all necessary information
   * or necessary bytes to finish the encoding correctly
   */
  virtual void EndDocument() = 0;

  //@{
  /**
   * Starts/ends a new X3D node specified via nodeID. The list of
   * nodeIds can be found in svtkX3DExportWriterSymbols.h. The EndNode
   * function closes the last open node. So there must be
   * corresponding Start/EndNode() calls for every node
   */
  virtual void StartNode(int nodeID) = 0;
  virtual void EndNode() = 0;
  //@}

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is SFString and MFString
   * virtual void SetField(int attributeID, const std::string &value) = 0;
   */
  virtual void SetField(int attributeID, const char* value, bool mfstring = false) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is SFInt32
   */
  virtual void SetField(int attributeID, int) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is SFFloat
   */
  virtual void SetField(int attributeID, float) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is SFDouble
   */
  virtual void SetField(int attributeID, double) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is SFBool
   */
  virtual void SetField(int attributeID, bool) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is specified with type
   * Supported types: SFVEC3F, SFCOLOR, SFROTATION
   */
  virtual void SetField(int attributeID, int type, const double* a) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is specified with type
   * Supported types: MFVEC3F, MFVEC2F
   */
  virtual void SetField(int attributeID, int type, svtkDataArray* a) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is specified with type
   * Supported types: MFCOLOR
   */
  virtual void SetField(int attributeID, const double* values, size_t size) = 0;

  /**
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is specified with type
   * It is possible to specify that the field is an
   * image for optimized formatting or compression
   * Supported types: MFINT32, SFIMAGE
   */
  virtual void SetField(int attributeID, const int* values, size_t size, bool image = false) = 0;

  /*
   * Sets the field specified with attributeID
   * of the active node to the given value.
   * The type of the field is specified with type
   * Supported types: MFString
   */
  // virtual void SetField(int attributeID, int type, std::string) = 0;

protected:
  svtkX3DExporterWriter();
  ~svtkX3DExporterWriter() override;

  char* OutputString;
  svtkIdType OutputStringLength;
  svtkTypeBool WriteToOutputString;

private:
  svtkX3DExporterWriter(const svtkX3DExporterWriter&) = delete;
  void operator=(const svtkX3DExporterWriter&) = delete;
};
#endif
