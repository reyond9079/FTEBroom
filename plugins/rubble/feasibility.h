// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
namespace rubble {
using P=std::array<double,2>;
using Plane=std::array<double,4>;
using Polygon=std::vector<P>;
struct Check {bool allowed=false,certified=false;int axis=0;std::string reason;std::vector<P> centers;};
inline Polygon clip(const Polygon& in,double x,double y,double d){
    double length=std::hypot(x,y);if(length<1e-10)return d>=0?in:Polygon{};
    x/=length;y/=length;d/=length;Polygon out;if(in.empty())return out;
    P a=in.back();double da=a[0]*x+a[1]*y-d;
    for(P b:in){double db=b[0]*x+b[1]*y-d;if((da<=0)!=(db<=0)){double t=da/(da-db);out.push_back({a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t});}if(db<=0)out.push_back(b);a=b;da=db;}
    return out;
}
inline double area(const Polygon& p){double a=0;if(p.empty())return 0;P o=p[0];for(size_t i=1;i+1<p.size();i++)a+=(p[i][0]-o[0])*(p[i+1][1]-o[1])-(p[i][1]-o[1])*(p[i+1][0]-o[0]);return std::abs(a)*.5;}
inline double perimeter(const Polygon& p){double v=0;if(p.empty())return 0;P a=p.back();for(P b:p){v+=std::hypot(b[0]-a[0],b[1]-a[1]);a=b;}return v;}
inline bool inside(const Polygon& p,P q){if(p.empty())return false;P a=p.back();for(P b:p){if((b[0]-a[0])*(q[1]-a[1])-(b[1]-a[1])*(q[0]-a[0]) < -1e-7)return false;a=b;}return true;}
inline Check checkAxis(const std::vector<Plane>& faces,const std::array<double,6>& bounds,int axis,int count,double diameter,double spacing){
    Check result;result.axis=axis;const int u=(axis+1)%3,v=(axis+2)%3;
    Polygon shape{{bounds[u],bounds[v]},{bounds[u+3],bounds[v]},{bounds[u+3],bounds[v+3]},{bounds[u],bounds[v+3]}};
    // Eliminate the through coordinate. Pairing upper and lower planes proves
    // a positive through-thickness, including the geometry kernel's margins.
    for(auto f:faces)if(std::abs(f[axis])<1e-7)shape=clip(shape,f[u],f[v],f[3]-.03);
    for(auto upper:faces)if(upper[axis]>1e-7)for(auto lower:faces)if(lower[axis]<-1e-7)
        shape=clip(shape,upper[u]/upper[axis]-lower[u]/lower[axis],upper[v]/upper[axis]-lower[v]/lower[axis],(upper[3]-.03)/upper[axis]-(lower[3]-.03)/lower[axis]-.06);
    double r=diameter*.5;
    if(shape.size()<3||area(shape)<1e-8){result.reason="The brush has no usable through-section on this axis.";return result;}
    Polygon centers=shape;P a=shape.back();for(P b:shape){double x=b[1]-a[1],y=a[0]-b[0],d=x*a[0]+y*a[1];centers=clip(centers,x,y,d-(r+.01)*std::hypot(x,y));a=b;}
    if(centers.size()<3||area(centers)<1e-9){result.reason="The minimum diameter / clearance circle does not fit this through-section. Choose Auto, another axis or a smaller diameter.";return result;}
    constexpr double pi=3.14159265358979323846;
    if(count*pi*r*r>area(shape)+1e-6){result.reason="The minimum-size holes require more section area than the brush provides.";return result;}
    double distance=diameter+std::max(.05,spacing),maximumDistance=0;
    for(P p:centers)for(P q:centers)maximumDistance=std::max(maximumDistance,std::hypot(p[0]-q[0],p[1]-q[1]));
    if(count>1&&maximumDistance+1e-7<distance){result.reason="Even two minimum-size holes cannot maintain the requested spacing.";return result;}
    double expanded=area(centers)+perimeter(centers)*distance*.5+pi*distance*distance*.25;
    if(count*pi*distance*distance*.25>expanded+1e-6){result.reason="The hole count and spacing exceed a provable packing-area bound.";return result;}
    result.allowed=true;
    // A constructive certificate is stronger than a necessary area bound.
    // Failure to find this grid is inconclusive, never called impossibility.
    for(int swap=0;swap<2&&!result.certified;swap++)for(int phase=0;phase<4&&!result.certified;phase++){
        P low=centers[0],high=low;for(P p:centers)for(int i=0;i<2;i++){low[i]=std::min(low[i],p[i]);high[i]=std::max(high[i],p[i]);}
        double step=std::max(distance+.001,std::max(high[0]-low[0],high[1]-low[1])/30.0);std::vector<P> placed;
        P middle{(low[0]+high[0])*.5,(low[1]+high[1])*.5};if(inside(centers,middle))placed.push_back(middle);
        for(int row=0;row<36&&int(placed.size())<count;row++)for(int col=0;col<36&&int(placed.size())<count;col++){
            P q{};int i=swap,j=1-swap;q[i]=low[i]+(col+.25*(phase&1)+.5*(row&1))*step;q[j]=low[j]+(row+.25*(phase>>1))*step*.866025403784;
            bool ok=inside(centers,q);for(P old:placed)if(std::hypot(q[0]-old[0],q[1]-old[1])<distance)ok=false;if(ok)placed.push_back(q);
        }
        if(int(placed.size())>=count){placed.resize(count);result.centers=std::move(placed);result.certified=true;}
    }
    return result;
}
inline Check check(const std::vector<Plane>& planes,const std::array<double,6>& bounds,int requested,int count,double diameter,double spacing){
    if(planes.size()<4||planes.size()>64||count<1||count>16||!std::isfinite(diameter)||diameter<=.1||!std::isfinite(spacing)||spacing<0){Check r;r.reason="Invalid brush or hole parameters.";return r;}
    for(auto f:planes)for(double v:f)if(!std::isfinite(v)){Check r;r.reason="Invalid source planes.";return r;}
    for(double v:bounds)if(!std::isfinite(v)){Check r;r.reason="Invalid source bounds.";return r;}
    for(int a=0;a<3;a++)if(bounds[a]>=bounds[a+3]){Check r;r.reason="Empty source bounds.";return r;}
    if(requested>=0&&requested<3)return checkAxis(planes,bounds,requested,count,diameter,spacing);
    std::array<int,3> order{0,1,2};std::stable_sort(order.begin(),order.end(),[&](int a,int b){return bounds[a+3]-bounds[a]<bounds[b+3]-bounds[b];});
    Check possible,last;for(int axis:order){auto r=checkAxis(planes,bounds,axis,count,diameter,spacing);if(r.certified)return r;if(r.allowed&&!possible.allowed)possible=r;last=r;}
    if(possible.allowed)return possible;last.reason="No axis fits the minimum diameter, count and spacing constraints. "+last.reason;return last;
}
}
