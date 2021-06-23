/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKInteractorAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using SVTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/svtksupport.html
=========================================================================*/

// .NAME QSVTKInteractorAdapter - Handle Qt events.
// .SECTION Description
// QSVTKInteractor handles relaying Qt events to SVTK.

#ifndef Q_SVTK_INTERACTOR_ADAPTER_H
#define Q_SVTK_INTERACTOR_ADAPTER_H

#include "QSVTKWin32Header.h"
#include "svtkGUISupportQtModule.h" // For export macro
#include <QtCore/QObject>

class svtkRenderWindowInteractor;
class QEvent;

// .NAME QSVTKInteractorAdapter - A QEvent translator.
// .SECTION Description
// QSVTKInteractorAdapter translates QEvents and send them to a
// svtkRenderWindowInteractor.
class SVTKGUISUPPORTQT_EXPORT QSVTKInteractorAdapter : public QObject
{
  Q_OBJECT
public:
  // Description:
  // Constructor: takes QObject parent
  QSVTKInteractorAdapter(QObject* parent = nullptr);

  // Description:
  // Destructor
  ~QSVTKInteractorAdapter() override;

  // Description:
  // Set the device pixel ratio, this defaults to 1.0, but in Qt 5 can be != 1.0.
  void SetDevicePixelRatio(float ratio, svtkRenderWindowInteractor* iren = nullptr);
  float GetDevicePixelRatio() { return this->DevicePixelRatio; }

  // Description:
  // Process a QEvent and send it to the interactor
  // returns whether the event was recognized and processed
  bool ProcessEvent(QEvent* e, svtkRenderWindowInteractor* iren);

protected:
  int AccumulatedDelta;
  float DevicePixelRatio;
  static const double DevicePixelRatioTolerance;
};

#endif
