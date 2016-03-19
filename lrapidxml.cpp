#include <map>
#include <iostream>
#include <stdexcept>

#include <rapidxml.hpp>
#include "lrapidxml.hpp"

#define MAX_STACK   1024
#define MAX_MSG_LEN 256
#define MARK_ERROR(x,what) snprintf( msg,MAX_MSG_LEN,"%s",what )

void lua_rapidxml_error( lua_State *L,const char *msg )
{
    luaL_error( L,msg );
}

int decode_attribute( lua_State *L,rapidxml::xml_node<> *node,int index )
{
    assert( LUA_TTABLE == lua_type( L,index ) );
    if ( !(node->first_attribute()) ) return 0;

    /* create a metatable */
    lua_newtable( L );

    /* all attributes are store at metatable */
    for ( rapidxml::xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute() )
    {
        lua_pushlstring( L,attr->name(),attr->name_size() );
        lua_pushlstring( L,attr->value(),attr->value_size() );

        lua_rawset( L,-3 );
    }

    lua_setmetatable( L,index );

    return 0;
}

int decode_node( lua_State *L,rapidxml::xml_node<> *node,char *msg )
{
    int top = lua_gettop( L );
    lua_newtable( L );

    int index = 1;
    for ( rapidxml::xml_node<> *child = node; child; child = child->next_sibling() )
    {
        assert( rapidxml::node_element == child->type() );

        switch( child->type() )
        {
            case rapidxml::node_element:
            {
                lua_createtable( L,0,2 );

                lua_pushstring( L,"name" );
                lua_pushlstring( L,child->name(),child->name_size() );
                lua_rawset( L,-3 );

                lua_pushstring( L,"value" );
                if ( decode_node( L,child->first_node(),msg ) < 0 )
                {
                    lua_settop( L,top );
                    return -1;
                }
                lua_rawset( L,-3 );

                if ( decode_attribute( L,child,-2 ) < 0 )
                {
                    MARK_ERROR( msg,"decode_attribute error" );
                    lua_settop( L,top );
                    return -1;
                }
            }break;
            case rapidxml::node_data:  /* fall through */
            case rapidxml::node_cdata:
                lua_pushlstring( L,child->value(),child->value_size() );
                break;
            default:
                MARK_ERROR( msg,"unsupport xml type" );
                lua_settop( L,top );
                return -1;
        }

        lua_rawseti( L,top + 1,index );
        ++index;
    }

    return 1;
}

int decode( lua_State *L )
{
    const char *str = luaL_checkstring( L,1 );

    int return_code = 0;
    char msg[MAX_MSG_LEN] = { 0 };
    /* luaL_error do a longjump,conflict with C++ stack unwind */
    {
        rapidxml::xml_document<> doc;
        try
        {
            /* nerver modify str */
            doc.parse<rapidxml::parse_non_destructive>( const_cast<char *>(str) );
        }
        catch ( const std::runtime_error& e )
        {
            MARK_ERROR( msg,e.what() );
            return -1;
        }
        catch (const rapidxml::parse_error& e)
        {
            MARK_ERROR( msg,e.what() );
            return -1;
        }
        catch (const std::exception& e)
        {
            MARK_ERROR( msg,e.what() );
            return -1;
        }
        catch (...)
        {
            MARK_ERROR( msg,"unknow error" );
            return -1;
        }

        return_code = decode_node( L,doc.first_node(),msg );
    }
    if ( return_code < 0 ) lua_rapidxml_error( L,msg );

    return return_code;
}

/* ====================LIBRARY INITIALISATION FUNCTION======================= */


/* ========================== lua 5.1 start ================================= */
/* this function was copy from src of lua5.3.1 */
LUALIB_API void luaL_setfuncs_ex (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}

#ifndef luaL_newlibtable
    #define luaL_newlibtable(L,l)	\
        lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)
#endif

#ifndef luaL_newlib
    #define luaL_newlib(L,l)  \
        (luaL_newlibtable(L,l), luaL_setfuncs_ex(L,l,0))
#endif

/* ========================== lua 5.1 end =================================== */

static const luaL_Reg lua_rapidxml_lib[] =
{
    //{"encode", encode},
    {"decode", decode},
    //{"encode_to_file", encode_to_file},
    //{"decode_from_file", decode_from_file},
    {NULL, NULL}
};


int luaopen_lua_rapidxml( lua_State *L )
{
    luaL_newlib(L, lua_rapidxml_lib);
    return 1;
}
