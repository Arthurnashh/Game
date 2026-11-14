
__forceinline float QuaternionRB::Size()
{
  return sqrtf(_w*_w + _x*_x + _y*_y + _z*_z);
}

__forceinline void QuaternionRB::Normalize()
{
	float fSize = Size();
	//ASSERT(fSize != 0.0f);

	if (fSize != 0.0f)
	{
		_w /= fSize;
		_x /= fSize;
		_y /= fSize;
		_z /= fSize;
	}
}

// Operators
__forceinline const QuaternionRB& QuaternionRB::operator=( const QuaternionRB& cSecond)
{
	memcpy(_m, cSecond._m, 4 * sizeof(*_m));

	return *this;
}

__forceinline const QuaternionRB& QuaternionRB::operator=(  Vector3& cSecond)
{
	_m[0] = 0.0f;
  _m[1] = cSecond[0];
  _m[2] = cSecond[1];
  _m[3] = cSecond[2];	

	return *this;
}

__forceinline QuaternionRB QuaternionRB::operator+( const QuaternionRB& cSecond) const
{
	return QuaternionRB(_w + cSecond._w, _x + cSecond._x, _y + cSecond._y, _z + cSecond._z);
}

__forceinline QuaternionRB QuaternionRB::operator-( const QuaternionRB& cSecond) const
{
	return QuaternionRB(_w - cSecond._w, _x - cSecond._x, _y - cSecond._y, _z - cSecond._z);
}

__forceinline QuaternionRB QuaternionRB::operator*( const QuaternionRB& cSecond) const
{
	// (w1 * w2 - v1 * v1, w1 * v2 + w2 * v1 + v1 x v2)
	return QuaternionRB(_w * cSecond._w - _x * cSecond._x - _y * cSecond._y - _z * cSecond._z,
		_w * cSecond._x + cSecond._w * _x + _y * cSecond._z - _z * cSecond._y,
		_w * cSecond._y + cSecond._w * _y + _z * cSecond._x - _x * cSecond._z,
		_w * cSecond._z + cSecond._w * _z + _x * cSecond._y - _y * cSecond._x);
}

__forceinline QuaternionRB QuaternionRB::operator*( float fSecond) const
{
	return QuaternionRB(_w * fSecond, _x * fSecond, _y * fSecond, _z * fSecond);
}

__forceinline const QuaternionRB& QuaternionRB::operator+=( const QuaternionRB& cSecond)
{
	_w += cSecond._w;
	_x += cSecond._x;
	_y += cSecond._y;
	_z += cSecond._z;

	return *this;
}
__forceinline const QuaternionRB& QuaternionRB::operator-=( const QuaternionRB& cSecond)
{
	_w -= cSecond._w;
	_x -= cSecond._x;
	_y -= cSecond._y;
	_z -= cSecond._z;

	return *this;
}

__forceinline const QuaternionRB& QuaternionRB::operator*=( const QuaternionRB& cSecond)
{
	QuaternionRB cNew(_w * cSecond._w - _x * cSecond._x - _y * cSecond._y - _z * cSecond._z,
		_w * cSecond._x + cSecond._w * _x + _y * cSecond._z - _z * cSecond._y,
		_w * cSecond._y + cSecond._w * _y + _z * cSecond._x - _x * cSecond._z,
		_w * cSecond._z + cSecond._w * _z + _x * cSecond._y - _y * cSecond._x);

	*this = cNew;
	return *this;
}

__forceinline const QuaternionRB& QuaternionRB::operator*=( float fSecond)
{
	_w *= fSecond;
	_x *= fSecond;
	_y *= fSecond;
	_z *= fSecond;

	return *this;
}

__forceinline bool QuaternionRB::operator==( const QuaternionRB& cSecond) const
{
  return _w == cSecond._w && _x == cSecond._x && _y == cSecond._y && _z == cSecond._z;
}

__forceinline Matrix3 QuaternionRB::GetMatrix() const
{
  Matrix3 out;

	float fTemp = _x * _x;

	out(1,1) = -fTemp;
	out(2,2) = -fTemp;

	fTemp = _y * _y;

	out(0,0) = -fTemp;
	out(2,2) += -fTemp;

	fTemp = _z * _z;

	out(0,0) += -fTemp;
	out(1,1) += -fTemp;

	fTemp = _x * _y;

	out(0,1) = fTemp;
	out(1,0) = fTemp;

	fTemp = _x * _z;

	out(0,2) = fTemp;
	out(2,0) = fTemp;

	fTemp = _y * _z;

	out(1,2) = fTemp;
	out(2,1) = fTemp;

	fTemp = _x * _w;

	out(1,2) += -fTemp;
	out(2,1) += fTemp;

	fTemp = _y * _w;

	out(0,2) += fTemp;
	out(2,0) += -fTemp;

	fTemp = _z * _w;

	out(0,1) += -fTemp;
	out(1,0) += fTemp;

	out *= 2.0f;

	out(0,0) += 1.0f;
	out(1,1) += 1.0f;
	out(2,2) += 1.0f;

	return out;
}


__forceinline void QuaternionRB::FromMatrixRotation(Matrix3Par m)
{
  const static int nxt[3] = {1, 2, 0};

  float tr = m(0,0) + m(1,1) + m(2,2);

  // check the diagonal
  if (tr > 0.0f)
  {
    float s = (float)sqrt (tr + 1.0f);
    _w = s*0.5f;
    s = 0.5f / s;
    _x = -(m(1,2) - m(2,1)) * s;
    _y = -(m(2,0) - m(0,2)) * s;
    _z = -(m(0,1) - m(1,0)) * s;
  }
  else
  {		
    float  s, q[4];

    const static int nxt[3] = {1, 2, 0};

    // diagonal is negative
    int i = 0;
    if (m(1,1) > m(0,0)) i = 1;
    if (m(2,2) > m(i,i)) i = 2;
    int j = nxt[i];
    int k = nxt[j];


    s = (float)sqrt ((m(i,i) - (m(j,j) + m(k,k))) + 1.0f);

    q[i] = s * 0.5f;

    if (s != 0.0f) s = 0.5f / s;


    q[3] = (m(j,k) - m(k,j)) * s;
    q[j] = (m(i,j) + m(j,i)) * s;
    q[k] = (m(i,k) + m(k,i)) * s;


    _x = -q[0];
    _y = -q[1];
    _z = -q[2];
    _w = q[3];
  }
}