/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package bicdkeys;

import java.io.Serializable;
import javax.persistence.*;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;
import javax.xml.bind.annotation.XmlRootElement;

/**
 *
 * @author ondra
 */
@Entity
@Table(name = "ProductKeys")
@XmlRootElement
@NamedQueries({
    @NamedQuery(name = "ProductKeys.findAll", query = "SELECT p FROM ProductKeys p"),
    @NamedQuery(name = "ProductKeys.findByProductId", query = "SELECT p FROM ProductKeys p WHERE p.productId = :productId"),
    @NamedQuery(name = "ProductKeys.findByProductNumId", query = "SELECT p FROM ProductKeys p WHERE p.productCode = :productId")})
public class ProductKeys implements Serializable {
    @Lob
    @Column(name = "privateKey")
    private byte[] privateKey;
    @Lob
    @Column(name = "publicKey")
    private byte[] publicKey;
    private static final long serialVersionUID = 1L;
    @Id
    @Basic(optional = false)
    @NotNull
    @Size(min = 0, max = 5)
    @Column(name = "productId")
    private String productId;

    @Column(name = "productCode")
    private Integer productCode;
    
    public ProductKeys() {
    }

    public ProductKeys(String productId) {
        this.productId = productId;
    }

    public String getProductId() {
        return productId;
    }

    public void setProductId(String productId) {
        this.productId = productId;
    }

    @Override
    public int hashCode() {
        int hash = 0;
        hash += (productId != null ? productId.hashCode() : 0);
        return hash;
    }

    @Override
    public boolean equals(Object object) {
        // TODO: Warning - this method won't work in the case the id fields are not set
        if (!(object instanceof ProductKeys)) {
            return false;
        }
        ProductKeys other = (ProductKeys) object;
        if ((this.productId == null && other.productId != null) || (this.productId != null && !this.productId.equals(other.productId))) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "bicdkeys.ProductKeys[ productId=" + productId + " ]";
    }

    public byte[] getPrivateKey() {
        return privateKey;
    }

    public void setPrivateKey(byte[] privateKey) {
        this.privateKey = privateKey;
    }

    public byte[] getPublicKey() {
        return publicKey;
    }

    public void setPublicKey(byte[] publicKey) {
        this.publicKey = publicKey;
    }

    public Integer getProductCode() {
        return productCode;
    }

    public void setProductCode(Integer productCode) {
        this.productCode = productCode;
    }
    
}
