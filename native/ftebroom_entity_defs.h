// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace ftebroom
{
struct EntityChoice
{
  std::string value, label;
  bool enabled = false;
};
struct EntityProperty
{
  std::string key, type = "string", label, value, description;
  std::vector<EntityChoice> choices;
};
struct EntityDefinition
{
  std::string name, description, model, modelKey;
  std::string modelSkin, modelFrame, modelScale;
  bool brush = false, base = false, hasBounds = false, hasColor = false;
  std::array<float, 3> color{.65f, .8f, .4f}, mins{-8, -8, -8}, maxs{8, 8, 8};
  std::vector<std::string> bases;
  std::map<std::string, EntityProperty> properties;
};
using EntityDefinitions = std::map<std::string, EntityDefinition>;
inline std::string entityTrim(std::string s)
{
  auto first = s.find_first_not_of(" \t\r\n"), last = s.find_last_not_of(" \t\r\n");
  return first == std::string::npos ? "" : s.substr(first, last - first + 1);
}
inline std::vector<float> entityNumbers(std::string s)
{
  for (auto& c : s)
    if (c == ',' || c == '(' || c == ')')
      c = ' ';
  std::istringstream in(s);
  std::vector<float> out;
  float n;
  while (in >> n)
  {
    if (!std::isfinite(n))
      throw std::runtime_error("Non-finite entity bounds");
    out.push_back(n);
  }
  return out;
}
struct EntityToken
{
  std::string text;
  bool quoted = false;
  int line = 1;
};
inline std::vector<EntityToken> entityTokens(const std::string& s)
{
  std::vector<EntityToken> out;
  size_t i = 0;
  int line = 1;
  while (i < s.size())
  {
    if (std::isspace((unsigned char)s[i]))
    {
      if (s[i++] == '\n')
        line++;
      continue;
    }
    if (s.compare(i, 2, "//") == 0)
    {
      while (i < s.size() && s[i] != '\n')
        i++;
      continue;
    }
    if (s.compare(i, 2, "/*") == 0)
    {
      size_t end = s.find("*/", i + 2);
      if (end == std::string::npos)
        throw std::runtime_error("Unclosed comment");
      while (i < end + 2)
        if (s[i++] == '\n')
          line++;
      continue;
    }
    EntityToken t;
    t.line = line;
    if (s[i] == '"')
    {
      t.quoted = true;
      i++;
      while (i < s.size() && s[i] != '"')
      {
        char c = s[i++];
        if (c == '\\' && i < s.size() && (s[i] == '"' || s[i] == '\\'))
          c = s[i++];
        if (c == '\n')
          line++;
        t.text += c;
      }
      if (i == s.size())
        throw std::runtime_error("Unclosed quoted string");
      i++;
    }
    else if (std::string("()[]=:,+").find(s[i]) != std::string::npos)
      t.text += s[i++];
    else
    {
      size_t start = i;
      while (i < s.size() && !std::isspace((unsigned char)s[i])
             && std::string("()[]=:,+\"").find(s[i]) == std::string::npos)
        i++;
      t.text = s.substr(start, i - start);
    }
    out.push_back(t);
  }
  return out;
}
class EntityFgdReader
{
  std::vector<EntityToken> tokens;
  size_t pos = 0;
  std::string peek(size_t n = 0) const
  {
    return pos + n < tokens.size() ? tokens[pos + n].text : "";
  }
  std::string take()
  {
    if (pos == tokens.size())
      throw std::runtime_error("Unexpected end of FGD");
    return tokens[pos++].text;
  }
  bool accept(const char* s)
  {
    if (peek() != s)
      return false;
    pos++;
    return true;
  }
  void expect(const char* s)
  {
    if (!accept(s))
      throw std::runtime_error(std::string("Expected '") + s + "' in FGD");
  }
  std::string scalar()
  {
    std::string s = take();
    while (accept("+"))
      s += take();
    return s;
  }
  std::string bracket(const char* open, const char* close)
  {
    expect(open);
    int depth = 1;
    std::string value;
    while (pos < tokens.size() && depth)
    {
      auto t = take();
      if (t == open)
        depth++;
      if (t == close)
        depth--;
      if (depth)
        value += (value.empty() ? "" : " ") + t;
    }
    if (depth)
      throw std::runtime_error("Unclosed FGD block");
    return value;
  }

public:
  explicit EntityFgdReader(const std::string& s)
    : tokens(entityTokens(s))
  {
  }
  EntityDefinitions read()
  {
    EntityDefinitions result;
    while (pos < tokens.size())
    {
      auto kind = take();if(kind=="@baseclass")kind="@BaseClass";if(kind=="@pointclass")kind="@PointClass";if(kind=="@solidclass")kind="@SolidClass";
      if (kind == "@include")
        throw std::runtime_error("FGD @include must be resolved before parsing");
      if (kind != "@PointClass" && kind != "@SolidClass" && kind != "@BaseClass")
      {
        if (peek() == "(")
          bracket("(", ")");
        else if (pos < tokens.size())
          take();
        continue;
      }
      EntityDefinition d;
      d.brush = kind == "@SolidClass";
      d.base = kind == "@BaseClass";
      while (pos < tokens.size() && peek() != "=")
      {
        auto key = take();
        std::string value;
        if (peek() == "(")
          value = bracket("(", ")");
        if (key == "base")
        {
          std::istringstream in(value);
          std::string b;
          while (in >> b)
            if (b != ",")
              d.bases.push_back(b);
        }
        if (key == "size")
        {
          auto n = entityNumbers(value);
          if (n.size() == 6)
          {
            std::copy_n(n.begin(), 3, d.mins.begin());
            std::copy_n(n.begin() + 3, 3, d.maxs.begin());
            d.hasBounds = true;
          }
          else if (n.size() == 3)
          {
            for (int a = 0; a < 3; a++)
            {
              d.mins[a] = -n[a] / 2;
              d.maxs[a] = n[a] / 2;
            }
            d.hasBounds = true;
          }
        }
        if (key == "color")
        {
          auto n = entityNumbers(value);
          if (n.size() == 3)
          {
            for (int a = 0; a < 3; a++)
              d.color[a] = n[a] / 255.f;
            d.hasColor = true;
          }
        }
        if (key == "model" || key == "studio"){
          d.model=value;
          if(value.find('{')!=std::string::npos){
            auto literal=[&](const char* field){auto at=value.find(field);if(at==value.npos)return std::string{};at=value.find(':',at);if(at==value.npos)return std::string{};at=value.find_first_not_of(" \t",at+1);if(at==value.npos)return std::string{};
              if(value[at]=='\"'){auto end=value.find('\"',at+1);return end==value.npos?std::string{}:value.substr(at+1,end-at-1);}auto end=value.find_first_of(",} \t",at);return value.substr(at,end-at);};
            d.model=literal("path");if(!d.model.empty()&&d.model[0]==':')d.model.erase(0,1);d.modelSkin=literal("skin");d.modelFrame=literal("frame");d.modelScale=literal("scale");
          }
        }
        if (key == "skin")
          d.modelSkin = value;
        if (key == "frame")
          d.modelFrame = value;
        if (key == "scale")
          d.modelScale = value;
        if (key == "studioprop")
          d.modelKey = value;
      }
      expect("=");
      d.name = take();
      if (accept(":"))
        d.description = scalar();
      expect("[");
      while (peek() != "]")
      {
        EntityProperty p;
        p.key = take();
        p.label = p.key;
        if (peek() == "(")
          p.type = bracket("(", ")");
        // Read optional readonly/report modifiers before the colon.
        while (peek() != "[" && peek() != "=" && peek() != ":" && peek() != "]"
               && pos < tokens.size() && tokens[pos].line == tokens[pos - 1].line)
          take();
        if (accept(":"))
        {
          if (peek() != ":" && peek() != "=" && peek() != "]")
            p.label = scalar();
        }
        if (accept(":"))
        {
          if (peek() != ":" && peek() != "=" && peek() != "]")
            p.value = scalar();
        }
        if (accept(":"))
        {
          if (peek() != "=" && peek() != "]")
            p.description = scalar();
        }
        if (accept("="))
        {
          expect("[");
          unsigned long flags = 0;
          while (peek() != "]")
          {
            EntityChoice c;
            c.value = take();
            expect(":");
            c.label = scalar();
            if (accept(":"))
              c.enabled = take() != "0";
            if (c.enabled && p.type == "flags")
              try
              {
                flags |= std::stoul(c.value);
              }
              catch (...)
              {
              }
            p.choices.push_back(c);
          }
          expect("]");
          if (p.type == "flags")
            p.value = std::to_string(flags);
        }
        d.properties[p.key] = std::move(p);
      }
      expect("]");
      result[d.name] = std::move(d);
    }
    return result;
  }
};
inline std::string entityXmlDecode(std::string value)
{
  for (const auto& p : std::vector<std::pair<std::string, std::string>>{
         {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&amp;", "&"}})
  {
    size_t at = 0;
    while ((at = value.find(p.first, at)) != std::string::npos)
    {
      value.replace(at, p.first.size(), p.second);
      at += p.second.size();
    }
  }
  return value;
}
struct EntityXmlNode
{
  std::string tag, text;
  std::map<std::string, std::string> attrs;
  std::vector<EntityXmlNode> children;
};
class EntityXmlReader
{
  const std::string& s;
  size_t p = 0;
  void space()
  {
    while (p < s.size() && std::isspace((unsigned char)s[p]))
      p++;
  }
  std::string word()
  {
    size_t start = p;
    while (
      p < s.size()
      && (std::isalnum((unsigned char)s[p]) || s[p] == '_' || s[p] == '-' || s[p] == ':'))
      p++;
    if (p == start)
      throw std::runtime_error("Invalid XML name");
    return s.substr(start, p - start);
  }
  EntityXmlNode node(int depth)
  {
    if (depth > 64)
      throw std::runtime_error("XML nesting limit exceeded");
    if (p >= s.size() || s[p++] != '<')
      throw std::runtime_error("Expected XML element");
    EntityXmlNode n;
    n.tag = word();
    while (true)
    {
      space();
      if (p >= s.size())
        throw std::runtime_error("Unclosed XML element");
      if (s[p] == '/' || s[p] == '>')
        break;
      auto key = word();
      space();
      if (p >= s.size() || s[p++] != '=')
        throw std::runtime_error("Expected XML attribute value");
      space();
      if (p >= s.size() || (s[p] != '"' && s[p] != '\''))
        throw std::runtime_error("Expected quoted XML attribute");
      char quote = s[p++];
      size_t end = s.find(quote, p);
      if (end == std::string::npos)
        throw std::runtime_error("Unclosed XML attribute");
      n.attrs[key] = entityXmlDecode(s.substr(p, end - p));
      p = end + 1;
    }
    if (s[p] == '/')
    {
      p++;
      if (p >= s.size() || s[p++] != '>')
        throw std::runtime_error("Invalid XML close");
      return n;
    }
    p++;
    while (p < s.size())
    {
      if (s.compare(p, 4, "<!--") == 0)
      {
        auto end = s.find("-->", p + 4);
        if (end == std::string::npos)
          throw std::runtime_error("Unclosed XML comment");
        p = end + 3;
        continue;
      }
      if (s.compare(p, 9, "<![CDATA[") == 0)
      {
        auto end = s.find("]]>", p + 9);
        if (end == std::string::npos)
          throw std::runtime_error("Unclosed CDATA");
        n.text += s.substr(p + 9, end - p - 9);
        p = end + 3;
        continue;
      }
      if (s.compare(p, 2, "</") == 0)
      {
        p += 2;
        auto close = word();
        space();
        if (close != n.tag || p >= s.size() || s[p++] != '>')
          throw std::runtime_error("Mismatched XML close");
        return n;
      }
      if (s[p] == '<')
        n.children.push_back(node(depth + 1));
      else
      {
        auto end = s.find('<', p);
        if (end == std::string::npos)
          throw std::runtime_error("Unclosed XML text");
        n.text += entityXmlDecode(s.substr(p, end - p));
        p = end;
      }
    }
    throw std::runtime_error("Unclosed XML element");
  }

public:
  explicit EntityXmlReader(const std::string& source)
    : s(source)
  {
  }
  EntityXmlNode read()
  {
    space();
    if (s.compare(p, 5, "<?xml") == 0)
    {
      auto end = s.find("?>", p);
      if (end == std::string::npos)
        throw std::runtime_error("Unclosed XML declaration");
      p = end + 2;
      space();
    }
    while (s.compare(p, 4, "<!--") == 0)
    {
      auto end = s.find("-->", p);
      if (end == std::string::npos)
        throw std::runtime_error("Unclosed XML comment");
      p = end + 3;
      space();
    }
    auto root = node(0);
    space();
    if (p != s.size())
      throw std::runtime_error("Unexpected XML trailing content");
    return root;
  }
};
inline EntityDefinitions entityReadEnt(const std::string& s)
{
  auto root = EntityXmlReader(s).read();
  EntityDefinitions out;
  std::map<std::string, std::vector<EntityChoice>> lists;
  for (auto& n : root.children)
    if (n.tag == "list")
      for (auto& item : n.children)
        lists[n.attrs["name"]].push_back(
          {item.attrs["value"], item.attrs["name"], false});
  for (auto& n : root.children)
    if (n.tag == "point" || n.tag == "group")
    {
      EntityDefinition d;
      d.name = n.attrs["name"];
      if (d.name.empty())
        continue;
      d.brush = n.tag == "group";
      d.description = entityTrim(n.text);
      d.model = n.attrs["model"];
      d.modelSkin = n.attrs["skin"];
      d.modelFrame = n.attrs["frame"];
      d.modelScale = n.attrs["scale"];
      auto color = entityNumbers(n.attrs["color"]);
      if (color.size() == 3)
      {
        std::copy_n(color.begin(), 3, d.color.begin());
        d.hasColor = true;
      }
      auto box = entityNumbers(n.attrs["box"]);
      if (box.size() == 6)
      {
        std::copy_n(box.begin(), 3, d.mins.begin());
        std::copy_n(box.begin() + 3, 3, d.maxs.begin());
        d.hasBounds = true;
      }
      for (auto& child : n.children)
      {
        if (child.tag == "flag")
        {
          auto& p = d.properties["spawnflags"];
          p.key = "spawnflags";
          p.label = "Spawn flags";
          p.type = "flags";
          int bit = std::stoi(child.attrs["bit"]);
          if (bit < 0 || bit > 30)
            throw std::runtime_error("Spawn flag bit outside 0..30");
          p.choices.push_back({std::to_string(1u << bit), child.attrs["name"], false});
          continue;
        }
        EntityProperty p;
        p.key = child.attrs["key"];
        if (p.key.empty())
          continue;
        p.type = child.tag;
        p.label = child.attrs["name"];
        if (p.label.empty())
          p.label = p.key;
        p.value = child.attrs["value"];
        p.description = entityTrim(child.text);
        if (lists.count(p.type))
        {
          p.choices = lists[p.type];
          p.type = "choices";
        }
        d.properties[p.key] = std::move(p);
      }
      out[d.name] = std::move(d);
    }
  return out;
}
inline EntityDefinitions entityReadDef(const std::string& s)
{
  EntityDefinitions out;
  size_t pos = 0;
  while ((pos = s.find("/*QUAKED", pos)) != std::string::npos)
  {
    auto end = s.find("*/", pos);
    if (end == std::string::npos)
      throw std::runtime_error("Unclosed QUAKED definition");
    auto lineEnd = std::min(s.find('\n', pos), end);
    auto header = s.substr(pos + 8, lineEnd - pos - 8);
    auto tokens = entityTokens(header);
    size_t i = 0;
    if (tokens.empty())
    {
      pos = end + 2;
      continue;
    }
    EntityDefinition d;
    d.name = tokens[i++].text;
    auto vec = [&]() {
      std::array<float, 3> v{};
      if (i == tokens.size() || tokens[i++].text != "(")
        throw std::runtime_error("Expected QUAKED vector");
      for (float& n : v)
      {
        if (i == tokens.size())
          throw std::runtime_error("Incomplete QUAKED vector");
        n = std::stof(tokens[i++].text);
      }
      if (i == tokens.size() || tokens[i++].text != ")")
        throw std::runtime_error("Unclosed QUAKED vector");
      return v;
    };
    d.color = vec();
    d.hasColor = true;
    if (i < tokens.size() && tokens[i].text == "(")
    {
      d.mins = vec();
      d.maxs = vec();
      d.hasBounds = true;
    }
    else
    {
      d.brush = true;
      if (i < tokens.size() && tokens[i].text == "?")
        i++;
    }
    EntityProperty flags;
    flags.key = "spawnflags";
    flags.label = "Spawn flags";
    flags.type = "flags";
    int bit = 0;
    while (i < tokens.size() && bit < 31)
    {
      auto flag = tokens[i++].text;
      if (flag != "-")
        flags.choices.push_back({std::to_string(1u << bit), flag, false});
      bit++;
    }
    if (!flags.choices.empty())
      d.properties[flags.key] = flags;
    d.description = entityTrim(s.substr(lineEnd, end - lineEnd));
    out[d.name] = std::move(d);
    pos = end + 2;
  }
  return out;
}
inline EntityDefinitions entityResolve(EntityDefinitions definitions)
{
  std::set<std::string> done, visiting;
  auto resolve = [&](auto&& self, const std::string& name) -> void {
    if (done.count(name))
      return;
    if (!definitions.count(name))
      throw std::runtime_error("Unknown FGD base: " + name);
    if (!visiting.insert(name).second)
      throw std::runtime_error("Cyclic FGD inheritance: " + name);
    auto& d = definitions.at(name);
    std::map<std::string, EntityProperty> properties;
    for (const auto& base : d.bases)
    {
      self(self, base);
      const auto& b = definitions.at(base);
      for (const auto& p : b.properties)
        properties[p.first] = p.second;
      if (!d.hasBounds && b.hasBounds)
      {
        d.mins = b.mins;
        d.maxs = b.maxs;
        d.hasBounds = true;
      }
      if (!d.hasColor && b.hasColor)
      {
        d.color = b.color;
        d.hasColor = true;
      }
      if (d.model.empty())
        d.model = b.model;
      if (d.modelSkin.empty())
        d.modelSkin = b.modelSkin;
      if (d.modelFrame.empty())
        d.modelFrame = b.modelFrame;
      if (d.modelScale.empty())
        d.modelScale = b.modelScale;
      if (d.modelKey.empty())
        d.modelKey = b.modelKey;
    }
    for (const auto& p : d.properties)
      properties[p.first] = p.second;
    d.properties = std::move(properties);
    visiting.erase(name);
    done.insert(name);
    for (int a = 0; a < 3; a++)
      if (
        !std::isfinite(d.mins[a]) || !std::isfinite(d.maxs[a]) || d.mins[a] >= d.maxs[a])
        throw std::runtime_error("Invalid bounds for " + name);
  };
  for (const auto& d : definitions)
    resolve(resolve, d.first);
  for (auto it = definitions.begin(); it != definitions.end();)
    if (it->second.base)
      it = definitions.erase(it);
    else
      ++it;
  return definitions;
}
inline EntityDefinitions entityRead(const std::string& s, std::string extension)
{
  if (s.size() > 16 * 1024 * 1024)
    throw std::runtime_error("Entity definitions exceed 16 MiB");
  std::transform(
    extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
      return char(std::tolower(c));
    });
  auto result = extension == ".ent"   ? entityReadEnt(s)
                : extension == ".def" ? entityReadDef(s)
                : extension == ".fgd" ? EntityFgdReader(s).read()
                                      : EntityDefinitions{};
  if (result.empty())
    throw std::runtime_error("No entity definitions found (.ent, .def, .fgd supported)");
  return entityResolve(std::move(result));
}
} // namespace ftebroom
