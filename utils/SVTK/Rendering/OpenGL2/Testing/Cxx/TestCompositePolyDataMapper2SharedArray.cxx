/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositePolyDataMapper2SharedArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkAppendFilter.h>
#include <svtkCellArray.h>
#include <svtkCellArrayIterator.h>
#include <svtkCompositeDataIterator.h>
#include <svtkCompositePolyDataMapper2.h>
#include <svtkCubeSource.h>
#include <svtkDataObjectTreeIterator.h>
#include <svtkInformation.h>
#include <svtkInformationVector.h>
#include <svtkMultiBlockDataGroupFilter.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkMultiBlockDataSetAlgorithm.h>
#include <svtkNew.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkProperty.h>
#include <svtkRandomAttributeGenerator.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkUnstructuredGrid.h>

class svtkDualCubeSource : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkDualCubeSource* New();
  svtkTypeMacro(svtkDualCubeSource, svtkMultiBlockDataSetAlgorithm);

protected:
  svtkDualCubeSource() { this->SetNumberOfInputPorts(0); }

  ~svtkDualCubeSource() override = default;

  int RequestData(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector) override
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    // get the output
    svtkMultiBlockDataSet* output =
      svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

    svtkNew<svtkCubeSource> cube1;
    svtkNew<svtkRandomAttributeGenerator> id1;
    id1->SetDataTypeToFloat();
    id1->GeneratePointScalarsOn();
    id1->GenerateCellScalarsOn();
    id1->SetInputConnection(cube1->GetOutputPort());

    svtkNew<svtkCubeSource> cube2;
    cube2->SetCenter(1.5, 0., 0.);
    svtkNew<svtkRandomAttributeGenerator> id2;
    id2->SetInputConnection(cube2->GetOutputPort());
    id2->SetDataTypeToFloat();
    id2->GeneratePointScalarsOn();
    id2->GenerateCellScalarsOn();

    svtkNew<svtkCubeSource> cube3;
    cube3->SetCenter(0.75, -1.5, 0.);
    svtkNew<svtkRandomAttributeGenerator> id3;
    id3->SetInputConnection(cube3->GetOutputPort());
    id3->SetDataTypeToFloat();
    id3->GeneratePointScalarsOn();
    id3->GenerateCellScalarsOn();
    id3->Update();

    // Append geometry of the two first meshes
    svtkNew<svtkAppendFilter> append;
    append->AddInputConnection(id1->GetOutputPort());
    append->AddInputConnection(id2->GetOutputPort());
    append->Update();
    svtkUnstructuredGrid* aug = append->GetOutput();

    // Transfer appended geometry (not topology) to first and second meshes
    svtkPolyData* pd1 = svtkPolyData::SafeDownCast(id1->GetOutput());
    svtkIdType cube1npts = pd1->GetNumberOfPoints();
    pd1->SetPoints(aug->GetPoints());
    pd1->GetPointData()->ShallowCopy(aug->GetPointData());

    svtkPolyData* pd2 = svtkPolyData::SafeDownCast(id2->GetOutput());
    pd2->SetPoints(aug->GetPoints());
    pd2->GetPointData()->ShallowCopy(aug->GetPointData());

    { // Update connectivity of second mesh by shifting point ids
      svtkCellArray* polys = pd2->GetPolys();
      auto cellIter = svtk::TakeSmartPointer(polys->NewIterator());
      svtkNew<svtkIdList> cell;
      for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
      {
        cellIter->GetCurrentCell(cell);
        for (svtkIdType i = 0; i < cell->GetNumberOfIds(); i++)
        {
          cell->SetId(i, cell->GetId(i) + cube1npts);
        }
        cellIter->ReplaceCurrentCell(cell);
      }
    }

    // Create the multiblock dataset with the different meshes
    svtkNew<svtkMultiBlockDataGroupFilter> group;
    group->AddInputData(pd1);
    group->AddInputData(id3->GetOutput()); // This mesh has different arrays than the other two
    group->AddInputData(pd2);
    group->Update();

    output->ShallowCopy(group->GetOutput());
    return 1;
  }

private:
  svtkDualCubeSource(const svtkDualCubeSource&) = delete;
  void operator=(const svtkDualCubeSource&) = delete;
};
svtkStandardNewMacro(svtkDualCubeSource);

int TestCompositePolyDataMapper2SharedArray(int argc, char* argv[])
{
  svtkNew<svtkDualCubeSource> source;

  svtkNew<svtkRenderer> renderer;

  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  svtkNew<svtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetScalarModeToUsePointData();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .4, .5);

  renderer->ResetCamera();

  int retVal = svtkRegressionTestImageThreshold(renderWindow, 15);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
