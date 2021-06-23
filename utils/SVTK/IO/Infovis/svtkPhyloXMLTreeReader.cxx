/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPhyloXMLTreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPhyloXMLTreeReader.h"

#include "svtkBitArray.h"
#include "svtkCharArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkShortArray.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"

svtkStandardNewMacro(svtkPhyloXMLTreeReader);

//----------------------------------------------------------------------------
svtkPhyloXMLTreeReader::svtkPhyloXMLTreeReader()
{
  svtkTree* output = svtkTree::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallelism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  this->NumberOfNodes = 0;
  this->HasBranchColor = false;
}

//----------------------------------------------------------------------------
svtkPhyloXMLTreeReader::~svtkPhyloXMLTreeReader() = default;

//----------------------------------------------------------------------------
svtkTree* svtkPhyloXMLTreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkTree* svtkPhyloXMLTreeReader::GetOutput(int idx)
{
  return svtkTree::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::SetOutput(svtkTree* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
const char* svtkPhyloXMLTreeReader::GetDataSetName()
{
  return "phylogeny";
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::SetupEmptyOutput()
{
  this->GetOutput()->Initialize();
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadXMLData()
{
  svtkXMLDataElement* rootElement = this->XMLParser->GetRootElement();
  this->CountNodes(rootElement);
  svtkNew<svtkMutableDirectedGraph> builder;

  // Initialize the edge weight array
  svtkNew<svtkDoubleArray> weights;
  weights->SetNumberOfComponents(1);
  weights->SetName("weight");
  // the number of edges = number of nodes -1 for a tree
  weights->SetNumberOfValues(this->NumberOfNodes - 1);
  weights->FillComponent(0, 0.0);
  builder->GetEdgeData()->AddArray(weights);

  // Initialize the names array
  svtkNew<svtkStringArray> names;
  names->SetNumberOfComponents(1);
  names->SetName("node name");
  names->SetNumberOfValues(this->NumberOfNodes);
  builder->GetVertexData()->AddArray(names);

  // parse the input file to create the tree
  this->ReadXMLElement(rootElement, builder, -1);

  svtkTree* output = this->GetOutput();
  if (!output->CheckedDeepCopy(builder))
  {
    svtkErrorMacro(<< "Edges do not create a valid tree.");
    return;
  }

  // assign branch color from parent to child where none was specified.
  this->PropagateBranchColor(output);

  // check if our input file contained edge weight information
  bool haveWeights = false;
  for (svtkIdType i = 0; i < weights->GetNumberOfTuples(); ++i)
  {
    if (weights->GetValue(i) != 0.0)
    {
      haveWeights = true;
      break;
    }
  }
  if (!haveWeights)
  {
    return;
  }

  svtkNew<svtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfValues(output->GetNumberOfVertices());

  // set node weights
  svtkNew<svtkTreeDFSIterator> treeIterator;
  treeIterator->SetStartVertex(output->GetRoot());
  treeIterator->SetTree(output);
  while (treeIterator->HasNext())
  {
    svtkIdType vertex = treeIterator->Next();
    svtkIdType parent = output->GetParent(vertex);
    double weight = 0.0;
    if (parent >= 0)
    {
      weight = weights->GetValue(output->GetEdgeId(parent, vertex));
      weight += nodeWeights->GetValue(parent);
    }
    nodeWeights->SetValue(vertex, weight);
  }

  nodeWeights->SetName("node weight");
  output->GetVertexData()->AddArray(nodeWeights);
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::CountNodes(svtkXMLDataElement* element)
{
  if (strcmp(element->GetName(), "clade") == 0)
  {
    this->NumberOfNodes++;
  }

  int numNested = element->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    this->CountNodes(element->GetNestedElement(i));
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadXMLElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  bool inspectNested = true;
  if (strcmp(element->GetName(), "clade") == 0)
  {
    svtkIdType child = this->ReadCladeElement(element, g, vertex);
    // update our current vertex to this newly created one.
    vertex = child;
  }

  else if (strcmp(element->GetName(), "name") == 0)
  {
    this->ReadNameElement(element, g, vertex);
  }
  else if (strcmp(element->GetName(), "description") == 0)
  {
    this->ReadDescriptionElement(element, g);
  }

  else if (strcmp(element->GetName(), "property") == 0)
  {
    this->ReadPropertyElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "branch_length") == 0)
  {
    this->ReadBranchLengthElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "confidence") == 0)
  {
    this->ReadConfidenceElement(element, g, vertex);
  }

  else if (strcmp(element->GetName(), "color") == 0)
  {
    this->ReadColorElement(element, g, vertex);
    inspectNested = false;
  }

  else if (strcmp(element->GetName(), "phyloxml") != 0 &&
    strcmp(element->GetName(), "phylogeny") != 0)
  {
    svtkWarningMacro(<< "Unsupported PhyloXML tag encountered: " << element->GetName());
  }

  if (!inspectNested)
  {
    return;
  }

  int numNested = element->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    this->ReadXMLElement(element->GetNestedElement(i), g, vertex);
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkPhyloXMLTreeReader::ReadCladeElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType parent)
{
  // add a new vertex to the graph
  svtkIdType vertex = -1;
  if (parent == -1)
  {
    vertex = g->AddVertex();
  }
  else
  {
    vertex = g->AddChild(parent);
    // check for branch length attribute
    double weight = 0.0;
    element->GetScalarAttribute("branch_length", weight);
    g->GetEdgeData()->GetAbstractArray("weight")->SetVariantValue(
      g->GetEdgeId(parent, vertex), svtkVariant(weight));
  }

  // set a default (blank) name for this vertex here since
  // svtkStringArray does not support a default value.
  g->GetVertexData()->GetAbstractArray("node name")->SetVariantValue(vertex, svtkVariant(""));

  return vertex;
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadNameElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  std::string name;
  if (element->GetCharacterData() != nullptr)
  {
    name = this->GetTrimmedString(element->GetCharacterData());
  }
  // support for phylogeny-level name (as opposed to clade-level name)
  if (vertex == -1)
  {
    svtkNew<svtkStringArray> treeName;
    treeName->SetNumberOfComponents(1);
    treeName->SetName("phylogeny.name");
    treeName->SetNumberOfValues(1);
    treeName->SetValue(0, name);
    g->GetVertexData()->AddArray(treeName);
  }
  else
  {
    g->GetVertexData()->GetAbstractArray("node name")->SetVariantValue(vertex, svtkVariant(name));
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadDescriptionElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g)
{
  std::string description;
  if (element->GetCharacterData() != nullptr)
  {
    description = this->GetTrimmedString(element->GetCharacterData());
  }
  svtkNew<svtkStringArray> treeDescription;
  treeDescription->SetNumberOfComponents(1);
  treeDescription->SetName("phylogeny.description");
  treeDescription->SetNumberOfValues(1);
  treeDescription->SetValue(0, description);
  g->GetVertexData()->AddArray(treeDescription);
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadPropertyElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  const char* datatype = element->GetAttribute("datatype");
  if (!datatype)
  {
    svtkErrorMacro(<< "property element is missing the datatype attribute");
    return;
  }

  const char* ref = element->GetAttribute("ref");
  if (!ref)
  {
    svtkErrorMacro(<< "property element is missing the ref attribute");
    return;
  }

  const char* appliesTo = element->GetAttribute("applies_to");
  if (!appliesTo)
  {
    svtkErrorMacro(<< "property element is missing the applies_to attribute");
    return;
  }

  // get the name of this property from the ref tag.
  std::string propertyName = "property.";
  propertyName += this->GetStringAfterColon(ref);

  // get the authority for this property from the ref tag
  std::string authority = this->GetStringBeforeColon(ref);

  // get what type of data will be stored in this array.
  std::string typeOfData = this->GetStringAfterColon(datatype);

  // get the value for this property as a string
  std::string propertyValue = this->GetTrimmedString(element->GetCharacterData());

  // check if this property applies to a clade, or to the whole tree
  unsigned int numValues = this->NumberOfNodes;
  if (vertex == -1)
  {
    propertyName = "phylogeny." + propertyName;
    numValues = 1;
    vertex = 0;
  }

  if (typeOfData.compare("string") == 0 || typeOfData.compare("duration") == 0 ||
    typeOfData.compare("dateTime") == 0 || typeOfData.compare("time") == 0 ||
    typeOfData.compare("date") == 0 || typeOfData.compare("gYearMonth") == 0 ||
    typeOfData.compare("gYear") == 0 || typeOfData.compare("gMonthDay") == 0 ||
    typeOfData.compare("gDay") == 0 || typeOfData.compare("gMonth") == 0 ||
    typeOfData.compare("anyURI") == 0 || typeOfData.compare("normalizedString") == 0 ||
    typeOfData.compare("token") == 0 || typeOfData.compare("hexBinary") == 0 ||
    typeOfData.compare("base64Binary") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkStringArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(propertyValue));
  }

  else if (typeOfData.compare("boolean") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkBitArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    int prop = 0;
    if (propertyValue.compare("true") == 0 || propertyValue.compare("1") == 0)
    {
      prop = 1;
    }
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("decimal") == 0 || typeOfData.compare("float") == 0 ||
    typeOfData.compare("double") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkDoubleArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    double prop = strtod(propertyValue.c_str(), nullptr);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("int") == 0 || typeOfData.compare("integer") == 0 ||
    typeOfData.compare("nonPositiveInteger") == 0 || typeOfData.compare("negativeInteger") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkIntArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    int prop = strtol(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("long") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkLongArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    long prop = strtol(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("short") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkShortArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    short prop = strtol(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("byte") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkCharArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    char prop = strtol(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  else if (typeOfData.compare("nonNegativeInteger") == 0 ||
    typeOfData.compare("positiveInteger") == 0 || typeOfData.compare("unsignedInt") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkUnsignedIntArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    unsigned int prop = strtoul(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedLong") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkUnsignedLongArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    unsigned long prop = strtoul(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedShort") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkUnsignedShortArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    unsigned short prop = strtoul(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }
  else if (typeOfData.compare("unsignedByte") == 0)
  {
    if (!g->GetVertexData()->HasArray(propertyName.c_str()))
    {
      svtkNew<svtkUnsignedCharArray> propertyArray;
      propertyArray->SetNumberOfComponents(1);
      propertyArray->SetNumberOfValues(numValues);
      propertyArray->SetName(propertyName.c_str());
      g->GetVertexData()->AddArray(propertyArray);
    }
    unsigned char prop = strtoul(propertyValue.c_str(), nullptr, 0);
    g->GetVertexData()
      ->GetAbstractArray(propertyName.c_str())
      ->SetVariantValue(vertex, svtkVariant(prop));
  }

  svtkAbstractArray* propertyArray = g->GetVertexData()->GetAbstractArray(propertyName.c_str());

  // add annotations to this array if it was just created.
  if (propertyArray->GetInformation()->GetNumberOfKeys() == 0)
  {
    // authority (required attribute)
    svtkInformationStringKey* authorityKey =
      svtkInformationStringKey::MakeKey("authority", "svtkPhyloXMLTreeReader");
    propertyArray->GetInformation()->Set(authorityKey, authority.c_str());

    // applies_to (required attribute)
    svtkInformationStringKey* appliesToKey =
      svtkInformationStringKey::MakeKey("applies_to", "svtkPhyloXMLTreeReader");
    propertyArray->GetInformation()->Set(appliesToKey, appliesTo);

    // unit (optional attribute)
    const char* unit = element->GetAttribute("unit");
    if (unit)
    {
      svtkInformationStringKey* unitKey =
        svtkInformationStringKey::MakeKey("unit", "svtkPhyloXMLTreeReader");
      propertyArray->GetInformation()->Set(unitKey, unit);
    }
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadBranchLengthElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  std::string weightStr = this->GetTrimmedString(element->GetCharacterData());
  double weight = strtod(weightStr.c_str(), nullptr);

  // This assumes that the vertex only has one incoming edge.
  // We have to use GetInEdge because g does not have a GetParent()
  // method.
  g->GetEdgeData()->GetAbstractArray("weight")->SetVariantValue(
    g->GetInEdge(vertex, 0).Id, svtkVariant(weight));
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadConfidenceElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  // get the confidence value
  double confidence = 0.0;
  if (element->GetCharacterData() != nullptr)
  {
    std::string confidenceStr = this->GetTrimmedString(element->GetCharacterData());
    confidence = strtod(confidenceStr.c_str(), nullptr);
  }

  // get the confidence type
  const char* type = element->GetAttribute("type");

  // support for phylogeny-level name (as opposed to clade-level name)
  if (vertex == -1)
  {
    svtkNew<svtkDoubleArray> treeConfidence;
    treeConfidence->SetNumberOfComponents(1);
    treeConfidence->SetName("phylogeny.confidence");
    treeConfidence->SetNumberOfValues(1);
    treeConfidence->SetValue(0, confidence);

    // add the confidence type as an Information type on this array
    svtkInformationStringKey* key =
      svtkInformationStringKey::MakeKey("type", "svtkPhyloXMLTreeReader");
    treeConfidence->GetInformation()->Set(key, type);

    g->GetVertexData()->AddArray(treeConfidence);
  }
  else
  {
    if (!g->GetVertexData()->HasArray("confidence"))
    {
      svtkNew<svtkDoubleArray> confidenceArray;
      confidenceArray->SetNumberOfComponents(1);
      confidenceArray->SetNumberOfValues(this->NumberOfNodes);
      confidenceArray->SetName("confidence");

      // add the confidence type as an Information type on this array
      svtkInformationStringKey* key =
        svtkInformationStringKey::MakeKey("type", "svtkPhyloXMLTreeReader");
      confidenceArray->GetInformation()->Set(key, type);

      g->GetVertexData()->AddArray(confidenceArray);
    }
    g->GetVertexData()
      ->GetAbstractArray("confidence")
      ->SetVariantValue(vertex, svtkVariant(confidence));
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::ReadColorElement(
  svtkXMLDataElement* element, svtkMutableDirectedGraph* g, svtkIdType vertex)
{
  // get the color values
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  int numNested = element->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* childElement = element->GetNestedElement(i);
    if (childElement->GetCharacterData() == nullptr)
    {
      continue;
    }
    std::string childVal = this->GetTrimmedString(childElement->GetCharacterData());
    unsigned char val = static_cast<unsigned char>(strtod(childVal.c_str(), nullptr));
    if (strcmp(childElement->GetName(), "red") == 0)
    {
      red = val;
    }
    else if (strcmp(childElement->GetName(), "green") == 0)
    {
      green = val;
    }
    else if (strcmp(childElement->GetName(), "blue") == 0)
    {
      blue = val;
    }
  }

  // initialize the color array if necessary
  if (!g->GetVertexData()->HasArray("color"))
  {
    svtkNew<svtkUnsignedCharArray> colorArray;
    colorArray->SetNumberOfComponents(3);
    colorArray->SetComponentName(0, "red");
    colorArray->SetComponentName(1, "green");
    colorArray->SetComponentName(2, "blue");
    colorArray->SetNumberOfTuples(this->NumberOfNodes);
    colorArray->SetName("color");
    colorArray->FillComponent(0, 0);
    colorArray->FillComponent(1, 0);
    colorArray->FillComponent(2, 0);
    g->GetVertexData()->AddArray(colorArray);
    this->HasBranchColor = true;

    // also set up an array so we can keep track of which vertices
    // have color.
    this->ColoredVertices = svtkSmartPointer<svtkBitArray>::New();
    this->ColoredVertices->SetNumberOfComponents(1);
    this->ColoredVertices->SetName("colored vertices");
    for (int i = 0; i < this->NumberOfNodes; ++i)
    {
      this->ColoredVertices->InsertNextValue(0);
    }
  }

  // store this color value in the array
  svtkUnsignedCharArray* colorArray =
    svtkArrayDownCast<svtkUnsignedCharArray>(g->GetVertexData()->GetAbstractArray("color"));
  colorArray->SetTuple3(vertex, red, green, blue);
  this->ColoredVertices->SetValue(vertex, 1);
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::PropagateBranchColor(svtkTree* tree)
{
  if (!this->HasBranchColor)
  {
    return;
  }

  svtkUnsignedCharArray* colorArray =
    svtkArrayDownCast<svtkUnsignedCharArray>(tree->GetVertexData()->GetAbstractArray("color"));
  if (!colorArray)
  {
    return;
  }

  for (svtkIdType vertex = 1; vertex < tree->GetNumberOfVertices(); ++vertex)
  {
    if (this->ColoredVertices->GetValue(vertex) == 0)
    {
      svtkIdType parent = tree->GetParent(vertex);
      double* color = colorArray->GetTuple3(parent);
      colorArray->SetTuple3(vertex, color[0], color[1], color[2]);
    }
  }
}

//----------------------------------------------------------------------------
std::string svtkPhyloXMLTreeReader::GetTrimmedString(const char* input)
{
  std::string trimmedString;
  std::string whitespace = " \t\r\n";
  std::string untrimmed = input;
  size_t strBegin = untrimmed.find_first_not_of(whitespace);
  if (strBegin != std::string::npos)
  {
    size_t strEnd = untrimmed.find_last_not_of(whitespace);
    trimmedString = untrimmed.substr(strBegin, strEnd - strBegin + 1);
  }
  return trimmedString;
}

//----------------------------------------------------------------------------
std::string svtkPhyloXMLTreeReader::GetStringBeforeColon(const char* input)
{
  std::string fullStr(input);
  size_t strEnd = fullStr.find(':');
  std::string retStr = fullStr.substr(0, strEnd);
  return retStr;
}

//----------------------------------------------------------------------------
std::string svtkPhyloXMLTreeReader::GetStringAfterColon(const char* input)
{
  std::string fullStr(input);
  size_t strBegin = fullStr.find(':') + 1;
  std::string retStr = fullStr.substr(strBegin, fullStr.size() - strBegin + 1);
  return retStr;
}

//----------------------------------------------------------------------------
int svtkPhyloXMLTreeReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTree");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
