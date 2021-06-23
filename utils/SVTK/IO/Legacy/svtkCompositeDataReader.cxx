/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataReader.h"

#include "svtkAMRBox.h"
#include "svtkAMRInformation.h"
#include "svtkDataObjectTypes.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkGenericDataObjectReader.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPartitionedDataSet.h"
#include "svtkPartitionedDataSetCollection.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"

#include <sstream>
#include <svtksys/RegularExpression.hxx>
#include <svtksys/SystemTools.hxx>

#include <vector>

svtkStandardNewMacro(svtkCompositeDataReader);
//----------------------------------------------------------------------------
svtkCompositeDataReader::svtkCompositeDataReader() = default;

//----------------------------------------------------------------------------
svtkCompositeDataReader::~svtkCompositeDataReader() = default;

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataReader::GetOutput(int idx)
{
  return svtkCompositeDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkCompositeDataReader::SetOutput(svtkCompositeDataSet* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkCompositeDataReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCompositeDataReader::CreateOutput(svtkDataObject* currentOutput)
{
  if (this->GetFileName() == nullptr &&
    (this->GetReadFromInputString() == 0 ||
      (this->GetInputArray() == nullptr && this->GetInputString() == nullptr)))
  {
    svtkWarningMacro(<< "FileName must be set");
    return nullptr;
  }

  int outputType = this->ReadOutputType();
  if (outputType < 0)
  {
    svtkErrorMacro("Failed to read data-type.");
    return nullptr;
  }

  if (currentOutput && (currentOutput->GetDataObjectType() == outputType))
  {
    return currentOutput;
  }

  return svtkDataObjectTypes::NewDataObject(outputType);
}

//----------------------------------------------------------------------------
int svtkCompositeDataReader::ReadOutputType()
{
  char line[256];
  if (!this->OpenSVTKFile() || !this->ReadHeader())
  {
    return -1;
  }

  // Determine dataset type
  //
  if (!this->ReadString(line))
  {
    svtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
  }

  if (strncmp(this->LowerCase(line), "dataset", static_cast<unsigned long>(7)) == 0)
  {
    // See iftype is recognized.
    //
    if (!this->ReadString(line))
    {
      svtkDebugMacro(<< "Premature EOF reading type");
      this->CloseSVTKFile();
      return -1;
    }
    this->CloseSVTKFile();

    if (strncmp(this->LowerCase(line), "multiblock", strlen("multiblock")) == 0)
    {
      return SVTK_MULTIBLOCK_DATA_SET;
    }
    if (strncmp(this->LowerCase(line), "multipiece", strlen("multipiece")) == 0)
    {
      return SVTK_MULTIPIECE_DATA_SET;
    }
    if (strncmp(this->LowerCase(line), "overlapping_amr", strlen("overlapping_amr")) == 0)
    {
      return SVTK_OVERLAPPING_AMR;
    }
    if (strncmp(this->LowerCase(line), "non_overlapping_amr", strlen("non_overlapping_amr")) == 0)
    {
      return SVTK_NON_OVERLAPPING_AMR;
    }
    if (strncmp(this->LowerCase(line), "hierarchical_box", strlen("hierarchical_box")) == 0)
    {
      return SVTK_HIERARCHICAL_BOX_DATA_SET;
    }
    if (strncmp(
          this->LowerCase(line), "partitioned_collection", strlen("partitioned_collection")) == 0)
    {
      return SVTK_PARTITIONED_DATA_SET_COLLECTION;
    }
    if (strncmp(this->LowerCase(line), "partitioned", strlen("partitioned")) == 0)
    {
      return SVTK_PARTITIONED_DATA_SET;
    }
  }

  return -1;
}

//----------------------------------------------------------------------------
int svtkCompositeDataReader::ReadMeshSimple(const std::string& fname, svtkDataObject* output)
{
  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 0;
  }

  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(output);
  svtkMultiPieceDataSet* mp = svtkMultiPieceDataSet::SafeDownCast(output);
  svtkHierarchicalBoxDataSet* hb = svtkHierarchicalBoxDataSet::SafeDownCast(output);
  svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(output);
  svtkNonOverlappingAMR* noamr = svtkNonOverlappingAMR::SafeDownCast(output);
  svtkPartitionedDataSet* pd = svtkPartitionedDataSet::SafeDownCast(output);
  svtkPartitionedDataSetCollection* pdc = svtkPartitionedDataSetCollection::SafeDownCast(output);

  // Read the data-type description line which was already read in
  // RequestDataObject() so we just skip it here without any additional
  // validation.
  char line[256];
  if (!this->ReadString(line) || !this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 0;
  }

  if (mb)
  {
    this->ReadCompositeData(mb);
  }
  else if (mp)
  {
    this->ReadCompositeData(mp);
  }
  else if (hb)
  {
    this->ReadCompositeData(hb);
  }
  else if (oamr)
  {
    this->ReadCompositeData(oamr);
  }
  else if (noamr)
  {
    this->ReadCompositeData(noamr);
  }
  else if (pd)
  {
    this->ReadCompositeData(pd);
  }
  else if (pdc)
  {
    this->ReadCompositeData(pdc);
  }

  return 1;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkMultiBlockDataSet* mb)
{
  char line[256];

  if (!this->ReadString(line))
  {
    svtkErrorMacro("Failed to read block-count");
    return false;
  }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
  {
    svtkErrorMacro("Failed to read CHILDREN.");
    return false;
  }

  unsigned int num_blocks = 0;
  if (!this->Read(&num_blocks))
  {
    svtkErrorMacro("Failed to read number of blocks");
    return false;
  }

  mb->SetNumberOfBlocks(num_blocks);
  for (unsigned int cc = 0; cc < num_blocks; cc++)
  {
    if (!this->ReadString(line))
    {
      svtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
    }

    int type;
    if (!this->Read(&type))
    {
      svtkErrorMacro("Failed to read child type.");
      return false;
    }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);
    // if "line" has text enclosed in [] then that's the composite name.
    svtksys::RegularExpression regEx("\\s*\\[(.*)\\]");
    if (regEx.find(line))
    {
      std::string name = regEx.match(1);
      mb->GetMetaData(cc)->Set(svtkCompositeDataSet::NAME(), name.c_str());
    }

    if (type != -1)
    {
      svtkDataObject* child = this->ReadChild();
      if (!child)
      {
        svtkErrorMacro("Failed to read child.");
        return false;
      }
      mb->SetBlock(cc, child);
      child->FastDelete();
    }
    else
    {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
    }
  }

  if (this->ReadString(line) && strncmp(this->LowerCase(line), "field", 5) == 0)
  {
    svtkSmartPointer<svtkFieldData> fd = svtkSmartPointer<svtkFieldData>::Take(this->ReadFieldData());
    mb->SetFieldData(fd);
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkHierarchicalBoxDataSet* hb)
{
  (void)hb;
  svtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkOverlappingAMR* oamr)
{
  char line[256];

  // read GRID_DESCRIPTION.
  int description;
  if (!this->ReadString(line) ||
    strncmp(this->LowerCase(line), "grid_description", strlen("grid_description")) != 0 ||
    !this->Read(&description))
  {
    svtkErrorMacro("Failed to read GRID_DESCRIPTION (or its value).");
    return false;
  }

  // read ORIGIN
  double origin[3];
  if (!this->ReadString(line) || strncmp(this->LowerCase(line), "origin", strlen("origin")) != 0 ||
    !this->Read(&origin[0]) || !this->Read(&origin[1]) || !this->Read(&origin[2]))
  {
    svtkErrorMacro("Failed to read ORIGIN (or its value).");
    return false;
  }

  // read LEVELS.
  int num_levels;
  if (!this->ReadString(line) || strncmp(this->LowerCase(line), "levels", strlen("levels")) != 0 ||
    !this->Read(&num_levels))
  {
    svtkErrorMacro("Failed to read LEVELS (or its value).");
    return false;
  }

  std::vector<int> blocksPerLevel;
  blocksPerLevel.resize(num_levels);

  std::vector<double> spacing;
  spacing.resize(num_levels * 3);

  int total_blocks = 0;
  for (int cc = 0; cc < num_levels; cc++)
  {
    if (!this->Read(&blocksPerLevel[cc]))
    {
      svtkErrorMacro("Failed to read number of datasets for level " << cc);
      return false;
    }
    if (!this->Read(&spacing[3 * cc + 0]) || !this->Read(&spacing[3 * cc + 1]) ||
      !this->Read(&spacing[3 * cc + 2]))
    {
      svtkErrorMacro("Failed to read spacing for level " << cc);
      return false;
    }
    total_blocks += blocksPerLevel[cc];
  }

  // initialize the AMR.
  oamr->Initialize(num_levels, &blocksPerLevel[0]);
  oamr->SetGridDescription(description);
  oamr->SetOrigin(origin);
  for (int cc = 0; cc < num_levels; cc++)
  {
    oamr->GetAMRInfo()->SetSpacing(cc, &spacing[3 * cc]);
  }

  // read in the amr boxes0
  if (!this->ReadString(line))
  {
    svtkErrorMacro("Failed to read AMRBOXES' line");
  }
  else
  {
    if (strncmp(this->LowerCase(line), "amrboxes", strlen("amrboxes")) != 0)
    {
      svtkErrorMacro("Failed to read AMRBOXES' line");
    }
    else
    {
      // now read the amrbox information.
      svtkIdType num_tuples, num_components;
      if (!this->Read(&num_tuples) || !this->Read(&num_components))
      {
        svtkErrorMacro("Failed to read values for AMRBOXES.");
        return false;
      }

      svtkSmartPointer<svtkIntArray> idata;
      idata.TakeReference(
        svtkArrayDownCast<svtkIntArray>(this->ReadArray("int", num_tuples, num_components)));
      if (!idata || idata->GetNumberOfComponents() != 6 ||
        idata->GetNumberOfTuples() != static_cast<svtkIdType>(oamr->GetTotalNumberOfBlocks()))
      {
        svtkErrorMacro("Failed to read meta-data");
        return false;
      }

      unsigned int metadata_index = 0;
      for (int level = 0; level < num_levels; level++)
      {
        unsigned int num_datasets = oamr->GetNumberOfDataSets(level);
        for (unsigned int index = 0; index < num_datasets; index++, metadata_index++)
        {
          int tuple[6];
          idata->GetTypedTuple(metadata_index, tuple);

          svtkAMRBox box;
          box.SetDimensions(&tuple[0], &tuple[3], description);
          oamr->SetAMRBox(level, index, box);
        }
      }
    }
  }

  // read in the actual data

  for (int cc = 0; cc < total_blocks; cc++)
  {
    if (!this->ReadString(line))
    {
      // we may reach end of file sooner than total_blocks since not all blocks
      // may be present in the data.
      break;
    }

    if (strncmp(this->LowerCase(line), "child", strlen("child")) == 0)
    {
      unsigned int level = 0, index = 0;
      if (!this->Read(&level) || !this->Read(&index))
      {
        svtkErrorMacro("Failed to read level and index information");
        return false;
      }
      this->ReadLine(line);
      svtkDataObject* child = this->ReadChild();
      if (!child)
      {
        svtkErrorMacro("Failed to read dataset at " << level << ", " << index);
        return false;
      }
      if (child->IsA("svtkImageData"))
      {
        svtkUniformGrid* grid = svtkUniformGrid::New();
        grid->ShallowCopy(child);
        oamr->SetDataSet(level, index, grid);
        grid->FastDelete();
        child->Delete();
      }
      else
      {
        svtkErrorMacro("svtkImageData expected at " << level << ", " << index);
        child->Delete();
        return false;
      }
    }
    else
    {
      svtkErrorMacro("Failed to read 'CHILD' line");
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkNonOverlappingAMR* hb)
{
  (void)hb;
  svtkErrorMacro("This isn't supported yet.");
  return false;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkMultiPieceDataSet* mp)
{
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro("Failed to read block-count");
    return false;
  }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
  {
    svtkErrorMacro("Failed to read CHILDREN.");
    return false;
  }

  unsigned int num_pieces = 0;
  if (!this->Read(&num_pieces))
  {
    svtkErrorMacro("Failed to read number of pieces.");
    return false;
  }

  mp->SetNumberOfPieces(num_pieces);
  for (unsigned int cc = 0; cc < num_pieces; cc++)
  {
    if (!this->ReadString(line))
    {
      svtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
    }

    int type;
    if (!this->Read(&type))
    {
      svtkErrorMacro("Failed to read child type.");
      return false;
    }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);
    // if "line" has text enclosed in [] then that's the composite name.
    svtksys::RegularExpression regEx("\\s*\\[(.*)\\]");
    if (regEx.find(line))
    {
      std::string name = regEx.match(1);
      mp->GetMetaData(cc)->Set(svtkCompositeDataSet::NAME(), name.c_str());
    }

    if (type != -1)
    {
      svtkDataObject* child = this->ReadChild();
      if (!child)
      {
        svtkErrorMacro("Failed to read child.");
        return false;
      }
      mp->SetPiece(cc, child);
      child->FastDelete();
    }
    else
    {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkPartitionedDataSet* mp)
{
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro("Failed to read block-count");
    return false;
  }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
  {
    svtkErrorMacro("Failed to read CHILDREN.");
    return false;
  }

  unsigned int num_partitions = 0;
  if (!this->Read(&num_partitions))
  {
    svtkErrorMacro("Failed to read number of pieces.");
    return false;
  }

  mp->SetNumberOfPartitions(num_partitions);
  for (unsigned int cc = 0; cc < num_partitions; cc++)
  {
    if (!this->ReadString(line))
    {
      svtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
    }

    int type;
    if (!this->Read(&type))
    {
      svtkErrorMacro("Failed to read child type.");
      return false;
    }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);

    if (type != -1)
    {
      svtkDataObject* child = this->ReadChild();
      if (!child)
      {
        svtkErrorMacro("Failed to read child.");
        return false;
      }
      mp->SetPartition(cc, child);
      child->FastDelete();
    }
    else
    {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkCompositeDataReader::ReadCompositeData(svtkPartitionedDataSetCollection* mp)
{
  char line[256];
  if (!this->ReadString(line))
  {
    svtkErrorMacro("Failed to read block-count");
    return false;
  }

  if (strncmp(this->LowerCase(line), "children", strlen("children")) != 0)
  {
    svtkErrorMacro("Failed to read CHILDREN.");
    return false;
  }

  unsigned int num_datasets = 0;
  if (!this->Read(&num_datasets))
  {
    svtkErrorMacro("Failed to read number of pieces.");
    return false;
  }

  mp->SetNumberOfPartitionedDataSets(num_datasets);
  for (unsigned int cc = 0; cc < num_datasets; cc++)
  {
    if (!this->ReadString(line))
    {
      svtkErrorMacro("Failed to read 'CHILD <type>' line");
      return false;
    }

    int type;
    if (!this->Read(&type))
    {
      svtkErrorMacro("Failed to read child type.");
      return false;
    }
    // eat up the "\n" and other whitespace at the end of CHILD <type>.
    this->ReadLine(line);

    if (type != -1)
    {
      svtkPartitionedDataSet* child = svtkPartitionedDataSet::SafeDownCast(this->ReadChild());
      if (!child)
      {
        svtkErrorMacro("Failed to read child.");
        return false;
      }
      mp->SetPartitionedDataSet(cc, child);
      child->FastDelete();
    }
    else
    {
      // eat up the ENDCHILD marker.
      this->ReadString(line);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCompositeDataReader::ReadChild()
{
  // This is tricky. Simplistically speaking, we need to read the string for the
  // child and then pass it to a svtkGenericDataObjectReader and get it read.
  // Identifying where the child ends can be tricky since the child itself may
  // be a composite-dataset. Also, we need to avoid copy memory if we are
  // already reading from a string.

  // We can optimize this further when reading in from a string buffer to avoid
  // copying. But we'll do that later.

  unsigned int child_stack_depth = 1;

  std::ostringstream child_data;
  char line[512];
  while (child_stack_depth > 0)
  // read until ENDCHILD (passing over any nested CHILD-ENDCHILD correctly).
  {
    bool new_line = true;
    while (true)
    // read a full line until "\n". This maybe longer than 512 and hence this
    // extra loop.
    {
      this->IS->get(line, 512);
      if (this->IS->fail())
      {
        if (this->IS->eof())
        {
          svtkErrorMacro("Premature EOF.");
          return nullptr;
        }
        else
        {
          // else it's possible that we read in an empty line. istream still marks
          // that as fail().
          this->IS->clear();
        }
      }

      if (new_line)
      {
        // these comparisons need to happen only when a new line is
        // started, hence this "if".
        if (strncmp(line, "ENDCHILD", strlen("ENDCHILD")) == 0)
        {
          child_stack_depth--;
        }
        else if (strncmp(line, "CHILD", strlen("CHILD")) == 0)
        {
          // should not match CHILDREN :(
          if (strncmp(line, "CHILDREN", strlen("CHILDREN")) != 0)
          {
            child_stack_depth++;
          }
        }
      }

      if (child_stack_depth > 0)
      {
        // except the last ENDCHILD, all the read content is to passed to the
        // child-reader.
        child_data.write(line, this->IS->gcount());
      }
      new_line = false;
      if (this->IS->peek() == '\n')
      {
        this->IS->ignore(SVTK_INT_MAX, '\n');
        // end of line reached.
        child_data << '\n';
        break;
      }
    } // while (true);
  }   // while (child_stack_depth > 0);

  svtkGenericDataObjectReader* reader = svtkGenericDataObjectReader::New();
  reader->SetBinaryInputString(child_data.str().c_str(), static_cast<int>(child_data.str().size()));
  reader->ReadFromInputStringOn();
  reader->Update();

  svtkDataObject* child = reader->GetOutput(0);
  if (child)
  {
    child->Register(this);
  }
  reader->Delete();
  return child;
}

//----------------------------------------------------------------------------
void svtkCompositeDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
