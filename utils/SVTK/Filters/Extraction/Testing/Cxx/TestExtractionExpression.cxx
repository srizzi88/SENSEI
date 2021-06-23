#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCell.h>
#include <svtkCellData.h>
#include <svtkDataSetMapper.h>
#include <svtkDoubleArray.h>
#include <svtkExtractSelection.h>
#include <svtkIdList.h>
#include <svtkIdTypeArray.h>
#include <svtkImageData.h>
#include <svtkInformation.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyDataWriter.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSelection.h>
#include <svtkSelectionNode.h>
#include <svtkUnstructuredGrid.h>
#include <svtkUnstructuredGridWriter.h>
#include <svtkVector.h>
#include <svtkXMLDataSetWriter.h>

#include <numeric>

#define XCELLS 15
#define YCELLS 15
#define ZCELLS 15

namespace
{

enum
{
  COLORBYCELL,
  COLORBYPOINT
};

void showMe(
  svtkDataSet* result, int X, int Y, int CellOrPoint, svtkDataArray* array, svtkRenderer* renderer)
{
  svtkSmartPointer<svtkDataSet> copy = svtkSmartPointer<svtkDataSet>::NewInstance(result);
  copy->DeepCopy(result);
  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputData(copy);
  double* range = array->GetRange();
  if (CellOrPoint == COLORBYCELL)
  {
    copy->GetCellData()->SetActiveScalars(array->GetName());
    mapper->SetScalarModeToUseCellData();
  }
  else
  {
    copy->GetPointData()->SetActiveScalars(array->GetName());
    mapper->SetScalarModeToUsePointData();
  }
  mapper->SetScalarRange(range[0], range[1]);
  svtkNew<svtkActor> actor;
  actor->SetPosition(X * 20, Y * 20, 0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetPointSize(6.0);
  renderer->AddActor(actor);
}

svtkSmartPointer<svtkDataSet> createTestData()
{
  //--------------------------------------------------------------------------
  // create a test data set with known structure and data values
  // the structure will look like a Rubix' cube
  // the values will be:
  // three double arrays containing X,Y,and Z coordinates for
  // each point and cell, where the cell coordinates are the center of the cell
  // two id type arrays containing Id's or labels that range from 10 to
  // numpts/cells+10, with one array being the reverse of the other
  // the scalars datasetattibute will be the X array
  // the globalids datasetattribute will be the forward running id array

  auto sampleData = svtkSmartPointer<svtkImageData>::New();
  sampleData->Initialize();
  sampleData->SetSpacing(1.0, 1.0, 1.0);
  sampleData->SetOrigin(0.0, 0.0, 0.0);
  sampleData->SetDimensions(XCELLS + 1, YCELLS + 1, ZCELLS + 1);
  sampleData->AllocateScalars(SVTK_DOUBLE, 1);

  svtkNew<svtkIdTypeArray> pia;
  pia->SetNumberOfComponents(1);
  pia->SetName("Point Counter");
  sampleData->GetPointData()->AddArray(pia);

  svtkNew<svtkIdTypeArray> piaF;
  piaF->SetNumberOfComponents(1);
  piaF->SetName("Forward Point Ids");
  sampleData->GetPointData()->AddArray(piaF);

  svtkNew<svtkIdTypeArray> piaR;
  piaR->SetNumberOfComponents(1);
  piaR->SetName("Reverse Point Ids");
  sampleData->GetPointData()->AddArray(piaR);

  svtkNew<svtkDoubleArray> pxa;
  pxa->SetNumberOfComponents(1);
  pxa->SetName("Point X");
  sampleData->GetPointData()->AddArray(pxa);

  svtkNew<svtkDoubleArray> pya;
  pya->SetNumberOfComponents(1);
  pya->SetName("Point Y");
  sampleData->GetPointData()->AddArray(pya);

  svtkNew<svtkDoubleArray> pza;
  pza->SetNumberOfComponents(1);
  pza->SetName("Point Z");
  sampleData->GetPointData()->AddArray(pza);

  // svtkPoints *points = svtkPoints::New();
  svtkIdType pcnt = 0;
  for (int i = 0; i < ZCELLS + 1; i++)
  {
    for (int j = 0; j < YCELLS + 1; j++)
    {
      for (int k = 0; k < XCELLS + 1; k++)
      {
        // points->InsertNextPoint(k,j,i);

        pia->InsertNextValue(pcnt);
        int idF = pcnt + 10;
        int idR = (XCELLS + 1) * (YCELLS + 1) * (ZCELLS + 1) - 1 - pcnt + 10;
        piaF->InsertNextValue(idF);
        piaR->InsertNextValue(idR);
        pcnt++;

        pxa->InsertNextValue((double)k);
        pya->InsertNextValue((double)j);
        pza->InsertNextValue((double)i);
      }
    }
  }

  // sampleData->SetPoints(points);
  // points->Delete();

  // svtkIdList *ids = svtkIdList::New();

  svtkNew<svtkIdTypeArray> cia;
  cia->SetNumberOfComponents(1);
  cia->SetName("Cell Count");
  sampleData->GetCellData()->AddArray(cia);

  svtkNew<svtkIdTypeArray> ciaF;
  ciaF->SetNumberOfComponents(1);
  ciaF->SetName("Forward Cell Ids");
  sampleData->GetCellData()->AddArray(ciaF);

  svtkNew<svtkIdTypeArray> ciaR;
  ciaR->SetNumberOfComponents(1);
  ciaR->SetName("Reverse Cell Ids");
  sampleData->GetCellData()->AddArray(ciaR);

  svtkNew<svtkDoubleArray> cxa;
  cxa->SetNumberOfComponents(1);
  cxa->SetName("Cell X");
  sampleData->GetCellData()->AddArray(cxa);

  svtkNew<svtkDoubleArray> cya;
  cya->SetNumberOfComponents(1);
  cya->SetName("Cell Y");
  sampleData->GetCellData()->AddArray(cya);

  svtkNew<svtkDoubleArray> cza;
  cza->SetNumberOfComponents(1);
  cza->SetName("Cell Z");
  sampleData->GetCellData()->AddArray(cza);

  svtkIdType ccnt = 0;
  for (int i = 0; i < ZCELLS; i++)
  {
    for (int j = 0; j < YCELLS; j++)
    {
      for (int k = 0; k < XCELLS; k++)
      {
        /*
        ids->Reset();
        if (ZCELLS > 1)
          {
          ids->InsertId(0, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(1, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(2, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(3, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));
          ids->InsertId(4, (i+1)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(5, (i+1)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(6, (i+1)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(7, (i+1)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));
          sampleData->InsertNextCell(SVTK_VOXEL, ids);
          }
        else
          {
          ids->InsertId(0, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k));
          ids->InsertId(1, (i)*(YCELLS+1)*(XCELLS+1) + (j)*(XCELLS+1) + (k+1));
          ids->InsertId(2, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k));
          ids->InsertId(3, (i)*(YCELLS+1)*(XCELLS+1) + (j+1)*(XCELLS+1) + (k+1));
          sampleData->InsertNextCell(SVTK_PIXEL, ids);
          }
        */
        cia->InsertNextValue(ccnt);

        int idF = ccnt + 10;
        int idR = (XCELLS) * (YCELLS) * (ZCELLS)-1 - ccnt + 10;
        ciaF->InsertNextValue(idF);
        ciaR->InsertNextValue(idR);
        ccnt++;

        cxa->InsertNextValue(((double)k + 0.5));
        cya->InsertNextValue(((double)j + 0.5));
        cza->InsertNextValue(((double)i + 0.5));
      }
    }
  }
  // ids->Delete();

  sampleData->GetPointData()->SetGlobalIds(piaF);
  sampleData->GetPointData()->SetScalars(pxa);

  sampleData->GetCellData()->SetGlobalIds(ciaF);
  sampleData->GetCellData()->SetScalars(cxa);

  return sampleData;
}

}

int TestExtractionExpression(int argc, char* argv[])
{
  int DoWrite = 0;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-W"))
    {
      DoWrite = 1;
    }
  }

  //--------------------------------------------------------------------------
  // create a visualization pipeline to see the results
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renwin;
  renwin->SetMultiSamples(0);
  renwin->SetSize(600, 600);
  renwin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> rwi;
  rwi->SetRenderWindow(renwin);

  auto sampleData = createTestData();
  // save the test data set
  svtkNew<svtkXMLDataSetWriter> xwriter;
  xwriter->SetInputData(sampleData);
  xwriter->SetFileName("sampleData.vti");
  if (DoWrite)
  {
    xwriter->Write();
  }

  //-------------------------------------------------------------------------
  // Setup the components of the pipeline
  svtkNew<svtkSelection> selection;
  svtkNew<svtkSelectionNode> sel1;
  svtkNew<svtkSelectionNode> sel2;
  svtkNew<svtkSelectionNode> sel3;
  svtkNew<svtkSelectionNode> sel4;
  svtkNew<svtkSelectionNode> sel5;
  selection->AddNode(sel1);
  selection->AddNode(sel2);
  selection->AddNode(sel3);
  selection->AddNode(sel4);
  selection->AddNode(sel5);

  svtkNew<svtkExtractSelection> ext;
  ext->SetInputData(0, sampleData);
  ext->SetInputData(1, selection);
  ext->PreserveTopologyOff();

  sel1->Initialize();
  sel1->SetContentType(svtkSelectionNode::FRUSTUM);
  sel1->SetFieldType(svtkSelectionNode::CELL);

  svtkNew<svtkDoubleArray> frustcorners;
  frustcorners->SetNumberOfComponents(4);
  frustcorners->SetNumberOfTuples(8);

  frustcorners->SetTuple4(0, 0.1, 2.5, 9.5, 0.0);
  frustcorners->SetTuple4(1, 0.1, 2.5, 2.5, 0.0);
  frustcorners->SetTuple4(2, 0.1, 9.5, 9.5, 0.0);
  frustcorners->SetTuple4(3, 0.1, 9.5, 2.5, 0.0);
  frustcorners->SetTuple4(4, 8.2, 3.2, 4.3, 0.0);
  frustcorners->SetTuple4(5, 8.2, 3.2, 3.2, 0.0);
  frustcorners->SetTuple4(6, 8.2, 4.3, 4.3, 0.0);
  frustcorners->SetTuple4(7, 8.2, 4.3, 3.2, 0.0);
  sel1->SetSelectionList(frustcorners);

  sel2->Initialize();
  sel2->SetContentType(svtkSelectionNode::FRUSTUM);
  sel2->SetFieldType(svtkSelectionNode::CELL);

  svtkNew<svtkDoubleArray> frustcorners2;
  frustcorners2->SetNumberOfComponents(4);
  frustcorners2->SetNumberOfTuples(8);

  frustcorners2->SetTuple4(0, 0.1, 3.7, 3.1, 0.0);
  frustcorners2->SetTuple4(1, 0.1, 3.7, 0.1, 0.0);
  frustcorners2->SetTuple4(2, 7.3, 8.9, 3.1, 0.0);
  frustcorners2->SetTuple4(3, 7.3, 8.9, 0.1, 0.0);
  frustcorners2->SetTuple4(4, 2.5, 3.7, 3.1, 0.0);
  frustcorners2->SetTuple4(5, 2.5, 3.7, 0.1, 0.0);
  frustcorners2->SetTuple4(6, 9.4, 8.9, 3.1, 0.0);
  frustcorners2->SetTuple4(7, 9.4, 8.9, 0.1, 0.0);
  sel2->SetSelectionList(frustcorners2);

  // add id based selection.
  sel3->SetContentType(svtkSelectionNode::INDICES);
  sel3->SetFieldType(svtkSelectionNode::CELL);

  svtkNew<svtkIdTypeArray> ids;
  ids->SetNumberOfTuples(20);
  std::iota(ids->GetPointer(0), ids->GetPointer(0) + 20, 0);
  sel3->SetSelectionList(ids);

  // add location based selection
  sel4->SetContentType(svtkSelectionNode::LOCATIONS);
  sel4->SetFieldType(svtkSelectionNode::CELL);
  svtkNew<svtkDoubleArray> locations;
  locations->SetNumberOfComponents(3);
  locations->SetNumberOfTuples(XCELLS);

  double index = 0.5;
  std::generate_n(reinterpret_cast<svtkVector3d*>(locations->GetPointer(0)),
    locations->GetNumberOfTuples(), [&]() { return svtkVector3d(index++); });
  sel4->SetSelectionList(locations);

  // add threshold-based selection
  sel5->SetContentType(svtkSelectionNode::THRESHOLDS);
  sel5->SetFieldType(svtkSelectionNode::CELL);
  svtkNew<svtkIdTypeArray> thresholds;
  thresholds->SetName("Cell Count");
  thresholds->SetNumberOfComponents(2);
  thresholds->SetNumberOfTuples(2);
  thresholds->SetTuple2(0, 3350, 4000);
  thresholds->SetTuple2(1, 2000, 2010);
  sel5->SetSelectionList(thresholds);
  sel5->GetProperties()->Set(svtkSelectionNode::CONNECTED_LAYERS(), 1);

  ext->Update();
  auto extGrid = svtkUnstructuredGrid::SafeDownCast(ext->GetOutput());
  showMe(extGrid, 0, 0, COLORBYCELL, sampleData->GetCellData()->GetArray(0), renderer);

  double bounds[6];
  sampleData->GetBounds(bounds);

  int retVal = svtkRegressionTestImageThreshold(renwin, 85);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    rwi->Start();
  }

  return !retVal;
}
