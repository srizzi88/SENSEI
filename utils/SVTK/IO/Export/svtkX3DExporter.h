/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkX3DExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkX3DExporter
 * @brief   create an x3d file
 *
 * svtkX3DExporter is a render window exporter which writes out the renderered
 * scene into an X3D file. X3D is an XML-based format for representation
 * 3D scenes (similar to VRML). Check out http://www.web3d.org/x3d/ for more
 * details.
 * @par Thanks:
 * X3DExporter is contributed by Christophe Mouton at EDF.
 */

#ifndef svtkX3DExporter_h
#define svtkX3DExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

class svtkActor;
class svtkActor2D;
class svtkDataArray;
class svtkLight;
class svtkPoints;
class svtkPolyData;
class svtkRenderer;
class svtkUnsignedCharArray;
class svtkX3DExporterWriter;

class SVTKIOEXPORT_EXPORT svtkX3DExporter : public svtkExporter
{
public:
  static svtkX3DExporter* New();
  svtkTypeMacro(svtkX3DExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the output file name.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the Speed of navigation. Default is 4.
   */
  svtkSetMacro(Speed, double);
  svtkGetMacro(Speed, double);
  //@}

  //@{
  /**
   * Turn on binary mode
   */
  svtkSetClampMacro(Binary, svtkTypeBool, 0, 1);
  svtkBooleanMacro(Binary, svtkTypeBool);
  svtkGetMacro(Binary, svtkTypeBool);
  //@}

  //@{
  /**
   * In binary mode use fastest instead of best compression
   */
  svtkSetClampMacro(Fastest, svtkTypeBool, 0, 1);
  svtkBooleanMacro(Fastest, svtkTypeBool);
  svtkGetMacro(Fastest, svtkTypeBool);
  //@}

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

protected:
  svtkX3DExporter();
  ~svtkX3DExporter() override;

  // Stream management
  svtkTypeBool WriteToOutputString;
  char* OutputString;
  svtkIdType OutputStringLength;

  /**
   * Write data to output.
   */
  void WriteData() override;

  void WriteALight(svtkLight* aLight, svtkX3DExporterWriter* writer);
  void WriteAnActor(svtkActor* anActor, svtkX3DExporterWriter* writer, int index);
  void WriteAPiece(svtkPolyData* piece, svtkActor* anActor, svtkX3DExporterWriter* writer, int index);
  void WritePointData(svtkPoints* points, svtkDataArray* normals, svtkDataArray* tcoords,
    svtkUnsignedCharArray* colors, svtkX3DExporterWriter* writer, int index);
  void WriteATextActor2D(svtkActor2D* anTextActor2D, svtkX3DExporterWriter* writer);
  void WriteATexture(svtkActor* anActor, svtkX3DExporterWriter* writer);
  void WriteAnAppearance(svtkActor* anActor, bool writeEmissiveColor, svtkX3DExporterWriter* writer);

  // Called to give subclasses a chance to write additional nodes to the file.
  // Default implementation does nothing.
  virtual void WriteAdditionalNodes(svtkX3DExporterWriter* svtkNotUsed(writer)) {}

  int HasHeadLight(svtkRenderer* ren);

  char* FileName;
  double Speed;
  svtkTypeBool Binary;
  svtkTypeBool Fastest;

private:
  svtkX3DExporter(const svtkX3DExporter&) = delete;
  void operator=(const svtkX3DExporter&) = delete;
};

#endif
