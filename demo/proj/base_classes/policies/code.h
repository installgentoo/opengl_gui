#pragma once
#include <string>
#include <array>
#include <vector>
#include <set>
#include <deque>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#define GLM_FORCE_RADIANS
#include <glm/fwd.hpp>

namespace code_policy
{

using glm::ivec2;
using glm::uvec2;
using glm::ivec3;
using glm::uvec3;
using glm::ivec4;
using glm::uvec4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::mat4x3;

using std::pair;
using std::string;
using std::array;
using std::vector;
using std::set;
using std::deque;
using std::map;
using std::unordered_map;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::move;
using std::forward;
using std::function;
using std::tuple;
using std::make_unique;
using std::make_shared;

typedef int8_t   byte;
typedef uint8_t  ubyte;
typedef int16_t  int16;
typedef uint16_t uint16;
typedef uint32_t uint;
typedef int64_t  int64;
typedef uint64_t uint64;
typedef uint     glenum;
typedef string   string8;

#define Val auto const&

}

namespace GUI
{
using namespace code_policy;
}

namespace AI
{
using namespace code_policy;
}
