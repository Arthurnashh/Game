<%-- 
    Document   : uploadKeys
    Created on : Jul 23, 2012, 1:17:55 PM
    Author     : ondra
--%>

<%@ page language="java" contentType="text/html" pageEncoding="UTF-8"%>
<HTML>
<HEAD><TITLE>Upload CD keys for a product</TITLE></HEAD>  
<% //  based on http://www.roseindia.net/jsp/file_upload/Sinle_upload.xhtml.shtml %>
  <BODY> 
                <br/><br/><br/>
          <center><table border="0" >
                    <tr><center/><td colspan="3"><p align="center"><B>CD Key management</B><center/></td></tr>
                    <tr>
                    <FORM ENCTYPE="multipart/form-data" ACTION="resources/bicdkeys.cdkeys/newKeys" METHOD=POST>
                        <td><b>List of CD Keys to upload:</b></td>
                        <td><INPUT NAME="cdKeysFile" TYPE="file"/></td>
                        <td><p align="right"><INPUT TYPE="submit" VALUE="Send File" ></p></td>
                        </FORM>                        
                    </tr>
                    <tr>
                    <FORM ACTION="resources/bicdkeys.cdkeys/new" METHOD=GET>
                        <td><b>Upload a single key:</b></td>
                        <td><INPUT NAME="key" size="30" TYPE="text"/></td>
                        <td><p align="right"><INPUT TYPE="submit" VALUE="Register" ></p></td>
                        </FORM>                        
                    </tr>
                    <tr>
                    <FORM ACTION="resources/bicdkeys.cdkeys/history" METHOD=GET>
                        <td><b>Check key history:</b></td>
                        <td><INPUT NAME="key" size="30" TYPE="text"/></td>
                        <td><p align="right"><INPUT TYPE="submit" VALUE="Check" ></p></td>
                        </FORM>                        
                    </tr>
                    <FORM ACTION="resources/bicdkeys.cdkeys/reset" METHOD=GET>
                        <td><b>Reset key history:</b></td>
                        <td><INPUT NAME="key" size="30" TYPE="text"/></td>
                        <td><p align="right"><INPUT TYPE="submit" VALUE="Reset" ></p></td>
                        </FORM>                        
                    </tr>
                    <FORM ACTION="resources/bicdkeys.cdkeys/stats" METHOD="GET">
                        <td><b>Product key statistics:</b></td>
                        <td><INPUT NAME="product" size="10" TYPE="text"/></td>
                        <td><p align="right"><INPUT TYPE="submit" VALUE="Stats" ></p></td>
                        </FORM>                        
                    </tr>
              </table>
     </center>      
     </FORM>
</BODY>
</HTML>

