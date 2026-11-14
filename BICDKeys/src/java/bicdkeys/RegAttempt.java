/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package bicdkeys;

import java.io.Serializable;
import java.util.Date;
import javax.persistence.*;
import javax.xml.bind.annotation.XmlRootElement;

/**
 *
 * @author ondra
 */
@Entity
@Table(name = "regAttempt")
@XmlRootElement
@NamedQueries({
    @NamedQuery(name = "RegAttempt.findAll", query = "SELECT r FROM RegAttempt r"),
    @NamedQuery(name = "RegAttempt.findByDate", query = "SELECT r FROM RegAttempt r WHERE r.date = :date"),
    @NamedQuery(name = "RegAttempt.findByUniqueId", query = "SELECT r FROM RegAttempt r WHERE r.uniqueId = :uniqueId")})
public class RegAttempt implements Serializable {
    private static final long serialVersionUID = 1L;
    @Column(name = "date")
    @Temporal(TemporalType.TIMESTAMP)
    private Date date;

    @Column(name = "ipAddress")
    private String ipAddress;
    
    @Id
    @GeneratedValue(strategy = GenerationType.SEQUENCE)
    @Basic(optional = false)
    @Column(name = "uniqueId")
    private Integer uniqueId;
    @JoinColumn(name = "playerId", referencedColumnName = "playerId")
    @ManyToOne
    private CDKeys playerId;

    public RegAttempt() {
    }

    public RegAttempt(Integer uniqueId) {
        this.uniqueId = uniqueId;
    }

    public Date getDate() {
        return date;
    }

    public void setDate(Date date) {
        this.date = date;
    }

    public Integer getUniqueId() {
        return uniqueId;
    }

    public void setUniqueId(Integer uniqueId) {
        this.uniqueId = uniqueId;
    }

    public CDKeys getPlayerId() {
        return playerId;
    }

    public void setPlayerId(CDKeys playerId) {
        this.playerId = playerId;
    }

    @Override
    public int hashCode() {
        int hash = 0;
        hash += (uniqueId != null ? uniqueId.hashCode() : 0);
        return hash;
    }

    @Override
    public boolean equals(Object object) {
        // TODO: Warning - this method won't work in the case the id fields are not set
        if (!(object instanceof RegAttempt)) {
            return false;
        }
        RegAttempt other = (RegAttempt) object;
        if ((this.uniqueId == null && other.uniqueId != null) || (this.uniqueId != null && !this.uniqueId.equals(other.uniqueId))) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "bicdkeys.RegAttempt[ uniqueId=" + uniqueId + " ]";
    }

    public String getIpAddress() {
        return ipAddress;
    }

    public void setIpAddress(String ipAddress) {
        this.ipAddress = ipAddress;
    }
    
}
