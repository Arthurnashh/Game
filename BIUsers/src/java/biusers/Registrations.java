/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package biusers;

import java.io.Serializable;
import javax.persistence.*;
import javax.validation.constraints.NotNull;
import javax.xml.bind.annotation.XmlRootElement;

/**
 *
 * @author ondra
 */
@Entity
@Table(name = "registrations")
@XmlRootElement
@NamedQueries({
    @NamedQuery(name = "Registrations.findAll", query = "SELECT r FROM Registrations r"),
    @NamedQuery(name = "Registrations.findById", query = "SELECT r FROM Registrations r WHERE r.id = :id"),
    @NamedQuery(name = "Registrations.findByChallenge", query = "SELECT r FROM Registrations r WHERE r.challenge = :challenge")})
public class Registrations implements Serializable {
    private static final long serialVersionUID = 1L;
    @Column(name = "ID")
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Basic(optional = false)
    private Integer id;
    @Column(name = "challenge")
    private Integer challenge;

    public Registrations() {
    }

    public Registrations(Integer challenge) {
        this.challenge = challenge;
    }

    public Integer getId() {
        return id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    public Integer getChallenge() {
        return challenge;
    }

    public void setChallenge(Integer challenge) {
        this.challenge = challenge;
    }

    @Override
    public int hashCode() {
        int hash = 0;
        hash += (challenge != null ? challenge.hashCode() : 0);
        return hash;
    }

    @Override
    public boolean equals(Object object) {
        // TODO: Warning - this method won't work in the case the id fields are not set
        if (!(object instanceof Registrations)) {
            return false;
        }
        Registrations other = (Registrations) object;
        if ((this.challenge == null && other.challenge != null) || (this.challenge != null && !this.challenge.equals(other.challenge))) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "biusers.Registrations[ challenge=" + challenge + " ]";
    }
    
}
