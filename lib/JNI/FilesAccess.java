package com.bistudio.JNIScripting;

import java.nio.file.Path;
import java.nio.file.Paths;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class FilesAccess extends SecurityManager
{
    // disable creation from ouside
    private FilesAccess()
    {
        super();
    }

    // set
    public static boolean init()
    {
        FilesAccess instance = new FilesAccess();
        try
        {
            System.setSecurityManager(instance);
            return true;
        }
        catch (Throwable e)
        {
            return false;
        }
    }

    public native String[] getReadableDirectories();
    public native String[] getWritableDirectories();
    
    private boolean isSubDirectory(Path base, Path child)
    {
      Path parent = child;
      while (parent != null)
      {
          if (base.equals(parent)) return true;
          parent = parent.getParent();
      }
      return false;
    }
    private void checkDirectory(String file, String[] dirs)
    {
        Path path = Paths.get(file).normalize();
        for (String dir : dirs)
        {
            Path root = Paths.get(dir).normalize();
            if (isSubDirectory(root, path)) return; // no exception, path is ok
        }
        throw new SecurityException("Access not enabled outside game directories (" + file + ")");
    }

    @Override
    public void checkRead(String file)
    {
        checkDirectory(file, getReadableDirectories());
    }
    @Override
    public void checkRead(String file, Object context)
    {
        checkDirectory(file, getReadableDirectories());
    }
    @Override
    public void checkWrite(String file)
    {
        checkDirectory(file, getWritableDirectories());
    }
    @Override
    public void checkDelete(String file)
    {
        checkDirectory(file, getWritableDirectories());
    }
    @Override
    public void checkCreateClassLoader()
    {
        // we are using our own class loader
    }
    @Override
    public void checkPackageAccess(String pkg) 
    {
        if (pkg != null)
        {
            // packages needed for the serialization
            if (pkg.startsWith("sun.reflect")) return;
            if (pkg.startsWith("sun.security.provider")) return;
            // UI rendering
            if (pkg.startsWith("sun.awt")) return;
            if (pkg.startsWith("com.sun.java.swing")) return;
        }
        
        super.checkPackageAccess(pkg);
    }
}
