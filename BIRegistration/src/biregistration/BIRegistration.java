/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package biregistration;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.security.*;
import java.security.cert.CertificateFactory;
import java.security.spec.X509EncodedKeySpec;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.prefs.Preferences;
import javax.xml.bind.DatatypeConverter;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author ondra
 */
public class BIRegistration {

    public static JSONObject GetResponse(String query, JSONObject data) throws IOException
    {
        try {
            String server = "localhost";
            Integer port = 8080;
            String path = "/BIUsers/resources/biusers.userids/";
            URL requestURL = new URL("http",server,port,path+query);
            HttpURLConnection connection = (HttpURLConnection)requestURL.openConnection();

            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setDoOutput(true);
            {
                OutputStream outputStream = connection.getOutputStream();
                OutputStreamWriter out = new OutputStreamWriter(outputStream);
                data.write(out);
                out.flush();
                out.close();
                outputStream.flush();
                outputStream.close();
            }
            StringBuilder sb;
            {
                InputStream response = connection.getInputStream();
                try (BufferedReader br = new BufferedReader(new InputStreamReader(response))) {
                    sb = new StringBuilder();
                    String line;
                    while ((line = br.readLine()) != null) sb.append(line).append("\n");
                }
            }
            String result = sb.toString();            
            return new JSONObject(result);
        } catch (JSONException ex) {
            Logger.getLogger(BIRegistration.class.getName()).log(Level.SEVERE, null, ex);
        }
        return null;
    }
    public static String splitLines(String text, int len)
    {
        StringBuilder result = new StringBuilder();
        for (int i=0; i<text.length(); i+=len)
        {
            result.append(text.substring(i, Math.min(text.length(), i+len))).append("\n");
        }
        return result.toString();
    }
    public static String splitLines(String text) {return splitLines(text,64);}
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args)
    {
        try
        {
String serverCertBase64=
"MIIDQjCCAiqgAwIBAgIQJwoKNRIuTAGBKAkC1vuGnzANBgkqhkiG9w0BAQUFADBc"+
"MQswCQYDVQQGEwJDWjETMBEGA1UEChMKSURFQSBHYW1lczEaMBgGA1UECxMRVXNl"+
"ciBSZWdpc3RyYXRpb24xHDAaBgNVBAMTE1JlZ2lzdHJhdGlvbiBTZXJ2ZXIwIBcN"+
"MTIwMzI5MTEzNjUzWhgPNDk5OTEyMzExMjM2NTNaMFwxCzAJBgNVBAYTAkNaMRMw"+
"EQYDVQQKEwpJREVBIEdhbWVzMRowGAYDVQQLExFVc2VyIFJlZ2lzdHJhdGlvbjEc"+
"MBoGA1UEAxMTUmVnaXN0cmF0aW9uIFNlcnZlcjCCASIwDQYJKoZIhvcNAQEBBQAD"+
"ggEPADCCAQoCggEBAIrKww57oT0xmTtZlvM+sD3Z/qgbcj69HWDijPEZH+AQgUn0"+
"pm4FEE1NkOK18l3qmXrvDo6HXwDydAZuIwCKkQdTlLNAat6HIi6sTiw0no7rm0qN"+
"QqB1krIUQMsS7T9ZqbVz+eYMk8zu+LpBhkb0Ju3xYkRqiGc560thFVOU2WNizRMw"+
"5UQWaq+eiuOkUVbQqcUMmAbuPsrclDV6cHirLY9PEclsOdlqJemG9xWUq8eew0wh"+
"e2j8iFjE/0hlqCEI86qySXaA6fQ0hn82VcFHAFaBeQVU8ODBvUXLls5p+nKmKSbs"+
"FErFXrtEMkWxezYFJmxoLEKVGX2P3TqGXfEtZWUCAwEAATANBgkqhkiG9w0BAQUF"+
"AAOCAQEAf714U6WUUJPc9zRZ+GSLccIPsPK3GAHiQANvSueS1o7gyuXz47cxzH+A"+
"iU5yLV/VuEx2RcEd/mzJfXmOcx/ngn4H6vFPAn5m+gd+n9keB36v+0sDtlS1vJI9"+
"5Qcce7wYiY/MGC17MYgxdndu+/L9K8DjWjIK2JtWRsN0l3KONW9yVzMbbeGtXXaB"+
"bGstpB4C+bSjnOZtWf/uIJDm98ISuQAdWQdgCsPZpRD9j7b79YH+sULIQOsQSqnP"+
"+ed6PF0YCMzQKFNHpAmbAF99ItcNa577qI6XgJCR9z8VM9q1UeU2ru6Gz0/y9KY7"+
"v9BzigkonRHFO7S1dgmvPvB3Bc7D2g=="+
"";

            // verify we can load the certificate, verify it against known CA public key
            CertificateFactory cf = CertificateFactory.getInstance("X.509");
            java.security.cert.Certificate caCert = cf.generateCertificate(new ByteArrayInputStream(DatatypeConverter.parseBase64Binary(serverCertBase64)));
            PublicKey caPublicKey = caCert.getPublicKey();
                
            // user ID used to store licenses, this is system wide?
            Preferences userRoot = Preferences.userRoot();
            Preferences node = userRoot.node("com.bistudio.user");
            int oldId = node.getInt("id",0);
            if (oldId!=0 && args.length<=0 || !args[0].equals("-force"))
            {
                // id + certificate already exists, use it
                String certString = node.get("certificate", "");
                byte[] certBytes = DatatypeConverter.parseBase64Binary(certString);

                // load it and verify it
                java.security.cert.Certificate verify = cf.generateCertificate(new ByteArrayInputStream(certBytes));
                verify.verify(caPublicKey);
                return;
            }
            
            JSONObject start = GetResponse("register-start", new JSONObject());
            // decode challenge
            // response was id + challenge
            Integer regId = start.getInt("id");
            Integer challenge = start.getInt("challenge");


            // generate a private / public key pair

            KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");

            keyGen.initialize(1024);
            KeyPair keyPair = keyGen.generateKeyPair();
            PublicKey publicKey = keyPair.getPublic();
            byte [] publicEncoded = publicKey.getEncoded();

            Signature rsa = Signature.getInstance("SHA256withRSA");
            rsa.initSign(keyPair.getPrivate());
            rsa.update(String.valueOf(challenge).getBytes());
            byte [] challengeSignature = rsa.sign();

            JSONObject registerPars = new JSONObject();
            registerPars.put("signature",DatatypeConverter.printBase64Binary(challengeSignature));
            registerPars.put("publicKey",DatatypeConverter.printBase64Binary(publicEncoded));

            JSONObject register = GetResponse(String.format("register-new-id/%d",regId),registerPars);
            Integer id = register.getInt("id");
            String certificate = register.getString("certificate");
            byte[] certBytes = DatatypeConverter.parseBase64Binary(certificate);

            java.security.cert.Certificate verify = cf.generateCertificate(new ByteArrayInputStream(certBytes));
            verify.verify(caPublicKey);

            // we have the id and the certificate - store somewhere, including the private key (use restrictive ACL?)
            node.putInt("id", id);
            node.put("certificate", certificate);
        }
        catch (Exception e)
        {
            // something has failed - what now?
            e.printStackTrace();
        }
    }
}
