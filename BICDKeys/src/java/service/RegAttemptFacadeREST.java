/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package service;

import bicdkeys.RegAttempt;
import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;

/**
 *
 * @author Ondra
 */
@Stateless
@Path("bicdkeys.regattempt")
public class RegAttemptFacadeREST extends AbstractFacade<RegAttempt> {
    @PersistenceContext(unitName = "BICDKeysPU")
    private EntityManager em;

    public RegAttemptFacadeREST() {
        super(RegAttempt.class);
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
