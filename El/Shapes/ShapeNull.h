#pragma once
#include "ishape.h"

namespace Shapes
{
	class ShapeNull : public IShape
	{
	public:
		ShapeNull(VertexArray *vx):IShape(vx) {}
		virtual unsigned int GetVertexCount() const {return 0;}
		virtual unsigned int GetPartCount() const {return 0;}
		virtual unsigned int GetIndex(unsigned int pos) const {return 0;}
		const DVertex &GetVertex(unsigned int pos) const {return DVertex::GetInvalidVertex();}
		virtual unsigned int GetPartIndex(unsigned int partId) const {return 0;}
		virtual unsigned int GetPartSize(unsigned int partId) const {return 0;}
		virtual bool PtInside(const DVector &vx) const {return false;}
		virtual DVector NearestInside(const DVector &vx) const {return vx;}
		virtual IShape *Clip(double xn, double yn, double pos, VertexArray *vxArr=0) const {return 0;}
		virtual IShape *Copy( VertexArray *vxArr=0) const {return 0;}
		IShape *Crop(double left, double top, double right, double bottom, VertexArray *vxArr=0) const {return 0;}
		virtual double GetPerimeter() const {return 0;}
		virtual double GetArea()  const {return 0;}
		Shapes::IShape* ExtractPart(unsigned int) const {return 0;}
	};
}
