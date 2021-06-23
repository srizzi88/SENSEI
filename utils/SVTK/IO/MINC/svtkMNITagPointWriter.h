/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNITagPointWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
/**
 * @class   svtkMNITagPointWriter
 * @brief   A writer for MNI tag point files.
 *
 * The MNI .tag file format is used to store tag points, for use in
 * either registration or labeling of data volumes.  This file
 * format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Tag points can be stored for either one volume or two volumes,
 * and this filter can take one or two inputs.  Alternatively, the
 * points to be written can be specified by calling SetPoints().
 * @sa
 * svtkMINCImageReader svtkMNIObjectReader svtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkMNITagPointWriter_h
#define svtkMNITagPointWriter_h

#include "svtkIOMINCModule.h" // For export macro
#include "svtkWriter.h"

class svtkDataSet;
class svtkPointSet;
class svtkStringArray;
class svtkDoubleArray;
class svtkIntArray;
class svtkPoints;

class SVTKIOMINC_EXPORT svtkMNITagPointWriter : public svtkWriter
{
public:
  svtkTypeMacro(svtkMNITagPointWriter, svtkWriter);

  static svtkMNITagPointWriter* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".tag"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI tags"; }

  //@{
  /**
   * Set the points (unless you set them as inputs).
   */
  virtual void SetPoints(int port, svtkPoints* points);
  virtual void SetPoints(svtkPoints* points) { this->SetPoints(0, points); }
  virtual svtkPoints* GetPoints(int port);
  virtual svtkPoints* GetPoints() { return this->GetPoints(0); }
  //@}

  //@{
  /**
   * Set the labels (unless the input PointData has an
   * array called LabelText). Labels are optional.
   */
  virtual void SetLabelText(svtkStringArray* a);
  svtkGetObjectMacro(LabelText, svtkStringArray);
  //@}

  //@{
  /**
   * Set the weights (unless the input PointData has an
   * array called Weights).  Weights are optional.
   */
  virtual void SetWeights(svtkDoubleArray* a);
  svtkGetObjectMacro(Weights, svtkDoubleArray);
  //@}

  //@{
  /**
   * Set the structure ids (unless the input PointData has
   * an array called StructureIds).  These are optional.
   */
  virtual void SetStructureIds(svtkIntArray* a);
  svtkGetObjectMacro(StructureIds, svtkIntArray);
  //@}

  //@{
  /**
   * Set the structure ids (unless the input PointData has
   * an array called PatientIds).  These are optional.
   */
  virtual void SetPatientIds(svtkIntArray* a);
  svtkGetObjectMacro(PatientIds, svtkIntArray);
  //@}

  //@{
  /**
   * Set comments to be added to the file.
   */
  svtkSetStringMacro(Comments);
  svtkGetStringMacro(Comments);
  //@}

  /**
   * Write the file.
   */
  int Write() override;

  /**
   * Get the MTime.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify file name of svtk polygon data file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkMNITagPointWriter();
  ~svtkMNITagPointWriter() override;

  svtkPoints* Points[2];
  svtkStringArray* LabelText;
  svtkDoubleArray* Weights;
  svtkIntArray* StructureIds;
  svtkIntArray* PatientIds;
  char* Comments;

  void WriteData() override {}
  virtual void WriteData(svtkPointSet* inputs[2]);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;

  int FileType;

  ostream* OpenFile();
  void CloseFile(ostream* fp);

private:
  svtkMNITagPointWriter(const svtkMNITagPointWriter&) = delete;
  void operator=(const svtkMNITagPointWriter&) = delete;
};

#endif
