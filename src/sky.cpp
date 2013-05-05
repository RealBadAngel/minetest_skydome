#include "sky.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "S3DVertex.h"
#include "tile.h" // getTexturePath
#include "noise.h" // easeCurve
#include "main.h" // g_profiler
#include "profiler.h"
#include "util/numeric.h" // MYMIN
#include <IAnimatedMeshSceneNode.h>
#include <IMeshManipulator.h>
#include <ITextSceneNode.h>
#include <IBillboardSceneNode.h>
#include <shader.h>
#include <gamedef.h>
#include <client.h>
#include <mesh.h>
#include <ITexture.h>

//! constructor
Sky::Sky(scene::ISceneNode* parent, scene::ISceneManager* mgr, s32 id, Client *client):
		scene::ISceneNode(parent, mgr, id),
		m_first_update(true),
		m_brightness(0.5),
		m_cloud_brightness(0.5),
		m_bgcolor_bright_f(1,1,1,1),
		m_skycolor_bright_f(1,1,1,1),
		m_cloudcolor_bright_f(1,1,1,1),
		m_sunpos(0,0,0),
		m_client(client)
{
	//setAutomaticCulling(scene::EAC_OFF);
	//Box.MaxEdge.set(0,0,0);
	//Box.MinEdge.set(0,0,0);
	
	//video::SMaterial mat;
	//mat.Lighting = false;
	//mat.ZBuffer = video::ECFN_NEVER;
	//mat.ZWriteEnable = false;
	//mat.AntiAliasing=0;
	//mat.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	//mat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	//mat.BackfaceCulling = false;
	
	scene::IAnimatedMesh *mesh;
	mesh = mgr->getMesh("sky_sphere.x");
	m_sky_sphere = mgr->addAnimatedMeshSceneNode(mesh, NULL);
	mesh->drop();
	m_sky_sphere->animateJoints();
	m_sky_sphere->setFrameLoop(1, 250);
	m_sky_sphere->setAnimationSpeed(0);
	m_sky_sphere->setPosition(v3f(0,400,0));
	m_sky_sphere->setScale(v3f(10,10,10));
	u8 li = 255;
	setMeshColor(m_sky_sphere->getMesh(), video::SColor(255,li,li,li));
	m_sky_sphere->setMaterialFlag(video::EMF_LIGHTING, false);
	m_sky_sphere->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	m_sky_sphere->setMaterialFlag(video::EMF_FOG_ENABLE, false);
	m_sky_sphere->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
	m_sky_sphere->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
	
	//video::E_MATERIAL_TYPE sky_shader = m_client->getShaderSource()->getShader("test_shader_4").material;
	//sky_sphere->setMaterialType(sky_shader);
	//sky_sphere->setMaterialTexture(0, mgr->getVideoDriver()->getTexture(getTexturePath("sky.png").c_str()));
	//sky_sphere->setMaterialTexture(1, mgr->getVideoDriver()->getTexture(getTexturePath("glow.png").c_str()));
	//sky_sphere->setMaterialTexture(2, mgr->getVideoDriver()->getTexture(getTexturePath("stars.jpg").c_str()));
	//sky_sphere->setMaterialTexture(3, mgr->getVideoDriver()->getTexture(getTexturePath("moon.png").c_str()));
	/*
	scene::IMesh *mesh1;
	scene::IMeshSceneNode *sky_dome;
	mesh1 = mgr->getMesh("sky_dome.3ds");
	sky_dome = mgr->addMeshSceneNode(mesh, NULL);
	li = 255;
	//setMeshColor(sky_dome->getMesh(), video::SColor(255,li,li,li));
	sky_dome->setPosition(v3f(0,100,0));
	sky_dome->setScale(v3f(10,10,10));
	
	sky_dome->setMaterialFlag(video::EMF_LIGHTING, true);
	sky_dome->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	sky_dome->setMaterialFlag(video::EMF_FOG_ENABLE, false);
	video::E_MATERIAL_TYPE sky_shader = m_client->getShaderSource()->getShader("test_shader_4").material;
	sky_dome->setMaterialType(sky_shader);
	*/
}

void Sky::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SKY_BOX);

	scene::ISceneNode::OnRegisterSceneNode();
}

const core::aabbox3d<f32>& Sky::getBoundingBox() const
{
	return Box;
}

//! renders the node.
void Sky::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;
	
	;

	ScopeProfiler sp(g_profiler, "Sky::render()", SPT_AVG);
	
	m_sky_sphere->setCurrentFrame ((int)m_time_of_day*250);
	m_sunpos.Y = sin((m_time_of_day/(2*3.14159)));
	// draw perspective skybox

	core::matrix4 translate(AbsoluteTransformation);
	translate.setTranslation(camera->getAbsolutePosition());

	// Draw the sky box between the near and far clip plane
	const f32 viewDistance = (camera->getNearValue() + camera->getFarValue()) * 0.5f;
	core::matrix4 scale;
	scale.setScale(core::vector3df(viewDistance, viewDistance, viewDistance));

	driver->setTransform(video::ETS_WORLD, translate * scale);

	if(m_sunlight_seen)
	{
		float sunsize = 0.07;
		video::SColorf suncolor_f(1, 1, 0, 1);
		suncolor_f.r = 1;
		suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.7+m_time_brightness*(0.5)));
		suncolor_f.b = MYMAX(0.0, m_brightness*0.95);
		video::SColorf suncolor2_f(1, 1, 1, 1);
		suncolor_f.r = 1;
		suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.85+m_time_brightness*(0.5)));
		suncolor_f.b = MYMAX(0.0, m_brightness);

		float moonsize = 0.04;
		video::SColorf mooncolor_f(0.50, 0.57, 0.65, 1);
		video::SColorf mooncolor2_f(0.85, 0.875, 0.9, 1);
		
		float nightlength = 0.415;
		float wn = nightlength / 2;
		float wicked_time_of_day = 0;
		if(m_time_of_day > wn && m_time_of_day < 1.0 - wn)
			wicked_time_of_day = (m_time_of_day - wn)/(1.0-wn*2)*0.5 + 0.25;
		else if(m_time_of_day < 0.5)
			wicked_time_of_day = m_time_of_day / wn * 0.25;
		else
			wicked_time_of_day = 1.0 - ((1.0-m_time_of_day) / wn * 0.25);
		/*std::cerr<<"time_of_day="<<m_time_of_day<<" -> "
				<<"wicked_time_of_day="<<wicked_time_of_day<<std::endl;*/

		video::SColor suncolor = suncolor_f.toSColor();
		video::SColor suncolor2 = suncolor2_f.toSColor();
		video::SColor mooncolor = mooncolor_f.toSColor();
		video::SColor mooncolor2 = mooncolor2_f.toSColor();

		const f32 t = 1.0f;
		const f32 o = 0.0f;
	}
}

void Sky::update(float time_of_day, float time_brightness,
		float direct_brightness, bool sunlight_seen)
{
	// Stabilize initial brightness and color values by flooding updates
	if(m_first_update){
		/*dstream<<"First update with time_of_day="<<time_of_day
				<<" time_brightness="<<time_brightness
				<<" direct_brightness="<<direct_brightness
				<<" sunlight_seen="<<sunlight_seen<<std::endl;*/
		m_first_update = false;
		for(u32 i=0; i<100; i++){
			update(time_of_day, time_brightness, direct_brightness,
					sunlight_seen);
		}
		return;
	}

	m_time_of_day = time_of_day;
	m_time_brightness = time_brightness;
	m_sunlight_seen = sunlight_seen;
	m_sunpos.Y = sin((time_of_day/(2*3.14159)));
	bool is_dawn = (time_brightness >= 0.20 && time_brightness < 0.35);

	//video::SColorf bgcolor_bright_normal_f(170./255,200./255,230./255, 1.0);
	video::SColorf bgcolor_bright_normal_f(155./255,193./255,240./255, 1.0);
	video::SColorf bgcolor_bright_indoor_f(100./255,100./255,100./255, 1.0);
	//video::SColorf bgcolor_bright_dawn_f(0.666,200./255*0.7,230./255*0.5,1.0);
	//video::SColorf bgcolor_bright_dawn_f(0.666,0.549,0.220,1.0);
	//video::SColorf bgcolor_bright_dawn_f(0.666*1.2,0.549*1.0,0.220*1.0, 1.0);
	//video::SColorf bgcolor_bright_dawn_f(0.666*1.2,0.549*1.0,0.220*1.2,1.0);
	video::SColorf bgcolor_bright_dawn_f
			(155./255*1.2,193./255,240./255, 1.0);

	video::SColorf skycolor_bright_normal_f =
			video::SColor(255, 140, 186, 250);
	video::SColorf skycolor_bright_dawn_f =
			video::SColor(255, 180, 186, 250);
	
	video::SColorf cloudcolor_bright_normal_f =
			video::SColor(255, 240,240,255);
	//video::SColorf cloudcolor_bright_dawn_f(1.0, 0.591, 0.4);
	//video::SColorf cloudcolor_bright_dawn_f(1.0, 0.65, 0.44);
	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.7, 0.5);

	if(sunlight_seen){
		//m_brightness = m_brightness * 0.95 + direct_brightness * 0.05;
		m_brightness = m_brightness * 0.95 + time_brightness * 0.05;
	}
	else{
		if(direct_brightness < m_brightness)
			m_brightness = m_brightness * 0.95 + direct_brightness * 0.05;
		else
			m_brightness = m_brightness * 0.98 + direct_brightness * 0.02;
	}
	
	m_clouds_visible = true;
	float color_change_fraction = 0.98;
	if(sunlight_seen){
		if(is_dawn){
			m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_dawn_f, color_change_fraction);
			m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_dawn_f, color_change_fraction);
			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
					cloudcolor_bright_dawn_f, color_change_fraction);
		} else {
			m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_normal_f, color_change_fraction);
			m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_normal_f, color_change_fraction);
			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
					cloudcolor_bright_normal_f, color_change_fraction);
		}
	} else {
		m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
				bgcolor_bright_indoor_f, color_change_fraction);
		m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
				cloudcolor_bright_normal_f, color_change_fraction);
		m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
				bgcolor_bright_indoor_f, color_change_fraction);
		m_clouds_visible = false;
	}
	video::SColor bgcolor_bright = m_bgcolor_bright_f.toSColor();
	m_bgcolor = video::SColor(
			255,
			bgcolor_bright.getRed() * m_brightness,
			bgcolor_bright.getGreen() * m_brightness,
			bgcolor_bright.getBlue() * m_brightness);
	
	video::SColor skycolor_bright = m_skycolor_bright_f.toSColor();
	m_skycolor = video::SColor(
			255,
			skycolor_bright.getRed() * m_brightness,
			skycolor_bright.getGreen() * m_brightness,
			skycolor_bright.getBlue() * m_brightness);
	
	float cloud_direct_brightness = 0;
	if(sunlight_seen){
		cloud_direct_brightness = time_brightness;
		if(time_brightness >= 0.2 && time_brightness < 0.7)
				cloud_direct_brightness *= 1.3;
	} else {
		cloud_direct_brightness = direct_brightness;
	}
	m_cloud_brightness = m_cloud_brightness * 0.95 +
			cloud_direct_brightness * (1.0 - 0.95);
	m_cloudcolor_f = video::SColorf(
			m_cloudcolor_bright_f.getRed() * m_cloud_brightness,
			m_cloudcolor_bright_f.getGreen() * m_cloud_brightness,
			m_cloudcolor_bright_f.getBlue() * m_cloud_brightness,
			1.0);

}


