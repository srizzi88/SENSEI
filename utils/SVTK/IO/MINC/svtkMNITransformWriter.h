/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNITransformWriter.h

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
 * @class   svtkMNITransformWriter
 * @brief   A writer for MNI transformation files.
 *
 * The MNI .xfm file format is used to store geometrical
 * transformations.  Three kinds of transformations are supported by
 * the file format: affine, thin-plate spline, and grid transformations.
 * This file format was developed at the McConnell Brain Imaging Centre
 * at the Montreal Neurological Institute and is used by their software.
 * @sa
 * svtkMINCImageWriter svtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to SVTK.
 */

#ifndef svtkMNITransformWriter_h
#define svtkMNITransformWriter_h

#include "svtkAlgorithm.h"
#include "svtkIOMINCModule.h" // For export macro

class svtkAbstractTransform;
class svtkHomogeneousTransform;
class svtkThinPlateSplineTransform;
class svtkGridTransform;
class svtkCollection;

class SVTKIOMINC_EXPORT svtkMNITransformWriter : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkMNITransformWriter, svtkAlgorithm);

  static svtkMNITransformWriter* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the file name.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".xfm"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI Transform"; }

  //@{
  /**
   * Set the transform.
   */
  virtual void SetTransform(svtkAbstractTransform* transform);
  virtual svtkAbstractTransform* GetTransform() { return this->Transform; }
  //@}

  /**
   * Add another transform to the file.  The next time that
   * SetTransform is called, all added transforms will be
   * removed.
   */
  virtual void AddTransform(svtkAbstractTransform* transform);

  /**
   * Get the number of transforms that will be written.
   */
  virtual int GetNumberOfTransforms();

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
  virtual void Write();

protected:
  svtkMNITransformWriter();
  ~svtkMNITransformWriter() override;

  char* FileName;
  svtkAbstractTransform* Transform;
  svtkCollection* Transforms;
  char* Comments;

  int WriteLinearTransform(ostream& outfile, svtkHomogeneousTransform* transform);
  int WriteThinPlateSplineTransform(ostream& outfile, svtkThinPlateSplineTransform* transform);
  int WriteGridTransform(ostream& outfile, svtkGridTransform* transform);

  virtual int WriteTransform(ostream& outfile, svtkAbstractTransform* transform);

  virtual int WriteFile();

  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

private:
  svtkMNITransformWriter(const svtkMNITransformWriter&) = delete;
  void operator=(const svtkMNITransformWriter&) = delete;
};

#endif
