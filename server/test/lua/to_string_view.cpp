#include <catch2/catch.hpp>
#include "../../src/lua/to.hpp"
#include <string_view>

#define REQUIRE_TRY_TO(x) \
  { \
    TestType v; \
    REQUIRE(Lua::to<TestType>(L, -1, v)); \
    REQUIRE(v == x); \
  }

#define REQUIRE_TRY_TO_FAIL() \
  { \
    const TestType org{"unchanged"}; \
    TestType v{org}; \
    REQUIRE_FALSE(Lua::to<TestType>(L, -1, v)); \
    REQUIRE(v.data() == org.data()); \
    REQUIRE(v.size() == org.size()); \
  }

TEMPLATE_TEST_CASE("Lua::to<>", "[lua][lua-to]", std::string_view)
{
  lua_State* L = luaL_newstate();

  INFO("nil")
  lua_pushnil(L);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  INFO("false")
  lua_pushboolean(L, false);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  INFO("true")
  lua_pushboolean(L, true);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  INFO("123")
  lua_pushinteger(L, 123);
  REQUIRE(Lua::to<TestType>(L, -1) == "123");
  REQUIRE(lua_type(L, -1) == LUA_TSTRING);
  lua_pop(L, 1);
  lua_pushinteger(L, 123);
  REQUIRE_TRY_TO("123");
  REQUIRE(lua_type(L, -1) == LUA_TSTRING);
  lua_pop(L, 1);

  INFO("0.5")
  lua_pushnumber(L, 0.5);
  REQUIRE(Lua::to<TestType>(L, -1) == "0.5");
  REQUIRE(lua_type(L, -1) == LUA_TSTRING);
  lua_pop(L, 1);
  lua_pushnumber(L, 0.5);
  REQUIRE_TRY_TO("0.5");
  REQUIRE(lua_type(L, -1) == LUA_TSTRING);
  lua_pop(L, 1);

  INFO("\"test\"")
  lua_pushliteral(L, "test");
  REQUIRE(Lua::to<TestType>(L, -1) == "test");
  REQUIRE_TRY_TO("test");
  lua_pop(L, 1);

  INFO("table")
  lua_newtable(L);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  INFO("userdata")
  lua_newuserdata(L, 0);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  INFO("lightuserdata")
  lua_pushlightuserdata(L, nullptr);
  REQUIRE(Lua::to<TestType>(L, -1).empty());
  REQUIRE_TRY_TO_FAIL();
  lua_pop(L, 1);

  lua_close(L);
}