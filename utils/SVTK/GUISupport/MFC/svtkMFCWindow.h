/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMFCWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkMFCWindow_h
#define svtkMFCWindow_h

#include "afxwin.h"
#include "svtkGUISupportMFCModule.h" // For export macro

class svtkWin32OpenGLRenderWindow;
class svtkRenderWindowInteractor;

#include "svtkMFCConfigure.h"

//! class to display a SVTK window in an MFC window
class SVTKGUISUPPORTMFC_EXPORT svtkMFCWindow : public CWnd
{
public:
  //! constructor requires a parent
  svtkMFCWindow(CWnd* pcWnd);
  //! destructor
  virtual ~svtkMFCWindow();

#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  //! draw to a device context
  void DrawDC(CDC* pDC);

  //! give an instance of a svtk render window to the mfc window
  virtual void SetRenderWindow(svtkWin32OpenGLRenderWindow*);
  //! get the render window
  virtual svtkWin32OpenGLRenderWindow* GetRenderWindow();
  //! get the interactor
  virtual svtkRenderWindowInteractor* GetInteractor();

protected:
  //! handle size events
  afx_msg void OnSize(UINT nType, int cx, int cy);
  //! handle paint events
  afx_msg void OnPaint();
  //! handle destroy events
  afx_msg void OnDestroy();
  //! don't clear background
  BOOL OnEraseBkgnd(CDC* pDC);

  afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
  afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnTimer(UINT_PTR nIDEvent);

  //! the svtk window
  svtkWin32OpenGLRenderWindow* psvtkWin32OpenGLRW;

  DECLARE_MESSAGE_MAP()
};

#endif

// SVTK-HeaderTest-Exclude: svtkMFCWindow.h
