package com.bistudio.JNIScripting;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class NativeObject implements Serializable
{
    // encoded pointer to GameValue
    private long _peer = 0;

    // tell the C++ to destroy GameValue
    private static native void destroyPeer(long peer);
    // tell the C++ to save GameValue
    private static native byte[] savePeer(long peer);
    // tell the C++ to load GameValue
    private static native long loadPeer(byte[] data);
    
    @Override
    protected void finalize()
    {
        destroyPeer(_peer);
    }
    
    private static final long serialVersionUID = 1;
    private void writeObject(ObjectOutputStream out) throws IOException
    {
        byte[] data = savePeer(_peer);
        out.writeInt(data.length);
        out.write(data);
    }
    private void readObject(ObjectInputStream in) throws IOException
    {
        int size = in.readInt();
        byte[] data = new byte[size];
        in.read(data);
        _peer = loadPeer(data);
    }
}
