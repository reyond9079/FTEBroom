!!ver 400-450
#include "sys/defs.h"
varying vec4 vc;
#ifdef VERTEX_SHADER
void main(){vc=v_colour;precise vec3 delta=e_eyepos-vec3(v_lmcoord.xy,v_lmcoord2.x);precise vec3 product=delta*v_normal.xyz;precise float side=(product.x+product.y)+product.z;gl_Position=side < -0.01 ? vec4(2.0,2.0,2.0,1.0) : ftetransform();}
#endif
#ifdef FRAGMENT_SHADER
void main(){gl_FragColor=vc;}
#endif
