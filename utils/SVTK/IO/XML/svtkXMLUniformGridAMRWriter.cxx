/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLUniformGridAMRWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLUniformGridAMRWriter.h"

#include "svtkAMRBox.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGrid.h"
#include "svtkXMLDataElement.h"

#include <cassert>

svtkStandardNewMacro(svtkXMLUniformGridAMRWriter);
//----------------------------------------------------------------------------
svtkXMLUniformGridAMRWriter::svtkXMLUniformGridAMRWriter() = default;

//----------------------------------------------------------------------------
svtkXMLUniformGridAMRWriter::~svtkXMLUniformGridAMRWriter() = default;

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUniformGridAMR");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLUniformGridAMRWriter::WriteComposite(
  svtkCompositeDataSet* compositeData, svtkXMLDataElement* parent, int& writerIdx)
{
  svtkUniformGridAMR* amr = svtkUniformGridAMR::SafeDownCast(compositeData);
  assert(amr != nullptr);

  svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(amr);

  // For svtkOverlappingAMR, we need to add additional meta-data to the XML.
  if (oamr)
  {
    const double* origin = oamr->GetOrigin();
    // I cannot decide what case to use. The other SVTK-XML format used mixed
    // case for attributes, but the composite files are using all lower case
    // attributes. For consistency, I'm sticking with that.
    parent->SetVectorAttribute("origin", 3, origin);
    const char* gridDescription = "";
    switch (oamr->GetGridDescription())
    {
      case SVTK_XY_PLANE:
        gridDescription = "XY";
        break;

      case SVTK_YZ_PLANE:
        gridDescription = "YZ";
        break;

      case SVTK_XZ_PLANE:
        gridDescription = "XZ";
        break;

      case SVTK_XYZ_GRID:
      default:
        gridDescription = "XYZ";
        break;
    }
    parent->SetAttribute("grid_description", gridDescription);
  }

  unsigned int numLevels = amr->GetNumberOfLevels();

  // Iterate over each level.
  for (unsigned int level = 0; level < numLevels; level++)
  {
    svtkSmartPointer<svtkXMLDataElement> block = svtkSmartPointer<svtkXMLDataElement>::New();
    block->SetName("Block");
    block->SetIntAttribute("level", level);
    if (oamr)
    {
      // save spacing for each level.
      double spacing[3];
      oamr->GetSpacing(level, spacing);
      block->SetVectorAttribute("spacing", 3, spacing);

      // we no longer save the refinement ratios since those can be deduced from
      // the spacing very easily.
    }

    unsigned int numDS = amr->GetNumberOfDataSets(level);
    for (unsigned int cc = 0; cc < numDS; cc++)
    {
      svtkUniformGrid* ug = amr->GetDataSet(level, cc);

      svtkSmartPointer<svtkXMLDataElement> datasetXML = svtkSmartPointer<svtkXMLDataElement>::New();
      datasetXML->SetName("DataSet");
      datasetXML->SetIntAttribute("index", cc);
      if (oamr)
      {
        // AMRBox meta-data is available only for svtkOverlappingAMR. Also this
        // meta-data is expected to be consistent (and available) on all
        // processes so we don't have to worry about missing amr-box
        // information.
        const svtkAMRBox& amrBox = oamr->GetAMRBox(level, cc);

        int box_buffer[6];
        box_buffer[0] = amrBox.GetLoCorner()[0];
        box_buffer[1] = amrBox.GetHiCorner()[0];
        box_buffer[2] = amrBox.GetLoCorner()[1];
        box_buffer[3] = amrBox.GetHiCorner()[1];
        box_buffer[4] = amrBox.GetLoCorner()[2];
        box_buffer[5] = amrBox.GetHiCorner()[2];
        // Don't use svtkAMRBox.Serialize() since it writes the box in a different
        // order than we wrote the box in traditionally. The expected order is
        // (xLo, xHi, yLo, yHi, zLo, zHi).
        datasetXML->SetVectorAttribute("amr_box", 6, box_buffer);
      }

      svtkStdString fileName = this->CreatePieceFileName(writerIdx);
      if (!fileName.empty())
      {
        // if fileName is empty, it implies that no file is written out for this
        // node, so don't add a filename attribute for it.
        datasetXML->SetAttribute("file", fileName);
      }
      block->AddNestedElement(datasetXML);

      // if this->WriteNonCompositeData() returns 0, it doesn't meant it's an
      // error, it just means that it didn't write a file for the current node.
      this->WriteNonCompositeData(ug, datasetXML, writerIdx, fileName.c_str());

      if (this->GetErrorCode() != svtkErrorCode::NoError)
      {
        return 0;
      }
    }
    parent->AddNestedElement(block);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLUniformGridAMRWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
