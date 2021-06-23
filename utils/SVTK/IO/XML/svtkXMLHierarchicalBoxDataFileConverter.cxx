/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLHierarchicalBoxDataFileConverter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLHierarchicalBoxDataFileConverter.h"

#include "svtkBoundingBox.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredData.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"
#include "svtkXMLImageDataReader.h"
#include "svtksys/SystemTools.hxx"

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkXMLHierarchicalBoxDataFileConverter);
//----------------------------------------------------------------------------
svtkXMLHierarchicalBoxDataFileConverter::svtkXMLHierarchicalBoxDataFileConverter()
{
  this->InputFileName = nullptr;
  this->OutputFileName = nullptr;
  this->FilePath = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLHierarchicalBoxDataFileConverter::~svtkXMLHierarchicalBoxDataFileConverter()
{
  this->SetInputFileName(nullptr);
  this->SetOutputFileName(nullptr);
  this->SetFilePath(nullptr);
}

//----------------------------------------------------------------------------
bool svtkXMLHierarchicalBoxDataFileConverter::Convert()
{
  if (!this->InputFileName)
  {
    svtkErrorMacro("Missing InputFileName.");
    return false;
  }

  if (!this->OutputFileName)
  {
    svtkErrorMacro("Missing OutputFileName.");
    return false;
  }

  svtkSmartPointer<svtkXMLDataElement> dom;
  dom.TakeReference(this->ParseXML(this->InputFileName));
  if (!dom)
  {
    return false;
  }

  // Ensure this file we can convert.
  if (dom->GetName() == nullptr || strcmp(dom->GetName(), "SVTKFile") != 0 ||
    dom->GetAttribute("type") == nullptr ||
    strcmp(dom->GetAttribute("type"), "svtkHierarchicalBoxDataSet") != 0 ||
    dom->GetAttribute("version") == nullptr || strcmp(dom->GetAttribute("version"), "1.0") != 0)
  {
    svtkErrorMacro("Cannot convert the input file: " << this->InputFileName);
    return false;
  }

  dom->SetAttribute("version", "1.1");
  dom->SetAttribute("type", "svtkOverlappingAMR");

  // locate primary element.
  svtkXMLDataElement* ePrimary = dom->FindNestedElementWithName("svtkHierarchicalBoxDataSet");
  if (!ePrimary)
  {
    svtkErrorMacro("Failed to locate primary element.");
    return false;
  }

  ePrimary->SetName("svtkOverlappingAMR");

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->InputFileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }
  this->SetFilePath(filePath.c_str());

  // We need origin for level 0, and spacing for all levels.
  double origin[3];
  double* spacing = nullptr;

  int gridDescription = this->GetOriginAndSpacing(ePrimary, origin, spacing);
  if (gridDescription < SVTK_XY_PLANE || gridDescription > SVTK_XYZ_GRID)
  {
    delete[] spacing;
    svtkErrorMacro("Failed to determine origin/spacing/grid description.");
    return false;
  }

  //  cout << "Origin: " << origin[0] <<", " << origin[1] << ", " << origin[2]
  //    << endl;
  //  cout << "Spacing: " << spacing[0] << ", " << spacing[1] << ", " << spacing[2]
  //    << endl;

  const char* grid_description = "XYZ";
  switch (gridDescription)
  {
    case SVTK_XY_PLANE:
      grid_description = "XY";
      break;
    case SVTK_XZ_PLANE:
      grid_description = "XZ";
      break;
    case SVTK_YZ_PLANE:
      grid_description = "YZ";
      break;
  }

  ePrimary->SetAttribute("grid_description", grid_description);
  ePrimary->SetVectorAttribute("origin", 3, origin);

  // Now iterate over all "<Block>" elements and update them.
  for (int cc = 0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
  {
    int level = 0;
    svtkXMLDataElement* block = ePrimary->GetNestedElement(cc);
    // iterate over all <DataSet> inside the current block
    // and replace the folder for the file attribute.
    for (int i = 0; i < block->GetNumberOfNestedElements(); ++i)
    {
      svtkXMLDataElement* dataset = block->GetNestedElement(i);
      std::string file(dataset->GetAttribute("file"));
      std::string fileNoDir(svtksys::SystemTools::GetFilenameName(file));
      std::string dir(svtksys::SystemTools::GetFilenameWithoutLastExtension(this->OutputFileName));
      dataset->SetAttribute("file", (dir + '/' + fileNoDir).c_str());
    }
    if (block && block->GetName() && strcmp(block->GetName(), "Block") == 0 &&
      block->GetScalarAttribute("level", level) && level >= 0)
    {
    }
    else
    {
      continue;
    }
    block->SetVectorAttribute("spacing", 3, &spacing[3 * level]);
    block->RemoveAttribute("refinement_ratio");
  }
  delete[] spacing;

  // now save the xml out.
  dom->PrintXML(this->OutputFileName);
  return true;
}

//----------------------------------------------------------------------------
svtkXMLDataElement* svtkXMLHierarchicalBoxDataFileConverter::ParseXML(const char* fname)
{
  assert(fname);

  svtkNew<svtkXMLDataParser> parser;
  parser->SetFileName(fname);
  if (!parser->Parse())
  {
    svtkErrorMacro("Failed to parse input XML: " << fname);
    return nullptr;
  }

  svtkXMLDataElement* element = parser->GetRootElement();
  element->Register(this);
  return element;
}

//----------------------------------------------------------------------------
int svtkXMLHierarchicalBoxDataFileConverter::GetOriginAndSpacing(
  svtkXMLDataElement* ePrimary, double origin[3], double*& spacing)
{
  // Build list of filenames for all levels.
  std::map<int, std::set<std::string> > filenames;

  for (int cc = 0; cc < ePrimary->GetNumberOfNestedElements(); cc++)
  {
    int level = 0;

    svtkXMLDataElement* child = ePrimary->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Block") == 0 &&
      child->GetScalarAttribute("level", level) && level >= 0)
    {
    }
    else
    {
      continue;
    }

    for (int kk = 0; kk < child->GetNumberOfNestedElements(); kk++)
    {
      svtkXMLDataElement* dsElement = child->GetNestedElement(cc);
      if (dsElement && dsElement->GetName() && strcmp(dsElement->GetName(), "DataSet") == 0 &&
        dsElement->GetAttribute("file") != nullptr)
      {
        std::string file = dsElement->GetAttribute("file");
        if (file.c_str()[0] != '/' && file.c_str()[1] != ':')
        {
          std::string prefix = this->FilePath;
          if (prefix.length())
          {
            prefix += "/";
          }
          file = prefix + file;
        }
        filenames[level].insert(file);
      }
    }
  }

  svtkBoundingBox bbox;
  int gridDescription = SVTK_UNCHANGED;
  spacing = new double[3 * filenames.size() + 1];
  memset(spacing, 0, (3 * filenames.size() + 1) * sizeof(double));

  // Now read all the datasets at level 0.
  for (std::set<std::string>::iterator iter = filenames[0].begin(); iter != filenames[0].end();
       ++iter)
  {
    svtkNew<svtkXMLImageDataReader> imageReader;
    imageReader->SetFileName((*iter).c_str());
    imageReader->Update();

    svtkImageData* image = imageReader->GetOutput();
    if (image && svtkMath::AreBoundsInitialized(image->GetBounds()))
    {
      if (!bbox.IsValid())
      {
        gridDescription = svtkStructuredData::GetDataDescription(image->GetDimensions());
      }
      bbox.AddBounds(image->GetBounds());
    }
  }

  if (bbox.IsValid())
  {
    bbox.GetMinPoint(origin[0], origin[1], origin[2]);
  }

  // Read 1 dataset from each level to get information about spacing.
  for (std::map<int, std::set<std::string> >::iterator iter = filenames.begin();
       iter != filenames.end(); ++iter)
  {
    if (iter->second.empty())
    {
      continue;
    }

    std::string filename = (*iter->second.begin());
    svtkNew<svtkXMLImageDataReader> imageReader;
    imageReader->SetFileName(filename.c_str());
    imageReader->UpdateInformation();
    svtkInformation* outInfo = imageReader->GetExecutive()->GetOutputInformation(0);
    if (outInfo->Has(svtkDataObject::SPACING()))
    {
      assert(outInfo->Length(svtkDataObject::SPACING()) == 3);
      outInfo->Get(svtkDataObject::SPACING(), &spacing[3 * iter->first]);
    }
  }

  return gridDescription;
}

//----------------------------------------------------------------------------
void svtkXMLHierarchicalBoxDataFileConverter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFileName: " << (this->InputFileName ? this->InputFileName : "(none)")
     << endl;
  os << indent << "OutputFileName: " << (this->OutputFileName ? this->OutputFileName : "(none)")
     << endl;
}
