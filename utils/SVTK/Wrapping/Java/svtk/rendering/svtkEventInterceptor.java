package svtk.rendering;

import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;

/**
 * This interface defines what should be implemented to intercept interaction
 * events and create custom behavior.
 *
 * @see {@link MouseMotionListener} {@link MouseListener} {@link MouseWheelListener}
 *      {@link KeyListener}
 *
 * @author    Sebastien Jourdain - sebastien.jourdain@kitware.com, Kitware Inc 2012
 * @copyright This work was supported by CEA/CESTA
 *            Commissariat a l'Energie Atomique et aux Energies Alternatives,
 *            15 avenue des Sablieres, CS 60001, 33116 Le Barp, France.
 */
public interface svtkEventInterceptor {

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean keyPressed(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean keyReleased(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean keyTyped(KeyEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseDragged(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseMoved(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseClicked(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseEntered(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseExited(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mousePressed(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseReleased(MouseEvent e);

  /**
   * @param e
   *            event
   * @return true if the event has been consumed and should not be forwarded
   *         to the svtkInteractor
   */
  boolean mouseWheelMoved(MouseWheelEvent e);
}
