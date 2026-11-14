#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CLIPVERT_HPP
#define _CLIPVERT_HPP

// check if point needs to be clipped
ClipFlags NeedsClipping
(
	const Matrix4 &transform, const Camera &camera, Vector3Par point
);

/*
ClipFlags IsClipped
(
	Vector3Par point,
	const Camera &camera,  float r
);

ClipFlags MayBeClipped
(
	Vector3Par point, Vector3Par wPoint,
	const Camera &camera,  float r, int userPlane
);
*/

#endif
