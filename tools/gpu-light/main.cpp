// SPDX-License-Identifier: GPL-2.0-or-later
// Small-map GPU light baker. Input geometry is immutable; output is a new BSP/LIT pair.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <vector>
using V=std::array<float,3>;
V add(V a,V b){for(int i=0;i<3;i++)a[i]+=b[i];return a;} V mul(V a,float s){for(auto& x:a)x*=s;return a;} float dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];} V cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
std::string load(std::filesystem::path p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read input");return {std::istreambuf_iterator<char>(f),{}};}
void range(const std::string& s,size_t at,size_t n){if(at>s.size()||n>s.size()-at)throw std::runtime_error("Invalid BSP bounds");}
template<class T>T get(const std::string&s,size_t a){range(s,a,sizeof(T));T v;memcpy(&v,s.data()+a,sizeof v);return v;}
template<class T>void put(std::string&s,size_t a,T v){range(s,a,sizeof v);memcpy(s.data()+a,&v,sizeof v);}
V vec(const std::string&s,size_t a){V v{get<float>(s,a),get<float>(s,a+4),get<float>(s,a+8)};for(float x:v)if(!std::isfinite(x)||std::abs(x)>1000000)throw std::runtime_error("Invalid geometry value");return v;}
void save(std::filesystem::path p,const std::string&s){std::ofstream f(p,std::ios::binary);f.write(s.data(),s.size());if(!f)throw std::runtime_error("Cannot write output");}
#define GLFUN(type,name) auto name=(type)SDL_GL_GetProcAddress(#name);if(!name)throw std::runtime_error("Missing OpenGL function " #name)
int main(int argc,char**argv){try{
 auto start=std::chrono::steady_clock::now();if(argc<3)throw std::runtime_error("Usage: gpu-light input.bsp output.bsp [bounce-rays 0..64]");
 auto input=std::filesystem::absolute(std::filesystem::u8path(argv[1])),output=std::filesystem::absolute(std::filesystem::u8path(argv[2]));if(input==output||std::filesystem::exists(output))throw std::runtime_error("Choose a NEW output BSP; existing files are never overwritten");
 auto litpath=output;litpath.replace_extension(".lit");if(std::filesystem::exists(litpath))throw std::runtime_error("Output LIT already exists");
 int rays=argc>3?std::stoi(argv[3]):32;if(rays<0||rays>64)throw std::runtime_error("Bounce rays must be 0..64");
 if(std::filesystem::file_size(input)>32*1024*1024)throw std::runtime_error("Small-map input limit: 32 MiB");
 auto src=load(input);int version=get<int>(src,0);bool bsp2=src.substr(0,4)=="BSP2";if(version!=29&&!bsp2)throw std::runtime_error("Prototype supports Quake BSP29 and BSP2 only");
 std::array<std::string,15> l;for(int i=0;i<15;i++){auto a=get<unsigned>(src,4+i*8),n=get<unsigned>(src,8+i*8);range(src,a,n);l[i]=src.substr(a,n);}
 if(l[14].size()!=64)throw std::runtime_error("Prototype requires a static world without brush submodels");
 std::vector<float> triangles,positions,normals,lights;unsigned fs=bsp2?28:20,es=bsp2?8:4;size_t count=l[7].size()/fs;if(l[7].size()%fs||count>4096)throw std::runtime_error("Small-map prototype limit: 4096 faces");
 // World geometry only: moving brush submodels require separate lighting transforms.
 range(l[14],0,64);unsigned first=get<unsigned>(l[14],56),nf=get<unsigned>(l[14],60);if(first+nf>count)throw std::runtime_error("Invalid world model");
 std::string gray;
 for(unsigned fi=first;fi<first+nf;fi++){
  size_t a=size_t(fi)*fs;unsigned plane=bsp2?get<unsigned>(l[7],a):get<unsigned short>(l[7],a),side=bsp2?get<unsigned>(l[7],a+4):get<unsigned short>(l[7],a+2),edge=get<unsigned>(l[7],a+(bsp2?8:4)),n=bsp2?get<unsigned>(l[7],a+12):get<unsigned short>(l[7],a+8),ti=bsp2?get<unsigned>(l[7],a+16):get<unsigned short>(l[7],a+10);if(n<3||n>256)throw std::runtime_error("Invalid polygon");
  std::vector<V> poly;for(unsigned i=0;i<n;i++){int e=get<int>(l[13],size_t(edge+i)*4);if(e==INT32_MIN)throw std::runtime_error("Invalid edge");size_t ea=size_t(std::abs(e))*es;unsigned vi=bsp2?get<unsigned>(l[12],ea+(e<0?4:0)):get<unsigned short>(l[12],ea+(e<0?2:0));poly.push_back(vec(l[3],size_t(vi)*12));}
  for(unsigned i=1;i+1<n;i++)for(auto p:{poly[0],poly[i],poly[i+1]}){triangles.insert(triangles.end(),p.begin(),p.end());triangles.push_back(0);}
  V normal=vec(l[1],size_t(plane)*20);float d=get<float>(l[1],size_t(plane)*20+12);if(side){normal=mul(normal,-1);d=-d;}
  size_t t=size_t(ti)*40;V u=vec(l[6],t),v=vec(l[6],t+16);float us=get<float>(l[6],t+12),vs=get<float>(l[6],t+28);int flags=get<int>(l[6],t+36);size_t styles=a+(bsp2?20:12),off=a+(bsp2?24:16);
  for(int j=0;j<4;j++)l[7][styles+j]=char(255);put<int>(l[7],off,-1);if(flags&1)continue;
  float lo[2]={1e30f,1e30f},hi[2]={-1e30f,-1e30f};for(auto p:poly){float uv[2]={dot(u,p)+us,dot(v,p)+vs};for(int k=0;k<2;k++){lo[k]=std::min(lo[k],uv[k]);hi[k]=std::max(hi[k],uv[k]);}}
  int x0=int(floor(lo[0]/16)),y0=int(floor(lo[1]/16)),w=int(ceil(hi[0]/16))-x0+1,h=int(ceil(hi[1]/16))-y0+1;if(w<1||h<1||w>256||h>256||gray.size()+size_t(w)*h>262144)throw std::runtime_error("Small-map sample limit exceeded (262144)");
  float det=dot(u,cross(v,normal));if(std::abs(det)<1e-8f)throw std::runtime_error("Degenerate texinfo");
  put<int>(l[7],off,int(gray.size()));l[7][styles]=0;
  for(int y=0;y<h;y++)for(int x=0;x<w;x++){V p=mul(add(add(mul(cross(v,normal),(x+x0)*16-us),mul(cross(normal,u),(y+y0)*16-vs)),mul(cross(u,v),d)),1/det);
   // Pull border luxels toward the polygon centre if rounding put them outside.
   V center{};for(auto q:poly)center=add(center,mul(q,1.f/n));
   for(int pass=0;pass<8;pass++){bool inside=true;for(unsigned j=0;j<n;j++){V edgev=add(poly[(j+1)%n],mul(poly[j],-1)),inward=cross(normal,edgev);if(dot(inward,add(center,mul(poly[j],-1)))<0)inward=mul(inward,-1);if(dot(inward,add(p,mul(poly[j],-1)))<-.01f)inside=false;}if(inside)break;p=add(mul(p,.75f),mul(center,.25f));}
   p=add(p,mul(normal,.2f));positions.insert(positions.end(),{p[0],p[1],p[2],1});normals.insert(normals.end(),{normal[0],normal[1],normal[2],0});gray.push_back(0);
  }
 }
 if(triangles.size()/12>8192)throw std::runtime_error("Small-map triangle limit exceeded (8192)");
 std::regex entity(R"(\{([^{}]*)\})"),kv("\"([^\"]*)\"\\s*\"([^\"]*)\"");
 for(auto it=std::sregex_iterator(l[0].begin(),l[0].end(),entity);it!=std::sregex_iterator();++it){std::string e=(*it)[1],name,origin;float power=300;V color{1,1,1};int style=0;for(auto k=std::sregex_iterator(e.begin(),e.end(),kv);k!=std::sregex_iterator();++k){auto key=(*k)[1].str(),value=(*k)[2].str();if(key=="classname")name=value;else if(key=="origin")origin=value;else if(key=="light")power=std::stof(value);else if(key=="target"||key=="_sun"||key=="delay")throw std::runtime_error("Prototype supports untargeted linear point lights only");else if(key=="style")style=std::stoi(value);else if(key=="_color"){std::istringstream s(value);s>>color[0]>>color[1]>>color[2];}}
  if(name!="light")continue;if(style)throw std::runtime_error("Styled lights are not supported by this prototype");V p{};std::istringstream s(origin);if(!(s>>p[0]>>p[1]>>p[2])||!std::isfinite(power)||power<0)throw std::runtime_error("Invalid light");float m=std::max({color[0],color[1],color[2]});if(m>1)color=mul(color,1/255.f);lights.insert(lights.end(),{p[0],p[1],p[2],power,color[0],color[1],color[2],0});}
 if(uint64_t(gray.size())*(triangles.size()/12)*(1+rays)*(1+lights.size()/8)>200000000)throw std::runtime_error("Prototype work budget exceeded; use a smaller map or fewer rays");
 if(lights.empty()||lights.size()/8>128)throw std::runtime_error("Prototype needs 1..128 ordinary static point lights");
 if(SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);auto window=SDL_CreateWindow("ReyondBroom GPU light",0,0,64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);if(!window)throw std::runtime_error(SDL_GetError());auto context=SDL_GL_CreateContext(window);if(!context)throw std::runtime_error(SDL_GetError());
 std::cout<<"GPU: "<<glGetString(GL_RENDERER)<<"\n";
 GLFUN(PFNGLCREATESHADERPROC,glCreateShader);GLFUN(PFNGLSHADERSOURCEPROC,glShaderSource);GLFUN(PFNGLCOMPILESHADERPROC,glCompileShader);GLFUN(PFNGLGETSHADERIVPROC,glGetShaderiv);GLFUN(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog);GLFUN(PFNGLCREATEPROGRAMPROC,glCreateProgram);GLFUN(PFNGLATTACHSHADERPROC,glAttachShader);GLFUN(PFNGLLINKPROGRAMPROC,glLinkProgram);GLFUN(PFNGLGETPROGRAMIVPROC,glGetProgramiv);GLFUN(PFNGLUSEPROGRAMPROC,glUseProgram);GLFUN(PFNGLGENVERTEXARRAYSPROC,glGenVertexArrays);GLFUN(PFNGLBINDVERTEXARRAYPROC,glBindVertexArray);GLFUN(PFNGLGENFRAMEBUFFERSPROC,glGenFramebuffers);GLFUN(PFNGLBINDFRAMEBUFFERPROC,glBindFramebuffer);GLFUN(PFNGLFRAMEBUFFERTEXTURE2DPROC,glFramebufferTexture2D);GLFUN(PFNGLCHECKFRAMEBUFFERSTATUSPROC,glCheckFramebufferStatus);GLFUN(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation);GLFUN(PFNGLUNIFORM1IPROC,glUniform1i);GLFUN(PFNGLACTIVETEXTUREPROC,glActiveTexture);
 const char* vertex=R"(#version 330
 void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);})";
 const char* fragment=R"(#version 330
 uniform sampler2D tris,pos,norm,lamps;uniform int nt,nl,rays,total;out vec4 result;
 vec4 data(sampler2D t,int i){return texelFetch(t,ivec2(i%1024,i/1024),0);}
 bool hit(vec3 o,vec3 d,float limit,out vec3 hp,out vec3 hn){float best=limit;bool found=false;for(int i=0;i<nt;i++){vec3 a=data(tris,i*3).xyz,b=data(tris,i*3+1).xyz,c=data(tris,i*3+2).xyz,e=b-a,f=c-a,p=cross(d,f);float det=dot(e,p);if(abs(det)<1e-6)continue;vec3 t=o-a;float u=dot(t,p)/det;if(u<0.||u>1.)continue;vec3 q=cross(t,e);float v=dot(d,q)/det;if(v<0.||u+v>1.)continue;float dist=dot(f,q)/det;if(dist>.05&&dist<best){best=dist;hn=normalize(cross(e,f));if(dot(hn,d)>0.)hn=-hn;found=true;}}hp=o+d*best;return found;}
 vec3 direct(vec3 p,vec3 n){vec3 sum=vec3(0);for(int i=0;i<nl;i++){vec4 l=data(lamps,i*2);vec3 delta=l.xyz-p;float dist=length(delta);if(dist<.01)continue;vec3 dir=delta/dist;float value=max(0.,l.w-dist)*(.5+.5*max(0.,dot(n,dir)));if(value<=0.)continue;vec3 h,hn;if(!hit(p,dir,dist-.3,h,hn))sum+=data(lamps,i*2+1).rgb*value;}return sum;}
 void main(){int id=int(gl_FragCoord.x)+int(gl_FragCoord.y)*1024;if(id>=total){result=vec4(0);return;}vec3 p=data(pos,id).xyz,n=data(norm,id).xyz;vec3 energy=direct(p,n);vec3 u=normalize(cross(n,abs(n.z)<.9?vec3(0,0,1):vec3(0,1,0))),v=cross(n,u);for(int i=0;i<rays;i++){float t=(float(i)+.5)/float(rays),angle=float(i)*2.39996323;vec3 dir=u*(sqrt(t)*cos(angle))+v*(sqrt(t)*sin(angle))+n*sqrt(1.-t);vec3 h,hn;if(hit(p,dir,65536.,h,hn))energy+=direct(h+hn*.2,hn)*.55/float(rays);}result=vec4(energy/255.,1);})";
 auto shader=[&](GLenum kind,const char*s){auto id=glCreateShader(kind);glShaderSource(id,1,&s,nullptr);glCompileShader(id);int ok;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(id,sizeof log,nullptr,log);throw std::runtime_error(log);}return id;};auto program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,vertex));glAttachShader(program,shader(GL_FRAGMENT_SHADER,fragment));glLinkProgram(program);int linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);if(!linked)throw std::runtime_error("Shader link failed");glUseProgram(program);
 auto texture=[&](const char*name,int unit,std::vector<float> data){size_t pixels=(data.size()+3)/4;int height=int((pixels+1023)/1024);data.resize(size_t(height)*1024*4);GLuint t;glGenTextures(1,&t);glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,1024,height,0,GL_RGBA,GL_FLOAT,data.data());glUniform1i(glGetUniformLocation(program,name),unit);};
 texture("tris",0,triangles);texture("pos",1,positions);texture("norm",2,normals);texture("lamps",3,lights);
 for(auto pair:{std::pair<const char*,int>{"nt",int(triangles.size()/12)},{"nl",int(lights.size()/8)},{"rays",rays},{"total",int(gray.size())}})glUniform1i(glGetUniformLocation(program,pair.first),pair.second);
 int height=int((gray.size()+1023)/1024);GLuint target,fbo,vao;glActiveTexture(GL_TEXTURE0+4);glGenTextures(1,&target);glBindTexture(GL_TEXTURE_2D,target);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,1024,height,0,GL_RGBA,GL_FLOAT,nullptr);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target,0);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Framebuffer unavailable");glGenVertexArrays(1,&vao);glBindVertexArray(vao);glViewport(0,0,1024,height);
 auto gpuStart=std::chrono::steady_clock::now();std::cout<<"Baking "<<gray.size()<<" luxels, "<<triangles.size()/12<<" triangles, "<<rays<<" bounce rays\n"<<std::flush;glDrawArrays(GL_TRIANGLES,0,3);std::vector<float> pixels(size_t(height)*1024*4);glReadPixels(0,0,1024,height,GL_RGBA,GL_FLOAT,pixels.data());if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("GPU bake failed");auto gpuEnd=std::chrono::steady_clock::now();
 std::string lit="QLIT";lit.append("\1\0\0\0",4);for(size_t i=0;i<gray.size();i++){float maxv=0;for(int c=0;c<3;c++){float f=pixels[i*4+c]*255;if(!std::isfinite(f))throw std::runtime_error("Non-finite lighting result");auto value=std::clamp(int(std::lround(f)),0,255);lit.push_back(char(value));maxv=std::max(maxv,float(value));}gray[i]=char(int(maxv));}l[8]=gray;
 // Drop submodel lighting rather than retaining offsets into replaced world lighting.
 for(unsigned i=0;i<count;i++)if(i<first||i>=first+nf){size_t a=size_t(i)*fs;put<int>(l[7],a+(bsp2?24:16),-1);for(int j=0;j<4;j++)l[7][a+(bsp2?20:12)+j]=char(255);}
 std::string dst(124,0);put<int>(dst,0,version);for(int i=0;i<15;i++){while(dst.size()%4)dst.push_back(0);put<unsigned>(dst,4+i*8,unsigned(dst.size()));put<unsigned>(dst,8+i*8,unsigned(l[i].size()));dst+=l[i];}
 save(output,dst);save(litpath,lit);SDL_GL_DeleteContext(context);SDL_DestroyWindow(window);SDL_Quit();std::cout<<"GPU seconds: "<<std::chrono::duration<double>(gpuEnd-gpuStart).count()<<"\nTotal seconds: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\nSaved "<<output.u8string()<<" and .lit\n";return 0;
}catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<"\n";return 1;}}
