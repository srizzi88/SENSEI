/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPhyloXMLTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPhyloXMLTreeWriter
 * @brief   write svtkTree data to PhyloXML format.
 *
 * svtkPhyloXMLTreeWriter is writes a svtkTree to a PhyloXML formatted file
 * or string.
 */

#ifndef svtkPhyloXMLTreeWriter_h
#define svtkPhyloXMLTreeWriter_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkSmartPointer.h"    // For SP ivars
#include "svtkStdString.h"       // For get/set ivars
#include "svtkXMLWriter.h"

class svtkStringArray;
class svtkTree;
class svtkXMLDataElement;

class SVTKIOINFOVIS_EXPORT svtkPhyloXMLTreeWriter : public svtkXMLWriter
{
public:
  static svtkPhyloXMLTreeWriter* New();
  svtkTypeMacro(svtkPhyloXMLTreeWriter, svtkXMLWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTree* GetInput();
  svtkTree* GetInput(int port);
  //@}

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

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

  /**
   * Do not include name the VertexData array in the PhyloXML output
   * of this writer.  Call this function once for each array that
   * you wish to ignore.
   */
  void IgnoreArray(const char* arrayName);

protected:
  svtkPhyloXMLTreeWriter();
  ~svtkPhyloXMLTreeWriter() override {}

  int WriteData() override;

  const char* GetDataSetName() override;
  int StartFile() override;
  int EndFile() override;

  /**
   * Check for an optional, tree-level element and write it out if it is
   * found.
   */
  void WriteTreeLevelElement(svtkTree* input, svtkXMLDataElement* rootElement,
    const char* elementName, const char* attributeName);

  /**
   * Search for any tree-level properties and write them out if they are found.
   */
  void WriteTreeLevelProperties(svtkTree* input, svtkXMLDataElement* rootElement);

  /**
   * Convert one vertex to PhyloXML.  This function calls itself recursively
   * for any children of the input vertex.
   */
  void WriteCladeElement(svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* parentElement);

  /**
   * Write the branch length attribute for the specified vertex.
   */
  void WriteBranchLengthAttribute(
    svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element);

  /**
   * Write the name element for the specified vertex.
   */
  void WriteNameElement(svtkIdType vertex, svtkXMLDataElement* element);

  /**
   * Write the confidence element for the specified vertex.
   */
  void WriteConfidenceElement(svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element);

  /**
   * Write the color element and its subelements (red, green, blue)
   * for the specified vertex.
   */
  void WriteColorElement(svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element);

  /**
   * Write a property element as a child of the specified svtkXMLDataElement.
   */
  void WritePropertyElement(svtkAbstractArray* array, svtkIdType vertex, svtkXMLDataElement* element);

  /**
   * Get the value of the requested attribute from the specified array's
   * svtkInformation.
   */
  const char* GetArrayAttribute(svtkAbstractArray* array, const char* attributeName);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkInformation* InputInformation;

  svtkStdString EdgeWeightArrayName;
  svtkStdString NodeNameArrayName;

  svtkAbstractArray* EdgeWeightArray;
  svtkAbstractArray* NodeNameArray;
  svtkSmartPointer<svtkStringArray> Blacklist;

private:
  svtkPhyloXMLTreeWriter(const svtkPhyloXMLTreeWriter&) = delete;
  void operator=(const svtkPhyloXMLTreeWriter&) = delete;
};

#endif
