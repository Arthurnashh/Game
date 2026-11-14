package com.bistudio.JNIScripting;

import java.awt.AWTKeyStroke;
import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Container;
import java.awt.Graphics;
import java.awt.KeyboardFocusManager;
import java.awt.Point;
import java.awt.event.FocusEvent;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.awt.image.Raster;
import java.util.ArrayList;
import java.util.LinkedList;
import javax.swing.JPanel;
import javax.swing.PopupFactory;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.text.DefaultCaret;
import javax.swing.text.JTextComponent;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class GUI 
{
    // area UI is drawn into
    private static BufferedImage _image = null;

    // hierarchy of panels
    private static class PanelInfo
    {
        public JPanel _panel;
        public int _offsetX;
        public int _offsetY;
        
        public PanelInfo(JPanel panel, int x, int y)
        {
            _panel = panel; _offsetX = x; _offsetY = y;
        }
    }
    private static ArrayList<PanelInfo> _panels = new ArrayList<PanelInfo>();
    public static void addPanel(JPanel panel, int x, int y, int width, int height)
    {
        // update panel to be usable in hierarchy
        preparePanel(panel, width, height);
        _panels.add(new PanelInfo(panel, x, y));
        resetFocus();
    }
    public static void removePanel()
    {
        int last = _panels.size() - 1;
        if (last >= 0)
        {
            _panels.remove(last);
            resetFocus();
        }
    }
    public static void movePanel(int x, int y)
    {
        int last = _panels.size() - 1;
        if (last >= 0)
        {
            PanelInfo info = _panels.get(last);
            info._offsetX = x;
            info._offsetY = y;
        }
    }
    public static boolean noPanel()
    {
        return _panels.isEmpty();
    }

    // placeholder for JComboBox, working correctly in the hierarchy
    public static class JComboBoxFixed extends javax.swing.JComboBox
    {
        public Point getLocationOnScreen() 
        {
            // return getLocation();
            return new Point(0, 0);
        }
    };
    // placeholder for DefaultCaret, working correctly in the hierarchy
    public static class CustomCaret extends DefaultCaret
    {
        private long _start;
        public CustomCaret()
        {
            _start = System.currentTimeMillis();
            setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);
            setBlinkRate(500);
        }
        public boolean isVisible()
        {
            JTextComponent component = getComponent();
            if (component == null || !component.hasFocus()) return false;
            int blinkRate = getBlinkRate();
            if (blinkRate <= 0) return true;
            long age = System.currentTimeMillis() - _start;
            return (age % (2 * blinkRate)) <= blinkRate;
        }
    };
    private static void changeCaret(Component c) 
    {
        if (c instanceof JTextComponent) ((JTextComponent)c).setCaret(new CustomCaret());
        if (c instanceof Container)
        {
            Container parent = (Container)c;
            for (Component child : parent.getComponents()) changeCaret(child);
        }
    }

    // GUI handling
    // initialization
    public static void init(int width, int height)
    {
        _image = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        try 
        {
            // Select Look and Feel
            UIManager.setLookAndFeel("com.sun.java.swing.plaf.windows.WindowsLookAndFeel");
        }
        catch (Exception ex) 
        {
            ex.printStackTrace();
        }
        
        // Implementation of popup windows
        PopupFactory.setSharedInstance(new LightPopupFactory());
    }
    // rendering
    public static int[] render()
    {
        if (_image == null) return null;
        if (_panels.isEmpty()) return null;

        Raster raster = _image.getRaster();
        DataBufferInt buffer = (DataBufferInt)raster.getDataBuffer();
        for (int i=0; i<buffer.getSize(); i++) buffer.setElem(i, 0);

        Graphics g = _image.getGraphics();
        for (int i=0; i<_panels.size(); i++)
        {
            PanelInfo info = _panels.get(i);
            Graphics pg = g.create(info._offsetX, info._offsetY, info._panel.getWidth(), info._panel.getHeight());
            info._panel.paint(pg);
        }
        
        return buffer.getData();
    }

    // events processing
    public static void sendMouseEvent(int id, int modifiers, int x, int y, int button)
    {
        // only the top most panel receive events
        int last = _panels.size() - 1;
        if (last < 0) return;
        PanelInfo info = _panels.get(last);
        
        x -= info._offsetX;
        y -= info._offsetY;
        
        long now = System.currentTimeMillis();
        Component target = SwingUtilities.getDeepestComponentAt(info._panel, x, y);
        switch (id)
        {
            case MouseEvent.MOUSE_PRESSED:
                if (button > 0 && button <= 3)
                {
                    focusComponent(info._panel, target);
                    if (target != null)
                    {
                        Point pt = SwingUtilities.convertPoint(info._panel, x, y, target);
                        _pressedInfo[button - 1] = new PressedInfo(target, pt);
                        MouseEvent event = new MouseEvent(target, id, now, modifiers,
                            pt.x, pt.y, pt.x, pt.y, 0, false, button);
                        target.dispatchEvent(event);
                    }
                }
                break;
            case MouseEvent.MOUSE_RELEASED:
                if (button > 0 && button <= 3)
                {
                    if (target != null)
                    {
                        Point pt = SwingUtilities.convertPoint(info._panel, x, y, target);
                        MouseEvent event = new MouseEvent(target, id, now, modifiers,
                            pt.x, pt.y, pt.x, pt.y, 0, false, button);
                        target.dispatchEvent(event);
                        // check if click event occurs
                        PressedInfo pressed = _pressedInfo[button - 1];
                        if (pressed != null && !pressed._moved && pressed._target == target && pressed._pt.equals(pt))
                        {
                            MouseEvent eventClick = new MouseEvent(target, MouseEvent.MOUSE_CLICKED, now, modifiers,
                                pt.x, pt.y, pt.x, pt.y, 0, false, button);
                            target.dispatchEvent(eventClick);
                        }
                    }
                    _pressedInfo[button - 1] = null;
                }
                break;
            case MouseEvent.MOUSE_MOVED:
                if (_lastX != x || _lastY != y)
                {
                    if (button > 0 && button <= 3)
                    {
                        PressedInfo pressed = _pressedInfo[button - 1];
                        if (pressed != null)
                        {
                            pressed._moved = true;
                            if (pressed._target != null)
                            {
                                target = pressed._target;
                                Point pt = SwingUtilities.convertPoint(info._panel, x, y, target);
                                MouseEvent event = new MouseEvent(target, MouseEvent.MOUSE_DRAGGED, now, modifiers,
                                    pt.x, pt.y, pt.x, pt.y, 0, false, button);
                                target.dispatchEvent(event);
                            }
                        }
                    }
                    else
                    {
                        if (target != null)
                        {
                            Point pt = SwingUtilities.convertPoint(info._panel, x, y, target);
                            MouseEvent event = new MouseEvent(target, id, now, modifiers,
                                pt.x, pt.y, pt.x, pt.y, 0, false, button);
                            target.dispatchEvent(event);
                        }
                    }
                }
                _lastX = x; _lastY = y;
                break;
        }
    }
    public static void sendMouseWheelEvent(int rotation, int modifiers, int x, int y)
    {
        // only the top most panel receive events
        int last = _panels.size() - 1;
        if (last < 0) return;
        PanelInfo info = _panels.get(last);

        x -= info._offsetX;
        y -= info._offsetY;

        long now = System.currentTimeMillis();
        Component target = SwingUtilities.getDeepestComponentAt(info._panel, x, y);
        if (target != null)
        {
            MouseWheelEvent event = new MouseWheelEvent(target, MouseEvent.MOUSE_WHEEL,
                    now, modifiers, x, y,
                    0, false, MouseWheelEvent.WHEEL_UNIT_SCROLL, 1, rotation);
            target.dispatchEvent(event);
        }
    }
    public static void sendKeyEvent(int id, int modifiers, int keyCode, char keyChar)
    {
        // only the top most panel receive events
        int last = _panels.size() - 1;
        if (last < 0) return;
        PanelInfo info = _panels.get(last);

        // handle focus ourselves
        KeyboardFocusManager manager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
        Component focused = manager.getFocusOwner();
        
        if (focused != null)
        {
            long now = System.currentTimeMillis();
            KeyEvent event = new KeyEvent(focused, id, now, modifiers, keyCode, keyChar, KeyEvent.KEY_LOCATION_UNKNOWN);
            
            // check if focus have to be changed
            if (focused.getFocusTraversalKeysEnabled())
            {
                AWTKeyStroke stroke = AWTKeyStroke.getAWTKeyStrokeForEvent(event);
                AWTKeyStroke oppStroke = AWTKeyStroke.getAWTKeyStroke(
                        stroke.getKeyCode(), stroke.getModifiers(), !stroke.isOnKeyRelease());
                
                if (handleStroke(info._panel, focused, KeyboardFocusManager.FORWARD_TRAVERSAL_KEYS, stroke, oppStroke)) return;
                if (handleStroke(info._panel, focused, KeyboardFocusManager.BACKWARD_TRAVERSAL_KEYS, stroke, oppStroke)) return;
/*
                if (handleStroke(focused, KeyboardFocusManager.UP_CYCLE_TRAVERSAL_KEYS, stroke, oppStroke)) return;
                if (handleStroke(focused, KeyboardFocusManager.DOWN_CYCLE_TRAVERSAL_KEYS, stroke, oppStroke)) return;
*/
            }
            
            // offer the key to the focused component
            try
            {
                manager.redispatchEvent(focused, event);
            }
            catch (Exception ex)
            {
                ex.printStackTrace();
            }
        }
    }
    
    // implementation
    private static void preparePanel(JPanel panel, int width, int height)
    {
        // tune focusing
        panel.addNotify();
        panel.setFocusCycleRoot(true);
            
        // use own caret
        synchronized (panel.getTreeLock()) 
        {
            changeCaret(panel);
        }

        // layout components
        panel.setSize(width, height);
        synchronized (panel.getTreeLock()) 
        {
            layoutRecursive(panel);
        }
    }
    private static void layoutRecursive(Container cont)
    {
        if (!cont.isValid()) cont.doLayout();

        for (Component comp : cont.getComponents())
        {
            if (comp instanceof Container) layoutRecursive((Container)comp);
            else comp.validate();
        }
        cont.validate();
    }

    // modified popup windows handling
    private static class LightPopupFactory extends javax.swing.PopupFactory
    {
        private class PopupComponent extends JPanel
        {
            public PopupComponent()
            {
                super(new BorderLayout(), true);
            }
            public void init(Component contents)
            {
                setOpaque(contents.isOpaque());
                add(contents, BorderLayout.CENTER);
                setSize(getPreferredSize());
                
                synchronized (getTreeLock()) 
                {
                    validateTree();
                }
            }
        };
        // Popup implementation that uses a JPanel as the popup.
        private class LightWeightPopup extends javax.swing.Popup
        {
            // TODO: possible to recycle components
            
            /** Implementation component */
            Component _component;
            /** Component we are to be added to. */
            Component _owner;
            /** Desired x location. */
            int _x;
            /** Desired y location. */
            int _y;
            
            public void hide() 
            {
                if (_component != null)
                {
                    Container parent = _component.getParent();
                    if (parent != null) parent.remove(_component);
                    _component = null;
                }
                _owner = null;
            }
            public void show() 
            {
                // find the parent to insert component into
                Component parent = _owner;
                while (parent.getParent() != null) parent = parent.getParent();

                if (parent != null && parent instanceof Container)
                {
                    Point p = SwingUtilities.convertPoint(_owner, _x, _y, parent);
                    _component.setLocation(p.x, p.y);
                    ((Container)parent).add(_component, javax.swing.JLayeredPane.POPUP_LAYER, 0);

                }
            }
            public void reset(Component owner, Component contents, int ownerX, int ownerY) 
            {
                _x = ownerX;
                _y = ownerY;
                _owner = owner;
                
                PopupComponent component = new PopupComponent();
                component.init(contents);
                _component = component;
            }
        };
        
        public javax.swing.Popup getPopup(Component owner, Component contents, int x, int y)
        {
            LightWeightPopup popup = new LightWeightPopup();
            popup.reset(owner, contents, x, y);
            return popup;
        }
    };
    
    // own focus handling
    private static class PressedInfo
    {
        public Component _target = null;
        public Point _pt = null;
        public boolean _moved = false;
        
        public PressedInfo(Component target, Point pt)
        {
            _target = target; _pt = pt;
        }
    }
    private static PressedInfo[] _pressedInfo = new PressedInfo[3];
    private static int _lastX = Integer.MIN_VALUE;
    private static int _lastY = Integer.MIN_VALUE;
    private static void resetFocus() 
    {
        _lastX = Integer.MIN_VALUE;
        _lastY = Integer.MIN_VALUE;
        for (PressedInfo info : _pressedInfo) info._target = null;
    }
    private static void focusComponent(JPanel panel, Component toFocus) 
    {
        KeyboardFocusManager manager = KeyboardFocusManager.getCurrentKeyboardFocusManager();
        Component focused = manager.getFocusOwner();

        if (toFocus == focused) return;
        
        if (focused != null)
        {
            FocusEvent fEvent = new FocusEvent(focused, FocusEvent.FOCUS_LOST, false);
            panel.dispatchEvent(fEvent);
        }
        focused = toFocus;
        if (focused != null)
        {
            FocusEvent fEvent = new FocusEvent(focused, FocusEvent.FOCUS_GAINED, false);
            panel.dispatchEvent(fEvent);
        }
    }

    // modified focus traversal policy
    private static class LayoutComparatorFixed implements java.util.Comparator<Component>, java.io.Serializable 
    {
        private static final int ROW_TOLERANCE = 10;

        @Override
        public int compare(Component a, Component b) 
        {
            if (a == b) return 0;

            // Row/Column algorithm only applies to siblings. If 'a' and 'b'
            // aren't siblings, then we need to find their most inferior
            // ancestors which share a parent. Compute the ancestory lists for
            // each Component and then search from the Window down until the
            // hierarchy branches.
            if (a.getParent() != b.getParent()) 
            {
                LinkedList<Component> aAncestory = new LinkedList<Component>();
                while (true)
                {
                    aAncestory.add(a);
                    if (a.getParent() == null) break;
                    a = a.getParent();
                }
                
                LinkedList<Component> bAncestory = new LinkedList<Component>();
                while (true)
                {
                    bAncestory.add(b);
                    if (b.getParent() == null) break;
                    b = b.getParent();
                }

                for (java.util.ListIterator<Component>
                         aIter = aAncestory.listIterator(aAncestory.size()),
                         bIter = bAncestory.listIterator(bAncestory.size()); ;) {
                    if (aIter.hasPrevious()) {
                        a = aIter.previous();
                    } else {
                        // a is an ancestor of b
                        return -1;
                    }

                    if (bIter.hasPrevious()) {
                        b = bIter.previous();
                    } else {
                        // b is an ancestor of a
                        return 1;
                    }

                    if (a != b) {
                        break;
                    }
                }
            }

            int ax = a.getX(), ay = a.getY(), bx = b.getX(), by = b.getY();

            int zOrder = a.getParent().getComponentZOrder(a) - b.getParent().getComponentZOrder(b);

            // LT - Western Europe (optional for Japanese, Chinese, Korean)

            if (Math.abs(ay - by) < ROW_TOLERANCE) 
                return (ax < bx) ? -1 : ((ax > bx) ? 1 : zOrder);
            else 
                return (ay < by) ? -1 : 1;
        }
    }
    private static class LayoutFocusTraversalPolicyFixed extends javax.swing.LayoutFocusTraversalPolicy
    {
        public LayoutFocusTraversalPolicyFixed()
        {
            this.setComparator(new LayoutComparatorFixed());
        }
    };
    private static LayoutFocusTraversalPolicyFixed _layoutPolicy = new LayoutFocusTraversalPolicyFixed();
    private static boolean handleStroke(JPanel panel, Component focused, int keysType, AWTKeyStroke stroke, AWTKeyStroke oppStroke) 
    {
        java.util.Set toTest = focused.getFocusTraversalKeys(keysType);
        if (toTest.contains(stroke))
        {
            Component toFocus = null;
            switch (keysType)
            {
            case KeyboardFocusManager.FORWARD_TRAVERSAL_KEYS:
                toFocus = _layoutPolicy.getComponentAfter(panel, focused);
                break;
            case KeyboardFocusManager.BACKWARD_TRAVERSAL_KEYS:
                toFocus = _layoutPolicy.getComponentBefore(panel, focused);
                break;
/*
            case KeyboardFocusManager.UP_CYCLE_TRAVERSAL_KEYS:
                break;
            case KeyboardFocusManager.DOWN_CYCLE_TRAVERSAL_KEYS:
                break;
*/
            }
            if (toFocus == null) toFocus = _layoutPolicy.getDefaultComponent(panel);
            focusComponent(panel, toFocus);
            return true;
        }
        return toTest.contains(oppStroke); // check if key handled
    }
}
