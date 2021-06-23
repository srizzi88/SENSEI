package svtk.rendering;

import java.util.concurrent.locks.ReentrantLock;

import svtk.svtkCamera;
import svtk.svtkGenericRenderWindowInteractor;
import svtk.svtkInteractorStyle;
import svtk.svtkRenderWindow;
import svtk.svtkRenderer;

/**
 * Generic API for any new SVTK based graphical components.
 *
 * @param <T>
 *            The concrete type of the graphical component that will contains
 *            the svtkRenderWindow.
 *
 * @author    Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */

public interface svtkComponent<T> {

  /**
   * @return the lock that is used to prevent concurrency inside this
   *         rendering component. This lock can also be used outside to make
   *         sure any SVTK processing happen in a save manner.
   */
  ReentrantLock getSVTKLock();

  /**
   * Adjust the camera position so any object in the scene will be fully seen.
   */
  void resetCamera();

  /**
   * Update the clipping range of the camera
   */
  void resetCameraClippingRange();

  /**
   * @return the active camera of the renderer
   */
  svtkCamera getActiveCamera();

  /**
   * @return a reference to the Renderer used internally
   */
  svtkRenderer getRenderer();

  /**
   * Useful for screen capture or exporter.
   *
   * @return a reference to the RenderWindow used internally
   */
  svtkRenderWindow getRenderWindow();

  /**
   * svtkWindowInteractor is useful if you want to attach 3DWidget into your
   * view.
   *
   * @return a reference to the svtkWindowInteractor used internally
   */
  svtkGenericRenderWindowInteractor getRenderWindowInteractor();

  /**
   * Shortcut method to bind an svtkInteractorStyle to our interactor.
   *
   * @param style
   */
  void setInteractorStyle(svtkInteractorStyle style);

  /**
   * Update width and height of the given component
   *
   * @param w
   * @param h
   */
  void setSize(int w, int h);

  /**
   * @return the concrete implementation of the graphical container such as
   *         java.awt.Canvas / java.swing.JComponent /
   *         org.eclipse.swt.opengl.GLCanvas
   */
  T getComponent();

  /**
   * Remove any reference from Java to svtkObject to allow the SVTK Garbage
   * collector to free any remaining memory. This is specially needed for
   * internal hidden reference to svtkObject.
   */
  void Delete();

  /**
   * Request a render.
   */
  void Render();

  /**
   * @return the svtkInteractor Java event converter.
   */
  svtkInteractorForwarder getInteractorForwarder();
}
