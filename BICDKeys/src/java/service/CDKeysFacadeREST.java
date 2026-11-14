/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package service;

import bicdkeys.CDKey;
import bicdkeys.CDKeys;
import bicdkeys.ProductKeys;
import bicdkeys.RegAttempt;
import bicdkeys.ServerKeys;
import com.sun.jersey.multipart.FormDataParam;
import java.io.*;
import java.math.BigInteger;
import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.*;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.text.DateFormat;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;
import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.*;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.xml.bind.DatatypeConverter;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import sun.security.x509.*;

/**
 *
 * @author Ondra
 */
@Stateless
@Path("bicdkeys.cdkeys")
public class CDKeysFacadeREST extends AbstractFacade<CDKeys> {
    @PersistenceContext(unitName = "BICDKeysPU")
    private EntityManager em;

    public CDKeysFacadeREST() {
        super(CDKeys.class);
    }

    @GET
    @Path("count")
    @Produces("text/plain")
    public String countREST() {
        return String.valueOf(super.count());
    }

    Integer productCodeFromProductId(String productId)
    {
            Integer productCodeFromId = null;
            Query byRegs = em.createNamedQuery("ProductKeys.findByProductId", CDKeys.class );
            byRegs.setParameter("productId",productId);
            List<ProductKeys> resultList = byRegs.getResultList();
            if (resultList.size()>0)
            {
                ProductKeys product = resultList.get(0);
                productCodeFromId = product.getProductCode();
                // make sure the keys exist for the product
                ProductKeysFacadeREST.CreateKeys(em, productId,false);

            }
            return productCodeFromId;
    }

    String productIdFromProductCode(int productCode)
    {
            String productIdFromCode = null;
            Query byRegs = em.createNamedQuery("ProductKeys.findByProductNumId", CDKeys.class );
            byRegs.setParameter("productId",productCode);
            List<ProductKeys> resultList = byRegs.getResultList();
            if (resultList.size()>0)
            {
                ProductKeys product = resultList.get(0);
                productIdFromCode = product.getProductId();
                // make sure the keys exist for the product
                ProductKeysFacadeREST.CreateKeys(em, productIdFromCode,false);

            }
            return productIdFromCode;
    }
    
    private class ProductHint
    {
        public Integer code;
        public String id;
    }
    private class NewKeyReturn
    {
        public String error;
        public boolean existed;
        public String playerId;
        public String productId;
        
        public NewKeyReturn(String error)
        {
            this.error = error;
        }
        public NewKeyReturn(boolean existed, String playerId, String productId)
        {
            this.existed = existed;
            this.playerId = playerId;
            this.productId = productId;
        }
    }
    private NewKeyReturn newKeyDo(String cdKeyText, ProductHint hint)
    {
        CDKey cdKey = new CDKey(cdKeyText);
        if (cdKey==null || cdKey._message==null)
        {
            return new NewKeyReturn("Error decoding CD Key");
        }
        String playerId = cdKey.getPlayerId();
        int productNum = cdKey.getProductId();

        
        String productId;
        if (hint !=null && hint.code!=null && productNum == hint.code)
        {
            productId = hint.id;
        }
        else
        {
            productId = productIdFromProductCode(productNum);
            if (productId!=null && hint!=null)
            {
                hint.id = productId;
                hint.code = productNum;
            }
        }

        if (productId==null)
        {
            return new NewKeyReturn(String.format("Unknown product code %d'", productNum));
        }
        // product code verified, proceed with a key registration
        // first check if the key already exists
        // Hex encode binary representation of the key (as in the registry)
        byte[] cdKeyBytes = cdKey.getBytes();
        String cdKeyHex = DatatypeConverter.printHexBinary(cdKeyBytes);

        Query byRegs = em.createNamedQuery("CDKeys.findByCdKey", CDKeys.class );
        byRegs.setParameter("cdKey",cdKeyHex);
        List<CDKeys> found = byRegs.getResultList();
        boolean existed = true;
        if (found.isEmpty())
        {
            CDKeys newCDKey = new CDKeys(playerId);
            newCDKey.setCdKey(cdKeyHex);
            newCDKey.setProductId(productId);
            em.persist(newCDKey);
            em.flush();
            existed = false;
        }

        return new NewKeyReturn(existed,playerId,productId);
    }       
    
    @GET
    @Path("new")
    @Produces("application/json")
    public Response newKey(@QueryParam("key") String cdKeyText)
    {
        NewKeyReturn error = newKeyDo(cdKeyText, null);
        if (error.error==null)
        {
            try {
                JSONObject ret = new JSONObject();
                ret.put("id",error.playerId+error.productId);
                return Response.ok(ret.toString(), MediaType.TEXT_PLAIN_TYPE).build();
            } catch (JSONException ex) {
                Logger.getLogger(CDKeysFacadeREST.class.getName()).log(Level.SEVERE, null, ex);
                return BuildOKResponse("Internal error");
            }
        }
        else
        {
            return BuildOKResponse(error.error);
        }
    }       

    @POST
    @Path("newKeys")
    @Consumes({"multipart/form-data"})
    @Produces("application/json")
    public Response newKeys(@FormDataParam("cdKeysFile") InputStream cdKeysFile)
    {
        JSONObject ret = new JSONObject();
        try {

            // parse cdKeysFile line by line
            try
            {
                // TODO: exception safety?
                ProductHint hint = new ProductHint();
                InputStreamReader is = new InputStreamReader(cdKeysFile, "utf-8");
                BufferedReader br = new BufferedReader(is);
                String cdKeyText;
                while ((cdKeyText = br.readLine()) != null) {
                    if (cdKeyText.isEmpty()) continue;
                    NewKeyReturn error = newKeyDo(cdKeyText,hint);
                    if (error.error!=null)
                    {
                        JSONArray err = ret.optJSONArray(error.error);
                        if (err==null)
                        {
                            err = new JSONArray();
                            ret.put(error.error,err);
                        }
                        err.put(cdKeyText);
                    }
                    else
                    {
                        JSONObject prodStats = ret.optJSONObject(error.productId);
                        if (prodStats==null)
                        {
                            prodStats = new JSONObject();
                            ret.put(error.productId,prodStats);
                        }
                        String name = error.existed ? "existing keys" : "new keys";
                        
                        prodStats.put(name,prodStats.optInt(name)+1);
                    }
                }
            }
            finally
            {
            }
            
            return Response.ok(ret.toString(0), MediaType.APPLICATION_JSON).build();
        } catch (JSONException ex) {
            Logger.getLogger(CDKeysFacadeREST.class.getName()).log(Level.SEVERE, null, ex);
        }
        catch (IOException ex)
        {
            Logger.getLogger(CDKeysFacadeREST.class.getName()).log(Level.SEVERE, null, ex);
        }
        return BuildOKResponse("Internal error");
    }
    
    @POST
    @Path("register/{productId}/{playerId}")
    @Consumes({"application/json"})
    @Produces("application/json")
    public Response register(@Context HttpServletRequest requestContext, String content, @PathParam("productId") String productId, @PathParam("playerId") String playerId)
    {
        // verify a request is legitimate

        JSONObject ret = new JSONObject();
        try
        {
            JSONObject input = new JSONObject(content);
            String inputCDKey = input.getString("cdKey");
            String publicKey = input.getString("publicKey");
            String cdKeySignature = input.getString("cdKeySignature");

            byte[] cdKey = DatatypeConverter.parseBase64Binary(inputCDKey);
            byte[] publicKeyBytes = DatatypeConverter.parseBase64Binary(publicKey);
            byte[] cdKeySignatureBytes = DatatypeConverter.parseBase64Binary(cdKeySignature);

            // verify the signature was done using the public key
            Signature rsa;
            rsa = Signature.getInstance("SHA256withRSA");

            X509EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(publicKeyBytes);
            KeyFactory keyFactory = KeyFactory.getInstance("RSA");

            PublicKey pubKey = keyFactory.generatePublic(pubKeySpec);

            rsa.initVerify(pubKey);

            rsa.update(cdKey);
            if (!rsa.verify(cdKeySignatureBytes)) {
                return BuildOKResponse("Signature verification failed");
            }

            // cd key needs to already exist in the database
            // if it does not, the registration will fail
            CDKeys user = super.find(playerId);
            if (user == null) {
                return BuildOKResponse("Invalid CD Key");
            }
            // database stores as HEX
            byte[] cdKeyUser = DatatypeConverter.parseHexBinary(user.getCdKey());
            if (!Arrays.equals(cdKeyUser, cdKey)) {
                return BuildOKResponse("CD Key not matching playerId");
            }

            boolean allow = allowRegistration(user);
            List<RegAttempt> history = user.getRegAttempts();
            RegAttempt regAttempt = new RegAttempt();
            regAttempt.setIpAddress(requestContext.getRemoteAddr());
            regAttempt.setDate(Calendar.getInstance().getTime());
            regAttempt.setPlayerId(user);
            history.add(regAttempt);
            getEntityManager().persist(regAttempt);
            getEntityManager().flush();

            ServerKeys keys = ObtainKeys(productId,false);

            try {
                CertificateFactory cf = CertificateFactory.getInstance("X.509");
                PublicKey caPublicKey = keys.getCert().getPublicKey();
                PrivateKey caPrivateKey = keys.getPrivateKey();

                // we have a key pair and id, create a certificate from it
                String userGroup = allow ? "User" : "Blocked";
                String userName = String.format("CN=%s%s, OU=User, O=IDEA Games, C=CZ", userGroup, user.getPlayerId()); // RFC 1779 name based on user ID
                String issuer = String.format("CN=Registration Server, OU=User Registration, O=IDEA Games, C=CZ");
                X509Certificate cert = generateCertificate(userName, issuer, pubKey, caPrivateKey);

                // load and verify the certificate
                byte[] certBytes = cert.getEncoded();

                Certificate verify = cf.generateCertificate(new ByteArrayInputStream(certBytes));

                // verify the certificate
                try {
                    verify.verify(caPublicKey);
                } catch (Exception e) {
                    throw new RuntimeException(e); // unexpected: certificate not verified
                }

                ret.put("certificate", DatatypeConverter.printBase64Binary(certBytes));

                List<CDKeys> blockedKeys = getBlockedKeys(productId);
                ret.put("blocked",SignedKeys(caPrivateKey,blockedKeys));

            } catch (Exception ex) {
                // if anything fails, do not store the user
                throw ex;
            }

            getEntityManager().flush();
        } catch (JSONException ex) {
            throw new RuntimeException(ex);
        } // JSON.put - no error possible
        catch (NoSuchAlgorithmException ex) {
            throw new RuntimeException(ex);
        } // No RSA
        catch (InvalidKeyException ex) { ex.printStackTrace();return BuildOKResponse("Invalid key");}// signature key wrong catch
        catch (InvalidKeySpecException ex) {ex.printStackTrace();return BuildOKResponse("Invalid key spec"); }// keyFactory.generatePublic
        catch (SignatureException ex) { ex.printStackTrace();return BuildOKResponse("Signature not valid");} // signature not valid
        catch (Exception ex) {
            ex.printStackTrace();
            return BuildOKResponse(String.format("Unhandled exception %s", ex.getLocalizedMessage()));
        }
        return Response.ok(ret.toString(), MediaType.APPLICATION_JSON).build();
    }

    private ServerKeys ObtainKeys(String productId, boolean canCreateProduct)
    {
        ProductKeys keys = ProductKeysFacadeREST.CreateKeys(em, productId,canCreateProduct);
        if (keys==null || keys.getPrivateKey()==null || keys.getPublicKey()==null) return null;
        
        try {
            CertificateFactory cf;
            cf = CertificateFactory.getInstance("X.509");
            Certificate caCert = cf.generateCertificate(new ByteArrayInputStream(keys.getPublicKey()));

            PKCS8EncodedKeySpec privateKeySpec = new PKCS8EncodedKeySpec(keys.getPrivateKey());

            KeyFactory keyFactory = KeyFactory.getInstance("RSA");
            PrivateKey caPrivateKey = keyFactory.generatePrivate(privateKeySpec);

            ServerKeys serverKeys = new ServerKeys();
            serverKeys.setPrivateKey(caPrivateKey);
            serverKeys.setCert(caCert);

            return serverKeys;
        } catch (InvalidKeySpecException ex) {
            throw new RuntimeException(ex);
        } catch (NoSuchAlgorithmException ex) {
            throw new RuntimeException(ex);
        } catch (CertificateException ex) {
            throw new RuntimeException(ex);
        }
    }
    
    @POST
    @Path("register/{playerId}")
    @Consumes({"application/json"})
    @Produces("application/json")
    public Response registerOA(@Context HttpServletRequest requestContext, String content, @PathParam("productId") String productId, @PathParam("playerId") String playerId)
    {
        return register(requestContext,content,"",playerId);
    }
    static private Response BuildResponse(Response.Status status)
    {
        return Response.status(status).build();
    }
    static private Response BuildOKResponse(String message)
    {
        JSONObject ret = new JSONObject();
        try {
            ret.put("error", message);
        } catch (JSONException ex) {
            Logger.getLogger(CDKeysFacadeREST.class.getName()).log(Level.SEVERE, null, ex);
            return null;
        }
        return Response.ok(ret.toString(),MediaType.APPLICATION_JSON).build();
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

    class RegAttemptComparator implements Comparator<RegAttempt> {
        @Override
        public int compare(RegAttempt e1, RegAttempt e2) {
            return e1.getDate().compareTo(e2.getDate());
        }

    }

    static private class AgeLimit
    {
        long age;
        int maxCount;
        
        AgeLimit(long age, int maxCount)
        {
           this.age=age;this.maxCount=maxCount;
        }
    };
    
    private static List<AgeLimit> ageLimits;
    
    static final int perDay = 10;
    static final int perWeek = 20;
    static final int perMonth = 30;
    static final int perYear = 100;

    private static List<AgeLimit> BuildAgeLimits()
    {
        List<AgeLimit> limits = new ArrayList<AgeLimit>();
        Collections.addAll(
            limits,
            new AgeLimit(1,perDay),
            new AgeLimit(7,perWeek),
            new AgeLimit(30,perMonth),
            new AgeLimit(365,perYear)
        );
        return limits;
        
    }
    static {
        ageLimits = BuildAgeLimits();
    };
    private boolean allowRegistration(CDKeys user)
    {
        
        List<RegAttempt> history = user.getRegAttempts();
        // when we are under the strictest limit, no processing is required
        if (history.size()<perDay) return true;

        int[] byAge = new int[ageLimits.size()+1];
                
        long now = System.currentTimeMillis();
        boolean ret = true;
        RegAttempt oldest = null;
        long oldestAge = 0;
        for (RegAttempt reg: history)
        {
            long age = now-reg.getDate().getTime();
            long ageInDays = age/(1000*60*60*24);
            int category = ageLimits.size();
            for (int c=0; c<ageLimits.size(); c++)
            {
                AgeLimit limit = ageLimits.get(c);
                if (ageInDays<limit.age) {category=c;break;}
            }
            if (category>=ageLimits.size())
            {
                // we do not care about very old entries, we expire them instead
                em.remove(reg);
                history.remove(reg);
            }
            else if (age>=oldestAge)
            {
                oldestAge = age;
                oldest = reg;
            }
            byAge[category]++;
            if (category<ageLimits.size() && byAge[category]>ageLimits.get(category).maxCount)
            {
                ret = false;
            }
        }
        if (oldest!=null && history.size()>ageLimits.get(ageLimits.size()-1).maxCount)
        {
            // it has no sense to keep more then maximum count for the longest period
            em.remove(oldest);
            history.remove(oldest);
        }
        em.flush();
        return ret;
    }

    
    public List<CDKeys> getRegisteredKeys(String productId, int minCount)
    {
//        Query byRegs = em.createNativeQuery(String.format("SELECT * FROM CDKeysJOIN (SELECT playerId, COUNT(*) AS Num FROM regAttempt GROUP BY playerId) AS regs WHERE CDKeys.playerId = regs.playerId AND Num > %d", minCount));
//        return byRegs.getResultList().size;
        /*
        String queryString =
                "SELECT COUNT(*) FROM CDKeys "+
                "LEFT JOIN (SELECT playerId, COUNT(*) AS Num FROM regAttempt GROUP BY playerId)  AS regs "+
                "ON CDKeys.playerId = regs.playerId "+
                "WHERE CDKeys.productId=\"%s\" AND COALESCE(Num, 0)>=%d";
        Query byRegs = em.createNativeQuery(String.format(queryString, productId, minCount));
        Object ret = byRegs.getSingleResult();
        return DatatypeConverter.parseInt(ret.toString());
        */

        Query byRegs = em.createNamedQuery("CDKeys.findByRegistrations", CDKeys.class );
        byRegs.setParameter("count",minCount);
        byRegs.setParameter("productId",productId);
        return byRegs.getResultList();
    }

    public List<CDKeys> getBlockedKeys(String productId)
    {
        Query byRegs = em.createNamedQuery("CDKeys.findByRegistrations",CDKeys.class);
        byRegs.setParameter("count",perDay);
        byRegs.setParameter("productId",productId);
        List<CDKeys> keys = byRegs.getResultList();
        List<CDKeys> blocked = new ArrayList<CDKeys>();
        for (CDKeys key:keys)
        {
            boolean allow = allowRegistration(key);
            if (!allow) blocked.add(key);
        }
        return blocked;
    }

    private JSONArray OutputKeys(List<CDKeys> blockedKeys)
    {
        JSONArray arr = new JSONArray();
        for (CDKeys key:blockedKeys) arr.put(key.getPlayerId());
        return arr;
    }
    
    private String SignKeys(PrivateKey privateKey, List<CDKeys> blockedKeys)
    {
        try {
            Signature rsa;
            rsa = Signature.getInstance("SHA256withRSA");
            rsa.initSign(privateKey);
            
            for (CDKeys key:blockedKeys)
            {
                rsa.update(key.getPlayerId().getBytes());
                rsa.update("\000".getBytes()); // separate IDs
            }
            byte[] signature = rsa.sign();
            return DatatypeConverter.printBase64Binary(signature);
        } catch (SignatureException ex) {
            throw new RuntimeException(ex);
        } catch (InvalidKeyException ex) {
            throw new RuntimeException(ex);
        } catch (NoSuchAlgorithmException ex) {
            throw new RuntimeException(ex);
        }
    }
    
    private JSONObject SignedKeys(PrivateKey privateKey, List<CDKeys> blockedKeys) throws JSONException
    {
        JSONObject ret = new JSONObject();
        ret.put("keys",OutputKeys(blockedKeys));
        ret.put("signature",SignKeys(privateKey,blockedKeys));
        return ret;
    }
        
    
    @GET
    @Path("disabledKeys")
    @Produces("application/json")
    public String disabledKeys(@DefaultValue("") @QueryParam("product") String productId)
    {
        try {
            ServerKeys keys = ObtainKeys(productId,false);
            PrivateKey caPrivateKey = keys.getPrivateKey();
            JSONObject ret = new JSONObject();
            if (caPrivateKey!=null)
            {
                List<CDKeys> blockedKeys = getBlockedKeys(productId);
                ret.put("blocked",SignedKeys(caPrivateKey,blockedKeys));
            }
            return ret.toString();
        } catch (JSONException ex) {
            return null;
        }
    }

    @GET
    @Path("stats")
    @Produces("application/json")
    public String stats(@DefaultValue("") @QueryParam("product") String productId)
    {
        try {
            JSONObject ret = new JSONObject();
            List<CDKeys> allKeys = getRegisteredKeys(productId,0);
            List<CDKeys> registeredKeys = getRegisteredKeys(productId,1);
            List<CDKeys> registeredKeys5x = getRegisteredKeys(productId,5);
            List<CDKeys> registeredKeys10x = getRegisteredKeys(productId,10);
            List<CDKeys> registeredKeys100x = getRegisteredKeys(productId,100);
            List<CDKeys> blockedKeys = getBlockedKeys(productId);
            ret.put("# free",allKeys.size()-registeredKeys.size());
            ret.put("# registered",registeredKeys.size());
            ret.put("# registered>=5x",registeredKeys5x.size());
            ret.put("# registered>=10x",registeredKeys10x.size());
            //ret.put("registered>=10x",OutputKeys(registeredKeys10x));
            ret.put("# registered>=100x",registeredKeys100x.size());
            ret.put("registered>=100x",OutputKeys(registeredKeys100x));
            ret.put("# blocked",blockedKeys.size());
            ret.put("blocked",OutputKeys(blockedKeys));
            return ret.toString(4);
        } catch (JSONException ex) {
            return null;
        }
    }

    private void listHistory(JSONObject ret, CDKeys key) throws JSONException
    {
        List<RegAttempt> history = key.getRegAttempts();
        if (!history.isEmpty())
        {
            JSONArray regDates = new JSONArray();
            DateFormat df = DateFormat.getDateInstance();
            for (RegAttempt r:history)
            {
                String dateStr = df.format(r.getDate());
                String ipStr = r.getIpAddress();
                //JSONArray item = new JSONArray();
                //item.put();
                //item.put(r.getIpAddress());
                //regDates.put(item);
                regDates.put(dateStr+" - "+ipStr);
            }
            ret.put("registrations",regDates);
        }
    }

    @GET
    @Path("history")
    @Produces("application/json")
    public Response history(@DefaultValue("") @QueryParam("key") String cdKeyText)
    {
        try {
            JSONObject ret = new JSONObject();

            CDKey cdKey = new CDKey(cdKeyText);
            if (cdKey==null || cdKey._message==null)
            {
                return BuildOKResponse("Error decoding CD Key");
            }

            byte[] cdKeyBytes = cdKey.getBytes();
            String cdKeyHex = DatatypeConverter.printHexBinary(cdKeyBytes);

            Query byRegs = em.createNamedQuery("CDKeys.findByCdKey", CDKeys.class );
            byRegs.setParameter("cdKey",cdKeyHex);
            List<CDKeys> found = byRegs.getResultList();
            if (found.isEmpty())
            {
                return BuildOKResponse("Key not found");
            }
            else
            {
                // list registration history
                CDKeys key = found.get(0);
                listHistory(ret,key);
            }
            return Response.ok(ret.toString(0), MediaType.APPLICATION_JSON).build();
        } catch (JSONException ex) {
            return null;
        }
    }

    @GET
    @Path("reset")
    @Produces("application/json")
    public Response reset(@DefaultValue("") @QueryParam("key") String cdKeyText)
    {
        try {
            JSONObject ret = new JSONObject();

            CDKey cdKey = new CDKey(cdKeyText);
            if (cdKey==null || cdKey._message==null)
            {
                return BuildOKResponse("Error decoding CD Key");
            }

            byte[] cdKeyBytes = cdKey.getBytes();
            String cdKeyHex = DatatypeConverter.printHexBinary(cdKeyBytes);

            Query byRegs = em.createNamedQuery("CDKeys.findByCdKey", CDKeys.class );
            byRegs.setParameter("cdKey",cdKeyHex);
            List<CDKeys> found = byRegs.getResultList();
            if (found.isEmpty())
            {
                return BuildOKResponse("Key not found");
            }
            else
            {
                // list registration history
                CDKeys key = found.get(0);
                listHistory(ret,key);
                List<RegAttempt> history = key.getRegAttempts();
                for (RegAttempt r: history)
                {
                    em.remove(r);
                }
                key.getRegAttempts().clear();
                em.flush();
            }
            return Response.ok(ret.toString(0), MediaType.APPLICATION_JSON).build();
        } catch (JSONException ex) {
            return null;
        }
    }
}
