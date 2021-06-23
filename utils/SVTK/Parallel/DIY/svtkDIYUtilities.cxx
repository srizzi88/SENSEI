/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDIYUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDIYUtilities.h"

#include "svtkBoundingBox.h"
#include "svtkCellCenters.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkLogger.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLDataObjectWriter.h"
#include "svtkXMLImageDataReader.h"
#include "svtkXMLUnstructuredGridReader.h"

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
#include "svtkMPI.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#endif

#include <cassert>

static unsigned int svtkDIYUtilitiesCleanupCounter = 0;
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
static svtkMPIController* svtkDIYUtilitiesCleanupMPIController = nullptr;
#endif
svtkDIYUtilitiesCleanup::svtkDIYUtilitiesCleanup()
{
  ++svtkDIYUtilitiesCleanupCounter;
}

svtkDIYUtilitiesCleanup::~svtkDIYUtilitiesCleanup()
{
  if (--svtkDIYUtilitiesCleanupCounter == 0)
  {
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
    if (svtkDIYUtilitiesCleanupMPIController)
    {
      svtkLogF(TRACE, "Cleaning up MPI controller created for DIY filters.");
      svtkDIYUtilitiesCleanupMPIController->Finalize();
      svtkDIYUtilitiesCleanupMPIController->Delete();
      svtkDIYUtilitiesCleanupMPIController = nullptr;
    }
#endif
  }
}

//----------------------------------------------------------------------------
svtkDIYUtilities::svtkDIYUtilities() {}

//----------------------------------------------------------------------------
svtkDIYUtilities::~svtkDIYUtilities() {}

//----------------------------------------------------------------------------
void svtkDIYUtilities::InitializeEnvironmentForDIY()
{
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
  {
    svtkLogF(TRACE,
      "Initializing MPI for DIY filters since process did not do so in an MPI enabled build.");
    assert(svtkDIYUtilitiesCleanupMPIController == nullptr);
    svtkDIYUtilitiesCleanupMPIController = svtkMPIController::New();

    static int argc = 0;
    static char** argv = { nullptr };
    svtkDIYUtilitiesCleanupMPIController->Initialize(&argc, &argv);
  }
#endif
}

//----------------------------------------------------------------------------
diy::mpi::communicator svtkDIYUtilities::GetCommunicator(svtkMultiProcessController* controller)
{
  svtkDIYUtilities::InitializeEnvironmentForDIY();

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  svtkMPICommunicator* svtkcomm =
    svtkMPICommunicator::SafeDownCast(controller ? controller->GetCommunicator() : nullptr);
  return svtkcomm ? diy::mpi::communicator(*svtkcomm->GetMPIComm()->GetHandle())
                 : diy::mpi::communicator(MPI_COMM_SELF);
#else
  (void)controller;
  return diy::mpi::communicator();
#endif
}

//----------------------------------------------------------------------------
void svtkDIYUtilities::AllReduce(diy::mpi::communicator& comm, svtkBoundingBox& bbox)
{
  if (comm.size() > 1)
  {
    std::vector<double> local_minpoint(3), local_maxpoint(3);
    bbox.GetMinPoint(&local_minpoint[0]);
    bbox.GetMaxPoint(&local_maxpoint[0]);

    std::vector<double> global_minpoint(3), global_maxpoint(3);
    diy::mpi::all_reduce(comm, local_minpoint, global_minpoint, diy::mpi::minimum<float>());
    diy::mpi::all_reduce(comm, local_maxpoint, global_maxpoint, diy::mpi::maximum<float>());

    bbox.SetMinPoint(&global_minpoint[0]);
    bbox.SetMaxPoint(&global_maxpoint[0]);
  }
}

//----------------------------------------------------------------------------
void svtkDIYUtilities::Save(diy::BinaryBuffer& bb, svtkDataSet* p)
{
  if (p)
  {
    diy::save(bb, p->GetDataObjectType());
    auto writer = svtkXMLDataObjectWriter::NewWriter(p->GetDataObjectType());
    if (writer)
    {
      writer->WriteToOutputStringOn();
      writer->SetCompressorTypeToLZ4();
      writer->SetEncodeAppendedData(false);
      writer->SetInputDataObject(p);
      writer->Write();
      diy::save(bb, writer->GetOutputString());
      writer->Delete();
    }
    else
    {
      svtkLogF(
        ERROR, "Cannot serialize `%s` yet. Aborting for debugging purposes.", p->GetClassName());
      abort();
    }
  }
  else
  {
    diy::save(bb, static_cast<int>(-1)); // can't be SVTK_VOID since SVTK_VOID == SVTK_POLY_DATA.
  }
}

//----------------------------------------------------------------------------
void svtkDIYUtilities::Load(diy::BinaryBuffer& bb, svtkDataSet*& p)
{
  p = nullptr;
  int type;
  diy::load(bb, type);
  if (type == -1)
  {
    p = nullptr;
  }
  else
  {
    std::string data;
    diy::load(bb, data);

    svtkSmartPointer<svtkDataSet> ds;
    switch (type)
    {
      case SVTK_UNSTRUCTURED_GRID:
      {
        svtkNew<svtkXMLUnstructuredGridReader> reader;
        reader->ReadFromInputStringOn();
        reader->SetInputString(data);
        reader->Update();
        ds = svtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
      }
      break;

      case SVTK_IMAGE_DATA:
      {
        svtkNew<svtkXMLImageDataReader> reader;
        reader->ReadFromInputStringOn();
        reader->SetInputString(data);
        reader->Update();
        ds = svtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
      }
      break;
      default:
        // aborting for debugging purposes.
        abort();
    }

    ds->Register(nullptr);
    p = ds.GetPointer();
  }
}

//----------------------------------------------------------------------------
diy::ContinuousBounds svtkDIYUtilities::Convert(const svtkBoundingBox& bbox)
{
  if (bbox.IsValid())
  {
    diy::ContinuousBounds bds(3);
    bds.min[0] = static_cast<float>(bbox.GetMinPoint()[0]);
    bds.min[1] = static_cast<float>(bbox.GetMinPoint()[1]);
    bds.min[2] = static_cast<float>(bbox.GetMinPoint()[2]);
    bds.max[0] = static_cast<float>(bbox.GetMaxPoint()[0]);
    bds.max[1] = static_cast<float>(bbox.GetMaxPoint()[1]);
    bds.max[2] = static_cast<float>(bbox.GetMaxPoint()[2]);
    return bds;
  }
  return diy::ContinuousBounds(3);
}

//----------------------------------------------------------------------------
svtkBoundingBox svtkDIYUtilities::Convert(const diy::ContinuousBounds& bds)
{
  double bounds[6];
  bounds[0] = bds.min[0];
  bounds[1] = bds.max[0];
  bounds[2] = bds.min[1];
  bounds[3] = bds.max[1];
  bounds[4] = bds.min[2];
  bounds[5] = bds.max[2];
  svtkBoundingBox bbox;
  bbox.SetBounds(bounds);
  return bbox;
}

//----------------------------------------------------------------------------
void svtkDIYUtilities::Broadcast(
  diy::mpi::communicator& comm, std::vector<svtkBoundingBox>& boxes, int source)
{
  // FIXME: can't this be made more elegant?
  std::vector<double> raw_bounds;
  if (comm.rank() == source)
  {
    raw_bounds.resize(6 * boxes.size());
    for (size_t cc = 0; cc < boxes.size(); ++cc)
    {
      boxes[cc].GetBounds(&raw_bounds[6 * cc]);
    }
  }
  diy::mpi::broadcast(comm, raw_bounds, source);
  if (comm.rank() != source)
  {
    boxes.resize(raw_bounds.size() / 6);
    for (size_t cc = 0; cc < boxes.size(); ++cc)
    {
      boxes[cc].SetBounds(&raw_bounds[6 * cc]);
    }
  }
}

//----------------------------------------------------------------------------
std::vector<svtkDataSet*> svtkDIYUtilities::GetDataSets(svtkDataObject* input)
{
  std::vector<svtkDataSet*> datasets;
  if (auto cd = svtkCompositeDataSet::SafeDownCast(input))
  {
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        datasets.push_back(ds);
      }
    }
    iter->Delete();
  }
  else if (auto ds = svtkDataSet::SafeDownCast(input))
  {
    datasets.push_back(ds);
  }

  return datasets;
}

//----------------------------------------------------------------------------
std::vector<svtkSmartPointer<svtkPoints> > svtkDIYUtilities::ExtractPoints(
  const std::vector<svtkDataSet*>& datasets, bool use_cell_centers)
{
  svtkNew<svtkCellCenters> cellCenterFilter;
  cellCenterFilter->SetVertexCells(false);
  cellCenterFilter->SetCopyArrays(false);

  svtkNew<svtkRectilinearGridToPointSet> convertorRG;
  svtkNew<svtkImageDataToPointSet> convertorID;

  std::vector<svtkSmartPointer<svtkPoints> > all_points;
  for (auto ds : datasets)
  {
    if (use_cell_centers)
    {
      cellCenterFilter->SetInputDataObject(ds);
      cellCenterFilter->Update();
      ds = cellCenterFilter->GetOutput();
    }
    if (auto ps = svtkPointSet::SafeDownCast(ds))
    {
      all_points.push_back(ps->GetPoints());
    }
    else if (auto rg = svtkRectilinearGrid::SafeDownCast(ds))
    {
      convertorRG->SetInputDataObject(rg);
      convertorRG->Update();
      all_points.push_back(convertorRG->GetOutput()->GetPoints());
    }
    else if (auto id = svtkImageData::SafeDownCast(ds))
    {
      convertorID->SetInputDataObject(id);
      convertorID->Update();
      all_points.push_back(convertorID->GetOutput()->GetPoints());
    }
    else
    {
      // need a placeholder for dataset.
      all_points.push_back(nullptr);
    }
  }
  return all_points;
}

//----------------------------------------------------------------------------
svtkBoundingBox svtkDIYUtilities::GetLocalBounds(svtkDataObject* dobj)
{
  double bds[6];
  svtkMath::UninitializeBounds(bds);
  if (auto ds = svtkDataSet::SafeDownCast(dobj))
  {
    ds->GetBounds(bds);
  }
  else if (auto cd = svtkCompositeDataSet::SafeDownCast(dobj))
  {
    cd->GetBounds(bds);
  }
  return svtkBoundingBox(bds);
}

//----------------------------------------------------------------------------
void svtkDIYUtilities::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
