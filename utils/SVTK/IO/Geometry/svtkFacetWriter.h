/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkFacetWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFacetWriter
 * @brief   reads a dataset in Facet format
 *
 * svtkFacetWriter creates an unstructured grid dataset. It reads ASCII files
 * stored in Facet format
 *
 * The facet format looks like this:
 * FACET FILE ...
 * nparts
 * Part 1 name
 * 0
 * npoints 0 0
 * p1x p1y p1z
 * p2x p2y p2z
 * ...
 * 1
 * Part 1 name
 * ncells npointspercell
 * p1c1 p2c1 p3c1 ... pnc1 materialnum partnum
 * p1c2 p2c2 p3c2 ... pnc2 materialnum partnum
 * ...
 */

#ifndef svtkFacetWriter_h
#define svtkFacetWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkInformation;

class SVTKIOGEOMETRY_EXPORT svtkFacetWriter : public svtkPolyDataAlgorithm
{
public:
  static svtkFacetWriter* New();
  svtkTypeMacro(svtkFacetWriter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of Facet datafile to read
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * Write data
   */
  void Write();

  void WriteToStream(ostream* ost);

protected:
  svtkFacetWriter();
  ~svtkFacetWriter() override;

  // This is called by the superclass.
  // This is the method you should override.
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int WriteDataToStream(ostream* ost, svtkPolyData* data);

  char* FileName;
  ostream* OutputStream;

private:
  svtkFacetWriter(const svtkFacetWriter&) = delete;
  void operator=(const svtkFacetWriter&) = delete;
};

#endif
