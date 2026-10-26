// Copyright (C) 2026 TrenchBroom contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Isolated Bullet simulation adapted from ReyondBroom BrushSettle.
#pragma once
#include <btBulletDynamicsCommon.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>
namespace ftebroom::settle {
using Point=std::array<double,3>;
struct Body {Point center{};double volume=0;std::vector<Point> vertices;};
struct Options {double friction=.7,restitution=.1,kick=32,duration=8,bound=65536;uint32_t seed=1;};
struct Transform {std::array<Point,3> columns{};Point translation{};};
using Triangle=std::array<Point,3>;
constexpr double Scale=1.0/32.0;
inline btVector3 physics(const Point& p){return {btScalar(p[0]*Scale),btScalar(p[1]*Scale),btScalar(p[2]*Scale)};}
inline Point mapPoint(const btVector3& p){return {p.x()/Scale,p.y()/Scale,p.z()/Scale};}
inline Point transformPoint(const Transform& m,const Point& p){Point out=m.translation;for(int a=0;a<3;a++)for(int j=0;j<3;j++)out[j]+=m.columns[a][j]*p[a];return out;}
class Simulation {
 btDefaultCollisionConfiguration configuration;
 btCollisionDispatcher dispatcher{&configuration};btDbvtBroadphase broadphase;btSequentialImpulseConstraintSolver solver;
 btDiscreteDynamicsWorld world{&dispatcher,&broadphase,&solver,&configuration};
 std::unique_ptr<btTriangleMesh> mesh;
 std::vector<std::unique_ptr<btCollisionShape>> shapes;
 std::vector<std::unique_ptr<btRigidBody>> bodies;
 std::vector<Body> sources;Options options;
 void validatePoint(const Point& p){for(auto value:p)if(!std::isfinite(value)||std::abs(value)>options.bound)throw std::runtime_error("Settle point exceeds world bounds.");}
 void add(const Body& source,bool dynamic){
  if(source.vertices.size()<4||source.vertices.size()>4096||!std::isfinite(source.volume)||source.volume<=.001)throw std::runtime_error("Settle requires closed convex brushes.");
  validatePoint(source.center);auto shape=std::make_unique<btConvexHullShape>();shape->setMargin(btScalar(.02*Scale));
  for(auto p:source.vertices){validatePoint(p);for(int a=0;a<3;a++)p[a]-=source.center[a];shape->addPoint(physics(p),false);}shape->recalcLocalAabb();
  const auto mass=dynamic?btScalar(std::clamp(source.volume*Scale*Scale*Scale,.001,10000.0)):btScalar(0);
  btVector3 inertia(0,0,0);if(dynamic)shape->calculateLocalInertia(mass,inertia);
  btRigidBody::btRigidBodyConstructionInfo info(mass,nullptr,shape.get(),inertia);info.m_friction=options.friction;info.m_restitution=options.restitution;info.m_linearDamping=.12;info.m_angularDamping=.25;
  auto body=std::make_unique<btRigidBody>(info);auto pose=btTransform::getIdentity();pose.setOrigin(physics(source.center));body->setWorldTransform(pose);body->setSleepingThresholds(.02,.02);
  if(dynamic){btVector3 low,high;shape->getAabb(btTransform::getIdentity(),low,high);auto radius=btMax(btScalar(.001),btMin(high.x()-low.x(),btMin(high.y()-low.y(),high.z()-low.z()))*.2f);body->setCcdSweptSphereRadius(radius);body->setCcdMotionThreshold(radius*.5f);}
  shapes.push_back(std::move(shape));bodies.push_back(std::move(body));world.addRigidBody(bodies.back().get());
 }
 bool penetrating(){world.performDiscreteCollisionDetection();for(int i=0;i<dispatcher.getNumManifolds();i++){auto m=dispatcher.getManifoldByIndexInternal(i);if(m->getBody0()->isStaticObject()&&m->getBody1()->isStaticObject())continue;for(int c=0;c<m->getNumContacts();c++)if(m->getContactPoint(c).getDistance()<-.25*Scale)return true;}return false;}
public:
 Simulation(std::vector<Body> selected,const std::vector<Body>& obstacles,const std::vector<Triangle>& patches,Options settings):sources(std::move(selected)),options(settings){
  if(sources.empty()||sources.size()>128||obstacles.size()>4096||patches.size()>262144)throw std::runtime_error("Settle supports 1-128 selected brushes, 4096 obstacles and 262144 patch triangles.");
  for(double v:{options.friction,options.restitution,options.kick,options.duration,options.bound})if(!std::isfinite(v))throw std::runtime_error("Non-finite Settle settings.");
  if(options.friction<0||options.friction>1||options.restitution<0||options.restitution>1||options.kick<0||options.kick>256||options.duration<.1||options.duration>20||options.bound<=0||options.bound>1000000)throw std::runtime_error("Invalid Settle settings.");
  shapes.reserve(sources.size()+obstacles.size()+1);bodies.reserve(sources.size()+obstacles.size()+1);
  try {
  world.setGravity({0,0,btScalar(-800*Scale)});world.getSolverInfo().m_numIterations=40;world.getSolverInfo().m_splitImpulse=true;world.getSolverInfo().m_splitImpulsePenetrationThreshold=0;world.getSolverInfo().m_solverMode&=~SOLVER_RANDMIZE_ORDER;
  for(const auto& source:sources)add(source,true);for(const auto& obstacle:obstacles)add(obstacle,false);
  if(!patches.empty()){mesh=std::make_unique<btTriangleMesh>();for(const auto& triangle:patches){for(const auto& p:triangle)validatePoint(p);auto a=physics(triangle[0]),b=physics(triangle[1]),c=physics(triangle[2]);if((b-a).cross(c-a).length2()>1e-12)mesh->addTriangle(a,b,c);}
   if(mesh->getNumTriangles()){auto shape=std::make_unique<btBvhTriangleMeshShape>(mesh.get(),true);shape->setMargin(btScalar(.02*Scale));btRigidBody::btRigidBodyConstructionInfo info(0,nullptr,shape.get());info.m_friction=options.friction;info.m_restitution=options.restitution;auto body=std::make_unique<btRigidBody>(info);world.addRigidBody(body.get());shapes.push_back(std::move(shape));bodies.push_back(std::move(body));}}
  if(penetrating())throw std::runtime_error("Selected brushes overlap. Separate them before Settle.");
  std::mt19937 random(options.seed);auto unit=[&](){return 2.0*(double(random())+.5)/4294967296.0-1;};
  for(size_t i=0;i<sources.size();i++){double x=unit(),y=unit(),z=unit();bodies[i]->setLinearVelocity({btScalar(x*options.kick*Scale),btScalar(y*options.kick*Scale),btScalar(std::abs(z)*options.kick*Scale)});double ax=unit(),ay=unit(),az=unit();bodies[i]->setAngularVelocity({btScalar(ax*options.kick/32),btScalar(ay*options.kick/32),btScalar(az*options.kick/32)});}
  }catch(...){for(auto& body:bodies)world.removeRigidBody(body.get());throw;}
 }
 ~Simulation(){for(auto& body:bodies)world.removeRigidBody(body.get());}
 int movingCount()const{int count=0;for(size_t i=0;i<sources.size();i++)if(bodies[i]->getLinearVelocity().length()>2*Scale||bodies[i]->getAngularVelocity().length()>.05)count++;return count;}
 std::vector<Transform> transforms()const{std::vector<Transform> result;for(size_t i=0;i<sources.size();i++){const auto& pose=bodies[i]->getWorldTransform();Transform t;for(int a=0;a<3;a++){auto column=pose.getBasis().getColumn(a);t.columns[a]={column.x(),column.y(),column.z()};}t.translation=mapPoint(pose.getOrigin()-pose.getBasis()*physics(sources[i].center));result.push_back(t);}return result;}
 std::vector<Transform> run(const std::function<bool(double,const std::vector<Transform>&)>& progress={}){
  int frames=int(std::ceil(options.duration*60));for(int frame=0;frame<frames;frame++){for(int step=0;step<4;step++)world.stepSimulation(btScalar(1.0/240),0);
   if(frame%6==0){auto current=transforms();for(size_t i=0;i<sources.size();i++)for(const auto& p:sources[i].vertices)validatePoint(transformPoint(current[i],p));if(progress&&!progress((frame+1)/double(frames),current))throw std::runtime_error("Settle cancelled.");}
  }
  if(penetrating())throw std::runtime_error("Settle ended with penetrating contacts. Try a longer duration or another seed.");auto result=transforms();for(size_t i=0;i<sources.size();i++)for(const auto& p:sources[i].vertices)validatePoint(transformPoint(result[i],p));if(progress&&!progress(1,result))throw std::runtime_error("Settle cancelled.");return result;
 }
};
}
