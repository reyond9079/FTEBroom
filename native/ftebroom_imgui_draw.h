// SPDX-License-Identifier: GPL-2.0-or-later
struct Vertex
{
  float x, y, u, v, r, g, b, a;
};
static Vertex lerp(const Vertex& a, const Vertex& b, float t)
{
  return {
    a.x + (b.x - a.x) * t,
    a.y + (b.y - a.y) * t,
    a.u + (b.u - a.u) * t,
    a.v + (b.v - a.v) * t,
    a.r + (b.r - a.r) * t,
    a.g + (b.g - a.g) * t,
    a.b + (b.b - a.b) * t,
    a.a + (b.a - a.a) * t};
}
// Clip triangles before submitting them as degenerate quads to FTE's batched 2D
// API.
static void triangle(std::array<Vertex, 8> polygon, const ImVec4& clip, qhandle_t image)
{
  // A triangle clipped against a rectangle has at most seven vertices.
  // Keep clipping on the stack and bypass it for fully contained triangles.
  size_t count = 3;
  unsigned outside = 15, anyOutside = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto& v = polygon[i];
    unsigned code = (v.x < clip.x ? 1 : 0) | (v.x > clip.z ? 2 : 0)
                  | (v.y < clip.y ? 4 : 0) | (v.y > clip.w ? 8 : 0);
    outside &= code; anyOutside |= code;
  }
  if (outside) return;
  for (int edge = 0; anyOutside && edge < 4 && count; ++edge)
  {
    auto distance = [&](const Vertex& p) {
      return edge == 0 ? p.x - clip.x : edge == 1 ? clip.z - p.x
           : edge == 2 ? p.y - clip.y : clip.w - p.y;
    };
    std::array<Vertex, 8> result{};
    size_t nextCount = 0;
    Vertex previous = polygon[count - 1];
    float pd = distance(previous);
    for (size_t i = 0; i < count; ++i) {
      Vertex current = polygon[i];
      float cd = distance(current);
      if ((cd >= 0) != (pd >= 0))
        result[nextCount++] = lerp(previous, current, pd / (pd - cd));
      if (cd >= 0) result[nextCount++] = current;
      previous = current; pd = cd;
    }
    polygon = result; count = nextCount;
  }
  for (size_t i = 1; i + 1 < count; ++i)
  {
    Vertex p[4] = {polygon[0], polygon[i], polygon[i + 1], polygon[i + 1]};
    // FTE's image shader culls the reverse winding. ImGui's antialias fringes
    // and custom draw lists can contain either winding, so normalize every
    // clipped triangle before encoding it as a degenerate quad.
    const float area = (p[1].x-p[0].x)*(p[2].y-p[0].y)
                     - (p[1].y-p[0].y)*(p[2].x-p[0].x);
    if (area < 0) std::swap(p[1], p[2]);
    p[3] = p[2];
    vec2_t xy[4], uv[4];
    vec4_t color[4];
    for (int j = 0; j < 4; ++j)
    {
      xy[j][0] = p[j].x;
      xy[j][1] = p[j].y;
      uv[j][0] = p[j].u;
      uv[j][1] = p[j].v;
      color[j][0] = p[j].r;
      color[j][1] = p[j].g;
      color[j][2] = p[j].b;
      color[j][3] = p[j].a;
    }
    draw->Image2dQuad(xy, uv, color, image);
  }
}
static void render()
{
  ImGui::Render();
  auto data = ImGui::GetDrawData();
  draw->Colour4f(1, 1, 1, 1);
  for (int n = 0; n < data->CmdListsCount; ++n)
  {
    auto list = data->CmdLists[n];
    for (auto& cmd : list->CmdBuffer)
    {
      if (cmd.UserCallback)
      {
        if (cmd.UserCallback != ImDrawCallback_ResetRenderState)
          cmd.UserCallback(list, &cmd);
        continue;
      }
      for (unsigned int i = 0; i + 2 < cmd.ElemCount; i += 3)
      {
        std::array<Vertex, 8> p{};
        for (int j = 0; j < 3; ++j)
        {
          auto& v =
            list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset + i + j] + cmd.VtxOffset];
          auto c = ImGui::ColorConvertU32ToFloat4(v.col);
          p[j] = {v.pos.x, v.pos.y, v.uv.x, v.uv.y, c.x, c.y, c.z, c.w};
        }
        triangle(p, cmd.ClipRect, (qhandle_t)cmd.GetTexID());
      }
    }
  }
}
