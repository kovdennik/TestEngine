#pragma once
#include "IRender.hpp"

struct IRenderAuxGeom
{
  virtual void DrawAABB(Vec3 min, Vec3 max) = 0;
	virtual void DrawLine(const Vec3& v0, const UCol& colV0, const Vec3& v1, const UCol& colV1, float thickness = 1.0f) = 0;
	virtual void DrawLines(const Vec3* v, uint32 numPoints, const UCol& col, float thickness = 1.0f) = 0;
  virtual void Flush() = 0;
};