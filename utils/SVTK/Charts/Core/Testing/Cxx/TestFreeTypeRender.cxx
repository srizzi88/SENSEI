/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkContextView.h"
#include "svtkImageData.h"
#include "svtkImageItem.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"

#include "svtkUnicodeString.h"

#include "svtkFreeTypeStringToImage.h"
#include "svtkQtStringToImage.h"

#include "svtkRegressionTestImage.h"

#include <QApplication>

//----------------------------------------------------------------------------
int TestFreeTypeRender(int argc, char* argv[])
{
  QApplication app(argc, argv);
  // Set up a 2D context view, context test object and add it to the scene
  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(300, 200);
  svtkSmartPointer<svtkImageItem> item = svtkSmartPointer<svtkImageItem>::New();
  svtkSmartPointer<svtkImageItem> item2 = svtkSmartPointer<svtkImageItem>::New();
  view->GetScene()->AddItem(item);
  view->GetScene()->AddItem(item2);

  // Now try to render some text using freetype...
  svtkSmartPointer<svtkQtStringToImage> qt = svtkSmartPointer<svtkQtStringToImage>::New();
  svtkSmartPointer<svtkFreeTypeStringToImage> freetype =
    svtkSmartPointer<svtkFreeTypeStringToImage>::New();
  svtkSmartPointer<svtkTextProperty> prop = svtkSmartPointer<svtkTextProperty>::New();
  prop->SetColor(0.0, 0.0, 0.0);
  prop->SetFontSize(24);
  double orientation = 0.0;
  prop->SetOrientation(orientation);
  svtkSmartPointer<svtkImageData> imageqt = svtkSmartPointer<svtkImageData>::New();
  int result =
    qt->RenderString(prop, svtkUnicodeString::from_utf8("My String\n AV \xe2\x84\xab"), imageqt);
  item->SetImage(imageqt);
  item->SetPosition(20, 20);

  svtkSmartPointer<svtkImageData> imageft = svtkSmartPointer<svtkImageData>::New();
  result = freetype->RenderString(
    prop, svtkUnicodeString::from_utf8("My String\n AV \xe2\x84\xab"), imageft);
  item2->SetImage(imageft);
  item2->SetPosition(80, 110 - orientation);

  // view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }
  return !retVal;
}
