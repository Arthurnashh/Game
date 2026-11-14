/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package bicdkeys;

import java.math.BigInteger;
import javax.xml.bind.DatatypeConverter;

/**
 *
 * @author Ondra
 */
public final class CDKey {
    static public final int KEY_BITS = 120;
    static public final int KEY_BYTES = KEY_BITS/8;

    static public final int DISTR_BITS = 6;
    static public final int SIMPLE_HASH_BITS = 4;
    static public final int PLAYER_BITS = 32 - DISTR_BITS - SIMPLE_HASH_BITS;
    
    public short[] _message;
    
    /*
static bool DecodeMsg(unsigned char *msg, const char *buffer)
{
	for (int i=0; i<3; i++)
	{
		unsigned __int64 value = 0;
		int offset = 0;
		for (int k=0; k<8; k++)
		{
			int c = *buffer++;
			while (c && !isalnum((int)c)) c = *buffer++;
			if (!c) return false;
			c = toupper(c);
			// FIX: there are no letters O and I in serial number
			// if users enters it, it must be misread number
			if (c=='O') c='0';
			if (c=='I') c='1';
			const char *pos = strchr(UsableChars, c);
			if (!pos) return false;
			c = pos - UsableChars;
			unsigned __int64 cc = c;
			value |= cc << offset;
			offset += 5;
		}
		offset = 4 * 8;
		for (int j=0; j<5; j++)
		{
			*msg++ = (byte)((value >> offset) & 0xff);
			offset -= 8;
		}
	}
	return true;
}
     */
    public static short[] Decode(String textKey)
    {
        // check: hex format or printed format?
        // try parsing as HEX, if does not work, try as text
        // HEX should be 15 bytes
        try
        {
            StringBuilder textCondensed = new StringBuilder();
            boolean matchCondensed = true;
            int t;
            for (t=0; t<textKey.length()-2; t+=3)
            {
                if (textKey.charAt(t+2)!=' ')
                {
                    matchCondensed = false;
                    break;
                }
                textCondensed.append(textKey.charAt(t));
                textCondensed.append(textKey.charAt(t+1));
            }
            if (matchCondensed) textCondensed.append(textKey.substring(t));
            
            byte[] binKey = DatatypeConverter.parseHexBinary(matchCondensed ? textCondensed.toString() : textKey);
            if (binKey.length==15)
            {
                short[] binKeyShort = new short[binKey.length];
                for (int i=0; i<binKey.length; i++) binKeyShort[i] = (short) (((short)binKey[i])&0xff);
                return binKeyShort;
            }
        }
        catch(IllegalArgumentException ex)
        {
            
        }
                
        try {
            short[] message = new short[KEY_BYTES];
            String UsableChars = "0123456789ABCDEFGHJKLMNPRSTVWXYZ";
            int input = 0;
            for (int i = 0; i < 3; i++) {
                BigInteger value = BigInteger.ZERO;
                int offset = 0;
                for (int k = 0; k < 8; k++) {
                    char c = Character.toUpperCase(textKey.charAt(input++));
                    while (!Character.isAlphabetic(c) && !Character.isDigit(c)) {
                        c = Character.toUpperCase(textKey.charAt(input++));
                    }
                    // FIX: there are no letters O and I in serial number
                    // if users enters it, it must be misread number
                    if (c == 'O') {
                        c = '0';
                    }
                    if (c == 'I') {
                        c = '1';
                    }
                    int ci = UsableChars.indexOf(c);
                    if (ci < 0) {
                        return null;
                    }
                    value = value.or(BigInteger.valueOf(ci).shiftLeft(offset));
                    offset += 5;
                }
                offset = 4 * 8;
                for (int j = 0; j < 5; j++) {
                    message[i * 5 + j] = (value.shiftRight(offset)).and(BigInteger.valueOf(0xff)).shortValue();
                    offset -= 8;
                }
            }
            return message;
        } catch (IndexOutOfBoundsException e) {
            return null;
        }
    }
    
    BigInteger getMessage()
    {
        BigInteger ret = BigInteger.ZERO;
        for (int i=0; i<4; i++)
        {
          ret = ret.or(BigInteger.valueOf(_message[i]).shiftLeft(i*8));
        }
        return ret;
    }
    public CDKey(short[] keyBytes)
    {
    }
    public CDKey(String keyText)
    {
        _message = Decode(keyText);
        if (_message==null) return;
        if (_message.length!=15)
        {
            _message = null;
            return;
        }

        BigInteger message = getMessage();
        BigInteger signature = message.shiftRight(DISTR_BITS + PLAYER_BITS);

        BigInteger compSignature =
                BigInteger.valueOf(_message[0] * 7)
                .add(BigInteger.valueOf(_message[1] * 11))
                .add(BigInteger.valueOf(_message[2] * 13))
                .add(BigInteger.valueOf(_message[3] * 17));
        BigInteger compSigMask = BigInteger.ONE.shiftLeft(SIMPLE_HASH_BITS).subtract(BigInteger.ONE);

        
        if (!signature.equals(compSignature.and(compSigMask)))
        {
            _message = null;
        }
    
    }

    public String getPlayerId()
    {
        BigInteger message = getMessage();
        // TODO: perform RSA verification?
        //if (GetKeySignature(message) != cdkey[4])
  
        BigInteger val = message.and(BigInteger.ONE.shiftLeft(PLAYER_BITS + DISTR_BITS).subtract(BigInteger.ONE));

        return String.format("%d",val);
    }
    
/// Update 8-bit CRC value using polynomial X^8 + X^5 + X^4 + 1
    public static short UpdateCRC8(short crc, short src) {
        final short polyVal = 0x8C;

        for (int i = 0; i < 8; i++) {
            if (((crc ^ src) & 1) != 0) {
                crc = (short) (((crc >> 1) ^ polyVal)&0xff);
            } else {
                crc >>= 1;
            }
            src >>= 1;
        }
        return (short) (crc&0xff);
    }

    public int getProductId()    {
        short crc = 0;
        for (int i=0; i<KEY_BYTES-1; i++) crc = UpdateCRC8(crc, _message[i]);
        
        return _message[KEY_BYTES - 1] ^ crc;
    }

    public byte[] getBytes() {
        byte[] bytes = new byte[_message.length];
        for(int i=0; i<_message.length; i++) bytes[i] = (byte) _message[i];
        return bytes;
    }

}
