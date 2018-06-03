#include "texture_atlas.h"
#include <algorithm>
#include <numeric>

using namespace code_policy;

struct box
{
  box(int _x, int _y, int _w, int _h)
    : x(_x), y(_y), w(_w), h(_h)
  { }
  bool intersects(box const&r)const { return !((x + w <= r.x) || (x >= r.x + r.w) || (y + h <= r.y) || (y >= r.y + r.h)); }
  bool contains(box const&r)const   { return !((r.x < x) || (r.x + r.w > x + w) || (r.y < y) || (r.y + r.h > y + h));     }

  int x2()const { return x + w; }
  int y2()const { return y + h; }
  int x, y, w, h;
};

static Val pack(int w, int h, vector<box> &empty, vector<box> &filled) {
  int min_y = std::numeric_limits<int>::max();
  auto n = empty.cbegin();

  for(auto i=empty.cbegin(); i!=empty.cend(); ++i)
    if(i->w >= w && i->h >= h)
    {
      Val y = std::accumulate(filled.cbegin(), filled.cend(), 0, [i](int v, Val j){ return v + 2 *
            (i->x == j.x2() && glm::abs((j.y - i->y) * 2 + j.h - i->h) < glm::max(j.h, i->h)); });

      if(min_y > i->y - y)
      {
        min_y = i->y - y;
        n = i;
      }
    }

  filled.emplace_back(n->y != min_y ? n->x : n->x2() - w, n->y, w, h);

  Val g = filled.back();
  for(auto i=empty.cbegin(); i!=empty.cend();)
    if(i->intersects(g))
    {
      if(g.x2() < i->x2()) empty.emplace_back(g.x2(), i->y,   i->x2() - g.x2(), i->h);
      if(g.y2() < i->y2()) empty.emplace_back(i->x,   g.y2(), i->w,             i->y2() - g.y2());
      if(g.x > i->x)       empty.emplace_back(i->x,   i->y,   g.x - i->x,       i->h);
      if(g.y > i->y)       empty.emplace_back(i->x,   i->y,   i->w,             g.y - i->y);
      empty.erase(i);
    }
    else ++i;

  for(auto i=empty.begin(); i!=empty.cend(); ++i)
    empty.erase(std::remove_if(i + 1, empty.end(), [i](Val j){ return i->contains(j); }), empty.cend());

  return g;
}

Val imgAdpt(uImage const&img) { return img;  }
Val imgAdpt(uImage const*img) { return *img; }

template<class m_key, class T> pair<unordered_map<m_key, Vtex>, map<m_key, T>> code_policy::MakeAtlas(uint max_w, uint max_h, uint channels, map<m_key, T> images)
{
  using tile = typename map<m_key, T>::iterator;
  vector<tile> tiles;

  for(auto i=images.begin(); i!=images.end(); ++i)
    tiles.emplace_back(i);

  std::sort(tiles.begin(), tiles.end(), [](Val _l, Val _r){ Val l = imgAdpt(_l->second); Val r = imgAdpt(_r->second); return l.height != r.height ? l.height > r.height : l.width > r.width; });

  Val area = std::accumulate(tiles.cbegin(), tiles.cend(), 0ul, [](uint64 v, Val i){ Val img = imgAdpt(i->second); return v + uint64(img.width) * img.height; });
  Val width = glm::min(max_w, cast<uint>(glm::pow(2, glm::ceil(glm::log(glm::sqrt(area)) / glm::log(2.)))));

  vector<box> empty, filled;
  filled.reserve(tiles.size());
  empty.reserve(tiles.size() * 2);
  empty.emplace_back(0, 0, cast<int>(width), max_h);
  vector<ubyte> atlas;

  unordered_map<m_key, Vtex> packed;
  map<m_key, T> leftovers;
  for(auto i=tiles.begin(); i!=tiles.end(); ++i)
    [&]{
    Val name = (*i)->first;
    Val img = imgAdpt((*i)->second);

    for(auto j=typename vector<tile>::const_reverse_iterator(i); j!=tiles.crend() &&
        img.height == imgAdpt((*j)->second).height &&
        img.width == imgAdpt((*j)->second).width; ++j)
      if(img == imgAdpt((*j)->second))
      {
        Val existing_coord = packed.find((*j)->first)->second.coord;
        packed.emplace(name, Vtex{ existing_coord, nullptr });
        return;
      }

    Val g = pack(cast<int>(img.width), cast<int>(img.height), empty, filled);
    if(g.y2() > cast<int>(max_h) || g.x2() > cast<int>(width))
    {
      leftovers.emplace(move(**i));
      i = tiles.erase(i);
      return;
    }

    packed.emplace(name, Vtex{ { g.x + 0.5, g.y + 0.5, g.x + g.w - 0.5, g.y + g.h - 0.5 }, nullptr });

    atlas.resize(glm::max(cast<size_t>(g.y2()) * width * channels, atlas.size()), 0);

    Val data = img.data.cbegin();
    Val c = cast<int>(channels);
    for(ptrdiff_t j=0; j<g.h; ++j)
      std::copy(data + j * g.w * c, data + (j+1) * g.w * c, atlas.begin() + ((j + g.y) * width + g.x) * c);
  }();

  Val height = cast<uint>(atlas.size() / (width * channels));

  Val texture = make_shared<GLtex2d>(uImage{ width, height, channels, move(atlas) }, channels, 1 );
  GLbind(*texture).Parameters(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  Val size = vec4(width, height, width, height);
  for(auto &i: packed)
  {
    i.second.tex = texture;
    i.second.coord /= size;
  }

  return { move(packed), move(leftovers) };
}
template pair<unordered_map<uint, Vtex>, map<uint, uImage>> code_policy::MakeAtlas(uint max_w, uint max_h, uint channels, map<uint, uImage> images);
template pair<unordered_map<string, Vtex>, map<string, uImage>> code_policy::MakeAtlas(uint max_w, uint max_h, uint channels, map<string, uImage> images);
