/*=========================================================================

  Program:   ParaView
  Module:    svtkCompositeDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataWriter.h"

#include "svtkAMRBox.h"
#include "svtkAMRInformation.h"
#include "svtkDoubleArray.h"
#include "svtkGenericDataObjectWriter.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkNew.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkUniformGrid.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkCompositeDataWriter);
//----------------------------------------------------------------------------
svtkCompositeDataWriter::svtkCompositeDataWriter() = default;

//----------------------------------------------------------------------------
svtkCompositeDataWriter::~svtkCompositeDataWriter() = default;

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataWriter::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataWriter::GetInput(int port)
{
  return svtkCompositeDataSet::SafeDownCast(this->GetInputDataObject(port, 0));
}

//----------------------------------------------------------------------------
int svtkCompositeDataWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkCompositeDataWriter::WriteData()
{
  ostream* fp;
  svtkCompositeDataSet* input = this->GetInput();

  svtkDebugMacro(<< "Writing svtk composite data...");
  if (!(fp = this->OpenSVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      if (this->FileName)
      {
        svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->CloseSVTKFile(fp);
        unlink(this->FileName);
      }
      else
      {
        this->CloseSVTKFile(fp);
        svtkErrorMacro("Could not read memory header. ");
      }
    }
    return;
  }

  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(input);
  svtkHierarchicalBoxDataSet* hb = svtkHierarchicalBoxDataSet::SafeDownCast(input);
  svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(input);
  svtkNonOverlappingAMR* noamr = svtkNonOverlappingAMR::SafeDownCast(input);
  svtkMultiPieceDataSet* mp = svtkMultiPieceDataSet::SafeDownCast(input);
  svtkPartitionedDataSet* pd = svtkPartitionedDataSet::SafeDownCast(input);
  svtkPartitionedDataSetCollection* pdc = svtkPartitionedDataSetCollection::SafeDownCast(input);
  if (mb)
  {
    *fp << "DATASET MULTIBLOCK\n";
    if (!this->WriteCompositeData(fp, mb))
    {
      svtkErrorMacro("Error writing multiblock dataset.");
    }
  }
  else if (hb)
  {
    *fp << "DATASET HIERARCHICAL_BOX\n";
    if (!this->WriteCompositeData(fp, hb))
    {
      svtkErrorMacro("Error writing hierarchical-box dataset.");
    }
  }
  else if (oamr)
  {
    *fp << "DATASET OVERLAPPING_AMR\n";
    if (!this->WriteCompositeData(fp, oamr))
    {
      svtkErrorMacro("Error writing overlapping amr dataset.");
    }
  }
  else if (noamr)
  {
    *fp << "DATASET NON_OVERLAPPING_AMR\n";
    if (!this->WriteCompositeData(fp, noamr))
    {
      svtkErrorMacro("Error writing non-overlapping amr dataset.");
    }
  }
  else if (mp)
  {
    *fp << "DATASET MULTIPIECE\n";
    if (!this->WriteCompositeData(fp, mp))
    {
      svtkErrorMacro("Error writing multi-piece dataset.");
    }
  }
  else if (pd)
  {
    *fp << "DATASET PARTITIONED\n";
    if (!this->WriteCompositeData(fp, pd))
    {
      svtkErrorMacro("Error writing partitioned dataset.");
    }
  }
  else if (pdc)
  {
    *fp << "DATASET PARTITIONED_COLLECTION\n";
    if (!this->WriteCompositeData(fp, pdc))
    {
      svtkErrorMacro("Error writing partitioned dataset collection.");
    }
  }
  else
  {
    svtkErrorMacro("Unsupported input type: " << input->GetClassName());
  }

  this->CloseSVTKFile(fp);
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkMultiBlockDataSet* mb)
{
  *fp << "CHILDREN " << mb->GetNumberOfBlocks() << "\n";
  for (unsigned int cc = 0; cc < mb->GetNumberOfBlocks(); cc++)
  {
    svtkDataObject* child = mb->GetBlock(cc);
    *fp << "CHILD " << (child ? child->GetDataObjectType() : -1);
    // add name if present.
    if (mb->HasMetaData(cc) && mb->GetMetaData(cc)->Has(svtkCompositeDataSet::NAME()))
    {
      *fp << " [" << mb->GetMetaData(cc)->Get(svtkCompositeDataSet::NAME()) << "]";
    }
    *fp << "\n";
    if (child)
    {
      if (!this->WriteBlock(fp, child))
      {
        return false;
      }
    }
    *fp << "ENDCHILD\n";
  }

  this->WriteFieldData(fp, mb->GetFieldData());
  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkMultiPieceDataSet* mp)
{
  *fp << "CHILDREN " << mp->GetNumberOfPieces() << "\n";
  for (unsigned int cc = 0; cc < mp->GetNumberOfPieces(); cc++)
  {
    svtkDataObject* child = mp->GetPieceAsDataObject(cc);
    *fp << "CHILD " << (child ? child->GetDataObjectType() : -1);
    // add name if present.
    if (mp->HasMetaData(cc) && mp->GetMetaData(cc)->Has(svtkCompositeDataSet::NAME()))
    {
      *fp << " [" << mp->GetMetaData(cc)->Get(svtkCompositeDataSet::NAME()) << "]";
    }
    *fp << "\n";

    if (child)
    {
      if (!this->WriteBlock(fp, child))
      {
        return false;
      }
    }
    *fp << "ENDCHILD\n";
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkPartitionedDataSet* pd)
{
  *fp << "CHILDREN " << pd->GetNumberOfPartitions() << "\n";
  for (unsigned int cc = 0; cc < pd->GetNumberOfPartitions(); cc++)
  {
    svtkDataSet* partition = pd->GetPartition(cc);
    *fp << "CHILD " << (partition ? partition->GetDataObjectType() : -1);
    *fp << "\n";

    if (partition)
    {
      if (!this->WriteBlock(fp, partition))
      {
        return false;
      }
    }
    *fp << "ENDCHILD\n";
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkPartitionedDataSetCollection* pd)
{
  *fp << "CHILDREN " << pd->GetNumberOfPartitionedDataSets() << "\n";
  for (unsigned int cc = 0; cc < pd->GetNumberOfPartitionedDataSets(); cc++)
  {
    svtkPartitionedDataSet* dataset = pd->GetPartitionedDataSet(cc);
    *fp << "CHILD " << (dataset ? dataset->GetDataObjectType() : -1);
    *fp << "\n";

    if (dataset)
    {
      if (!this->WriteBlock(fp, dataset))
      {
        return false;
      }
    }
    *fp << "ENDCHILD\n";
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkHierarchicalBoxDataSet* hb)
{
  (void)fp;
  (void)hb;
  svtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkOverlappingAMR* oamr)
{
  svtkAMRInformation* amrInfo = oamr->GetAMRInfo();

  *fp << "GRID_DESCRIPTION " << amrInfo->GetGridDescription() << "\n";

  const double* origin = oamr->GetOrigin();
  *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";

  unsigned int num_levels = oamr->GetNumberOfLevels();
  // we'll dump out all level information and then the individual blocks.
  *fp << "LEVELS " << num_levels << "\n";
  for (unsigned int level = 0; level < num_levels; level++)
  {
    // <num datasets> <spacing x> <spacing y> <spacing z>
    double spacing[3];
    amrInfo->GetSpacing(level, spacing);

    *fp << oamr->GetNumberOfDataSets(level) << " " << spacing[0] << " " << spacing[1] << " "
        << spacing[2] << "\n";
  }

  // now dump the amr boxes, if any.
  // Information about amrboxes can be "too much". So we compact it in
  // svtkDataArray subclasses to ensure that it can be written as binary data
  // with correct swapping, as needed.
  svtkNew<svtkIntArray> idata;
  // box.LoCorner[3], box.HiCorner[3]
  idata->SetName("IntMetaData");
  idata->SetNumberOfComponents(6);
  idata->SetNumberOfTuples(amrInfo->GetTotalNumberOfBlocks());
  unsigned int metadata_index = 0;
  for (unsigned int level = 0; level < num_levels; level++)
  {
    unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
    for (unsigned int index = 0; index < num_datasets; index++, metadata_index++)
    {
      const svtkAMRBox& box = oamr->GetAMRBox(level, index);
      int tuple[6];
      box.Serialize(tuple);
      idata->SetTypedTuple(metadata_index, tuple);
    }
  }
  *fp << "AMRBOXES " << idata->GetNumberOfTuples() << " " << idata->GetNumberOfComponents() << "\n";
  this->WriteArray(fp, idata->GetDataType(), idata, "", idata->GetNumberOfTuples(),
    idata->GetNumberOfComponents());

  // now dump the real data, if any.
  metadata_index = 0;
  for (unsigned int level = 0; level < num_levels; level++)
  {
    unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
    for (unsigned int index = 0; index < num_datasets; index++, metadata_index++)
    {
      svtkUniformGrid* dataset = oamr->GetDataSet(level, index);
      if (dataset)
      {
        *fp << "CHILD " << level << " " << index << "\n";
        // since we cannot write svtkUniformGrid's, we create a svtkImageData and
        // write it.
        svtkNew<svtkImageData> image;
        image->ShallowCopy(dataset);
        if (!this->WriteBlock(fp, image))
        {
          return false;
        }
        *fp << "ENDCHILD\n";
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteCompositeData(ostream* fp, svtkNonOverlappingAMR* hb)
{
  (void)fp;
  (void)hb;
  svtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataWriter::WriteBlock(ostream* fp, svtkDataObject* block)
{
  bool success = false;
  svtkGenericDataObjectWriter* writer = svtkGenericDataObjectWriter::New();
  writer->WriteToOutputStringOn();
  writer->SetFileType(this->FileType);
  writer->SetInputData(block);
  if (writer->Write())
  {
    fp->write(reinterpret_cast<const char*>(writer->GetBinaryOutputString()),
      writer->GetOutputStringLength());
    success = true;
  }
  writer->Delete();
  return success;
}

//----------------------------------------------------------------------------
void svtkCompositeDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
