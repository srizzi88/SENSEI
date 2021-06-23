/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkFacetReader.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFacetReader
 * @brief   reads a dataset in Facet format
 *
 * svtkFacetReader creates a poly data dataset. It reads ASCII files
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

#ifndef svtkFacetReader_h
#define svtkFacetReader_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSHYBRID_EXPORT svtkFacetReader : public svtkPolyDataAlgorithm
{
public:
  static svtkFacetReader* New();
  svtkTypeMacro(svtkFacetReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of Facet datafile to read
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  static int CanReadFile(const char* filename);

protected:
  svtkFacetReader();
  ~svtkFacetReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;

private:
  svtkFacetReader(const svtkFacetReader&) = delete;
  void operator=(const svtkFacetReader&) = delete;
};

#endif
