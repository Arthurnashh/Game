#ifndef _RANDOMFLYMAN_H_
#define _RANDOMFLYMAN_H_


namespace TreeEngine
{
  #define RANDOMFLYMAN_TABLESIZE 32768

  /*!
    The fastest possible generator of random numbers. It is based on a table generated
    using RandomJames algorithm.
  */
  class CRandomFlyman {
  private:
    //! Random table to read data from.
    static float _RandomTable[RANDOMFLYMAN_TABLESIZE];
  public:
    //! Method will return Index-th random number.
    /*!
      There is no restriction on range of Index.
    */
    inline float Random(int Index) {
      return _RandomTable[Index & (RANDOMFLYMAN_TABLESIZE - 1)];
    };
    //! Returns random integer value from the range [0..RANDOMFLYMAN_TABLESIZE].
    inline int RandomIndex(int Index) {
      return (int) (Random(Index) * RANDOMFLYMAN_TABLESIZE);
    }
  };

}
#endif
