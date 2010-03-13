#include "libs.h"
#include "Gui.h"
#include "Pi.h"
#include "GalacticView.h"
#include "SystemInfoView.h"
#include "Player.h"
#include "Serializer.h"
#include "SectorView.h"
#include "Sector.h"
#include "Galaxy.h"
		
GalacticView::GalacticView(): GenericSystemView(GenericSystemView::MAP_GALACTIC)
{
	const SDL_Surface *s = Galaxy::GetGalaxyBitmap();
	glEnable(GL_TEXTURE_2D);
	glGenTextures (1, &m_texture);
	glBindTexture (GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, s->w, s->h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, s->pixels);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDisable(GL_TEXTURE_2D);

	SetTransparency(true);
	m_zoom = 1.0f;

	m_zoomInButton = new Gui::ImageButton("icons/zoom_in_f7.png");
	//m_zoomInButton->SetShortcut(SDLK_F6, KMOD_NONE);
	m_zoomInButton->SetToolTip("Zoom in");
	Add(m_zoomInButton, 700, 5);
	
	m_zoomOutButton = new Gui::ImageButton("icons/zoom_out_f8.png");
	//m_zoomOutButton->SetShortcut(SDLK_F7, KMOD_NONE);
	m_zoomOutButton->SetToolTip("Zoom out");
	Add(m_zoomOutButton, 732, 5);

	Pi::onMouseButtonDown.connect(sigc::mem_fun(this, &GalacticView::MouseButtonDown));
}

GalacticView::~GalacticView()
{
	glDeleteTextures(1, &m_texture);
}

void GalacticView::Save()
{
	using namespace Serializer::Write;
}

void GalacticView::Load()
{
	using namespace Serializer::Read;
}

/*void GalacticView::OnClickSystemInfo()
{
	Pi::SetView(Pi::systemInfoView);
}*/

struct galaclabel_t {
	const char *label;
	vector3d pos;
} s_labels[] = {
	{ "Norma arm", vector3d(0.0,-0.3,0.0) },
	{ "Persius arm", vector3d(0.57,0.0,0.0) },
	{ "Outer arm", vector3d(0.65,0.4,0.0) },
	{ "Sagittarius arm", vector3d(-.3,0.2,0.0) },
	{ "Scutum-Centaurus arm", vector3d(-.45,-0.45,0.0) },
	{ 0 }
};

void GalacticView::PutLabels(vector3d offset)
{
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev (GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev (GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv (GL_VIEWPORT, viewport);

	Gui::Screen::EnterOrtho();
	glColor3f(1,1,1);
	
	int i = 0;
	while (s_labels[i].label) {
		vector3d p = m_zoom * (s_labels[i].pos + offset);
		vector3d pos;
		if (Gui::Screen::Project (p.x, p.y, p.z, modelMatrix, projMatrix, viewport, &pos[0], &pos[1], &pos[2])) {
			Gui::Screen::RenderLabel(s_labels[i].label, (float)pos.x, (float)pos.y);
		}
		i++;
	}

	Gui::Screen::LeaveOrtho();
	glDisable(GL_LIGHTING);
}


void GalacticView::Draw3D()
{
	int secx, secy;
	Pi::sectorView->GetSector(&secx, &secy);
	float offset_x = (secx*Sector::SIZE + Galaxy::SOL_OFFSET_X)/Galaxy::GALAXY_RADIUS;
	float offset_y = (-secy*Sector::SIZE + Galaxy::SOL_OFFSET_Y)/Galaxy::GALAXY_RADIUS;
	GenericSystemView::Draw3D();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-Pi::GetScrAspect(), Pi::GetScrAspect(), 1.0, -1.0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	glScalef(m_zoom, m_zoom, 0.0f);
	glTranslatef(-offset_x, -offset_y, 0.0f);

	glBegin(GL_QUADS);
		float w = 1.0;
		float h = 1.0;
		glTexCoord2f(0,h);
		glVertex2f(-1.0,1.0);
		glTexCoord2f(w,h);
		glVertex2f(1.0,1.0);
		glTexCoord2f(w,0);
		glVertex2f(1.0,-1.0);
		glTexCoord2f(0,0);
		glVertex2f(-1.0,-1.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glColor3f(0.0,1.0,0.0);
	glPointSize(3.0);
	glBegin(GL_POINTS);
		glVertex2f(offset_x, offset_y);
	glEnd();
	
	glLoadIdentity();
	glColor3f(1,1,1);
	glPointSize(1.0);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.25,-0.93);
		glVertex2f(-0.25,-0.94);
		glVertex2f(0.25,-0.94);
		glVertex2f(0.25,-0.93);
	glEnd();

	PutLabels(-vector3d(offset_x, offset_y, 0.0));

	Gui::Screen::EnterOrtho();
	glTranslatef(500,10,0.0);
	Gui::Screen::RenderString(stringf(128, "%d ly", (int)(0.5*Galaxy::GALAXY_RADIUS/m_zoom)));
	Gui::Screen::LeaveOrtho();

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}
	
void GalacticView::Update()
{
	const float frameTime = Pi::GetFrameTime();
	
	if (m_zoomInButton->IsPressed()) m_zoom *= pow(4.0f, frameTime);
	if (m_zoomOutButton->IsPressed()) m_zoom *= pow(0.25f, frameTime);
	if (Pi::KeyState(SDLK_EQUALS)) m_zoom *= pow(4.0f, frameTime);
	if (Pi::KeyState(SDLK_MINUS)) m_zoom *= pow(0.25f, frameTime);
	m_zoom = CLAMP(m_zoom, 0.5, 100.0);
}

void GalacticView::MouseButtonDown(int button, int x, int y)
{
	const float ft = Pi::GetFrameTime();
	if (Pi::MouseButtonState(SDL_BUTTON_WHEELDOWN)) 
			m_zoom *= pow(0.25f, ft);
	if (Pi::MouseButtonState(SDL_BUTTON_WHEELUP)) 
			m_zoom *= pow(4.0f, ft);
}

