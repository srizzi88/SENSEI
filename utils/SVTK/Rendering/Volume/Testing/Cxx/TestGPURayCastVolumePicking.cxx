/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastVolumePicking.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers volume picking with svtkGPURayCastVolumePicking using
// svtkHardwareSelector.
// This test renders volume data along with polydata objects and selects
// the volume.
// Use 'p' for poin picking and 'r' for area selection.

#include "svtkInteractorStyleRubberBandPick.h"
#include "svtkRenderedAreaPicker.h"
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkDataArray.h>
#include <svtkGPUVolumeRayCastMapper.h>
#include <svtkImageChangeInformation.h>
#include <svtkImageData.h>
#include <svtkImageReader.h>
#include <svtkImageShiftScale.h>
#include <svtkNew.h>
#include <svtkOutlineFilter.h>
#include <svtkPiecewiseFunction.h>
#include <svtkPointData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTestUtilities.h>
#include <svtkTimerLog.h>
#include <svtkVolumeProperty.h>
#include <svtkXMLImageDataReader.h>

#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkHardwareSelector.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSphereSource.h"

namespace
{
class VolumePickingCommand : public svtkCommand
{
public:
  static VolumePickingCommand* New() { return new VolumePickingCommand; }

  void Execute(svtkObject* svtkNotUsed(caller), unsigned long svtkNotUsed(eventId),
    void* svtkNotUsed(callData)) override
  {
    assert(this->Renderer != nullptr);

    svtkNew<svtkHardwareSelector> selector;
    selector->SetRenderer(this->Renderer);
    selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);

    unsigned int const x1 = static_cast<unsigned int>(this->Renderer->GetPickX1());
    unsigned int const y1 = static_cast<unsigned int>(this->Renderer->GetPickY1());
    unsigned int const x2 = static_cast<unsigned int>(this->Renderer->GetPickX2());
    unsigned int const y2 = static_cast<unsigned int>(this->Renderer->GetPickY2());
    selector->SetArea(x1, y1, x2, y2);
    // std::cout << "->>> SetArea (x1, y1, x2, y2): (" << x1 << ", " << y1 << ", "
    //  << x2 << ", " << y2 << ")" << '\n';

    svtkSelection* result = selector->Select();
    // result->Print(std::cout);

    unsigned int const numProps = result->GetNumberOfNodes();

    for (unsigned int n = 0; n < numProps; n++)
    {
      svtkSelectionNode* node = result->GetNode(n);
      svtkInformation* properties = node->GetProperties();
      svtkInformationIntegerKey* infoIntKey = node->PROP_ID();

      svtkAbstractArray* abs = node->GetSelectionList();
      svtkIdType size = abs->GetSize();
      std::cout << "PropId: " << infoIntKey->Get(properties) << "/ Num. Attr.:  " << size << '\n';

      if (numProps > 1)
        continue;

      // Get the svtkAlgorithm instance of the prop to connect it to
      // the outline filter.
      svtkInformationObjectBaseKey* key = node->PROP();
      svtkObjectBase* keyObj = key->Get(properties);
      if (!keyObj)
        continue;

      svtkAbstractMapper3D* mapper = nullptr;
      svtkActor* actor = svtkActor::SafeDownCast(keyObj);
      svtkVolume* vol = svtkVolume::SafeDownCast(keyObj);
      if (actor)
        mapper = svtkAbstractMapper3D::SafeDownCast(actor->GetMapper());
      else if (vol)
        mapper = svtkAbstractMapper3D::SafeDownCast(vol->GetMapper());
      else
        continue;

      if (!mapper)
        continue;

      svtkAlgorithm* algo = mapper->GetInputAlgorithm();
      if (!algo)
        continue;

      this->OutlineFilter->SetInputConnection(algo->GetOutputPort());
    }

    result->Delete();
  };
  //////////////////////////////////////////////////////////////////////////////

  svtkSmartPointer<svtkRenderer> Renderer;
  svtkSmartPointer<svtkOutlineFilter> OutlineFilter;
};
}

// =============================================================================
int TestGPURayCastVolumePicking(int argc, char* argv[])
{
  // volume source and mapper
  svtkNew<svtkXMLImageDataReader> reader;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);

  delete[] volumeFile;

  svtkNew<svtkImageChangeInformation> changeInformation;
  changeInformation->SetInputConnection(reader->GetOutputPort());
  changeInformation->SetOutputSpacing(1, 2, 3);
  changeInformation->SetOutputOrigin(10, 20, 30);
  changeInformation->Update();

  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  double scalarRange[2];
  volumeMapper->SetInputConnection(changeInformation->GetOutputPort());
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  svtkNew<svtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  svtkSmartPointer<svtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  svtkNew<svtkVolume> volume;
  volume->PickableOn();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // polygonal sources and mappers
  svtkNew<svtkConeSource> cone;
  cone->SetHeight(100.0);
  cone->SetRadius(50.0);
  cone->SetResolution(200.0);
  cone->SetCenter(80, 100, 100);
  cone->Update();

  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());

  svtkNew<svtkActor> coneActor;
  coneActor->SetMapper(coneMapper);
  coneActor->PickableOn();

  svtkNew<svtkSphereSource> sphere;
  sphere->SetPhiResolution(20.0);
  sphere->SetThetaResolution(20.0);
  sphere->SetCenter(90, 40, 170);
  sphere->SetRadius(40.0);
  sphere->Update();

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->AddInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->PickableOn();

  // Add outline filter
  svtkNew<svtkActor> outlineActor;
  svtkNew<svtkPolyDataMapper> outlineMapper;
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(cone->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);
  outlineActor->PickableOff();

  // rendering setup
  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddActor(coneActor);
  ren->AddActor(sphereActor);
  ren->AddActor(outlineActor);
  ren->AddViewProp(volume);

  svtkNew<svtkRenderWindow> renWin;
  // renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  ren->ResetCamera();

  // interaction & picking
  svtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  svtkInteractorStyleRubberBandPick* rbp = svtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);
  svtkRenderedAreaPicker* areaPicker = svtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  // Add selection observer
  svtkNew<VolumePickingCommand> vpc;
  vpc->Renderer = ren;
  vpc->OutlineFilter = outlineFilter;
  rwi->AddObserver(svtkCommand::EndPickEvent, vpc);

  // run the actual test
  areaPicker->AreaPick(177, 125, 199, 206, ren);
  vpc->Execute(nullptr, 0, nullptr);
  renWin->Render();

  // initialize render loop
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  areaPicker->Delete();
  rbp->Delete();

  return !retVal;
}
