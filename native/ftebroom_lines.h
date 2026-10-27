// SPDX-License-Identifier: GPL-2.0-or-later
#include <sstream>
#include "ftebroom_map_index.h"
// Index structural opening braces, ignoring braces inside strings and comments.
static bool selectLineRequest;
static void documentIndexLines(const std::filesystem::path& path)
{
  std::string bytes;try{if(!path.empty())bytes=q1Read(path);}catch(...){}
  auto text = ftebroom::mapLineIndex(bytes);
  qhandle_t handle;
  if (files->Open("data/map-lines.txt", &handle, 2) < 0)
    return;
  files->Write(handle, text.data(), static_cast<int>(text.size()));
  files->Close(handle);
  command("ftebroom_line_index\n");
}
static void selectLineDialog()
{
  static int line = 1;
  if (selectLineRequest)
  {
    selectLineRequest = false;
    ImGui::OpenPopup("Select by Line Number");
  }
  if (uiDialog("Select by Line Number"))
  {
    ImGui::TextUnformatted("Line in the last opened or saved MAP file.");
    ImGui::InputInt("Line", &line);
    ImGui::BeginDisabled(line < 1);
    if (ImGui::Button("Select"))
    {
      action(("ftebroom_select_line " + std::to_string(line) + "\n").c_str());
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}
