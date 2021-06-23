/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtView.cxx

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkQtView.h"

#include "svtkObjectFactory.h"
#include <QApplication>
#include <QPixmap>
#include <QWidget>

//----------------------------------------------------------------------------
svtkQtView::svtkQtView() {}

//----------------------------------------------------------------------------
svtkQtView::~svtkQtView() {}

//----------------------------------------------------------------------------
void svtkQtView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkQtView::ProcessQtEvents()
{
  QApplication::processEvents();
}

//----------------------------------------------------------------------------
void svtkQtView::ProcessQtEventsNoUserInput()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//----------------------------------------------------------------------------
bool svtkQtView::SaveImage(const char* filename)
{
  return this->GetWidget() != nullptr ? this->GetWidget()->grab().save(filename) : false;
}
