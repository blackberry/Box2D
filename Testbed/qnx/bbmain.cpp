/*
* Copyright (c) 2006-2007 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "GLES-Render.h"
#include "Test.h"

#include <assert.h>
#include <screen/screen.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <input/screen_helpers.h>
#include <gestures/set.h>
#include <gestures/tap.h>
#include <gestures/pinch.h>
#include <gestures/double_tap.h>
#include <gestures/two_finger_pan.h>
#include <sys/keycodes.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "bbutil.h"

namespace
{
	int32 testIndex = 0;
	int32 testSelection = 0;
	int32 testCount = 0;
	TestEntry* entry;
	Test* test;
	Settings settings;
	int32 width = 640;
	int32 height = 480;
	int32 framePeriod = 16;  // = 1/60 * 1000
	int32 mainWindow;
	float settingsHz = 60.0;
	float32 viewZoom = 1.0f;
	int tx, ty, tw, th;
	bool rMouseDown;
	b2Vec2 lastp;
	GLESDebugDraw localDraw;
	float radius = 2.0f;
	struct gestures_set * set;
	b2Vec2 centerPrev;
	b2Vec2 centerNext;
	const font_t* font;
    screen_context_t screen_cxt;
}

// convert the timespec into milliseconds
static long time2millis(struct timespec *times)
{
    return times->tv_sec*1000 + times->tv_nsec/1000000;
}

static void Resize(int32 w, int32 h)
{
	width = w;
	height = h;

	tx = 0;
	ty = 0;
	tw = width;
	th = height;
	glViewport(tx, ty, tw, th);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float32 ratio = float32(tw) / float32(th);

	b2Vec2 extents(ratio * 25.0f, 25.0f);
	extents *= viewZoom;

	b2Vec2 lower = settings.viewCenter - extents;
	b2Vec2 upper = settings.viewCenter + extents;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrthof(lower.x, upper.x, lower.y, upper.y, -1024.0f, 1024.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static b2Vec2 ConvertScreenToWorld(int32 x, int32 y)
{
	float32 u = x / float32(tw);
	float32 v = (th - y) / float32(th);

	float32 ratio = float32(tw) / float32(th);
	b2Vec2 extents(ratio * 25.0f, 25.0f);
	extents *= viewZoom;

	b2Vec2 lower = settings.viewCenter - extents;
	b2Vec2 upper = settings.viewCenter + extents;

	b2Vec2 p;
	p.x = (1.0f - u) * lower.x + u * upper.x;
	p.y = (1.0f - v) * lower.y + v * upper.y;

	return p;
}

static void Keyboard(unsigned char key, int x, int y)
{
	B2_NOT_USED(x);
	B2_NOT_USED(y);

	switch (key)
	{
		case 27:
			exit(0);
			break;

			// Press 'z' to zoom out.
		case 'z':
			viewZoom = b2Min(1.1f * viewZoom, 20.0f);
			Resize(width, height);
			break;

			// Press 'x' to zoom in.
		case 'x':
			viewZoom = b2Max(0.9f * viewZoom, 0.02f);
			Resize(width, height);
			break;

			// Press 'r' to reset.
		case 'r':
			delete test;
			test = entry->createFcn();
			break;

			// Press space to launch a bomb.
		case ' ':
			if (test)
			{
				test->LaunchBomb();
			}
			break;

		case 'p':
			settings.pause = !settings.pause;
			break;

			// Press [ to prev test.
		case '[':
			--testSelection;
			if (testSelection < 0)
			{
				testSelection = testCount - 1;
			}
			break;

			// Press ] to next test.
		case ']':
			++testSelection;
			if (testSelection == testCount)
			{
				testSelection = 0;
			}
			break;

		default:
			if (test)
			{
				test->Keyboard(key);
			}
	}
}

static void KeyboardUp(unsigned char key, int x, int y)
{
	B2_NOT_USED(x);
	B2_NOT_USED(y);

	if (test)
	{
		test->KeyboardUp(key);
	}
}

static void MouseDown(int32 x, int32 y)
{
	if (x < centerPrev.x + radius &&
		x > centerPrev.x - radius &&
		y < centerPrev.y + radius &&
		y > centerPrev.y - radius)
	{
		testSelection--;
		if (testSelection < 0)
		{
			testSelection = testCount - 1;
		}
	}
	else if (x < centerNext.x + radius &&
			 x > centerNext.x - radius &&
			 y < centerNext.y + radius &&
			 y > centerNext.y - radius)
	{
		testSelection++;
		if (testSelection == testCount)
		{
			testSelection = 0;
		}
	}
	else
	{
		b2Vec2 p = ConvertScreenToWorld(x, y);
		test->MouseDown(p);
	}
}

static void MouseScrollBy(int32 x, int32 y)
{
	// convert to world sizes
	float32 ratio = float32(tw) / float32(th);

	b2Vec2 extents(ratio * 25.0f, 25.0f);
	extents *= viewZoom;

	float32 dx = x * (extents.x * 2) / tw;
	float32 dy = y * (extents.y * 2) / th;

	settings.viewCenter.x += dx;
	settings.viewCenter.y += dy;

	Resize(width, height);
}

/**
 * The callback invoked when a gesture is recognized or updated.
 */
static void gestureCallback(gesture_base_t* gesture, mtouch_event_t* event, void* param, int async)
{
	switch (gesture->type)
	{
		case GESTURE_TWO_FINGER_PAN:
		{
			int position[2];

			gesture_tfpan_t* tfpan = (gesture_tfpan_t*)gesture;

			if (tfpan->last_centroid.x && tfpan->last_centroid.y)
			{
				position[0] = tfpan->centroid.x - tfpan->last_centroid.x;
				position[1] = tfpan->centroid.y - tfpan->last_centroid.y;
				MouseScrollBy(-position[0], position[1]);
			}
			break;
		}

        case GESTURE_PINCH:
		{
			gesture_pinch_t* pinch = (gesture_pinch_t*) gesture;

			int dist_x = pinch->distance.x;
			int dist_y = pinch->distance.y;
			int last_dist_x = pinch->last_distance.x;
			int last_dist_y = pinch->last_distance.y;

			int reldist 	 = sqrt((dist_x) * (dist_x) + (dist_y) * (dist_y));
			int last_reldist = sqrt((last_dist_x) * (last_dist_x) + (last_dist_y) * (last_dist_y));

			if (reldist && last_reldist)
			{
				if (reldist < last_reldist)
					viewZoom = b2Min(1.01f * viewZoom, 20.0f);
				else if (reldist > last_reldist)
					viewZoom = b2Max(0.99f * viewZoom, 0.02f);

				Resize(width, height);
			}
			break;
		}

		case GESTURE_DOUBLE_TAP:
		{
			gesture_tap_t* d_tap = (gesture_tap_t*)gesture;
			if (test)
				test->LaunchBomb();

			break;
		}

		default:
		{
			break;
		}
	}
}

/**
 * Initialize the gestures sets
 */
static void initGestures()
{
    gesture_tap_t* 		  tap;
    gesture_double_tap_t* double_tap;

	set = gestures_set_alloc();
	if (NULL != set)
	{
		tap 	   = tap_gesture_alloc(NULL, gestureCallback, set);
		double_tap = double_tap_gesture_alloc(NULL, gestureCallback, set);
		tfpan_gesture_alloc(NULL, gestureCallback, set);
		pinch_gesture_alloc(NULL, gestureCallback, set);
	}
	else
	{
		fprintf(stderr, "Failed to allocate gestures set\n");
	}
}

static void gesturesCleanup()
{
	if (NULL != set)
	{
		gestures_set_free(set);
		set = NULL;
	}
}

void handleScreenEvent(bps_event_t *event)
{
    screen_event_t screen_event = screen_event_get_event(event);
	mtouch_event_t mtouch_event;
    int screen_val;
    int position[2];
    int rc;

    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);

    switch (screen_val)
    {
		case SCREEN_EVENT_MTOUCH_TOUCH:
		case SCREEN_EVENT_MTOUCH_MOVE:
		case SCREEN_EVENT_MTOUCH_RELEASE:
            rc = screen_get_mtouch_event(screen_event, &mtouch_event, 0);
            if (rc)
                fprintf(stderr, "Error: failed to get mtouch event\n");

            rc = gestures_set_process_event(set, &mtouch_event, NULL);

            // No gesture detected, pass through
            if (!rc)
            {
            	if (screen_val == SCREEN_EVENT_MTOUCH_TOUCH)
            		MouseDown(mtouch_event.x, mtouch_event.y);
            	else if (screen_val == SCREEN_EVENT_MTOUCH_MOVE)
            		test->MouseMove(ConvertScreenToWorld(mtouch_event.x, mtouch_event.y));
            	else
            		test->MouseUp(ConvertScreenToWorld(mtouch_event.x, mtouch_event.y));
            }

			break;

		case SCREEN_EVENT_KEYBOARD:
			screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_KEY_FLAGS, &screen_val);

			if (screen_val & KEY_DOWN)
			{
				screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_KEY_SYM, &screen_val);

				if (screen_val >= ' ' && screen_val < '~')
				{
					Keyboard(screen_val, 0, 0);
				}
				else
				{
					screen_val = screen_val - 0xf000;
					Keyboard(screen_val, 0, 0);
				}
			}

			break;
    }
}

int initialize()
{
    //Query width and height of the window surface created by utility code
    EGLint surface_width, surface_height;

	int dpi = bbutil_calculate_dpi(screen_cxt);

	font = bbutil_load_font("/usr/fonts/font_repository/monotype/tahoma.ttf", 6, dpi);
	if (!font)
		return EXIT_FAILURE;

	localDraw.SetFont(font);
	localDraw.SetScreenSize(width, height);

	eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &surface_width);
    eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &surface_height);

    width = surface_width;
    height = surface_height;

    // calculate the position of the next/prev buttons in world coordinates
    centerPrev.x = width / 2 - 88;
    centerNext.x = width / 2 + 88;
    centerPrev.y = height / 10;
    centerNext.y = height / 10;
    radius = 24;

	EGLint err = eglGetError();
	if (err != 0x3000)
	{
		fprintf(stderr, "Unable to query egl surface dimensions\n");
		return EXIT_FAILURE;
	}

    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepthf(1.0);

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Resize(width, height);

	return EXIT_SUCCESS;
}

void drawWidgets()
{
	const b2Color color(0.44f, 0.78f, 0.44f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0, width, height, 0, -1024.0f, 1024.0f);

	glMatrixMode(GL_MODELVIEW);

	localDraw.DrawCircle(centerPrev, radius, color);
	localDraw.DrawCircle(centerNext, radius, color);

	glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    // Render pass
    glEnableClientState(GL_VERTEX_ARRAY);

	test->SetTextLine(30);
	b2Vec2 oldCenter = settings.viewCenter;
	settings.hz = settingsHz;
	test->Step(&settings);
	if (oldCenter.x != settings.viewCenter.x || oldCenter.y != settings.viewCenter.y)
	{
		Resize(width, height);
	}

	test->DrawTitle(5, 15, entry->name);
    drawWidgets();

    glDisableClientState(GL_VERTEX_ARRAY);

	bbutil_swap();

	if (testSelection != testIndex)
	{
		testIndex = testSelection;
		delete test;
		entry = g_testEntries + testIndex;
		test = entry->createFcn();
		viewZoom = 1.0f;
		settings.viewCenter.Set(0.0f, 20.0f);
		Resize(width, height);
	}
}

int main(int argc, char *argv[])
{
    int exit_application = 0;
    int rc;

    //Create a screen context that will be used to create an EGL surface to to receive libscreen events
    screen_create_context(&screen_cxt, 0);

    initGestures();

    //Initialize BPS library
	bps_initialize();

	//Use utility code to initialize EGL for 2D rendering with GL ES 1.1
	if (EXIT_SUCCESS != bbutil_init_egl(screen_cxt, GL_ES_1, AUTO))
	{
		fprintf(stderr, "bbutil_init_egl failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_cxt);
		return 0;
	}

	//Initialize application logic
	if (EXIT_SUCCESS != initialize())
	{
		fprintf(stderr, "initialize failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_cxt);
		return 0;
	}

	//Signal BPS library that navigator and screen events will be requested
	if (BPS_SUCCESS != screen_request_events(screen_cxt))
	{
		fprintf(stderr, "screen_request_events failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_cxt);
		return 0;
	}

	if (BPS_SUCCESS != navigator_request_events(0))
	{
		fprintf(stderr, "navigator_request_events failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_cxt);
		return 0;
	}

	//Signal BPS library that navigator orientation is not to be locked
	if (BPS_SUCCESS != navigator_rotation_lock(false))
	{
		fprintf(stderr, "navigator_rotation_lock failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_cxt);
		return 0;
	}

	// set up our box2D tests
	fprintf(stderr, "Box2D Version %d.%d.%d\n", b2_version.major, b2_version.minor, b2_version.revision);

	testCount = 0;
	while (g_testEntries[testCount].createFcn != NULL)
	{
		++testCount;
	}

	testIndex = b2Clamp(testIndex, 0, testCount-1);
	testSelection = testIndex;

	entry = g_testEntries + testIndex;
	test = entry->createFcn();
	test->m_debugDraw.SetFont(font);
	test->m_debugDraw.SetScreenSize(width, height);

	struct timespec time_struct;

	clock_gettime(CLOCK_REALTIME, &time_struct);
	long update_time = time2millis(&time_struct);
	long current_time, last_time;

#ifdef FPS
	int frames = 0;
	last_time = update_time;
#endif

    for (;;)
    {
    	//Request and process BPS next available event
        bps_event_t *event = NULL;
        rc = bps_get_event(&event, 1);
        assert(rc == BPS_SUCCESS);

        if (event)
        {
            int domain = bps_event_get_domain(event);

            if (domain == screen_get_domain())
            {
                handleScreenEvent(event);
            }
            else if ((domain == navigator_get_domain()) && (NAVIGATOR_EXIT == bps_event_get_code(event)))
            {
            	exit_application = 1;
            }
        }

		clock_gettime(CLOCK_REALTIME, &time_struct);
		current_time = time2millis(&time_struct);

		if ((current_time - update_time) > framePeriod)
		{
			update_time = current_time;
			render();
#ifdef FPS
			frames++;
#endif
		}
		else
		{
			sleep(0);
		}

#ifdef FPS
		if (current_time - last_time > 1000)
		{
			fprintf(stderr, "fps: %d\n", frames);
			frames = 0;
			last_time = current_time;
		}
#endif

        if (exit_application)
        	break;
    }

    // clean up gestures
    gesturesCleanup();

    //Stop requesting events from libscreen
    screen_stop_events(screen_cxt);

    //Shut down BPS library for this process
    bps_shutdown();

	//Destroy the font
	bbutil_destroy_font(font);

    //Use utility code to terminate EGL setup
    bbutil_terminate();

    //Destroy libscreen context
    screen_destroy_context(screen_cxt);
    return 0;
}
