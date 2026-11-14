#ifndef _vectorSpace_h_
#define _vectorSpace_h_

#define MAX_DIMENSION 4

//! Ancestor of every item in the vector space
struct SVector : public RefCount {
  //! Id of the vector in the space
  int _id;
  //! Vector
  float _vector[MAX_DIMENSION];
  //! This flag determines, wheter vector is temporary or not
  int _temporary;
};

//! Vector space
template<class Type> class TVectorSpace : public RefCount {
private:
  //! Dimension of the space
  int _dimension;
  //! Array of vectors
  RefArray<Type> _vectors;
  //! Acceptable intervals
  float _interval[MAX_DIMENSION];
  //! Determines wheter vectors are in interval specified by _interval
  int Matches(float *vector1, float *vector2);
public:
  //! Initialization
  void Init(int dimension, float *interval);
  //! Removing of all vectors
  void Clear();
  //! Removing of all temporary vectors
  void ClearTemporary();
  //! Returns vector object near the specified vector or NULL if there isn't any
  Type *GetVector(float *vector);
  //! Add a vector to the vector space
  void AddVector(Ref<Type> vectorItem, float *vector);
  //! Returns number of vectors in the space
  int GetSize() const;
  //! Returns array of vectors
  RefArray<Type> &GetVectorArray() const;
};

template<class Type>
int TVectorSpace<Type>::Matches(float *vector1, float *vector2) {
  for (int i = 0; i < _dimension; i++) {
    if (fabs(vector2[i] - vector1[i]) > _interval[i]) return 0;
  }
  return 1;
}

template<class Type>
void TVectorSpace<Type>::Init(int dimension, float *interval) {
  Assert(dimension <= MAX_DIMENSION);
  _dimension = dimension;
  memcpy(_interval, interval, sizeof(float) * dimension);
}

template<class Type>
void TVectorSpace<Type>::Clear() {
  _vectors.Clear();
}

template<class Type>
void TVectorSpace<Type>::ClearTemporary() {
  for (int i = 0; i < _vectors.Size(); i++) {
    while ((i < _vectors.Size()) && (_vectors[i]->_temporary)) {
      _vectors.Delete(i);
    }
  }
}

template<class Type>
Type *TVectorSpace<Type>::GetVector(float *vector) {
  for (int i = 0; i < _vectors.Size(); i++) {
    if (Matches(_vectors[i]->_vector, vector)) {
      return _vectors[i];
    }
  }
  return NULL;
}

template<class Type>
void TVectorSpace<Type>::AddVector(Ref<Type> vectorItem, float *vector) {
  vectorItem->_id = _vectors.Size();
  memcpy(vectorItem->_vector, vector, sizeof(float) * _dimension);
  _vectors.Add(vectorItem);
}

template<class Type>
int TVectorSpace<Type>::GetSize() const {
  return _vectors.Size() ;
}

template<class Type>
RefArray<Type> &TVectorSpace<Type>::GetVectorArray() const {
  return const_cast<TVectorSpace<Type> *>(this)->_vectors;
}

#endif