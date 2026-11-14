#pragma once
#include "ishape.h"

namespace ShapeFiles
{
	/**
	* ShapeBase is base class derived from IShape interface. It introduces some
	* services common from all shapes. This class doesn't contains any extra data.   
	*/
	class ShapeBase : public IShape
	{
//    mutable DBox box;
	public:

		///Construction
		/**
		* @see IShape 
		*/
		ShapeBase(VertexArray* vertices)
		: IShape(vertices)/*,
				  box(DVertex(DBL_MIN,DBL_MIN),DVertex(DBL_MIN,DBL_MIN))*/
		{
		}

/*    virtual DBox CalcBoundingBox() const
    {
      if (box.lo.x==DBL_MIN && box.hi.x==DBL_MIN)
      {
        return box=IShape::CalcBoundingBox();
      }
      else
        return box;
    }*/


		///Calculates side of vertex.
		/**
		* For given line and the vertex it calculates vertex side. Line
		* is declared using a,b,c as equaletion ax+by+c=0, point is declared as x,y
		* @param a a-value
		* @param b b-value
		* @param c c-value
		* @param x x-coordinate of the point
		* @param y y-coordinate of the point
		* @retval -1 point lies on left side of line (other side, than direction of vector [a,b])
		* @retval 0 point lies on the line
		* @retval 1 point lies on right side of line (side in direction of vector [a,b])
		*/
		static int Side(double a, double b, double c, double x, double y)
		{
			double k = a * x + b * y + c;
			if (k > 0) return 1;
			if (k < 0) return -1;
			return 0;
		}

    /*
    ax+by=-c
    bx-ay=0
    */

		///Calculates point order on the line
		/**
		* Line is declared as ax+by+c=0. Point is declared as x,y and is should lay on the
		* line (or there should be epsilon distance).
		* @param a a-value
		* @param b b-value
		* @param c c-value
		* @param x x-coordinate of the point
		* @param y y-coordinate of the point
		* @return order number. This number can be used to sort points. Points will
		* be sorted in order of direction of the line.
		*/
		static double OrderOnLine(double a, double b, double c, double x, double y)
		{
			//Spocitej bod na primce nejblizsi k pocatku souradnic
			double dd = a * a + b * b;
			double dx = -c * a;
			double dy = -c * b;

			//toto je ten bod
			double Xref = dx / dd, Yref = dy / dd;

			//spocitej relativni vektor x,y od tohoto body
			dx = x - Xref;
			dy = y - Yref;

			//spocitej smernici primky z normaly (y,-x)
			double lx = -b, ly = a;

			//spocitej velikost ^ 2 relativniho vektoru
			dd = dx * dx + dy * dy;

			//pokud je relativni vektor na opacnou stranu nez smernice priomky, prehod znamenko
			if (lx * dx + ly * dy < 0) dd = -dd;

			//dd je cislo vhodne pro urcovani poradi bodu na primce
			return dd;
		}
	};
};
