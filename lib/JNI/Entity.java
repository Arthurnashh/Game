package com.bistudio.JNIScripting;

public class Entity implements java.io.Serializable
{
  private static final long serialVersionUID = 1;
  private int _jID = -1;
  /** 
returns the entity position
   */
  public native Vector3 getPosition();
  /** 
sets the entity position
   */
  public native void setPosition(Vector3 arg1);
}
