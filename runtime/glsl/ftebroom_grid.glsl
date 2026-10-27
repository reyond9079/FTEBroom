!!ver 130-450
!!cvarf ftebroom_grid_aa=1.005
!!cvarf ftebroom_grid_fade_start=512
!!cvarf ftebroom_grid_fade_end=1024
#include "sys/defs.h"
varying vec2 tc;
varying vec4 vc;
varying vec3 viewdir;
#ifdef VERTEX_SHADER
void main() {
    tc = v_texcoord;
    vc = v_colour;
    viewdir = e_eyepos - v_position.xyz;
    gl_Position = ftetransform();
}
#endif
#ifdef FRAGMENT_SHADER
uniform float cvar_ftebroom_grid_aa;
uniform float cvar_ftebroom_grid_fade_start;
uniform float cvar_ftebroom_grid_fade_end;
void main() {
    // Distance to the nearest grid line in screen pixels, independent of spacing.
    vec2 derivative = max(fwidth(tc), vec2(0.000001));
    float fadeEnd = max(0.0, cvar_ftebroom_grid_fade_end);
    float fadeStart = clamp(cvar_ftebroom_grid_fade_start, 0.0, max(0.0, fadeEnd-1.0));
    float distanceToEye = length(viewdir);
    if (fadeEnd > 0.0 && distanceToEye >= fadeEnd) discard;
    float distanceFade = fadeEnd > 0.0 ? 1.0-smoothstep(fadeStart, fadeEnd, distanceToEye) : 1.0;
    float spacing = exp2(clamp(ceil(log2(4.0 * max(derivative.x, derivative.y))), 0.0, 18.0));
    vec2 coordinate = tc / spacing;
    vec2 width = derivative / spacing;
    vec2 distanceToLine = abs(fract(coordinate + 0.5) - 0.5) / width;
    float feather = clamp(cvar_ftebroom_grid_aa, 0.05, 2.0);
    float coverage = 1.0 - smoothstep(max(0.0, 0.75-feather*0.5), 0.75+feather*0.5, min(distanceToLine.x, distanceToLine.y));
    gl_FragColor = vec4(vc.rgb, vc.a * coverage * distanceFade);
}
#endif
