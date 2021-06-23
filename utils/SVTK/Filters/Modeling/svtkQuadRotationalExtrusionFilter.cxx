/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkQuadRotationalExtrusionFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkQuadRotationalExtrusionFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkQuadRotationalExtrusionFilter);

// ----------------------------------------------------------------------
// Create object with capping on, angle of 360 degrees, resolution = 12, and
// no translation along z-axis.
// vector (0,0,1), and point (0,0,0).
svtkQuadRotationalExtrusionFilter::svtkQuadRotationalExtrusionFilter()
{
  this->Axis = 2;
  this->Capping = 1;
  this->DefaultAngle = 360.0;
  this->DeltaRadius = 0.0;
  this->Translation = 0.0;
  this->Resolution = 12; // 30 degree increments
}

// ----------------------------------------------------------------------
int svtkQuadRotationalExtrusionFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");

  return 1;
}

// ----------------------------------------------------------------------
int svtkQuadRotationalExtrusionFilter::RotateAroundAxis(double blockAngle, svtkIdType numPts,
  svtkPoints* inPts, svtkPoints* newPts, svtkPointData* pd, svtkPointData* outPD)
{
  // Calculate quantities necessary to the sweep operation
  int idx1;
  int idx2;
  if (!this->Axis)
  {
    idx1 = 1;
    idx2 = 2;
  }
  else if (this->Axis == 1)
  {
    idx1 = 0;
    idx2 = 2;
  }
  else if (this->Axis == 2)
  {
    idx1 = 0;
    idx2 = 1;
  }
  else
  {
    svtkErrorMacro(<< "Invalid axis number: " << this->Axis << "\n");
    return 0;
  }

  double radIncr = this->DeltaRadius / this->Resolution;
  double transIncr = this->Translation / this->Resolution;
  double angleIncr = svtkMath::RadiansFromDegrees(blockAngle) / this->Resolution;

  // Sweep over set resolution
  for (int i = 1; i <= this->Resolution; ++i)
  {
    this->UpdateProgress(.1 + .5 * (i - 1) / this->Resolution);
    for (svtkIdType ptId = 0; ptId < numPts; ++ptId)
    {
      // Get point coordinates
      double x[3];
      inPts->GetPoint(ptId, x);

      // Convert to cylindrical coordinates
      double newX[3];
      double radius = sqrt(x[idx1] * x[idx1] + x[idx2] * x[idx2]);
      if (radius > 0.)
      {
        double tempd = x[idx1] / radius;
        if (tempd < -1.)
        {
          tempd = -1.;
        }
        if (tempd > 1.)
        {
          tempd = 1.;
        }
        double theta = acos(tempd);

        tempd = x[idx2] / radius;
        if (tempd < -1.)
        {
          tempd = -1.;
        }
        if (tempd > 1.)
        {
          tempd = 1.;
        }
        double psi = asin(tempd);

        if (psi < 0.)
        {
          if (theta < (svtkMath::Pi() / 2.))
          {
            theta = 2. * svtkMath::Pi() + psi;
          }
          else
          {
            theta = svtkMath::Pi() - psi;
          }
        }

        // increment angle
        radius += i * radIncr;
        newX[this->Axis] = x[this->Axis] + i * transIncr;
        newX[idx1] = radius * cos(i * angleIncr + theta);
        newX[idx2] = radius * sin(i * angleIncr + theta);
      }
      else // radius is zero
      {
        newX[this->Axis] = x[this->Axis] + i * transIncr;
        newX[idx1] = 0.;
        newX[idx2] = 0.;
      }

      // Update swept mesh
      newPts->InsertPoint(ptId + i * numPts, newX);
      outPD->CopyData(pd, ptId, ptId + i * numPts);
    }
  }

  return 1;
}

// ----------------------------------------------------------------------
int svtkQuadRotationalExtrusionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get composite input
  svtkCompositeDataSet* compositeInput =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Get typed output
  svtkMultiBlockDataSet* compositeOutput =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (compositeInput == nullptr || compositeOutput == nullptr)
  {
    svtkErrorMacro(<< "Invalid algorithm connection\n");
    return 0;
  }

  svtkDebugMacro(<< "input=" << compositeInput->GetClassName() << "\n");

  // we want the output to have the same structure than the input
  compositeOutput->CopyStructure(compositeInput);

  // allocate composite iterator
  svtkCompositeDataIterator* inputIterator = compositeInput->NewIterator();
  inputIterator->SkipEmptyNodesOff();
  inputIterator->InitTraversal();

  svtkCompositeDataIterator* outputIterator = compositeOutput->NewIterator();
  outputIterator->SkipEmptyNodesOff();
  outputIterator->InitTraversal();

  while (inputIterator->IsDoneWithTraversal() == 0)
  {
    // get the input and output
    int blockId = inputIterator->GetCurrentFlatIndex();
    svtkPolyData* input = svtkPolyData::SafeDownCast(inputIterator->GetCurrentDataObject());
    inputIterator->GoToNextItem();
    if (!input)
    {
      outputIterator->GoToNextItem();
      continue;
    }

    svtkPolyData* output = svtkPolyData::New();
    compositeOutput->SetDataSet(outputIterator, output);
    outputIterator->GoToNextItem();
    output->Delete();

    svtkIdType numPts, numCells;
    numPts = input->GetNumberOfPoints();
    numCells = input->GetNumberOfCells();

    if (numPts > 0 && numCells > 0)
    {
      // Retrieve angle for each block, or angle by default
      double blockAngle = this->GetDefaultAngle();
      svtkDebugMacro(<< "DefaultAngle=" << blockAngle << "\n");

      std::map<svtkIdType, double>::iterator amit = this->PerBlockAngles.find(blockId);
      if (amit != this->PerBlockAngles.end())
      {
        svtkDebugMacro(<< "Found angle " << amit->second << " for block " << blockId << "\n");
        blockAngle = amit->second;
      }

      svtkDebugMacro(<< "process block " << blockId << ", angle=" << blockAngle
                    << ", resolution=" << this->Resolution << ", obj=" << output << "\n");

      svtkPointData* pd = input->GetPointData();
      svtkCellData* cd = input->GetCellData();
      svtkPolyData* mesh;
      svtkPoints* inPts;
      svtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
      int numEdges;
      const svtkIdType* pts = nullptr;
      svtkIdType ptId, ncells;
      svtkPoints* newPts;
      svtkCellArray *newLines = nullptr, *newPolys, *newStrips = nullptr;
      svtkCell* edge;
      svtkIdList* cellIds;
      svtkIdType p1, p2;
      svtkPointData* outPD = output->GetPointData();
      svtkCellData* outCD = output->GetCellData();
      int abort = 0;

      // Initialize / check input
      svtkDebugMacro(<< "Rotationally extruding data");

      // Build cell data structure.
      mesh = svtkPolyData::New();
      inPts = input->GetPoints();
      inVerts = input->GetVerts();
      inLines = input->GetLines();
      inPolys = input->GetPolys();
      inStrips = input->GetStrips();
      mesh->SetPoints(inPts);
      mesh->SetVerts(inVerts);
      mesh->SetLines(inLines);
      mesh->SetPolys(inPolys);
      mesh->SetStrips(inStrips);
      if (inPolys || inStrips)
      {
        mesh->BuildLinks();
      }

      // Allocate memory for output. We don't copy normals because surface geometry
      // is modified.
      outPD->CopyNormalsOff();
      outPD->CopyAllocate(pd, (this->Resolution + 1) * numPts);
      newPts = svtkPoints::New();
      newPts->Allocate((this->Resolution + 1) * numPts);
      if ((ncells = inVerts->GetNumberOfCells()) > 0)
      {
        newLines = svtkCellArray::New();
        newLines->AllocateEstimate(ncells, this->Resolution + 1);
      }
      // arbitrary initial allocation size
      ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
        inStrips->GetNumberOfCells() / 10;
      ncells = (ncells < 100 ? 100 : ncells);
      newPolys = svtkCellArray::New();
      newPolys->AllocateEstimate(ncells, 2 * (this->Resolution + 1));
      outCD->CopyNormalsOff();
      outCD->CopyAllocate(cd, ncells);

      // copy points
      for (ptId = 0; ptId < numPts; ptId++) // base level
      {
        newPts->InsertPoint(ptId, inPts->GetPoint(ptId));
        outPD->CopyData(pd, ptId, ptId);
      }
      this->UpdateProgress(0.1);

      // Rotate around selected axis
      if (!this->RotateAroundAxis(blockAngle, numPts, inPts, newPts, pd, outPD))
      {
        return 0;
      }

      // To ensure that cell attributes are in consistent order with the
      // cellId's, we process the verts, lines, polys and strips in order.
      svtkIdType newCellId = 0;
      int type;
      svtkIdType npts = 0;
      if (newLines) // there are verts which produce lines
      {
        for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
        {
          type = mesh->GetCellType(cellId);
          if (type == SVTK_VERTEX || type == SVTK_POLY_VERTEX)
          {
            mesh->GetCellPoints(cellId, npts, pts);
            for (int i = 0; i < npts; ++i)
            {
              ptId = pts[i];
              newLines->InsertNextCell(this->Resolution + 1);
              for (int j = 0; j <= this->Resolution; ++j)
              {
                newLines->InsertCellPoint(ptId + j * numPts);
              }
              outCD->CopyData(cd, cellId, newCellId++);
            }
          } // if a vertex or polyVertex
        }   // for all cells
      }     // if there are verts generating lines
      this->UpdateProgress(0.25);
      abort = this->GetAbortExecute();

      // If capping is on, copy 2D cells to output ( plus create cap ). Notice
      // that polygons are done first, then strips.
      //
      if (this->Capping &&
        (blockAngle != 360.0 || this->DeltaRadius != 0.0 || this->Translation != 0.0))
      {
        if (inPolys->GetNumberOfCells() > 0)
        {
          // newPolys = svtkCellArray::New();
          // newPolys->Allocate( inPolys->GetSize() );

          for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
          {
            type = mesh->GetCellType(cellId);
            if (type == SVTK_TRIANGLE || type == SVTK_QUAD || type == SVTK_POLYGON)
            {
              mesh->GetCellPoints(cellId, npts, pts);
              newPolys->InsertNextCell(npts, pts);
              outCD->CopyData(cd, cellId, newCellId++);
              newPolys->InsertNextCell(npts);
              for (int i = 0; i < npts; ++i)
              {
                svtkIdType idx = pts[i] + this->Resolution * numPts;
                newPolys->InsertCellPoint(idx);
              }
              outCD->CopyData(cd, cellId, newCellId++);
            }
          }
        }

        if (inStrips->GetNumberOfCells() > 0)
        {
          newStrips = svtkCellArray::New();
          newStrips->AllocateCopy(inStrips);

          for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
          {
            type = mesh->GetCellType(cellId);
            if (type == SVTK_TRIANGLE_STRIP)
            {
              mesh->GetCellPoints(cellId, npts, pts);
              newStrips->InsertNextCell(npts, pts);
              outCD->CopyData(cd, cellId, newCellId++);
              newStrips->InsertNextCell(npts);
              for (int i = 0; i < npts; ++i)
              {
                svtkIdType idx = pts[i] + this->Resolution * numPts;
                newStrips->InsertCellPoint(idx);
              }
              outCD->CopyData(cd, cellId, newCellId++);
            }
          }
        }
      } // if capping
      this->UpdateProgress(0.5);
      abort = this->GetAbortExecute();

      // Now process lines, polys and/or strips to produce strips
      //
      if (inLines->GetNumberOfCells() || inPolys->GetNumberOfCells() ||
        inStrips->GetNumberOfCells())
      {
        cellIds = svtkIdList::New();
        cellIds->Allocate(SVTK_CELL_SIZE);
        svtkGenericCell* cell = svtkGenericCell::New();

        for (svtkIdType cellId = 0; cellId < numCells && !abort; ++cellId)
        {
          type = mesh->GetCellType(cellId);
          if (type == SVTK_LINE || type == SVTK_POLY_LINE)
          {
            mesh->GetCellPoints(cellId, npts, pts);
            for (int i = 0; i < (npts - 1); ++i)
            {
              p1 = pts[i];
              p2 = pts[i + 1];
              svtkIdType idxPt;
              for (int k = 0; k < this->Resolution; ++k)
              {
                newPolys->InsertNextCell(4);
                idxPt = p1 + k * numPts;
                newPolys->InsertCellPoint(idxPt);
                idxPt = p2 + k * numPts;
                newPolys->InsertCellPoint(idxPt);
                idxPt = p2 + (k + 1) * numPts;
                newPolys->InsertCellPoint(idxPt);
                idxPt = p1 + (k + 1) * numPts;
                newPolys->InsertCellPoint(idxPt);
                outCD->CopyData(cd, cellId, newCellId++);
              }
            }
          } // if a line

          else if (type == SVTK_TRIANGLE || type == SVTK_QUAD || type == SVTK_POLYGON ||
            type == SVTK_TRIANGLE_STRIP)
          { // create strips from boundary edges
            mesh->GetCell(cellId, cell);
            numEdges = cell->GetNumberOfEdges();
            for (int i = 0; i < numEdges; ++i)
            {
              edge = cell->GetEdge(i);
              for (int j = 0; j < (edge->GetNumberOfPoints() - 1); ++j)
              {
                p1 = edge->PointIds->GetId(j);
                p2 = edge->PointIds->GetId(j + 1);
                mesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);

                if (cellIds->GetNumberOfIds() < 1) // generate strip
                {
                  svtkIdType idxPt;
                  for (int k = 0; k < this->Resolution; ++k)
                  {
                    newPolys->InsertNextCell(4);
                    idxPt = p1 + k * numPts;
                    newPolys->InsertCellPoint(idxPt);
                    idxPt = p2 + k * numPts;
                    newPolys->InsertCellPoint(idxPt);
                    idxPt = p2 + (k + 1) * numPts;
                    newPolys->InsertCellPoint(idxPt);
                    idxPt = p1 + (k + 1) * numPts;
                    newPolys->InsertCellPoint(idxPt);
                    outCD->CopyData(cd, cellId, newCellId++);
                  }
                } // if boundary edge
              }   // for each sub-edge
            }     // for each edge
          }       // for each polygon or triangle strip
        }         // for all cells

        cellIds->Delete();
        cell->Delete();
      } // if strips are being generated
      this->UpdateProgress(1.00);

      // Update ourselves and release memory
      output->SetPoints(newPts);
      newPts->Delete();
      mesh->Delete();

      if (newLines)
      {
        output->SetLines(newLines);
        newLines->Delete();
      }

      if (newPolys)
      {
        output->SetPolys(newPolys);
        newPolys->Delete();
      }

      if (newStrips)
      {
        output->SetStrips(newStrips);
        newStrips->Delete();
      }

      output->Squeeze();

    } /* if numPts>0 && numCells>0 */

  } /* Iterate over input blocks */

  inputIterator->Delete();
  outputIterator->Delete();

  return 1;
}

// ----------------------------------------------------------------------
void svtkQuadRotationalExtrusionFilter::RemoveAllPerBlockAngles()
{
  svtkDebugMacro(<< "RemoveAllPerBlockAngles\n");
  this->PerBlockAngles.clear();
  this->Modified();
}

// ----------------------------------------------------------------------
void svtkQuadRotationalExtrusionFilter::AddPerBlockAngle(svtkIdType blockId, double angle)
{
  svtkDebugMacro(<< "PerBlockAngles[" << blockId << "]=" << angle << "\n");
  this->PerBlockAngles[blockId] = angle;
  this->Modified();
}

// ----------------------------------------------------------------------
void svtkQuadRotationalExtrusionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Axis: " << this->Axis << "\n";
  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "DefaultAngle: " << this->DefaultAngle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
  os << indent << "PerBlockAngles:\n";
  for (std::map<svtkIdType, double>::iterator it = this->PerBlockAngles.begin();
       it != this->PerBlockAngles.end(); ++it)
  {
    os << indent.GetNextIndent() << "Block #" << it->first << " -> " << it->second << "\n";
  }
}
