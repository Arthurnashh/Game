#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   bitmask.hpp
    @brief  Variable-size bit mask.

    Copyright &copy; 2000-2002 by Josef Pelikan, MFF UK Prague
    @author PE
    @date   16.6.2002
*/

#ifndef _BITMASK_H
#define _BITMASK_H

#include <Es/Types/pointers.hpp>
#include <Es/Common/global.hpp>

//-----------------------------------------------------------
//  BitMask object:

class BitMaskMTS;

/**
    @brief  Bit-mask array addresing: higher part (array index).
*/
#define MaskHigh(addr)  mask[(addr)>>5]

/**
    @brief  Bit-mask array addressing: lower part (bit-index).
*/
#define MaskLow(addr)   (1<<((addr)&31))

/**
    @class  BitMask
    @brief  Variable-size bit-mask.

    Grows automatically, implements set of common operators including
    set operations, iterators, etc.
*/
class BitMask : public RefCount {

protected:

    /**
        @brief  Bit-mask array itself.
        One bit per value in one continuous /32-bit aligned/ memory segment.
    */
    unsigned32 *mask;

    /**
        @brief  Bit-mask array size.
        Number of allocated <code>mask[]</code> items (in 32-bit words).
    */
    int allocMask;

    /**
        @brief  Bit mask range.
        Invariants: <code>(max-min)/32 <= allocMask</code>, <code>(max-min) == k * 32</code>.
    */
    int min, max;

    /**
        Heap allocation routines.
    */
    virtual unsigned32 *newArray ( int size );

    virtual void deleteArray ( unsigned32 *&array );

    friend class BitMaskMTS;                // strange <= MSVC error?

public:

    /// Singular value for getFirst() and getNext().
    static const int END;

    /// Default (trivial) constructor. Creates an empty bit-mask.
    BitMask ();

    /// Copy-constructor. Does not bit-mask compaction.
    BitMask ( const BitMask &b );

    /// Assignment operator. Does not bit-mask compaction.
    BitMask& operator=( const BitMask &b );

    /// Recycles all used memory.
    virtual ~BitMask ();

    /**
        @brief  [Re]sets one bit-value (safe operation).
        @see    on(), off()
        @param  value Bit-index to be [re]set.
        @param  flag Boolean value.
    */
    void set ( int value, bool flag );

    /**
        @brief  Turns on the given bit-value (safe operation).
        @see    off()
        @param  value Bit-index to be set.
    */
    void on ( int value );
        
    /**
        @brief  Turns off the given bit-value (safe operation).
        @see    on()
        @param  value Bit-index to be reset.
    */
    void off ( int value );

    /**
        @brief  Prepares the bit-mask array to hold the given value.
        @see    setUnsafe(), getUnsafe()
        @param  value Bit-index the bit-mask to be prepared for.
     */
    void access ( int value );

	/**
        @brief  Compacts the bit-mask array (shrinks it as much as possible).
    */
	void compact ();

    /**
        Optimizes the mask for one-directional growing.
        @param  up Up direction = prepare for growing up to the higher numbers (and vice versa).
        @param  anchor Asserted base value (minimum value used in the mask for up=true).
    */
    void growOptimize ( bool up, int anchor =END );

    /**
        @brief  Sets one bit-value - <b>unsafe</b> operation!
        Relevant bit-indices have to be <code>access()</code>-ed before!
        @see    access()
        @param  value Bit-index to be [re]set.
        @param  flag Boolean value.
    */
    inline void setUnsafe ( int value, bool flag )
    {
        value -= min;
        if ( flag ) MaskHigh(value) |= MaskLow(value);
        else        MaskHigh(value) &= ~MaskLow(value);
    }

    /**
        @brief  Sets the given range (safe operation).
        @param  from The first bit-index to be [re]set.
        @param  len Segment size in bits (any value).
        @param  flag Boolean value.
    */
    void range ( int from, int len, bool flag );

    /**
        @brief  Gets one bit-value (safe operation).
        @param  value Bit-index to be asked.
        @return Actual boolean value.
    */
    inline bool get ( int value ) const
    {
        if ( value < min || value >= max ) return false;
        value -= min;
        return( (MaskHigh(value) & MaskLow(value)) != 0 );
    }

    /**
        @brief  Gets one bit-value - <b>unsafe</b> operation!
        Relevant bit-indices have to be <code>access()</code>-ed before!
        @param  value Bit-index to be asked.
        @return Actual boolean value.
    */
    inline bool getUnsafe ( int value ) const
    {
        value -= min;
        return( (MaskHigh(value) & MaskLow(value)) != 0 );
    }

    /// (Re)initializes the bit-mask. Recycles all used memory.
    void empty ();

    /**
        Clears the mask and optimizes it for one-directional growing.
        @param  up Up direction = prepare for growing up to the higher numbers (and vice versa).
        @param  origin Value which will be set the next time (or END to determine it automatically).
    */
    void emptyOptimize ( bool up, int origin =END );

    /**
        @brief  Bit-mask cardinality.
        @return Number of bits which are set.
    */
    int card () const;

    //@{
    /**
        @name   Iterators
        Iterator operations.
    */

    /**
        @brief  Iterator initialization.
        @see    getNext(), END
        @return The first bit-index or <code>END</code>.
    */
    int getFirst () const;

    /**
        @brief  Iteration.
        @see    getFirst(), END
        @param  i Previous bit-index.
        @return The next bit-index or <code>END</code>.
    */
    int getNext ( int i ) const;

    //@}

    /**
        @brief  Last bit-index lookup.
        @see    getFirst(), END
        @return The last bit-index that is set (or <code>END</code> if bit-mask is empty).
    */
    int getLast () const;

    //@{
    /** 
        @name   Set operations.
    */
    
    /**
        @brief  Bitwise OR.
        @param  b Bit-mask to be OR-ed.
        @return Reference to this bit-mask (holding operation result).
    */
	BitMask& operator|= ( const BitMask &b );

    /**
        @brief  Bitwise AND.
        @param  b Bit-mask to be AND-ed.
        @return Reference to this bit-mask (holding operation result).
    */
	BitMask& operator&= ( const BitMask &b );

    /**
        @brief  Bitwise XOR.
        @param  b Bit-mask to be XOR-ed.
        @return Reference to this bit-mask (holding operation result).
    */
	BitMask& operator^= ( const BitMask &b );

    /**
        @brief  Bitwise SUBTRACT.
        @param  b Bit-mask to be subtracted.
        @return Reference to this bit-mask (holding operation result).
    */
	BitMask& operator-= ( const BitMask &b );

    //@}

    void getStat ( int &minimum, int &maximum );

    };

/**
    @class  BitMaskMTS
    @brief  Variable-size bit-mask with multi-threaded safety feature.

    Successor of.BitMask.
*/
class BitMaskMTS : public BitMask {

protected:

    /**
        MT-safe heap allocation routines.
    */
    virtual unsigned32 *newArray ( int size );

    virtual void deleteArray ( unsigned32 *&array );

public:

    /// Default (trivial) constructor. Creates an empty bit-mask.
    BitMaskMTS ();

    /// Copy-constructor. Does not bit-mask compaction.
    BitMaskMTS ( const BitMaskMTS &b );

    /// Copy-constructor. Does not bit-mask compaction.
    BitMaskMTS ( const BitMask &b );

    /// Assignment operator. Does not bit-mask compaction.
    BitMaskMTS& operator=( const BitMaskMTS &b );

    /// Assignment operator. Does not bit-mask compaction.
    BitMaskMTS& operator=( const BitMask &b );

    /// Recycles all used memory.
    virtual ~BitMaskMTS ();

    };

#endif
