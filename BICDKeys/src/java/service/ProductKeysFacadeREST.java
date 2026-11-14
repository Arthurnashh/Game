/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package service;

import bicdkeys.ProductKeys;
import bicdkeys.ServerKeys;
import java.security.cert.CertificateException;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.ws.rs.*;
import javax.xml.bind.DatatypeConverter;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author ondra
 */
@Stateless
@Path("bicdkeys.productkeys")
public class ProductKeysFacadeREST extends AbstractFacade<ProductKeys> {
    @PersistenceContext(unitName = "BICDKeysPU")
    private EntityManager em;

    public ProductKeysFacadeREST() {
        super(ProductKeys.class);
    }

    static public ProductKeys CreateKeys(EntityManager em, String productId, boolean canCreateProduct)
    {
        ProductKeys keys = em.find(ProductKeys.class,productId);
        if (keys==null)
        {
            if (!canCreateProduct)
            {
                return null;
            }
            keys = new ProductKeys(productId);
            em.persist(keys);
        }
        {
            try {
                if (keys.getPrivateKey()==null || keys.getPublicKey()==null)
                {
                    ServerKeys serverKeys = ServerKeys.Generate(productId);
                    keys.setPrivateKey(serverKeys.getPrivateKey().getEncoded());
                    keys.setPublicKey(serverKeys.getCert().getEncoded());
                    em.flush();
                }
            } catch (CertificateException ex) {
                throw new RuntimeException(ex);
            }
            return keys;
        }
    }

    @GET
    @Path("key")
    @Produces({"application/json"})
    public String find(@DefaultValue("") @QueryParam("product") String id)
    {
        ProductKeys keys = super.find(id);
        if (keys==null)
        {
            JSONObject ret = new JSONObject();
            return ret.toString();
        }
        byte[] publicKey = keys.getPublicKey();
        if (publicKey==null)
        {
            // TODO: return specific HTTP error code
            JSONObject ret = new JSONObject();
            return ret.toString();
        }
        try {
            JSONObject ret = new JSONObject();
            ret.put("certificate", DatatypeConverter.printBase64Binary(publicKey));
            return ret.toString();
        } catch (JSONException ex) {
            return null; // TODO: return error code or error message
        }
    }

    @GET
    @Path("new")
    @Produces({"application/json"})
    public String newKey(@DefaultValue("") @QueryParam("product") String productId, @QueryParam("code") Integer productCode)
    {
        try {
            JSONObject ret = new JSONObject();
            ProductKeys keysDst = super.find(productId);
            if (keysDst==null)
            {
                keysDst = new ProductKeys(productId);
                em.persist(keysDst);
            }
            keysDst.setProductCode(productCode);
            
            CreateKeys(em, productId, true);
            
            ret.put("target", productId);
            return ret.toString();
        } catch (JSONException ex) {
            return null; // TODO: return error code or error message
        }
        
    }
    
    @GET
    @Path("copy")
    @Produces({"application/json"})
    public String copyKey(@DefaultValue("") @QueryParam("product") String id, @QueryParam("to") String idTarget, @QueryParam("toCode") Integer idTargetCode)
    {
        try {
            JSONObject ret = new JSONObject();
            ProductKeys keysSrc = super.find(id);
            if (keysSrc!=null)
            {
                ProductKeys keysDst = super.find(idTarget);
                if (keysDst==null)
                {
                    keysDst = new ProductKeys(idTarget);
                    em.persist(keysDst);
                }
                keysDst.setPublicKey(keysSrc.getPublicKey());
                keysDst.setPrivateKey(keysSrc.getPrivateKey());
                if (keysDst.getProductCode()==null)
                {
                    keysDst.setProductCode(idTargetCode);
                }
                em.flush();

            }
            ret.put("target", idTarget);
            return ret.toString();
        } catch (JSONException ex) {
            return null; // TODO: return error code or error message
        }
        
    }
    
    @GET
    @Path("count")
    @Produces("text/plain")
    public String countREST() {
        return String.valueOf(super.count());
    }

    @java.lang.Override
    protected EntityManager getEntityManager() {
        return em;
    }
    
}
