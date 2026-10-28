!!ver 100-450
!!samps img=0
// Test source texels before applying selection/drag opacity.
varying vec2 tc;
varying vec4 vc;
#ifdef VERTEX_SHADER
attribute vec2 v_texcoord;
attribute vec4 v_colour;
void main(){tc=v_texcoord;vc=v_colour;gl_Position=ftetransform();}
#endif
#ifdef FRAGMENT_SHADER
void main(){
    vec4 texel=texture2D(s_img,tc);
#ifdef GE128
    if(texel.a<0.5)discard;
#endif
#ifdef LT128
    if(texel.a>=0.5)discard;
#endif
#ifdef GT0
    if(texel.a<=0.0)discard;
#endif
    gl_FragColor=texel*vc;
}
#endif
