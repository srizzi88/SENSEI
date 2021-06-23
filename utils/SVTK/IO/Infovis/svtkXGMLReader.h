/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXGMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//-------------------------------------------------------------------------
// Copyright 2008 Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//-------------------------------------------------------------------------

/**
 * @class   svtkXGMLReader
 * @brief   Reads XGML graph files.
 * This reader is developed for a simple graph file format based
 * loosely on the "GML" notation.  This implementation is based
 * heavily on the svtkTulipReader class that forms part of the
 * Titan toolkit.
 *
 * @par Thanks:
 * Thanks to David Duke from the University of Leeds for providing this
 * implementation.
 */

#ifndef svtkXGMLReader_h
#define svtkXGMLReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkUndirectedGraphAlgorithm.h"

class SVTKIOINFOVIS_EXPORT svtkXGMLReader : public svtkUndirectedGraphAlgorithm
{
public:
  static svtkXGMLReader* New();
  svtkTypeMacro(svtkXGMLReader, svtkUndirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The XGML file name.
   */
  svtkGetStringMacro(FileName);
  svtkSetStringMacro(FileName);
  //@}

protected:
  svtkXGMLReader();
  ~svtkXGMLReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  char* FileName;

  svtkXGMLReader(const svtkXGMLReader&) = delete;
  void operator=(const svtkXGMLReader&) = delete;
};

#endif // svtkXGMLReader_h
