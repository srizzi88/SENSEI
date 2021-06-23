/*=========================================================================

  Program:   ParaView
  Module:    svtkExodusIIReaderParser.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExodusIIReaderParser
 * @brief   internal parser used by svtkExodusIIReader.
 *
 * svtkExodusIIReaderParser is an internal XML parser used by svtkExodusIIReader.
 * This is not for public use.
 */

#ifndef svtkExodusIIReaderParser_h
#define svtkExodusIIReaderParser_h

#include "svtkIOExodusModule.h" // For export macro
#include "svtkSmartPointer.h"
#include "svtkXMLParser.h"

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

class svtkMutableDirectedGraph;
class svtkStringArray;
class svtkUnsignedCharArray;

class SVTKIOEXODUS_EXPORT svtkExodusIIReaderParser : public svtkXMLParser
{
public:
  static svtkExodusIIReaderParser* New();
  svtkTypeMacro(svtkExodusIIReaderParser, svtkXMLParser);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Returns the SIL.
   * This is valid only after Go().
   */
  svtkGetObjectMacro(SIL, svtkMutableDirectedGraph);
  //@}

  /**
   * Trigger parsing of the XML file.
   */
  void Go(const char* filename);

  // Returns if the parser has some information about the block with given "id".
  // This is valid only after Go().
  bool HasInformationAboutBlock(int id)
  {
    return (this->BlockID_To_VertexID.find(id) != this->BlockID_To_VertexID.end());
  }

  /**
   * Given a block "id" return the name as determined from the xml.
   * This is valid only after Go().
   */
  std::string GetBlockName(int id);

  //@{
  /**
   * Fills up the blockIdsSet with the block ids referred to by the XML.
   * This is valid only after Go().
   */
  void GetBlockIds(std::set<int>& blockIdsSet)
  {
    std::map<int, svtkIdType>::iterator iter;
    for (iter = this->BlockID_To_VertexID.begin(); iter != this->BlockID_To_VertexID.end(); ++iter)
    {
      blockIdsSet.insert(iter->first);
    }
  }
  //@}

protected:
  svtkExodusIIReaderParser();
  ~svtkExodusIIReaderParser() override;

  void StartElement(const char* tagName, const char** attrs) override;
  void EndElement(const char* tagName) override;
  void FinishedParsing();

  const char* GetValue(const char* attr, const char** attrs)
  {
    int i;
    for (i = 0; attrs[i]; i += 2)
    {
      const char* name = strrchr(attrs[i], ':');
      if (!name)
      {
        name = attrs[i];
      }
      else
      {
        name++;
      }
      if (strcmp(attr, name) == 0)
      {
        return attrs[i + 1];
      }
    }
    return nullptr;
  }

  // Convenience methods to add vertices/edges to the SIL.
  svtkIdType AddVertexToSIL(const char* name);
  svtkIdType AddChildEdgeToSIL(svtkIdType src, svtkIdType dst);
  svtkIdType AddCrossEdgeToSIL(svtkIdType src, svtkIdType dst);

  /**
   * Returns the vertex id for the "part" with given
   * part_number_instance_string formed as
   * "{part-number} Instance: {part-instance}"
   */
  svtkIdType GetPartVertex(const char* part_number_instance_string);

  // For each of the blocks, this maps the "id" attribute in the XML to the
  // vertex id for the block in the SIL.
  std::map<int, svtkIdType> BlockID_To_VertexID;

  // Maps block "id"s to material names.
  std::map<int, std::string> BlockID_To_MaterialName;

  // Maps material name to vertex id.
  // This will be build only if <material-list> is present in the XML.
  std::map<std::string, svtkIdType> MaterialName_To_VertexID;

  std::map<svtkIdType, std::string> PartVertexID_To_Descriptions;

  // These save the values read from <material-specification /> element present
  // withint the <part /> elements.
  // key: part vertex id
  // value: material name = (desp + spec)
  std::map<svtkIdType, std::string> MaterialSpecifications;

  // Maps the "{part-number} Instance: {part-instance}" key for the vertex id
  // for the part vertex in the Assemblies hierarchy.
  std::map<std::string, svtkIdType> Part_To_VertexID;

  // Maps a block-id to the "{part-number} Instance: {part-instance}" string.
  std::map<int, std::string> BlockID_To_Part;

  svtkMutableDirectedGraph* SIL;
  svtkSmartPointer<svtkStringArray> NamesArray;
  svtkSmartPointer<svtkUnsignedCharArray> CrossEdgesArray;

  std::string BlockPartNumberString;

  svtkIdType RootVertex;
  svtkIdType BlocksVertex;
  svtkIdType AssembliesVertex;
  svtkIdType MaterialsVertex;
  std::vector<svtkIdType> CurrentVertex;

  bool InBlocks;
  bool InMaterialAssignments;

private:
  svtkExodusIIReaderParser(const svtkExodusIIReaderParser&) = delete;
  void operator=(const svtkExodusIIReaderParser&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkExodusIIReaderParser.h
