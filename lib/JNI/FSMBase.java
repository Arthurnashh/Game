package com.bistudio.JNIScripting;

import java.io.Serializable;
import java.util.ArrayList;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class FSMBase implements Serializable
{
    protected class FSMLink implements Serializable
    {
        protected double _priority = 0;
        protected FSMState _nextState = null;

        protected void precondition()
        {
        }
        protected boolean condition()
        {
            return false;
        }
        protected void action()
        {
        }
        protected FSMLink(double priority, FSMState nextState)
        {
            _priority = priority;
            _nextState = nextState;
        }
    }
    protected class FSMState implements Serializable
    {
        protected ArrayList<FSMLink> _links = new ArrayList<FSMLink>();
        protected void addLink(FSMLink link)
        {
            // add the link, keep the array sorted decsendent by priority
            for (int i=0; i<_links.size(); i++)
            {
                if (link._priority >= _links.get(i)._priority)
                {
                    _links.add(i, link);
                    return;
                }
            }
            _links.add(link);
        }
        
        protected boolean _final = false;
        public void create()
        {
        }
        protected void init()
        {
        }
        protected void precondition()
        {
        }
    }
    
    // current state
    private FSMState _current = null;
    // init() of current state was called already
    private boolean _initialized = false;
    
    protected void create(FSMState init)
    {
        _current = init;
    }
    // single simulation step, returns true if final state was reached
    public boolean update()
    {
        if (_current == null) return true;
        
        if (!_initialized)
        {
            _current.init();
            _initialized = true;
        }
        if (_current._final) return true;
        
        _current.precondition();
        for (FSMLink link : _current._links)
        {
            link.precondition();
            if (link.condition())
            {
                link.action();
                _current = link._nextState;
                _initialized = false;
                return false;
            }
        }
        return false;
    }

    private static final long serialVersionUID = 1;
}
