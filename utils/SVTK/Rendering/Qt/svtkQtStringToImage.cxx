/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkQtStringToImage.h"

#include "svtkImageData.h"
#include "svtkQImageToImageSource.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkUnicodeString.h"
#include "svtkVector.h"

#include "svtkObjectFactory.h"

// Qt classes
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QString>
#include <QTextDocument>
#include <QTextStream>

namespace
{
struct svtkQtLabelMapEntry
{
  QString Text;
  QColor Color;
  QFont Font;
};
} // End of anonymous namespace

class svtkQtStringToImage::Internals
{
public:
  QFont TextPropertyToFont(svtkTextProperty* tprop, int dpi)
  {
    QFont fontSpec(tprop->GetFontFamilyAsString());
    fontSpec.setBold(tprop->GetBold());
    fontSpec.setItalic(tprop->GetItalic());
    fontSpec.setPixelSize(static_cast<int>(tprop->GetFontSize() * (dpi / 72.)));
    return fontSpec;
  }

  QColor TextPropertyToColor(double* fc, double opacity)
  {
    QColor textColor(static_cast<int>(fc[0] * 255), static_cast<int>(fc[1] * 255),
      static_cast<int>(fc[2] * 255), static_cast<int>(opacity * 255));
    return textColor;
  }
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkQtStringToImage);

//-----------------------------------------------------------------------------
svtkQtStringToImage::svtkQtStringToImage()
{
  this->Implementation = new Internals;
  this->QImageToImage = svtkSmartPointer<svtkQImageToImageSource>::New();
}

//-----------------------------------------------------------------------------
svtkQtStringToImage::~svtkQtStringToImage()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkQtStringToImage::GetBounds(
  svtkTextProperty* property, const svtkUnicodeString& string, int dpi)
{
  svtkVector2i recti(0, 0);
  if (!QApplication::instance())
  {
    svtkErrorMacro("You must initialize a QApplication before using this class.");
    return recti;
  }

  if (!property)
  {
    return recti;
  }

  QFont fontSpec = this->Implementation->TextPropertyToFont(property, dpi);

  QString text = QString::fromUtf8(string.utf8_str());

  QRectF rect;
  QPainterPath path;
  path.addText(0, 0, fontSpec, text);
  rect = path.boundingRect();

  recti.SetX(static_cast<int>(rect.width()));
  recti.SetY(static_cast<int>(rect.height()));

  return recti;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkQtStringToImage::GetBounds(
  svtkTextProperty* property, const svtkStdString& string, int dpi)
{
  svtkVector2i recti(0, 0);
  if (!QApplication::instance())
  {
    svtkErrorMacro("You must initialize a QApplication before using this class.");
    return recti;
  }

  if (!property)
  {
    return recti;
  }

  QFont fontSpec = this->Implementation->TextPropertyToFont(property, dpi);

  QString text(string.c_str());

  QRectF rect;
  QPainterPath path;
  path.addText(0, 0, fontSpec, text);
  rect = path.boundingRect();

  recti.SetX(static_cast<int>(rect.width()));
  recti.SetY(static_cast<int>(rect.height()));

  return recti;
}

int svtkQtStringToImage::RenderString(svtkTextProperty* property, const svtkUnicodeString& string,
  int dpi, svtkImageData* data, int textDims[2])
{
  if (!QApplication::instance())
  {
    svtkErrorMacro("You must initialize a QApplication before using this class.");
    return 0;
  }
  // Get the required size, and initialize a new QImage to draw on.
  svtkVector2i box = this->GetBounds(property, string, dpi);
  if (box.GetX() == 0 || box.GetY() == 0)
  {
    return 0;
  }
  if (textDims)
  {
    textDims[0] = box.GetX();
    textDims[1] = box.GetY();
  }

  QString text = QString::fromUtf8(string.utf8_str());
  QFont fontSpec = this->Implementation->TextPropertyToFont(property, dpi);
  QFontMetrics fontMetric(fontSpec);

  // Get properties from text property
  double rotation = -property->GetOrientation();
  QColor textColor =
    this->Implementation->TextPropertyToColor(property->GetColor(), property->GetOpacity());

  int shOff[2];
  property->GetShadowOffset(shOff);
  double pixelPadding = 2;
  double pixelPaddingX = pixelPadding + shOff[0];
  double pixelPaddingY = pixelPadding - shOff[1];

  QPainterPath path;
  path.addText(0, 0, fontSpec, text);
  QRectF bounds = path.boundingRect();
  bounds.setWidth(bounds.width() + pixelPaddingX);
  bounds.setHeight(bounds.height() + pixelPaddingY);
  QTransform trans;
  trans.rotate(rotation);
  QRectF rotBounds = trans.mapRect(bounds);
  QImage image(static_cast<int>(ceil(rotBounds.width() + pixelPaddingX)),
    static_cast<int>(ceil(rotBounds.height() + pixelPaddingY)),
    QImage::Format_ARGB32_Premultiplied);
  image.fill(qRgba(0, 0, 0, 0));
  QPainter p(&image);
  p.setRenderHint(QPainter::TextAntialiasing, this->Antialias);
  p.setRenderHint(QPainter::Antialiasing, this->Antialias);
  p.translate(-rotBounds.left(), -rotBounds.top());
  p.rotate(rotation);

  if (property->GetShadow())
  {
    p.save();
    p.translate(shOff[0], -shOff[1]);
    double sc[3];
    property->GetShadowColor(sc);
    QColor shadowColor = this->Implementation->TextPropertyToColor(sc, property->GetOpacity());
    p.fillPath(path, shadowColor);
    p.restore();
  }

  p.fillPath(path, textColor);

  this->QImageToImage->SetQImage(&image);
  this->QImageToImage->Modified();
  this->QImageToImage->Update();
  data->DeepCopy(svtkImageData::SafeDownCast(this->QImageToImage->GetOutputDataObject(0)));

  this->QImageToImage->SetQImage(nullptr);
  return 1;
}

int svtkQtStringToImage::RenderString(svtkTextProperty* property, const svtkStdString& string, int dpi,
  svtkImageData* data, int textDims[2])
{
  return this->RenderString(property, svtkUnicodeString::from_utf8(string), dpi, data, textDims);
}

//-----------------------------------------------------------------------------
void svtkQtStringToImage::DeepCopy(svtkQtStringToImage*) {}

//-----------------------------------------------------------------------------
void svtkQtStringToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
