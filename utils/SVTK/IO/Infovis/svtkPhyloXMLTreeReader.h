/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPhyloXMLTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPhyloXMLTreeReader
 * @brief   read svtkTree from PhyloXML formatted file
 *
 * svtkPhyloXMLTreeReader is a source object that reads PhyloXML tree format
 * files.
 * The output of this reader is a single svtkTree data object.
 *
 *
 * @warning
 * This reader does not implement the entire PhyloXML specification.
 * It currently only supports the following tags:
 * phylogeny, name, description, confidence, property, clade, branch_length,
 * color, red, green, and blue.
 * This reader also only supports a single phylogeny per file.
 *
 * @sa
 * svtkTree svtkXMLReader svtkPhyloXMLTreeWriter
 */

#ifndef svtkPhyloXMLTreeReader_h
#define svtkPhyloXMLTreeReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkSmartPointer.h"    // For SP ivar
#include "svtkXMLReader.h"

class svtkBitArray;
class svtkMutableDirectedGraph;
class svtkTree;
class svtkXMLDataElement;

class SVTKIOINFOVIS_EXPORT svtkPhyloXMLTreeReader : public svtkXMLReader
{
public:
  static svtkPhyloXMLTreeReader* New();
  svtkTypeMacro(svtkPhyloXMLTreeReader, svtkXMLReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkTree* GetOutput();
  svtkTree* GetOutput(int idx);
  //@}

protected:
  svtkPhyloXMLTreeReader();
  ~svtkPhyloXMLTreeReader() override;

  /**
   * Read the input PhyloXML and populate our output svtkTree.
   */
  void ReadXMLData() override;

  /**
   * Read one particular XML element.  This method calls the more specific
   * methods (ReadCladeElement, ReadNameElement, etc) based on what type
   * of tag it encounters.
   */
  void ReadXMLElement(svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Read a clade element.  This method does not parse the subelements of
   * the clade.  Instead, this task is handled by other methods of this class.
   * This method returns the svtkIdType of the newly created vertex in our
   * output svtkTree.
   */
  svtkIdType ReadCladeElement(
    svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType parent);

  /**
   * Read a name and assign it to the specified vertex, or the whole tree
   * if vertex is -1.
   */
  void ReadNameElement(svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Read the description for the tree.
   */
  void ReadDescriptionElement(svtkXMLDataElement* element, svtkMutableDirectedGraph* g);

  /**
   * Read a property and assign it to our output svtkTree's VertexData for the
   * specified vertex.  If this property has not been encountered yet, this
   * method creates a new array and adds it to the VertexData.
   */
  void ReadPropertyElement(
    svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Read & store the branch length for this clade.  Branch length is defined
   * as the edge weight from this vertex to its parent.  Note that this
   * value can also be specified as an attribute of the clade element.
   */
  void ReadBranchLengthElement(
    svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Read confidence value and store it for the specified vertex, or the
   * whole tree is vertex is -1.
   */
  void ReadConfidenceElement(
    svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Read RGB color value for this vertex.  Note that this color is also
   * applied to all children of this vertex until a new value is specified.
   */
  void ReadColorElement(svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex);

  /**
   * Assign the parent's branch color to child vertices where none is
   * otherwise specified.
   */
  void PropagateBranchColor(svtkTree* tree);

  /**
   * Count the number of vertices in the tree.
   */
  void CountNodes(svtkXMLDataElement* element);

  /**
   * Return a copy of the input string with all leading & trailing
   * whitespace removed.
   */
  std::string GetTrimmedString(const char* input);

  /**
   * Return the portion of the input string that occurs before the
   * first colon (:).
   */
  std::string GetStringBeforeColon(const char* input);

  /**
   * Return the portion of the input string that occurs after the
   * first colon (:).
   */
  std::string GetStringAfterColon(const char* input);

  int FillOutputPortInformation(int, svtkInformation*) override;
  const char* GetDataSetName() override;
  void SetOutput(svtkTree* output);
  void SetupEmptyOutput() override;

private:
  svtkIdType NumberOfNodes;
  bool HasBranchColor;
  svtkSmartPointer<svtkBitArray> ColoredVertices;
  svtkPhyloXMLTreeReader(const svtkPhyloXMLTreeReader&) = delete;
  void operator=(const svtkPhyloXMLTreeReader&) = delete;
};

#endif
