/*
 * Copyright (C) 2012 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <sstream>

#include <ignition/math/Helpers.hh>
#include <ignition/math/Pose3.hh>
#include <ignition/math/Vector3.hh>

#ifndef _WIN32
  #include <dirent.h>
#else
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
  #include "gazebo/common/win_dirent.h"
#endif

#include "gazebo/rendering/ogre_gazebo.h"

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Events.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Mesh.hh"
#include "gazebo/common/MeshManager.hh"
#include "gazebo/common/Timer.hh"

#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/Conversions.hh"
#include "gazebo/rendering/Scene.hh"
#include <gazebo_ros_sonar_plugin/Sonar.hh>
#include <gazebo_ros_sonar_plugin/SonarPrivate.hh>

using namespace gazebo;
using namespace rendering;

int SonarPrivate::texCount = 0;

//////////////////////////////////////////////////
Sonar::Sonar(const std::string &_namePrefix, ScenePtr _scene,
                   const bool _autoRender)
: Camera(_namePrefix, _scene, _autoRender),
  dataPtr(new SonarPrivate)
{
  this->dataPtr->laserBuffer = NULL;
  this->dataPtr->laserScan = NULL;
  this->dataPtr->matFirstPass = NULL;
  this->dataPtr->matSecondPass = NULL;
  for (int i = 0; i < 3; ++i)
    this->dataPtr->firstPassTextures[i] = NULL;
  this->dataPtr->secondPassTexture = NULL;
  this->dataPtr->orthoCam = NULL;
  this->dataPtr->w2nd = 0;
  this->dataPtr->h2nd = 0;
  this->cameraCount = 0;
  this->dataPtr->textureCount = 0;
  this->dataPtr->visual.reset();
}

//////////////////////////////////////////////////
Sonar::~Sonar()
{
  delete [] this->dataPtr->laserBuffer;
  delete [] this->dataPtr->laserScan;

  for (unsigned int i = 0; i < this->dataPtr->textureCount; ++i)
  {
    if (this->dataPtr->firstPassTextures[i])
    {
      Ogre::TextureManager::getSingleton().remove(
          this->dataPtr->firstPassTextures[i]->getName());
    }
  }
  if (this->dataPtr->secondPassTexture)
  {
    Ogre::TextureManager::getSingleton().remove(
        this->dataPtr->secondPassTexture->getName());
  }

  if (this->scene && this->dataPtr->orthoCam)
    this->scene->OgreSceneManager()->destroyCamera(this->dataPtr->orthoCam);

  this->dataPtr->visual.reset();
  this->dataPtr->texIdx.clear();
  this->dataPtr->texCount = 0;
}

//////////////////////////////////////////////////
void Sonar::Load(sdf::ElementPtr _sdf)
{
  Camera::Load(_sdf);

}

//////////////////////////////////////////////////
void Sonar::Load()
{
  Camera::Load();
  this->captureData=true;
}

//////////////////////////////////////////////////
void Sonar::Init()
{
  Camera::Init();
  this->dataPtr->visual.reset(new Visual(this->Name()+"second_pass_canvas",
     this->GetScene()->WorldVisual()));
}

//////////////////////////////////////////////////
void Sonar::Fini()
{
  Camera::Fini();
}

//////////////////////////////////////////////////
void Sonar::CreateLaserTexture(const std::string &_textureName)
{
  this->camera->yaw(Ogre::Radian(this->horzHalfAngle));

  this->CreateOrthoCam();

  this->dataPtr->textureCount = this->cameraCount;

  if (this->dataPtr->textureCount == 2)
  {
    this->dataPtr->cameraYaws[0] = -this->hfov/2;
    this->dataPtr->cameraYaws[1] = +this->hfov;
    this->dataPtr->cameraYaws[2] = 0;
    this->dataPtr->cameraYaws[3] = -this->hfov/2;
  }
  else
  {
    this->dataPtr->cameraYaws[0] = -this->hfov;
    this->dataPtr->cameraYaws[1] = +this->hfov;
    this->dataPtr->cameraYaws[2] = +this->hfov;
    this->dataPtr->cameraYaws[3] = -this->hfov;
  }

  for (unsigned int i = 0; i < this->dataPtr->textureCount; ++i)
  {
    std::stringstream texName;
    texName << _textureName << "first_pass_" << i;
    this->dataPtr->firstPassTextures[i] =
      Ogre::TextureManager::getSingleton().createManual(
      texName.str(), "General", Ogre::TEX_TYPE_2D,
      this->ImageWidth(), this->ImageHeight(), 0,
      Ogre::PF_FLOAT32_RGB, Ogre::TU_RENDERTARGET).getPointer();

    this->Set1stPassTarget(
        this->dataPtr->firstPassTextures[i]->getBuffer()->getRenderTarget(), i);
<<<<<<< HEAD
>>>>>>> 3db1d56... Modification of main class to sonar
=======
>>>>>>> 7c8862b... 2

    this->dataPtr->firstPassTargets[i]->setAutoUpdated(false);
  }

  this->dataPtr->matFirstPass = (Ogre::Material*)(
  Ogre::MaterialManager::getSingleton().getByName("Gazebo/LaserScan1st").get());


  this->dataPtr->matFirstPass->load();
  this->dataPtr->matFirstPass->setCullingMode(Ogre::CULL_NONE);

  this->dataPtr->secondPassTexture =
      Ogre::TextureManager::getSingleton().createManual(
      _textureName + "second_pass",
      "General",
      Ogre::TEX_TYPE_2D,
      this->dataPtr->w2nd, this->dataPtr->h2nd, 0,
      Ogre::PF_FLOAT32_RGB,
      Ogre::TU_RENDERTARGET).getPointer();

  this->Set2ndPassTarget(
      this->dataPtr->secondPassTexture->getBuffer()->getRenderTarget());

  this->dataPtr->secondPassTarget->setAutoUpdated(false);

  this->dataPtr->matSecondPass = (Ogre::Material*)(
  Ogre::MaterialManager::getSingleton().getByName("Gazebo/LaserScan2nd").get());

  this->dataPtr->matSecondPass->load();

  Ogre::TextureUnitState *texUnit;
  for (unsigned int i = 0; i < this->dataPtr->textureCount; ++i)
  {
    unsigned int texIndex = this->dataPtr->texCount++;
    Ogre::Technique *technique = this->dataPtr->matSecondPass->getTechnique(0);
    GZ_ASSERT(technique, "Sonar material script error: technique not found");

    Ogre::Pass *pass = technique->getPass(0);
    GZ_ASSERT(pass, "Sonar material script error: pass not found");

    if (!pass->getTextureUnitState(
        this->dataPtr->firstPassTextures[i]->getName()))
    {
      texUnit = pass->createTextureUnitState(
            this->dataPtr->firstPassTextures[i]->getName(), texIndex);

      this->dataPtr->texIdx.push_back(texIndex);

      texUnit->setTextureFiltering(Ogre::TFO_NONE);
      texUnit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_MIRROR);
    }
  }

  this->CreateCanvas();
  this->newData = true;
<<<<<<< HEAD
>>>>>>> 3db1d56... Modification of main class to sonar
=======


  MyCamTex = Ogre::TextureManager::getSingleton().createManual(
    "RttTex", 
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
    Ogre::TEX_TYPE_2D, 
    720, 720, 
    0, 
    Ogre::PF_R8G8B8 , 
    Ogre::TU_RENDERTARGET).getPointer();
  this->MyCamTarget = MyCamTex->getBuffer()->getRenderTarget();
  this->MyCamTarget->addViewport(this->MyCam);
  this->MyCamTarget->getViewport(0)->setClearEveryFrame(true);
  this->MyCamTarget->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Black);
  this->MyCamTarget->getViewport(0)->setOverlaysEnabled(false);
  this->MyCamTarget->getViewport(0)->setShadowsEnabled(false);
  this->MyCamTarget->getViewport(0)->setSkiesEnabled(false);
  this->MyCamTarget->getViewport(0)->setVisibilityMask(GZ_VISIBILITY_ALL  & ~(GZ_VISIBILITY_GUI | GZ_VISIBILITY_SELECTABLE));

  this->MyCamMaterial = (Ogre::Material*)(
    Ogre::MaterialManager::getSingleton().getByName("GazeboRosSonar/NormalDepthMap").get());
  this->MyCamMaterial->load();

  {Ogre::Technique *technique = this->MyCamMaterial->getTechnique(0);
  GZ_ASSERT(technique, "Sonar material script error: technique not found");

  Ogre::Pass *pass = technique->getPass(0);
  GZ_ASSERT(pass, "Sonar material script error: pass not found");
  GZ_ASSERT(pass->hasVertexProgram(),"Must have vertex program");
  GZ_ASSERT(pass->hasFragmentProgram(),"Must have vertex program");}
}

void Sonar::PrintToFile(float _width,int _height, Ogre::Texture* _inTex)
{
  Ogre::HardwarePixelBufferSharedPtr pixelBuffer;
  pixelBuffer = _inTex->getBuffer();
  size_t size = Ogre::PixelUtil::getMemorySize(
                    _width, _height, 1, Ogre::PF_FLOAT32_RGB);

  uint8_t *imageBuffer = new uint8_t[size];
  memset(imageBuffer, 255, size);
  Ogre::PixelBox dstBox(_width, _height,
    1, Ogre::PF_R8G8B8 , imageBuffer);


  pixelBuffer->blitToMemory(dstBox);

  FILE* imageFile;
  imageFile = fopen("Teste2.date","w");
  for (int i = 0 ; i < _width*_height;i++)
  {
    fprintf(imageFile,"%u : %u %u %u \n",i,imageBuffer[i*3],
      imageBuffer[i*3+1],imageBuffer[i*3+2]);
  }
  fclose(imageFile);

>>>>>>> 7c8862b... 2
}

//////////////////////////////////////////////////
void Sonar::PostRender()
{
  for (unsigned int i = 0; i < this->dataPtr->textureCount; ++i)
  {
    this->dataPtr->firstPassTargets[i]->swapBuffers();
  }

  this->dataPtr->secondPassTarget->swapBuffers();

  //if (this->newData && this->captureData)
  {
    Ogre::HardwarePixelBufferSharedPtr pixelBuffer;
    Ogre::HardwarePixelBufferSharedPtr pixelBufferF0;

    unsigned int width = this->dataPtr->secondPassViewport->getActualWidth();
    unsigned int height = this->dataPtr->secondPassViewport->getActualHeight();

    unsigned int widthF0 = this->dataPtr->firstPassViewports[0]->getActualWidth();
    unsigned int heightF0 = this->dataPtr->firstPassViewports[0]->getActualHeight();

    // Get access to the buffer and make an image and write it to file
    pixelBuffer = this->dataPtr->secondPassTexture->getBuffer();
    pixelBufferF0 = this->dataPtr->firstPassTextures[0]->getBuffer();

    size_t size = Ogre::PixelUtil::getMemorySize(
                    width, height, 1, Ogre::PF_FLOAT32_RGB);
    size_t sizeF0 = Ogre::PixelUtil::getMemorySize(
                    widthF0, heightF0, 1, Ogre::PF_FLOAT32_RGB);

    
    // Blit the depth buffer if needed
    if (!this->dataPtr->laserBuffer)
      this->dataPtr->laserBuffer = new float[size];

    memset(this->dataPtr->laserBuffer, 255, size);

    Ogre::PixelBox dstBox(width, height,
        1, Ogre::PF_FLOAT32_RGB, this->dataPtr->laserBuffer);

    pixelBuffer->blitToMemory(dstBox);

    // Fist Blit
    float *laserBufferF0 = new float[sizeF0];
    memset(laserBufferF0, 255, size);
    Ogre::PixelBox dstBoxF0(widthF0, heightF0,
        1, Ogre::PF_FLOAT32_RGB, laserBufferF0);


    pixelBufferF0->blitToMemory(dstBoxF0);

    FILE* imageFile;
    imageFile = fopen("Teste.date","w");
    for (int i = 0 ; i < widthF0*heightF0;i++)
    {
      fprintf(imageFile,"%u : %f %f %f \n",i,laserBufferF0[i*3],
        laserBufferF0[i*3+1],laserBufferF0[i*3+2]);
    }
    fclose(imageFile);



    // Setup Image with correct settings
    // gzwarn << "Texture size " << size << std::endl;
    // Ogre::Image* img = new Ogre::Image();
    // // fill array
    // img->loadDynamicImage( reinterpret_cast<Ogre::uchar*>(laserBufferF0), widthF0, heightF0, 1, Ogre::PF_FLOAT32_RGBA, true );
    // img->save("test.png");

    
    Ogre::Image img;
    this->dataPtr->firstPassTextures[0]->convertToImage(img);    
    img.save("test.png");


    if (!this->dataPtr->laserScan)
    {
      int len = this->dataPtr->w2nd * this->dataPtr->h2nd * 3;
      this->dataPtr->laserScan = new float[len];
    }

    memcpy(this->dataPtr->laserScan, this->dataPtr->laserBuffer,
           this->dataPtr->w2nd * this->dataPtr->h2nd * 3 *
           sizeof(this->dataPtr->laserScan[0]));

    this->dataPtr->newLaserFrame(this->dataPtr->laserScan, this->dataPtr->w2nd,
        this->dataPtr->h2nd, 3, "BLABLA");
  }

  this->newData = false;
  // gzwarn << "--------------------Starte-----------------" << std::endl;
  //this->GetScene()->PrintSceneGraph();
  // gzwarn << "---------------------------ENDED--------------" << std::endl;
}

/////////////////////////////////////////////////
void Sonar::UpdateRenderTarget(Ogre::RenderTarget *_target,
                   Ogre::Material *_material, Ogre::Camera *_cam,
                   const bool _updateTex)
{
  Ogre::RenderSystem *renderSys;
  Ogre::Viewport *vp = NULL;
  Ogre::SceneManager *sceneMgr = this->scene->OgreSceneManager();
  Ogre::Pass *pass;

  renderSys = this->scene->OgreSceneManager()->getDestinationRenderSystem();
  // Get pointer to the material pass
  pass = _material->getBestTechnique()->getPass(0);

  // Render the depth texture
  // OgreSceneManager::_render function automatically sets farClip to 0.
  // Which normally equates to infinite distance. We don't want this. So
  // we have to set the distance every time.
  _cam->setFarClipDistance(this->FarClip());

  Ogre::AutoParamDataSource autoParamDataSource;

  vp = _target->getViewport(0);

  // Need this line to render the ground plane. No idea why it's necessary.
  renderSys->_setViewport(vp);
  sceneMgr->_setPass(pass, true, false);
  autoParamDataSource.setCurrentPass(pass);
  autoParamDataSource.setCurrentViewport(vp);
  autoParamDataSource.setCurrentRenderTarget(_target);
  autoParamDataSource.setCurrentSceneManager(sceneMgr);
  autoParamDataSource.setCurrentCamera(_cam, true);

  renderSys->setLightingEnabled(false);
  renderSys->_setFog(Ogre::FOG_NONE);

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
  pass->_updateAutoParamsNoLights(&autoParamDataSource);
#else
  pass->_updateAutoParams(&autoParamDataSource, 1);
#endif

  if (_updateTex)
  {
    pass->getFragmentProgramParameters()->setNamedConstant("tex1",
      this->dataPtr->texIdx[0]);
    if (this->dataPtr->texIdx.size() > 1)
    {
      pass->getFragmentProgramParameters()->setNamedConstant("tex2",
        this->dataPtr->texIdx[1]);
      if (this->dataPtr->texIdx.size() > 2)
        pass->getFragmentProgramParameters()->setNamedConstant("tex3",
          this->dataPtr->texIdx[2]);
    }
  }

  // NOTE: We MUST bind parameters AFTER updating the autos
  if (pass->hasVertexProgram())
  {
    renderSys->bindGpuProgram(
        pass->getVertexProgram()->_getBindingDelegate());

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
    renderSys->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
    pass->getVertexProgramParameters());
#else
    renderSys->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
      pass->getVertexProgramParameters(), 1);
#endif
  }

  if (pass->hasFragmentProgram())
  {
    renderSys->bindGpuProgram(
    pass->getFragmentProgram()->_getBindingDelegate());

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
    renderSys->bindGpuProgramParameters(Ogre::GPT_FRAGMENT_PROGRAM,
    pass->getFragmentProgramParameters());
#else
      renderSys->bindGpuProgramParameters(Ogre::GPT_FRAGMENT_PROGRAM,
      pass->getFragmentProgramParameters(), 1);
#endif
  }
}

/////////////////////////////////////////////////
void Sonar::notifyRenderSingleObject(Ogre::Renderable *_rend,
      const Ogre::Pass* /*pass*/, const Ogre::AutoParamDataSource* /*source*/,
      const Ogre::LightList* /*lights*/, bool /*supp*/)
{
  Ogre::Vector4 retro = Ogre::Vector4(0, 0, 0, 0);
  try
  {
    _rend->setCustomParameter(1, Ogre::Vector4(0, 0, 0, 0));//retro = _rend->getCustomParameter(1);
  }
  catch(Ogre::ItemIdentityException& e)
  {
    _rend->setCustomParameter(1, Ogre::Vector4(0, 0, 0, 0));
  }

  Ogre::Pass *pass = this->MyCamMaterial->getBestTechnique()->getPass(0);
  // Ogre::Pass *pass = this->dataPtr->currentMat->getBestTechnique()->getPass(0);

  Ogre::RenderSystem *renderSys =
                  this->scene->OgreSceneManager()->getDestinationRenderSystem();

  Ogre::AutoParamDataSource autoParamDataSource;

  // Ogre::Viewport *vp = this->dataPtr->currentTarget->getViewport(0);
  Ogre::Viewport *vp = this->MyCamTarget->getViewport(0);

  renderSys->_setViewport(vp);
  autoParamDataSource.setCurrentRenderable(_rend);
  autoParamDataSource.setCurrentPass(pass);
  autoParamDataSource.setCurrentViewport(vp);
  // autoParamDataSource.setCurrentRenderTarget(this->dataPtr->currentTarget);
  autoParamDataSource.setCurrentRenderTarget(this->MyCamTarget);
  autoParamDataSource.setCurrentSceneManager(this->scene->OgreSceneManager());
  autoParamDataSource.setCurrentCamera(this->camera, true);

  pass->_updateAutoParams(&autoParamDataSource,
      Ogre::GPV_GLOBAL || Ogre::GPV_PER_OBJECT);
  // pass->getFragmentProgramParameters()->setNamedConstant("retro", retro[0]);

  pass->getFragmentProgramParameters()->setNamedConstant("farPlane", float(5.0f));
  pass->getFragmentProgramParameters()->setNamedConstant("drawNormal", int(1));
  pass->getFragmentProgramParameters()->setNamedConstant("drawDepth", int(1));
  pass->getFragmentProgramParameters()->setNamedConstant("reflectance", 1.0f);
  pass->getFragmentProgramParameters()->setNamedConstant("attenuationCoeff", 0.0f);

  renderSys->bindGpuProgram(
      pass->getVertexProgram()->_getBindingDelegate());

  renderSys->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
      pass->getVertexProgramParameters(),
      Ogre::GPV_GLOBAL || Ogre::GPV_PER_OBJECT);

  renderSys->bindGpuProgram(
      pass->getFragmentProgram()->_getBindingDelegate());

  renderSys->bindGpuProgramParameters(Ogre::GPT_FRAGMENT_PROGRAM,
      pass->getFragmentProgramParameters(),
      Ogre::GPV_GLOBAL || Ogre::GPV_PER_OBJECT);
}

//////////////////////////////////////////////////
void Sonar::RenderImpl()
{
  common::Timer firstPassTimer, secondPassTimer;

  firstPassTimer.Start();

  Ogre::SceneManager *sceneMgr = this->scene->OgreSceneManager();

  sceneMgr->_suppressRenderStateChanges(true);
  sceneMgr->addRenderObjectListener(this);

  for (unsigned int i = 0; i < this->dataPtr->textureCount; ++i)
  {
    if (this->dataPtr->textureCount > 1)
    {
      // Cannot call Camera::RotateYaw because it rotates in world frame,
      // but we need rotation in camera local frame
      this->sceneNode->roll(Ogre::Radian(this->dataPtr->cameraYaws[i]));
    }

    this->dataPtr->currentMat = this->dataPtr->matFirstPass;
    this->dataPtr->currentTarget = this->dataPtr->firstPassTargets[i];

    this->UpdateRenderTarget(this->dataPtr->firstPassTargets[i],
                  this->dataPtr->matFirstPass, this->camera);
    this->dataPtr->firstPassTargets[i]->update(false);
  }

  gzwarn << " Camera Yaw " << this->dataPtr->cameraYaws[3] << std::endl;
  if (this->dataPtr->textureCount > 1)
      this->sceneNode->roll(Ogre::Radian(this->dataPtr->cameraYaws[3]));

  sceneMgr->removeRenderObjectListener(this);


  double firstPassDur = firstPassTimer.GetElapsed().Double();
  secondPassTimer.Start();

  this->dataPtr->visual->SetVisible(true);

  this->UpdateRenderTarget(this->dataPtr->secondPassTarget,
                this->dataPtr->matSecondPass, this->dataPtr->orthoCam, true);
  this->dataPtr->secondPassTarget->update(false);

  this->dataPtr->visual->SetVisible(false);

  sceneMgr->_suppressRenderStateChanges(false);

  double secondPassDur = secondPassTimer.GetElapsed().Double();
  

  this->dataPtr->lastRenderDuration = firstPassDur + secondPassDur;

  // this->GetScene()->WorldVisual()->SetVisible(true);
  // this->GetScene()->WorldVisual()->GetSceneNode()->setVisible(true,true);
  // this->scene->SetVisible("default",true);

  // gzwarn<< "Is visible: " << this->GetScene()->WorldVisual()->GetVisible() << std::endl;
  // this->GetScene()->WorldVisual()->ShowInertia(true);
  // gzwarn << "Scene Name " << this->scene->Name() << std::endl;


  sceneMgr->addRenderObjectListener(this);
  this->UpdateRenderTarget(this->MyCamTarget,
                  this->MyCamMaterial, this->MyCam,false);
  // this->MyCamTex->getBuffer()->getRenderTarget()->update(false);
  // Ogre::RenderTexture* renderTexture = MyCamTex->getBuffer()->getRenderTarget();
  this->MyCamTarget->update();
  this->MyCamTarget->writeContentsToFile("start.png");
  Ogre::Image img;
  this->MyCamTex->convertToImage(img);    
  img.save("MyCamtest.png");
  this->PrintToFile(720,720, this->MyCamTex);
  sceneMgr->removeRenderObjectListener(this);


}

//////////////////////////////////////////////////
const float* Sonar::GetLaserData()
{
  return this->LaserData();
}

//////////////////////////////////////////////////
const float* Sonar::LaserData() const
{
  return this->dataPtr->laserBuffer;
}

/////////////////////////////////////////////////
void Sonar::CreateOrthoCam()
{
  this->dataPtr->pitchNodeOrtho =
    this->GetScene()->WorldVisual()->GetSceneNode()->createChildSceneNode();

  this->dataPtr->orthoCam = this->scene->OgreSceneManager()->createCamera(
        this->dataPtr->pitchNodeOrtho->getName() + "_ortho_cam");

  // Use X/Y as horizon, Z up
  this->dataPtr->orthoCam->pitch(Ogre::Degree(90));

  // Don't yaw along variable axis, causes leaning
  this->dataPtr->orthoCam->setFixedYawAxis(true, Ogre::Vector3::UNIT_Z);

  this->dataPtr->orthoCam->setDirection(1, 0, 0);

  this->dataPtr->pitchNodeOrtho->attachObject(this->dataPtr->orthoCam);
  this->dataPtr->orthoCam->setAutoAspectRatio(true);

  if (this->dataPtr->orthoCam)
  {
    this->dataPtr->orthoCam->setNearClipDistance(0.01);
    this->dataPtr->orthoCam->setFarClipDistance(0.02);
    this->dataPtr->orthoCam->setRenderingDistance(0.02);

    this->dataPtr->orthoCam->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
  }
}

void Sonar::CreateMyCam()
{


  this->MyCamNode = this->GetScene()->WorldVisual()->GetSceneNode()->createChildSceneNode("my_sonar");
  
  

  //this->MyCamNode = this->scene->OgreSceneManager()->getSceneNode("car_sonar");
  this->MyCam = this->scene->OgreSceneManager()->createCamera("myCam");
  this->MyCamNode->setPosition(2, 2, 2);
  this->MyCamNode->lookAt(Ogre::Vector3(0, 0, 1), Ogre::Node::TransformSpace::TS_WORLD);
  this->MyCam->setNearClipDistance(0.01);
  this->MyCamNode->attachObject(this->MyCam);

  //this->scene->OgreSceneManager()->setDisplaySceneNodes(true);


  //common::MeshManager::Instance()->CreateSphere("contact_sphere", 0.5, 10, 10);

  //Add the mesh into OGRE
  // if (!this->MyCamNode->getCreator()->hasEntity("contact_sphere") &&
  //     common::MeshManager::Instance()->HasMesh("contact_sphere"))
  // {
  //   const common::Mesh *mesh =
  //     common::MeshManager::Instance()->GetMesh("contact_sphere");
  //   this->GetScene()->WorldVisual()->InsertMesh(mesh);
  // }

  // Ogre::Entity *obj = this->scene->OgreSceneManager()->createEntity(
  //                   "VISUAL__bal", "contact_sphere");


  // obj->setMaterialName("Gazebo/Blue");
  // obj->setVisibilityFlags(GZ_VISIBILITY_ALL);

  // this->MyCamNode->attachObject(obj);
  this->MyCamNode->setVisible(true);

}

/////////////////////////////////////////////////
Ogre::Matrix4 Sonar::BuildScaledOrthoMatrix(const float _left,
    const float _right, const float _bottom, const float _top,
    const float _near, const float _far)
{
  float invw = 1 / (_right - _left);
  float invh = 1 / (_top - _bottom);
  float invd = 1 / (_far - _near);

  Ogre::Matrix4 proj = Ogre::Matrix4::ZERO;
  proj[0][0] = 2 * invw;
  proj[0][3] = -(_right + _left) * invw;
  proj[1][1] = 2 * invh;
  proj[1][3] = -(_top + _bottom) * invh;
  proj[2][2] = -2 * invd;
  proj[2][3] = -(_far + _near) * invd;
  proj[3][3] = 1;

  return proj;
}

//////////////////////////////////////////////////
void Sonar::Set1stPassTarget(Ogre::RenderTarget *_target,
                                const unsigned int _index)
{
  this->dataPtr->firstPassTargets[_index] = _target;

  if (this->dataPtr->firstPassTargets[_index])
  {
    // Setup the viewport to use the texture
    this->dataPtr->firstPassViewports[_index] =
      this->dataPtr->firstPassTargets[_index]->addViewport(this->camera);
    this->dataPtr->firstPassViewports[_index]->setClearEveryFrame(true);
    this->dataPtr->firstPassViewports[_index]->setOverlaysEnabled(false);
    this->dataPtr->firstPassViewports[_index]->setShadowsEnabled(false);
    this->dataPtr->firstPassViewports[_index]->setSkiesEnabled(false);
    this->dataPtr->firstPassViewports[_index]->setBackgroundColour(
        Ogre::ColourValue(this->farClip, 0.0, 1.0));
    this->dataPtr->firstPassViewports[_index]->setVisibilityMask(
        GZ_VISIBILITY_ALL );
  }

  if (_index == 0)
  {
    this->camera->setAspectRatio(this->RayCountRatio());
    this->camera->setFOVy(Ogre::Radian(this->CosVertFOV()));
  }
}

//////////////////////////////////////////////////
void Sonar::Set2ndPassTarget(Ogre::RenderTarget *_target)
{
  this->dataPtr->secondPassTarget = _target;

  if (this->dataPtr->secondPassTarget)
  {
    // Setup the viewport to use the texture
    this->dataPtr->secondPassViewport =
        this->dataPtr->secondPassTarget->addViewport(this->dataPtr->orthoCam);
    this->dataPtr->secondPassViewport->setClearEveryFrame(true);
    this->dataPtr->secondPassViewport->setOverlaysEnabled(false);
    this->dataPtr->secondPassViewport->setShadowsEnabled(false);
    this->dataPtr->secondPassViewport->setSkiesEnabled(false);
    this->dataPtr->secondPassViewport->setBackgroundColour(
        Ogre::ColourValue(0.0, 1.0, 0.0));
    this->dataPtr->secondPassViewport->setVisibilityMask(
        GZ_VISIBILITY_ALL & ~(GZ_VISIBILITY_GUI | GZ_VISIBILITY_SELECTABLE));
  }
  Ogre::Matrix4 p = this->BuildScaledOrthoMatrix(
      0, static_cast<float>(this->dataPtr->w2nd / 10.0),
      0, static_cast<float>(this->dataPtr->h2nd / 10.0),
      0.01, 0.02);

  this->dataPtr->orthoCam->setCustomProjectionMatrix(true, p);
}

/////////////////////////////////////////////////
void Sonar::SetRangeCount(const unsigned int _w, const unsigned int _h)
{
  this->dataPtr->w2nd = _w;
  this->dataPtr->h2nd = _h;
}

/////////////////////////////////////////////////
void Sonar::CreateMesh()
{
  std::string meshName = this->Name() + "_undistortion_mesh";

  common::Mesh *mesh = new common::Mesh();
  mesh->SetName(meshName);

  common::SubMesh *submesh = new common::SubMesh();

  double dx, dy;
  submesh->SetPrimitiveType(common::SubMesh::POINTS);

  if (this->dataPtr->h2nd == 1)
  {
    dy = 0;
  }
  else
  {
    dy = 0.1;
  }

  dx = 0.1;

  // startX ranges from 0 to -(w2nd/10) at dx=0.1 increments
  // startY ranges from h2nd/10 to 0 at dy=0.1 decrements
  // see Sonar::Set2ndPassTarget() on how the ortho cam is set up
  double startX = dx;
  double startY = this->dataPtr->h2nd/10.0;

  // half of actual camera vertical FOV without padding
  double phi = this->VertFOV() / 2;
  double phiCamera = phi + std::abs(this->VertHalfAngle());
  double theta = this->CosHorzFOV() / 2;

  if (this->ImageHeight() == 1)
  {
    phi = 0;
  }

  // index of ray
  unsigned int ptsOnLine = 0;

  // total laser hfov
  double thfov = this->dataPtr->textureCount * this->CosHorzFOV();
  double hstep = thfov / (this->dataPtr->w2nd - 1);
  double vstep = 2 * phi / (this->dataPtr->h2nd - 1);

  if (this->dataPtr->h2nd == 1)
  {
    vstep = 0;
  }

  for (unsigned int j = 0; j < this->dataPtr->h2nd; ++j)
  {
    double gamma = 0;
    if (this->dataPtr->h2nd != 1)
    {
      // gamma: current vertical angle w.r.t. camera
      gamma = vstep * j - phi + this->VertHalfAngle();
    }

    for (unsigned int i = 0; i < this->dataPtr->w2nd; ++i)
    {
      // current horizontal angle from start of laser scan
      double delta = hstep * i;

      // index of texture that contains the depth value
      unsigned int texture = delta / this->CosHorzFOV();

      // cap texture index and horizontal angle
      if (texture > this->dataPtr->textureCount-1)
      {
        texture -= 1;
        delta -= hstep;
      }

      startX -= dx;
      if (ptsOnLine == this->dataPtr->w2nd)
      {
        ptsOnLine = 0;
        startX = 0;
        startY -= dy;
      }
      ptsOnLine++;

      // the texture/1000.0 value is used in the laser_2nd_pass.frag shader
      // as a trick to determine which camera texture to use when stitching
      // together the final depth image.
      submesh->AddVertex(texture/1000.0, startX, startY);

      // first compute angle from the start of current camera's horizontal
      // min angle, then set delta to be angle from center of current camera.
      delta = delta - (texture * this->CosHorzFOV());
      delta = delta - theta;

      // adjust uv coordinates of depth texture to match projection of current
      // laser ray the depth image plane.
      double u = 0.5 - tan(delta) / (2.0 * tan(theta));
      double v = 0.5 - (tan(gamma) * cos(theta)) /
          (2.0 * tan(phiCamera) * cos(delta));

      submesh->AddTexCoord(u, v);
      submesh->AddIndex(this->dataPtr->w2nd * j + i);
    }
  }

  mesh->AddSubMesh(submesh);

  this->dataPtr->undistMesh = mesh;

  common::MeshManager::Instance()->AddMesh(this->dataPtr->undistMesh);
}

/////////////////////////////////////////////////
void Sonar::CreateCanvas()
{
  this->CreateMesh();

  Ogre::Node *parent = this->dataPtr->visual->GetSceneNode()->getParent();
  parent->removeChild(this->dataPtr->visual->GetSceneNode());

  this->dataPtr->pitchNodeOrtho->addChild(
      this->dataPtr->visual->GetSceneNode());

  this->dataPtr->visual->InsertMesh(this->dataPtr->undistMesh);

  std::ostringstream stream;
  std::string meshName = this->dataPtr->undistMesh->GetName();
  stream << this->dataPtr->visual->GetSceneNode()->getName()
      << "_ENTITY_" << meshName;

  this->dataPtr->object = (Ogre::MovableObject*)
      (this->dataPtr->visual->GetSceneNode()->getCreator()->createEntity(
      stream.str(), meshName));

  this->dataPtr->visual->AttachObject(this->dataPtr->object);
  this->dataPtr->object->setVisibilityFlags(GZ_VISIBILITY_ALL
  );

  ignition::math::Pose3d pose;
  pose.Pos() = ignition::math::Vector3d(0.01, 0, 0);
  pose.Rot().Euler(ignition::math::Vector3d(0, 0, 0));

  this->dataPtr->visual->SetPose(pose);

  this->dataPtr->visual->SetMaterial("Gazebo/Green");
  this->dataPtr->visual->SetAmbient(common::Color(0, 1, 0, 1));
  this->dataPtr->visual->SetVisible(true);
  this->scene->AddVisual(this->dataPtr->visual);
}

//////////////////////////////////////////////////
void Sonar::SetHorzHalfAngle(const double _angle)
{
  this->horzHalfAngle = _angle;
}

//////////////////////////////////////////////////
void Sonar::SetVertHalfAngle(const double _angle)
{
  this->vertHalfAngle = _angle;
}

//////////////////////////////////////////////////
double Sonar::GetHorzHalfAngle() const
{
  return this->HorzHalfAngle();
}

//////////////////////////////////////////////////
double Sonar::HorzHalfAngle() const
{
  return this->horzHalfAngle;
}

//////////////////////////////////////////////////
double Sonar::GetVertHalfAngle() const
{
  return this->VertHalfAngle();
}

//////////////////////////////////////////////////
double Sonar::VertHalfAngle() const
{
  return this->vertHalfAngle;
}

//////////////////////////////////////////////////
void Sonar::SetIsHorizontal(const bool _horizontal)
{
  this->isHorizontal = _horizontal;
}

//////////////////////////////////////////////////
bool Sonar::IsHorizontal() const
{
  return this->isHorizontal;
}

//////////////////////////////////////////////////
double Sonar::GetHorzFOV() const
{
  return this->HorzFOV();
}

//////////////////////////////////////////////////
double Sonar::HorzFOV() const
{
  return this->hfov;
}

//////////////////////////////////////////////////
double Sonar::GetVertFOV() const
{
  return this->VertFOV();
}

//////////////////////////////////////////////////
double Sonar::VertFOV() const
{
  return this->vfov;
}

//////////////////////////////////////////////////
void Sonar::SetHorzFOV(const double _hfov)
{
  this->hfov = _hfov;
}

//////////////////////////////////////////////////
void Sonar::SetVertFOV(const double _vfov)
{
  this->vfov = _vfov;
}

//////////////////////////////////////////////////
double Sonar::GetCosHorzFOV() const
{
  return this->CosHorzFOV();
}

//////////////////////////////////////////////////
double Sonar::CosHorzFOV() const
{
  return this->chfov;
}

//////////////////////////////////////////////////
void Sonar::SetCosHorzFOV(const double _chfov)
{
  this->chfov = _chfov;
}

//////////////////////////////////////////////////
double Sonar::GetCosVertFOV() const
{
  return this->CosVertFOV();
}

//////////////////////////////////////////////////
double Sonar::CosVertFOV() const
{
  return this->cvfov;
}

//////////////////////////////////////////////////
void Sonar::SetCosVertFOV(const double _cvfov)
{
  this->cvfov = _cvfov;
}

//////////////////////////////////////////////////
double Sonar::GetNearClip() const
{
  return this->NearClip();
}

//////////////////////////////////////////////////
double Sonar::NearClip() const
{
  return this->nearClip;
}

//////////////////////////////////////////////////
double Sonar::GetFarClip() const
{
  return this->FarClip();
}

//////////////////////////////////////////////////
double Sonar::FarClip() const
{
  return this->farClip;
}

//////////////////////////////////////////////////
void Sonar::SetNearClip(const double _near)
{
  this->nearClip = _near;
}

//////////////////////////////////////////////////
void Sonar::SetFarClip(const double _far)
{
  this->farClip = _far;
}

//////////////////////////////////////////////////
double Sonar::GetCameraCount() const
{
  return this->CameraCount();
}

//////////////////////////////////////////////////
unsigned int Sonar::CameraCount() const
{
  return this->cameraCount;
}

//////////////////////////////////////////////////
void Sonar::SetCameraCount(const unsigned int _cameraCount)
{
  this->cameraCount = _cameraCount;
}

//////////////////////////////////////////////////
double Sonar::GetRayCountRatio() const
{
  return this->RayCountRatio();
}

//////////////////////////////////////////////////
double Sonar::RayCountRatio() const
{
  return this->rayCountRatio;
}

//////////////////////////////////////////////////
void Sonar::SetRayCountRatio(const double _rayCountRatio)
{
  this->rayCountRatio = _rayCountRatio;
}

//////////////////////////////////////////////////
event::ConnectionPtr Sonar::ConnectNewLaserFrame(
    std::function<void (const float *_frame, unsigned int _width,
    unsigned int _height, unsigned int _depth,
    const std::string &_format)> _subscriber)
{
  return this->dataPtr->newLaserFrame.Connect(_subscriber);
}

//////////////////////////////////////////////////
void Sonar::DisconnectNewLaserFrame(event::ConnectionPtr &_c)
{
  this->dataPtr->newLaserFrame.Disconnect(_c);
}