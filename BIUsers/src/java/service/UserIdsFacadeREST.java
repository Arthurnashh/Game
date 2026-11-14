/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package service;

import biusers.Registrations;
import biusers.UserIds;
import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.cert.*;
import java.security.cert.Certificate;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPrivateKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.ws.rs.*;
import javax.xml.bind.DatatypeConverter;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import sun.security.x509.*;

/**
 *
 * @author ondra
 */
@Stateless
@Path("biusers.userids")
public class UserIdsFacadeREST extends AbstractFacade<UserIds> {
    @PersistenceContext(unitName = "BIUsersPU")
    private EntityManager em;

    public UserIdsFacadeREST() {
        super(UserIds.class);
    }

    @POST
    @Override
    @Consumes({"application/xml", "application/json"})
    public void create(UserIds entity) {
        super.create(entity);
    }

    @PUT
    @Override
    @Consumes({"application/xml", "application/json"})
    public void edit(UserIds entity) {
        super.edit(entity);
    }

    @DELETE
    @Path("{id}")
    public void remove(@PathParam("id") Integer id) {
        super.remove(super.find(id));
    }

    @GET
    @Path("{id}")
    @Produces({"application/xml", "application/json"})
    public UserIds find(@PathParam("id") Integer id) {
        return super.find(id);
    }

    @GET
    @Override
    @Produces({"application/xml", "application/json"})
    public List<UserIds> findAll() {
        return super.findAll();
    }

    @GET
    @Path("{from}/{to}")
    @Produces({"application/xml", "application/json"})
    public List<UserIds> findRange(@PathParam("from") Integer from, @PathParam("to") Integer to) {
        return super.findRange(new int[]{from, to});
    }

    @GET
    @Path("count")
    @Produces("text/plain")
    public String countREST() {
        return String.valueOf(super.count());
    }

    @POST
    @Path("register-start")
    @Consumes({"application/json"})
    @Produces("application/json")
    public String registerStart(String content) {
        // generate a challenge
        try
        {
            Registrations registration = new Registrations();
            Random random = new Random();
            registration.setChallenge(random.nextInt());
            getEntityManager().persist(registration);
            getEntityManager().flush();
            JSONObject ret = new JSONObject();
            try {
                ret.put("id",registration.getId());
                ret.put("challenge",registration.getChallenge());
            } catch (JSONException ex) {
                Logger.getLogger(UserIdsFacadeREST.class.getName()).log(Level.SEVERE, null, ex);
            }
            return ret.toString();        
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return "";
    }
    
    @POST
    @Path("register-new-id/{regId}")
    @Consumes({"application/json"})
    @Produces("application/json")
    public String registerNewId(String content, @PathParam("regId") Integer regId)
    {
        // verify a request is legitimate

        Registrations registration = getEntityManager().find(Registrations.class,regId);
        
        if (registration==null)
        {
            return ""; // TODO: return error code
        }
        
        
        JSONObject ret = new JSONObject();
        try
        {
            Integer challenge = registration.getChallenge();
            
            // completing registration, record can be deleted now
            getEntityManager().remove(registration);
            
            JSONObject input = new JSONObject(content);
            String challengeSignature = input.getString("signature");
            String publicKey = input.getString("publicKey");
                    
            byte[] publicKeyBytes = DatatypeConverter.parseBase64Binary(publicKey);
            byte[] challengeSignatureBytes = DatatypeConverter.parseBase64Binary(challengeSignature);

            // verify the signature was done using the public key
            Signature rsa;
            rsa = Signature.getInstance("SHA256withRSA");

            X509EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(publicKeyBytes);
            KeyFactory keyFactory = KeyFactory.getInstance("RSA");

            PublicKey pubKey = keyFactory.generatePublic(pubKeySpec);            
            
            rsa.initVerify(pubKey);

            rsa.update(String.valueOf(challenge).getBytes());
            if (!rsa.verify(challengeSignatureBytes))
            {
                return ""; // TODO: return error code
            }
            
            UserIds newUser = new UserIds();
            super.create(newUser);

            getEntityManager().flush();

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
String serverPrivateKeyBase64=
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCKysMOe6E9MZk7"+
"WZbzPrA92f6oG3I+vR1g4ozxGR/gEIFJ9KZuBRBNTZDitfJd6pl67w6Oh18A8nQG"+
"biMAipEHU5SzQGrehyIurE4sNJ6O65tKjUKgdZKyFEDLEu0/Wam1c/nmDJPM7vi6"+
"QYZG9Cbt8WJEaohnOetLYRVTlNljYs0TMOVEFmqvnorjpFFW0KnFDJgG7j7K3JQ1"+
"enB4qy2PTxHJbDnZaiXphvcVlKvHnsNMIXto/IhYxP9IZaghCPOqskl2gOn0NIZ/"+
"NlXBRwBWgXkFVPDgwb1Fy5bOafpypikm7BRKxV67RDJFsXs2BSZsaCxClRl9j906"+
"hl3xLWVlAgMBAAECggEASYbXRPHr3n5LHiz9EgoQ1gOJrEc2A9SuHqiGYCeeQlac"+
"F7DbBof6Nem0qtYW+vgQrRa+EHXYt6i6HLn2IxBFeqslqLykoRTX3//6PeL2aOKa"+
"Vyl0tYrz1xFxreNt+lSm2nEBX4UkO7oR3KJf33DyurVZCMOjVYRSiZF5+dhuPf7M"+
"vkLUdjmrvT2vLZeJ3MZke4nAn7c3kEpnTpPdxD+UeOSroDv0o/dh0vE2jCXF+rgJ"+
"b143jnyIC5lXrhMEblREnnSgXWjorB7HSqWxXk5cb+eohxaZ2cZtM6EKe0Mkr/QD"+
"tIJ90g0xtBETensHZeOjXDwnYFtxS+QoEXIIMYgr1QKBgQDZhhicXO1PDysyCUvY"+
"3HxM5p9TFiNcAVIEyNYWdlQXQOVQpBUcPW7P5mhzmSWx2zEcVTyCK+/bFYDNxxTR"+
"6Mm6P/ioACobLOTvyvF+s8UmMEXO0FcvPN4YBiMgwZ6OwBoL6Lj53DetGwvTKmEr"+
"v3/hrm3zz9HCY1nbzbg+PyhHkwKBgQCjV4l6KE3ow96itCVJtJ10R7nP70g5xqED"+
"9mKA/AppjovvIo5jYpnlnaM9dv1QJTvIwQ5Jq649nMy7xuqT3PkLOVua+SwT6YF7"+
"ZaNgGKpR2TlD6qDyYi6cpLW5p7yE7cX5+qZRohRMceqVqLQzK+h0bWdQZqZV5AMz"+
"2u9OoX5KJwKBgQDEPDApp7/CiemrCy3spwRQD+H9/8PIdbx7an+Q5hD4pWf6tvwA"+
"bP9x+OeUGb4i1J5adB7RRDfvSlfXzFjhAYQzTRDS5ssVlwu1tDzm0+Dwka34ZIWy"+
"rNOjkq3QGDwlhfI5eiqd5mgn93VUfDALWohQnZ69zGbsWdqv68fiB/IblwKBgBl0"+
"Wcs8nq+IXpiSZEExMVxelRlPW70NNChu6/jXzjOW8D2q7++S/RKoVzkL4vvrYX6u"+
"fzCB3ZxiZBFbNVEnnXPfU2Q6MmFEAVaehzBBh8FLjb4EwWIkZMkqbdtpxVodx71m"+
"Dycnj+w90GXULYjP7eC+79u6s28tIkahdquVRR3/AoGAQEXZAuOjaxgbvb3VoXfb"+
"gaO/9r10+waUAMZooMvmXHDwHg7TaN8Bs5bTexcVNt+eraXFhg7ZWvMxxKKonCQa"+
"uePajNBzOHs3wYa2uYTMu7C92glZZR6fQfhkywRV4FTxD47gGylYTE5zNctMnfI4"+
"3+ULNYbf0nvw+YIydBMK0/Y="+
"";

            try
            {
                CertificateFactory cf = CertificateFactory.getInstance("X.509");
                Certificate caCert = cf.generateCertificate(new ByteArrayInputStream(DatatypeConverter.parseBase64Binary(serverCertBase64)));
                PublicKey caPublicKey = caCert.getPublicKey();
                
                PKCS8EncodedKeySpec privateKeySpec = new PKCS8EncodedKeySpec(DatatypeConverter.parseBase64Binary(serverPrivateKeyBase64));

                PrivateKey caPrivateKey = keyFactory.generatePrivate(privateKeySpec);
                
                // we have a key pair and id, create a certificate from it
                String userName = String.format("CN=User%d, OU=User, O=IDEA Games, C=CZ",newUser.getId()); // RFC 1779 name based on user ID
                String issuer = String.format("CN=Registration Server, OU=User Registration, O=IDEA Games, C=CZ");
                X509Certificate cert = generateCertificate(userName,issuer,pubKey,caPrivateKey);

                // load and verify the certificate
                byte[] certBytes = cert.getEncoded();

                Certificate verify = cf.generateCertificate(new ByteArrayInputStream(certBytes));

                // verify the certificate
                try
                {
                    verify.verify(caPublicKey);
                }
                catch (Exception e)
                {
                    super.remove(newUser);
                    throw new RuntimeException(e);
                }

                newUser.setCertificate(certBytes);
                ret.put("id",newUser.getId());
                ret.append("certificate",DatatypeConverter.printBase64Binary(certBytes));
            }
            catch (Exception ex)
            {
                // if anything fails, do not store the user
                super.remove(newUser);
                throw ex;
            }
            getEntityManager().flush();
        }
        catch(JSONException ex) {throw new RuntimeException(ex);} // JSON.put - no error possible
        catch (NoSuchAlgorithmException ex) {throw new RuntimeException(ex);} // No RSA
        /*
        catch (InvalidKeyException ex) {  }// signature key wrong
        catch (InvalidKeySpecException ex) {  }// keyFactory.generatePublic
        catch (SignatureException ex) { } // signature not valid
        */
        catch (Exception ex)
        {
            ex.printStackTrace();
            return "";
        }
                    
        // ID public key signature by registration private key
        return ret.toString();
    }
    
    @java.lang.Override
    protected EntityManager getEntityManager() {
        return em;
    }

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
}
