package com.bistudio.JNIScripting;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class Logging extends ByteArrayOutputStream
{
    // redirect the log to engine
    private native void logImpl(String message);
    
    // initialization of the logging system
    public static void init()
    {
       Logging stdout = new Logging("JVM OUT: ");
       System.setOut(new PrintStream(stdout, true));

       Logging stderr = new Logging("JVM ERR: ");
       System.setErr(new PrintStream(stderr, true));
    }
    
    // implementation
    private String _prefix;
    private String _lineSeparator;
    public Logging(String prefix)
    {
        super();
        _prefix = prefix;
        _lineSeparator = System.getProperty("line.separator");
    }
    public void flush() throws IOException
    {
        synchronized (this)
        {
            super.flush();
            String content = this.toString();
            super.reset();
            
            if (content.isEmpty() || content.equals(_lineSeparator)) return; // avoid empty records

            int index = content.indexOf(_lineSeparator);
            while (index >= 0)
            {
                logImpl(_prefix + content.substring(0, index));
                content = content.substring(index + _lineSeparator.length());
            }
            if (!content.isEmpty() && !content.equals(_lineSeparator)) logImpl(_prefix + content);
        }
    }
}
