/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPhyloXMLTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPhyloXMLTreeWriter.h"

#include "svtkDataSetAttributes.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationIterator.h"
#include "svtkInformationStringKey.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkPhyloXMLTreeWriter);

//----------------------------------------------------------------------------
svtkPhyloXMLTreeWriter::svtkPhyloXMLTreeWriter()
{
  this->EdgeWeightArrayName = "weight";
  this->NodeNameArrayName = "node name";

  this->EdgeWeightArray = nullptr;
  this->NodeNameArray = nullptr;
  this->Blacklist = svtkSmartPointer<svtkStringArray>::New();
}

//----------------------------------------------------------------------------
int svtkPhyloXMLTreeWriter::StartFile()
{
  ostream& os = *(this->Stream);
  os.imbue(std::locale::classic());

  // Open the document-level element.  This will contain the rest of
  // the elements.
  os << "<phyloxml xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
     << " xmlns=\"http://www.phyloxml.org\" xsi:schemaLocation=\""
     << "http://www.phyloxml.org http://www.phyloxml.org/1.10/phyloxml.xsd\">" << endl;

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPhyloXMLTreeWriter::EndFile()
{
  ostream& os = *(this->Stream);

  // Close the document-level element.
  os << "</phyloxml>\n";

  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(svtkErrorCode::GetLastSystemError());
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPhyloXMLTreeWriter::WriteData()
{
  svtkTree* const input = this->GetInput();

  this->EdgeWeightArray = input->GetEdgeData()->GetAbstractArray(this->EdgeWeightArrayName.c_str());

  this->NodeNameArray = input->GetVertexData()->GetAbstractArray(this->NodeNameArrayName.c_str());

  if (this->StartFile() == 0)
  {
    return 0;
  }

  svtkNew<svtkXMLDataElement> rootElement;
  rootElement->SetName("phylogeny");
  rootElement->SetAttribute("rooted", "true");

  // PhyloXML supports some optional elements for the entire tree.
  this->WriteTreeLevelElement(input, rootElement, "name", "");
  this->WriteTreeLevelElement(input, rootElement, "description", "");
  this->WriteTreeLevelElement(input, rootElement, "confidence", "type");
  this->WriteTreeLevelProperties(input, rootElement);

  // Generate PhyloXML for the vertices of the input tree.
  this->WriteCladeElement(input, input->GetRoot(), rootElement);

  rootElement->PrintXML(*this->Stream, svtkIndent());
  this->EndFile();
  return 1;
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteTreeLevelElement(svtkTree* input, svtkXMLDataElement* rootElement,
  const char* elementName, const char* attributeName)
{
  std::string arrayName = "phylogeny.";
  arrayName += elementName;
  svtkAbstractArray* array = input->GetVertexData()->GetAbstractArray(arrayName.c_str());
  if (array)
  {
    svtkNew<svtkXMLDataElement> element;
    element->SetName(elementName);
    svtkStdString val = array->GetVariantValue(0).ToString();
    element->SetCharacterData(val, static_cast<int>(val.length()));

    // set the attribute for this element if one was requested.
    if (strcmp(attributeName, "") != 0)
    {
      const char* attributeValue = this->GetArrayAttribute(array, attributeName);
      if (strcmp(attributeValue, "") != 0)
      {
        element->SetAttribute(attributeName, attributeValue);
      }
    }

    rootElement->AddNestedElement(element);

    // add this array to the blacklist so we don't try to write it again later
    this->Blacklist->InsertNextValue(arrayName.c_str());
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteTreeLevelProperties(svtkTree* input, svtkXMLDataElement* element)
{
  std::string prefix = "phylogeny.property.";
  for (int i = 0; i < input->GetVertexData()->GetNumberOfArrays(); ++i)
  {
    svtkAbstractArray* arr = input->GetVertexData()->GetAbstractArray(i);
    std::string arrName = arr->GetName();
    if (arrName.compare(0, prefix.length(), prefix) == 0)
    {
      this->WritePropertyElement(arr, -1, element);
    }
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteCladeElement(
  svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* parentElement)
{
  // create new clade element for this vertex
  svtkNew<svtkXMLDataElement> cladeElement;
  cladeElement->SetName("clade");

  // write out clade-level elements
  this->WriteBranchLengthAttribute(input, vertex, cladeElement);
  this->WriteNameElement(vertex, cladeElement);
  this->WriteConfidenceElement(input, vertex, cladeElement);
  this->WriteColorElement(input, vertex, cladeElement);

  // represent any other non-blacklisted VertexData arrays as PhyloXML
  // property elements.
  for (int i = 0; i < input->GetVertexData()->GetNumberOfArrays(); ++i)
  {
    svtkAbstractArray* array = input->GetVertexData()->GetAbstractArray(i);
    if (array == this->NodeNameArray || array == this->EdgeWeightArray)
    {
      continue;
    }

    if (this->Blacklist->LookupValue(array->GetName()) != -1)
    {
      continue;
    }

    this->WritePropertyElement(array, vertex, cladeElement);
  }

  // create clade elements for any children of this vertex.
  svtkIdType numChildren = input->GetNumberOfChildren(vertex);
  if (numChildren > 0)
  {
    for (svtkIdType child = 0; child < numChildren; ++child)
    {
      this->WriteCladeElement(input, input->GetChild(vertex, child), cladeElement);
    }
  }

  parentElement->AddNestedElement(cladeElement);
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteBranchLengthAttribute(
  svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element)
{
  if (!this->EdgeWeightArray)
  {
    return;
  }

  svtkIdType parent = input->GetParent(vertex);
  if (parent != -1)
  {
    svtkIdType edge = input->GetEdgeId(parent, vertex);
    if (edge != -1)
    {
      double weight = this->EdgeWeightArray->GetVariantValue(edge).ToDouble();
      element->SetDoubleAttribute("branch_length", weight);
    }
  }

  if (this->Blacklist->LookupValue(this->EdgeWeightArray->GetName()) == -1)
  {
    this->IgnoreArray(this->EdgeWeightArray->GetName());
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteNameElement(svtkIdType vertex, svtkXMLDataElement* element)
{
  if (!this->NodeNameArray)
  {
    return;
  }

  svtkStdString name = this->NodeNameArray->GetVariantValue(vertex).ToString();
  if (name.compare("") != 0)
  {
    svtkNew<svtkXMLDataElement> nameElement;
    nameElement->SetName("name");
    nameElement->SetCharacterData(name, static_cast<int>(name.length()));
    element->AddNestedElement(nameElement);
  }

  if (this->Blacklist->LookupValue(this->NodeNameArray->GetName()) == -1)
  {
    this->IgnoreArray(this->NodeNameArray->GetName());
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteConfidenceElement(
  svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element)
{
  svtkAbstractArray* confidenceArray = input->GetVertexData()->GetAbstractArray("confidence");
  if (!confidenceArray)
  {
    return;
  }

  svtkStdString confidence = confidenceArray->GetVariantValue(vertex).ToString();
  if (confidence.compare("") != 0)
  {
    svtkNew<svtkXMLDataElement> confidenceElement;
    confidenceElement->SetName("confidence");

    // set the type attribute for this element if possible.
    const char* type = this->GetArrayAttribute(confidenceArray, "type");
    if (strcmp(type, "") != 0)
    {
      confidenceElement->SetAttribute("type", type);
    }

    confidenceElement->SetCharacterData(confidence, static_cast<int>(confidence.length()));
    element->AddNestedElement(confidenceElement);
  }

  if (this->Blacklist->LookupValue("confidence") == -1)
  {
    this->IgnoreArray("confidence");
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WriteColorElement(
  svtkTree* const input, svtkIdType vertex, svtkXMLDataElement* element)
{
  svtkUnsignedCharArray* colorArray =
    svtkArrayDownCast<svtkUnsignedCharArray>(input->GetVertexData()->GetAbstractArray("color"));
  if (!colorArray)
  {
    return;
  }

  svtkNew<svtkXMLDataElement> colorElement;
  colorElement->SetName("color");

  svtkNew<svtkXMLDataElement> redElement;
  redElement->SetName("red");
  std::string r = svtkVariant(colorArray->GetComponent(vertex, 0)).ToString();
  redElement->SetCharacterData(r.c_str(), static_cast<int>(r.length()));

  svtkNew<svtkXMLDataElement> greenElement;
  greenElement->SetName("green");
  std::string g = svtkVariant(colorArray->GetComponent(vertex, 1)).ToString();
  greenElement->SetCharacterData(g.c_str(), static_cast<int>(g.length()));

  svtkNew<svtkXMLDataElement> blueElement;
  blueElement->SetName("blue");
  std::string b = svtkVariant(colorArray->GetComponent(vertex, 2)).ToString();
  blueElement->SetCharacterData(b.c_str(), static_cast<int>(b.length()));

  colorElement->AddNestedElement(redElement);
  colorElement->AddNestedElement(greenElement);
  colorElement->AddNestedElement(blueElement);

  element->AddNestedElement(colorElement);

  if (this->Blacklist->LookupValue("color") == -1)
  {
    this->IgnoreArray("color");
  }
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::WritePropertyElement(
  svtkAbstractArray* array, svtkIdType vertex, svtkXMLDataElement* element)
{
  // Search for attribute on this array.
  std::string authority;
  std::string appliesTo;
  std::string unit;

  svtkInformation* info = array->GetInformation();
  svtkNew<svtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal(); infoItr->GoToNextItem())
  {
    svtkInformationStringKey* key = svtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
    if (strcmp(key->GetName(), "authority") == 0)
    {
      authority = info->Get(key);
    }
    else if (strcmp(key->GetName(), "applies_to") == 0)
    {
      appliesTo = info->Get(key);
    }
    else if (strcmp(key->GetName(), "unit") == 0)
    {
      unit = info->Get(key);
    }
  }

  // authority is a required attribute.  Use "SVTK:" if one wasn't specified
  // on the array.
  if (authority.compare("") == 0)
  {
    authority = "SVTK";
  }

  // applies_to is also required.  Use "clade" if one was not specified.
  if (appliesTo.compare("") == 0)
  {
    appliesTo = "clade";
  }

  // construct the value for the "ref" attribute.
  std::string arrayName = array->GetName();
  std::string prefix = "property.";
  size_t strBegin = arrayName.find(prefix);
  if (strBegin == std::string::npos)
  {
    strBegin = 0;
  }
  else
  {
    strBegin += prefix.length();
  }
  std::string propertyName = arrayName.substr(strBegin, arrayName.size() - strBegin + 1);
  std::string ref = authority;
  ref += ":";
  ref += propertyName;

  // if vertex is -1, this means that this is a tree-level property.
  if (vertex == -1)
  {
    vertex = 0;
    this->IgnoreArray(arrayName.c_str());
  }

  // get the value for the "datatype" attribute.
  // This requiring converting the type as reported by SVTK variant
  // to an XML-compliant type.
  std::string variantType = array->GetVariantValue(vertex).GetTypeAsString();
  std::string datatype = "xsd:string";
  if (variantType.compare("short") == 0 || variantType.compare("long") == 0 ||
    variantType.compare("float") == 0 || variantType.compare("double") == 0)
  {
    datatype = "xsd:";
    datatype += variantType;
  }
  if (variantType.compare("int") == 0)
  {
    datatype = "xsd:integer";
  }
  else if (variantType.compare("bit") == 0)
  {
    datatype = "xsd:boolean";
  }
  else if (variantType.compare("char") == 0 || variantType.compare("signed char") == 0)
  {
    datatype = "xsd:byte";
  }
  else if (variantType.compare("unsigned char") == 0)
  {
    datatype = "xsd:unsignedByte";
  }
  else if (variantType.compare("unsigned short") == 0)
  {
    datatype = "xsd:unsignedShort";
  }
  else if (variantType.compare("unsigned int") == 0)
  {
    datatype = "xsd:unsignedInt";
  }
  else if (variantType.compare("unsigned long") == 0 ||
    variantType.compare("unsigned __int64") == 0 || variantType.compare("idtype") == 0)
  {
    datatype = "xsd:unsignedLong";
  }
  else if (variantType.compare("__int64") == 0)
  {
    datatype = "xsd:long";
  }

  // get the value for this property
  svtkStdString val = array->GetVariantValue(vertex).ToString();

  // create the new property element and add it to our document.
  svtkNew<svtkXMLDataElement> propertyElement;
  propertyElement->SetName("property");
  propertyElement->SetAttribute("datatype", datatype.c_str());
  propertyElement->SetAttribute("ref", ref.c_str());
  propertyElement->SetAttribute("applies_to", appliesTo.c_str());
  if (unit.compare("") != 0)
  {
    propertyElement->SetAttribute("unit", unit.c_str());
  }
  propertyElement->SetCharacterData(val, static_cast<int>(val.length()));

  element->AddNestedElement(propertyElement);
}

//----------------------------------------------------------------------------
int svtkPhyloXMLTreeWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
  return 1;
}

//----------------------------------------------------------------------------
svtkTree* svtkPhyloXMLTreeWriter::GetInput()
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkTree* svtkPhyloXMLTreeWriter::GetInput(int port)
{
  return svtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
const char* svtkPhyloXMLTreeWriter::GetDefaultFileExtension()
{
  return "xml";
}

//----------------------------------------------------------------------------
const char* svtkPhyloXMLTreeWriter::GetDataSetName()
{
  if (!this->InputInformation)
  {
    return "svtkTree";
  }
  svtkDataObject* hdInput =
    svtkDataObject::SafeDownCast(this->InputInformation->Get(svtkDataObject::DATA_OBJECT()));
  if (!hdInput)
  {
    return nullptr;
  }
  return hdInput->GetClassName();
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::IgnoreArray(const char* arrayName)
{
  this->Blacklist->InsertNextValue(arrayName);
}

//----------------------------------------------------------------------------
const char* svtkPhyloXMLTreeWriter::GetArrayAttribute(
  svtkAbstractArray* array, const char* attributeName)
{
  svtkInformation* info = array->GetInformation();
  svtkNew<svtkInformationIterator> infoItr;
  infoItr->SetInformation(info);
  for (infoItr->InitTraversal(); !infoItr->IsDoneWithTraversal(); infoItr->GoToNextItem())
  {
    if (strcmp(infoItr->GetCurrentKey()->GetName(), attributeName) == 0)
    {
      svtkInformationStringKey* key =
        svtkInformationStringKey::SafeDownCast(infoItr->GetCurrentKey());
      if (key)
      {
        return info->Get(key);
      }
    }
  }
  return "";
}

//----------------------------------------------------------------------------
void svtkPhyloXMLTreeWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EdgeWeightArrayName: " << this->EdgeWeightArrayName << endl;
  os << indent << "NodeNameArrayName: " << this->NodeNameArrayName << endl;
}
