/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNewickTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNewickTreeWriter
 * @brief   write svtkTree data to Newick format.
 *
 * svtkNewickTreeWriter is writes a svtkTree to a Newick formatted file
 * or string.
 */

#ifndef svtkNewickTreeWriter_h
#define svtkNewickTreeWriter_h

#include "svtkDataWriter.h"
#include "svtkIOInfovisModule.h" // For export macro
#include "svtkStdString.h"       // For get/set ivars

class svtkTree;

class SVTKIOINFOVIS_EXPORT svtkNewickTreeWriter : public svtkDataWriter
{
public:
  static svtkNewickTreeWriter* New();
  svtkTypeMacro(svtkNewickTreeWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTree* GetInput();
  svtkTree* GetInput(int port);
  //@}

  //@{
  /**
   * Get/Set the name of the input's tree edge weight array.
   * This array must be part of the input tree's EdgeData.
   * The default name is "weight".  If this array cannot be
   * found, then no edge weights will be included in the
   * output of this writer.
   */
  svtkGetMacro(EdgeWeightArrayName, svtkStdString);
  svtkSetMacro(EdgeWeightArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Get/Set the name of the input's tree node name array.
   * This array must be part of the input tree's VertexData.
   * The default name is "node name".  If this array cannot
   * be found, then no node names will be included in the
   * output of this writer.
   */
  svtkGetMacro(NodeNameArrayName, svtkStdString);
  svtkSetMacro(NodeNameArrayName, svtkStdString);
  //@}

protected:
  svtkNewickTreeWriter();
  ~svtkNewickTreeWriter() override {}

  void WriteData() override;

  /**
   * Write one vertex.  This function calls itself recursively for
   * any children of the input vertex.
   */
  void WriteVertex(ostream* fp, svtkTree* const input, svtkIdType vertex);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkStdString EdgeWeightArrayName;
  svtkStdString NodeNameArrayName;

  svtkAbstractArray* EdgeWeightArray;
  svtkAbstractArray* NodeNameArray;

private:
  svtkNewickTreeWriter(const svtkNewickTreeWriter&) = delete;
  void operator=(const svtkNewickTreeWriter&) = delete;
};

#endif
