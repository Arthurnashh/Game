package com.bistudio.JNIScripting;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author jirka
 */
public class ClassReloader extends ClassLoader
{
    private ClassReloader(ClassLoader parent) 
    {
        super(parent);
    }
    @Override
    public Class loadClass(String name) throws ClassNotFoundException
    {
        String path = name.replace('.', '\\');
        byte[] data = RVDirectories.loadClass(path);
        if (data.length > 0)
        {
            Class cls = null;
            try
            {
                cls = defineClass(name, data, 0, data.length);
            }
            catch (Throwable ex)
            {
                ex.printStackTrace();
            }
            // System.out.println(cls + " loaded by engine");
            return cls;
        }
        // System.out.println(cls + " loaded by JVM");
        return super.loadClass(name);
    }

    // reloads the class given by name and the content of .class file
    public static Class initClass(String name) throws ClassNotFoundException
    {
        // each reload need to use its own instance of class loader to enable reload
        ClassLoader parent = RVEngine.class.getClassLoader();
        ClassReloader loader = new ClassReloader(parent);
        return loader.loadClass(name);
    }
    // return instance of loader used in serialization
    public static ClassLoader createLoader()
    {
        ClassLoader parent = RVEngine.class.getClassLoader();
        return new ClassReloader(parent);
    }
}
