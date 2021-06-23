/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLUniformGridAMRReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLUniformGridAMRReader.h"

#include "svtkAMRBox.h"
#include "svtkAMRInformation.h"
#include "svtkAMRUtilities.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataObjectTypes.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGrid.h"
#include "svtkXMLDataElement.h"

#include <cassert>
#include <vector>

namespace
{
// Data type used to store a 3-tuple of doubles.
template <class T, int N>
class svtkTuple
{
public:
  T Data[N];
  svtkTuple()
  {
    for (int cc = 0; cc < N; cc++)
    {
      this->Data[cc] = 0;
    }
  }
  operator T*() { return this->Data; }
};

typedef svtkTuple<double, 3> svtkSpacingType;

// Helper routine to parse the XML to collect information about the AMR.
bool svtkReadMetaData(svtkXMLDataElement* ePrimary, std::vector<unsigned int>& blocks_per_level,
  std::vector<svtkSpacingType>& level_spacing, std::vector<std::vector<svtkAMRBox> >& amr_boxes)
{
  unsigned int numElems = ePrimary->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    svtkXMLDataElement* blockXML = ePrimary->GetNestedElement(cc);
    if (!blockXML || !blockXML->GetName() || strcmp(blockXML->GetName(), "Block") != 0)
    {
      continue;
    }

    int level = 0;
    if (!blockXML->GetScalarAttribute("level", level))
    {
      svtkGenericWarningMacro("Missing 'level' on 'Block' element in XML. Skipping");
      continue;
    }
    if (level < 0)
    {
      // sanity check.
      continue;
    }
    if (blocks_per_level.size() <= static_cast<size_t>(level))
    {
      blocks_per_level.resize(level + 1, 0);
      level_spacing.resize(level + 1);
      amr_boxes.resize(level + 1);
    }

    double spacing[3];
    if (blockXML->GetVectorAttribute("spacing", 3, spacing))
    {
      level_spacing[level][0] = spacing[0];
      level_spacing[level][1] = spacing[1];
      level_spacing[level][2] = spacing[2];
    }

    // now read the <DataSet/> elements for boxes and counting the number of
    // nodes per level.
    int numDatasets = blockXML->GetNumberOfNestedElements();
    for (int kk = 0; kk < numDatasets; kk++)
    {
      svtkXMLDataElement* datasetXML = blockXML->GetNestedElement(kk);
      if (!datasetXML || !datasetXML->GetName() || strcmp(datasetXML->GetName(), "DataSet") != 0)
      {
        continue;
      }

      int index = 0;
      if (!datasetXML->GetScalarAttribute("index", index))
      {
        svtkGenericWarningMacro("Missing 'index' on 'DataSet' element in XML. Skipping");
        continue;
      }
      if (index >= static_cast<int>(blocks_per_level[level]))
      {
        blocks_per_level[level] = index + 1;
      }
      if (static_cast<size_t>(index) >= amr_boxes[level].size())
      {
        amr_boxes[level].resize(index + 1);
      }
      int box[6];
      // note: amr-box is not provided for non-overlapping AMR.
      if (!datasetXML->GetVectorAttribute("amr_box", 6, box))
      {
        continue;
      }
      // box is xLo, xHi, yLo, yHi, zLo, zHi.
      amr_boxes[level][index] = svtkAMRBox(box);
    }
  }
  return true;
}

bool svtkReadMetaData(svtkXMLDataElement* ePrimary, std::vector<unsigned int>& blocks_per_level)
{
  std::vector<svtkSpacingType> spacings;
  std::vector<std::vector<svtkAMRBox> > amr_boxes;
  return svtkReadMetaData(ePrimary, blocks_per_level, spacings, amr_boxes);
}
}

svtkStandardNewMacro(svtkXMLUniformGridAMRReader);
//----------------------------------------------------------------------------
svtkXMLUniformGridAMRReader::svtkXMLUniformGridAMRReader()
{
  this->OutputDataType = nullptr;
  this->MaximumLevelsToReadByDefault = 1;
}

//----------------------------------------------------------------------------
svtkXMLUniformGridAMRReader::~svtkXMLUniformGridAMRReader()
{
  this->SetOutputDataType(nullptr);
}

//----------------------------------------------------------------------------
void svtkXMLUniformGridAMRReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MaximumLevelsToReadByDefault: " << this->MaximumLevelsToReadByDefault << endl;
}

//----------------------------------------------------------------------------
const char* svtkXMLUniformGridAMRReader::GetDataSetName()
{
  if (!this->OutputDataType)
  {
    svtkWarningMacro("We haven't determine a valid output type yet.");
    return "svtkUniformGridAMR";
  }

  return this->OutputDataType;
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRReader::CanReadFileWithDataType(const char* dsname)
{
  return (dsname &&
           (strcmp(dsname, "svtkOverlappingAMR") == 0 ||
             strcmp(dsname, "svtkNonOverlappingAMR") == 0 ||
             strcmp(dsname, "svtkHierarchicalBoxDataSet") == 0))
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRReader::ReadSVTKFile(svtkXMLDataElement* eSVTKFile)
{
  // this->Superclass::ReadSVTKFile(..) calls this->GetDataSetName().
  // GetDataSetName() needs to know the data type we're reading and hence it's
  // essential to read the "type" before calling the superclass' method.

  // NOTE: eSVTKFile maybe totally invalid, so proceed with caution.
  const char* type = eSVTKFile->GetAttribute("type");
  if (type == nullptr ||
    (strcmp(type, "svtkHierarchicalBoxDataSet") != 0 && strcmp(type, "svtkOverlappingAMR") != 0 &&
      strcmp(type, "svtkNonOverlappingAMR") != 0))
  {
    svtkErrorMacro("Invalid 'type' specified in the file: " << (type ? type : "(none)"));
    return 0;
  }

  this->SetOutputDataType(type);
  return this->Superclass::ReadSVTKFile(eSVTKFile);
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  if (this->GetFileMajorVersion() == -1 && this->GetFileMinorVersion() == -1)
  {
    // for old files, we don't support providing meta-data for
    // RequestInformation() pass.
    this->Metadata = nullptr;
    return 1;
  }

  if (strcmp(ePrimary->GetName(), "svtkNonOverlappingAMR") == 0)
  {
    // this is a non-overlapping AMR. We don't have meta-data for
    // non-overlapping AMRs.
    this->Metadata = nullptr;
    return 1;
  }

  // Read the xml to build the metadata.
  this->Metadata = svtkSmartPointer<svtkOverlappingAMR>::New();

  // iterate over the XML to fill up the AMRInformation with meta-data.
  std::vector<unsigned int> blocks_per_level;
  std::vector<svtkSpacingType> level_spacing;
  std::vector<std::vector<svtkAMRBox> > amr_boxes;
  svtkReadMetaData(ePrimary, blocks_per_level, level_spacing, amr_boxes);

  if (!blocks_per_level.empty())
  {
    // initialize svtkAMRInformation.
    this->Metadata->Initialize(
      static_cast<int>(blocks_per_level.size()), reinterpret_cast<int*>(&blocks_per_level[0]));

    double origin[3] = { 0, 0, 0 };
    if (!ePrimary->GetVectorAttribute("origin", 3, origin))
    {
      svtkWarningMacro("Missing 'origin'. Using (0, 0, 0).");
    }
    this->Metadata->SetOrigin(origin);

    const char* grid_description = ePrimary->GetAttribute("grid_description");
    int iGridDescription = SVTK_XYZ_GRID;
    if (grid_description && strcmp(grid_description, "XY") == 0)
    {
      iGridDescription = SVTK_XY_PLANE;
    }
    else if (grid_description && strcmp(grid_description, "YZ") == 0)
    {
      iGridDescription = SVTK_YZ_PLANE;
    }
    else if (grid_description && strcmp(grid_description, "XZ") == 0)
    {
      iGridDescription = SVTK_XZ_PLANE;
    }
    this->Metadata->SetGridDescription(iGridDescription);

    // pass refinement ratios.
    for (size_t cc = 0; cc < level_spacing.size(); cc++)
    {
      this->Metadata->GetAMRInfo()->SetSpacing(static_cast<unsigned int>(cc), level_spacing[cc]);
    }
    //  pass amr boxes.
    for (size_t level = 0; level < amr_boxes.size(); level++)
    {
      for (size_t index = 0; index < amr_boxes[level].size(); index++)
      {
        const svtkAMRBox& box = amr_boxes[level][index];
        if (!box.Empty())
        {
          this->Metadata->GetAMRInfo()->SetAMRBox(
            static_cast<unsigned int>(level), static_cast<unsigned int>(index), box);
        }
      }
    }
  }

  this->Metadata->GenerateParentChildInformation();
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRReader::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (!this->ReadXMLInformation())
  {
    return 0;
  }

  svtkDataObject* output = svtkDataObject::GetData(outputVector, 0);
  if (!output || !output->IsA(this->OutputDataType))
  {
    svtkDataObject* newDO = svtkDataObjectTypes::NewDataObject(this->OutputDataType);
    if (newDO)
    {
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newDO);
      newDO->FastDelete();
      return 1;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  if (this->Metadata)
  {
    outputVector->GetInformationObject(0)->Set(
      svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA(), this->Metadata);
  }
  else
  {
    outputVector->GetInformationObject(0)->Remove(
      svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA());
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLUniformGridAMRReader::ReadComposite(svtkXMLDataElement* element,
  svtkCompositeDataSet* composite, const char* filePath, unsigned int& dataSetIndex)
{
  svtkUniformGridAMR* amr = svtkUniformGridAMR::SafeDownCast(composite);
  if (!amr)
  {
    svtkErrorMacro("Dataset must be a svtkUniformGridAMR.");
    return;
  }

  if (this->GetFileMajorVersion() == -1 && this->GetFileMinorVersion() == -1)
  {
    svtkErrorMacro("Version not supported. Use svtkXMLHierarchicalBoxDataReader instead.");
    return;
  }

  svtkInformation* outinfo = this->GetCurrentOutputInformation();
  bool has_block_requests = outinfo->Has(svtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS()) != 0;

  svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(amr);
  svtkNonOverlappingAMR* noamr = svtkNonOverlappingAMR::SafeDownCast(amr);
  assert(oamr != nullptr || noamr != nullptr);

  if (oamr)
  {
    // we don;t have the parse the structure. Just pass the inform from
    // this->Metadata.
    oamr->SetAMRInfo(this->Metadata->GetAMRInfo());
  }
  else if (noamr)
  {
    // We process the XML to collect information about the structure.
    std::vector<unsigned int> blocks_per_level;
    svtkReadMetaData(element, blocks_per_level);
    noamr->Initialize(
      static_cast<int>(blocks_per_level.size()), reinterpret_cast<int*>(&blocks_per_level[0]));
  }

  // Now, simply scan the xml for dataset elements and read them as needed.

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    svtkXMLDataElement* blockXML = element->GetNestedElement(cc);
    if (!blockXML || !blockXML->GetName() || strcmp(blockXML->GetName(), "Block") != 0)
    {
      continue;
    }

    int level = 0;
    if (!blockXML->GetScalarAttribute("level", level) || level < 0)
    {
      continue;
    }

    // now read the <DataSet/> elements for boxes and counting the number of
    // nodes per level.
    int numDatasets = blockXML->GetNumberOfNestedElements();
    for (int kk = 0; kk < numDatasets; kk++)
    {
      svtkXMLDataElement* datasetXML = blockXML->GetNestedElement(kk);
      if (!datasetXML || !datasetXML->GetName() || strcmp(datasetXML->GetName(), "DataSet") != 0)
      {
        continue;
      }

      int index = 0;
      if (!datasetXML->GetScalarAttribute("index", index) || index < 0)
      {
        continue;
      }

      if (this->ShouldReadDataSet(dataSetIndex))
      {
        // if has_block_requests==false, then we don't read any blocks greater
        // than the MaximumLevelsToReadByDefault.
        if (has_block_requests == false && this->MaximumLevelsToReadByDefault > 0 &&
          static_cast<unsigned int>(level) >= this->MaximumLevelsToReadByDefault)
        {
          // don't actually read the data.
        }
        else
        {
          svtkSmartPointer<svtkDataSet> ds;
          ds.TakeReference(this->ReadDataset(datasetXML, filePath));
          if (ds && !ds->IsA("svtkUniformGrid"))
          {
            svtkErrorMacro("svtkUniformGridAMR can only contain svtkUniformGrids.");
          }
          else
          {
            amr->SetDataSet(static_cast<unsigned int>(level), static_cast<unsigned int>(index),
              svtkUniformGrid::SafeDownCast(ds));
          }
        }
      }
      dataSetIndex++;
    }
  }

  if ((oamr != nullptr) && !has_block_requests)
  {
    svtkAMRUtilities::BlankCells(oamr);
  }
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLUniformGridAMRReader::ReadDataset(
  svtkXMLDataElement* xmlElem, const char* filePath)
{
  svtkDataSet* ds = this->Superclass::ReadDataset(xmlElem, filePath);
  if (ds && ds->IsA("svtkImageData"))
  {
    // Convert svtkImageData to svtkUniformGrid as needed by
    // svtkHierarchicalBoxDataSet.
    svtkUniformGrid* ug = svtkUniformGrid::New();
    ug->ShallowCopy(ds);
    ds->Delete();
    return ug;
  }
  return ds;
}
