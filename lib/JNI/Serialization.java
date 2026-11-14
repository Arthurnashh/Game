package com.bistudio.JNIScripting;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class Serialization 
{
    public static byte[] saveObject(Object object)
    {
        ObjectOutputStream out = null;
        try 
        {
            ByteArrayOutputStream stream = new ByteArrayOutputStream();
            out = new ObjectOutputStream(stream);
            out.writeObject(object);
            return stream.toByteArray();
        } 
        catch (Throwable ex) 
        {
            return null;
        }
        finally 
        {
            try 
            {
                out.close();
            }
            catch (IOException ex) 
            {
            }
        }
    }
    // helper extension of ObjectInputStream using our custom class loader
    private static class ObjectInputStreamExt extends ObjectInputStream
    {
        private ClassLoader _loader = null;
        public ObjectInputStreamExt(InputStream in) throws IOException
        {
            super(in);
            _loader = ClassReloader.createLoader();
        }
        @Override
        protected Class resolveClass(ObjectStreamClass desc) throws ClassNotFoundException
        {
            return Class.forName(desc.getName(), false, _loader);
        }
    };
    public static Object loadObject(byte[] array)
    {
        ObjectInputStream in = null;
        try 
        {
            ByteArrayInputStream stream = new ByteArrayInputStream(array);
            in = new ObjectInputStreamExt(stream);
            return in.readObject();
        }
        catch (Throwable ex) 
        {
            return null;
        }
        finally 
        {
            try 
            {
                in.close();
            }
            catch (IOException ex) 
            {
            }
        }
    }
};
