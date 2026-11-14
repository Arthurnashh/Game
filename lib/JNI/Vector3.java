package com.bistudio.JNIScripting;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Jirka
 */
public class Vector3 implements java.io.Serializable
{
  private static final long serialVersionUID = 1;

  public float _x;
  public float _y;
  public float _z;

  public Vector3()
  {
    _x = 0; _y = 0; _z = 0;
  }
  public Vector3(float x, float y, float z)
  {
    _x = x; _y = y; _z = z;
  }
};
