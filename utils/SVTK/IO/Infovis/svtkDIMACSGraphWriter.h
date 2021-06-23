/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIMACSGraphWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkDIMACSGraphWriter
 * @brief   write svtkGraph data to a DIMACS
 * formatted file
 *
 *
 * svtkDIMACSGraphWriter is a sink object that writes
 * svtkGraph data files into a generic DIMACS (.gr) format.
 *
 * Output files contain a problem statement line:
 *
 * p graph <num_verts> <num_edges>
 *
 * Followed by |E| edge descriptor lines that are formatted as:
 *
 * e <source> <target> <weight>
 *
 * Vertices are numbered from 1..n in DIMACS formatted files.
 *
 * See webpage for format details.
 * http://prolland.free.fr/works/research/dsat/dimacs.html
 *
 * @sa
 * svtkDIMACSGraphReader
 *
 */

#ifndef svtkDIMACSGraphWriter_h
#define svtkDIMACSGraphWriter_h

#include "svtkDataWriter.h"
#include "svtkIOInfovisModule.h" // For export macro

class svtkGraph;

class SVTKIOINFOVIS_EXPORT svtkDIMACSGraphWriter : public svtkDataWriter
{
public:
  static svtkDIMACSGraphWriter* New();
  svtkTypeMacro(svtkDIMACSGraphWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkGraph* GetInput();
  svtkGraph* GetInput(int port);
  //@}

protected:
  svtkDIMACSGraphWriter() {}
  ~svtkDIMACSGraphWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDIMACSGraphWriter(const svtkDIMACSGraphWriter&) = delete;
  void operator=(const svtkDIMACSGraphWriter&) = delete;
};

#endif
