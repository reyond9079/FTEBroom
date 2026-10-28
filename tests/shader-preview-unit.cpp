#include "../native/ftebroom_shader_preview.h"
#include <cassert>
#include <iostream>
using namespace ftebroom::shaderpreview;
Material read(const std::string& source){size_t at=0;assert(token(source,at)=="{");return parse(source,at);}
int main(){
 auto glass=read("{ qer_trans 0.27 surfaceparm trans { map glass.tga blendFunc blend } }");assert(glass.valid&&glass.opacity==.27f&&glass.blend=="blend"&&(glass.tags&64));
 auto explicitBlend=read("{ { map glass.tga alphaGen const 0.6 blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA } }");assert(explicitBlend.opacity==.6f&&explicitBlend.blend=="gl_src_alpha gl_one_minus_src_alpha");
 auto grate=read("{ surfaceparm trans { map grate.tga alphaFunc GE128 depthWrite } }");assert(grate.masked&&grate.opacity==1&&grate.blend.empty()&&grate.alphaTest=="ge128");
 assert(read("{ { map inverse.tga alphaFunc LT128 } }").alphaTest=="lt128");
 auto lightmap=read("{ { map $lightmap } { map wall.tga blendFunc filter } }");assert(lightmap.opacity==1&&lightmap.blend.empty()&&lightmap.preview=="wall.tga");
 auto opaque=read("{ { map wall.tga alphaGen const 0.2 } }");assert(opaque.opacity==1);
 assert(read("{ qer_trans 0.2 { map glass.tga blendFunc filter } }").blend=="blend");
 auto clamped=read("{ qer_trans 4 { map glass.tga blendFunc blend } }");assert(clamped.opacity==1);
 auto add=read("{ /* braces {ignored} */ qer_editorimage \"preview.tga\" { map flare.tga blendFunc add } }");assert(add.blend=="add"&&add.preview=="preview.tga");
 assert(!read("{ { map glass.tga }").valid);
 std::cout<<"PASS shader preview: qer_trans, alphaGen, masks, blend modes, lightmap opacity, bounds and incomplete script\n";
}
