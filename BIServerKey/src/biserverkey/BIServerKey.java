/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package biserverkey;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.security.*;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Calendar;
import java.util.Date;
import java.util.UUID;
import javax.xml.bind.DatatypeConverter;
import sun.security.x509.*;

/**
 *
 * @author ondra
 */
public class BIServerKey {
    public static String splitLines(String text, int len)
    {
        StringBuilder result = new StringBuilder();
        for (int i=0; i<text.length(); i+=len)
        {
            result.append("\"").append(text.substring(i, Math.min(text.length(), i+len))).append("\"+\n");
        }
        return result.toString();
    }
    public static String splitLines(String text) {return splitLines(text,64);}

    public static byte[] uuidToByteArray(UUID uuid)
    {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(baos);
 
        try
        {
            dos.writeLong(uuid.getMostSignificantBits());
            dos.writeLong(uuid.getLeastSignificantBits());
 
            return baos.toByteArray();
        }
        catch (IOException e)
        {
            throw new RuntimeException(e);  //should never happen
        }
    }
 
    public static BigInteger uuidToBigInteger(UUID uuid)
    {
        return new BigInteger(1, uuidToByteArray(uuid));
    }
    
    public static X509Certificate generateCertificate(String dn, String caName, PublicKey pubKey, PrivateKey caPrivateKey)
    throws GeneralSecurityException, IOException
    {
        X509CertInfo info = new X509CertInfo();
        Calendar cal = Calendar.getInstance();
        Date from = new Date();
        cal.set(5000,0,0); // year 5000 - should be enough?
        CertificateValidity interval = new CertificateValidity(from, cal.getTime());

        info.set(X509CertInfo.VALIDITY, interval);
        UUID uid = UUID.randomUUID();
        BigInteger sn = uuidToBigInteger(uid);
        info.set(X509CertInfo.SERIAL_NUMBER, new CertificateSerialNumber(sn));
        info.set(X509CertInfo.SUBJECT, new CertificateSubjectName(new X500Name(dn)));
        info.set(X509CertInfo.ISSUER, new CertificateIssuerName(new X500Name(caName)));
        info.set(X509CertInfo.KEY, new CertificateX509Key(pubKey));
        info.set(X509CertInfo.VERSION, new CertificateVersion(CertificateVersion.V3));
        AlgorithmId algo = new AlgorithmId(AlgorithmId.sha1WithRSAEncryption_oid);
        info.set(X509CertInfo.ALGORITHM_ID, new CertificateAlgorithmId(algo));

//        // Sign the cert to identify the algorithm that's used.
//        X509CertImpl cert = new X509CertImpl(info);
//        cert.sign(privkey, algorithm);

//        // Update the algorith, and resign.
//        algo = (AlgorithmId)cert.get(X509CertImpl.SIG_ALG);
//        info.set(CertificateAlgorithmId.NAME + "." + CertificateAlgorithmId.ALGORITHM, algo);
        X509CertImpl cert = new X509CertImpl(info);
        cert.sign(caPrivateKey, "SHA1withRSA");
        return cert;
    }   
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        try
        {
            KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");

            keyGen.initialize(2048);

            KeyPair keyPair = keyGen.generateKeyPair();
            PublicKey publicKey = keyPair.getPublic();
            PrivateKey privateKey = keyPair.getPrivate();
            // output the key:

            // we have a key pair and id, create a certificate from it
            String issuer = String.format("CN=Registration Server, OU=User Registration, O=IDEA Games, C=CZ");
            CertificateFactory cf = CertificateFactory.getInstance("X.509");
            X509Certificate cert = generateCertificate(issuer,issuer,publicKey,privateKey);

            // load and verify the certificate

            // verify the certificate
            try
            {
                cert.verify(publicKey);
            }
            catch (Exception e)
            {
                throw new RuntimeException(e);
            }

            System.out.format("String serverCertBase64=\n%s\"\";\n",splitLines(DatatypeConverter.printBase64Binary(cert.getEncoded())));
            System.out.format("String serverPrivateKeyBase64=\n%s\"\";\n",splitLines(DatatypeConverter.printBase64Binary(privateKey.getEncoded())));
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
        }
    }
}
