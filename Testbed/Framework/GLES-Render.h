#ifndef _GLESRENDER_H_
#define _GLESRENDER_H_

#include <Box2D/Box2D.h>
#include <GLES/gl.h>

#ifdef __QNX__
#include <bbutil.h>
#endif

struct b2AABB;

// This class implements debug drawing callbacks that are invoked
// inside b2World::Step.
class GLESDebugDraw : public b2Draw
{
	float32 mRatio;
public:
	GLESDebugDraw();

	GLESDebugDraw( float32 ratio );

	virtual void DrawPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color);

	virtual void DrawSolidPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color);

	virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);

	virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);

	virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

	virtual void DrawTransform(const b2Transform& xf);

    virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color);

    virtual void DrawString(int x, int y, const char* string, ...); 

    virtual void DrawAABB(b2AABB* aabb, const b2Color& color);

#ifdef __QNX__
    void 		 SetFont(const font_t *font) { _font = font; }
    void		 SetScreenSize(float width, float height);

private:
    const font_t		*_font;
    float				 _screenWidth;
    float			 	 _screenHeight;
#endif
};


#endif
