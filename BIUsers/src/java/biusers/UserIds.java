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
@Table(name = "userIds")
@XmlRootElement
@NamedQueries({
    @NamedQuery(name = "UserIds.findAll", query = "SELECT u FROM UserIds u"),
    @NamedQuery(name = "UserIds.findById", query = "SELECT u FROM UserIds u WHERE u.id = :id")})
public class UserIds implements Serializable {
    private static final long serialVersionUID = 1L;
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    @Basic(optional = false)
    @Column(name = "ID")
    private Integer id;
    @Lob
    @Column(name = "Certificate")
    private byte[] certificate;

    public UserIds() {
    }

    public UserIds(Integer id) {
        this.id = id;
    }

    public Integer getId() {
        return id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    public byte[] getCertificate() {
        return certificate;
    }

    public void setCertificate(byte[] certificate) {
        this.certificate = certificate;
    }

    @Override
    public int hashCode() {
        int hash = 0;
        hash += (id != null ? id.hashCode() : 0);
        return hash;
    }

    @Override
    public boolean equals(Object object) {
        // TODO: Warning - this method won't work in the case the id fields are not set
        if (!(object instanceof UserIds)) {
            return false;
        }
        UserIds other = (UserIds) object;
        if ((this.id == null && other.id != null) || (this.id != null && !this.id.equals(other.id))) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "biusers.UserIds[ id=" + id + " ]";
    }
    
}
