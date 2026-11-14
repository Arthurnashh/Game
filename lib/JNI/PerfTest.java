package com.bistudio.JNIScripting;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */

public class PerfTest
{
    public native static float sin(float x);

    public static float perfTestJava(float interval)
    {
      float result = 0;
      for (float t=0; t<360; t+=interval)
      {
        double rad = t * Math.PI / 180.0;
        result += Math.sin(rad);
      }
      return result;
    }
    public static float perfTestJNI(float interval)
    {
      float result = 0;
      for (float t=0; t<360; t+=interval)
      {
        result += sin(t);
      }
      return result;
    }
    public static float perfTestJNIRV(float interval)
    {
      float result = 0;
      for (float t=0; t<360; t+=interval)
      {
        result += RVEngine.sin(t);
      }
      return result;
    }
}
