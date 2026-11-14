/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package bicdkeys;

import java.io.Serializable;
import java.util.List;
import javax.persistence.*;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlTransient;

/**
 *
 * @author ondra
 */
@Entity
@Table(name = "CDKeys")
@XmlRootElement
@NamedQueries({
    @NamedQuery(name = "CDKeys.findAll", query = "SELECT c FROM CDKeys c"),
    @NamedQuery(name = "CDKeys.findByCdKey", query = "SELECT c FROM CDKeys c WHERE c.cdKey = :cdKey"),
    @NamedQuery(name = "CDKeys.findByPlayerId", query = "SELECT c FROM CDKeys c WHERE c.playerId = :playerId"),
    @NamedQuery(name = "CDKeys.findByProductId", query = "SELECT c FROM CDKeys c WHERE c.productId = :productId"),
    @NamedQuery(name = "CDKeys.findByRegistrations", query = "SELECT DISTINCT c FROM CDKeys c WHERE c.productId=:productId AND SIZE(c.regAttemptCollection)>=:count")})


public class CDKeys implements Serializable {
    private static final long serialVersionUID = 1L;
    @Size(max = 255)
    @Column(name = "cdKey")
    private String cdKey;
    @Id
    @Basic(optional = false)
    @NotNull
    @Size(min = 1, max = 30)
    @Column(name = "playerId")
    private String playerId;
    @Size(max = 10)
    @Column(name = "productId")
    private String productId;
    @OneToMany(mappedBy = "playerId")
    private List<RegAttempt> regAttemptCollection;

    public CDKeys() {
    }

    public CDKeys(String playerId) {
        this.playerId = playerId;
    }

    public String getCdKey() {
        return cdKey;
    }

    public void setCdKey(String cdKey) {
        this.cdKey = cdKey;
    }

    public String getPlayerId() {
        return playerId;
    }

    public void setPlayerId(String playerId) {
        this.playerId = playerId;
    }

    public String getProductId() {
        return productId;
    }

    public void setProductId(String productId) {
        this.productId = productId;
    }

    @XmlTransient
    public List<RegAttempt> getRegAttempts() {
        return regAttemptCollection;
    }

    public void setRegAttempts(List<RegAttempt> regAttemptCollection) {
        this.regAttemptCollection = regAttemptCollection;
    }

    @Override
    public int hashCode() {
        int hash = 0;
        hash += (playerId != null ? playerId.hashCode() : 0);
        return hash;
    }

    @Override
    public boolean equals(Object object) {
        // TODO: Warning - this method won't work in the case the id fields are not set
        if (!(object instanceof CDKeys)) {
            return false;
        }
        CDKeys other = (CDKeys) object;
        if ((this.playerId == null && other.playerId != null) || (this.playerId != null && !this.playerId.equals(other.playerId))) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "bicdkeys.CDKeys[ playerId=" + playerId + " ]";
    }
    
}
