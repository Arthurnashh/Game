package com.bistudio.JNIScripting;

public class RVDirectories implements java.io.Serializable
{
  private static final long serialVersionUID = 1;
  private int _jID = -1;
  /** 
returns the campaign directory
   */
  public native static String getCampaignDirectory();
  /** 
returns the mission directory
   */
  public native static String getMissionDirectory();
  /** 
returns the user directory
   */
  public native static String getUserDirectory();
  /** 
find the class in the mission or campaign directory and load its data
   */
  public native static byte[] loadClass(String arg1);
}
