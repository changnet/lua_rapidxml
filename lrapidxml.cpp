#include <iostream>
#include <stdexcept>

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include "lrapidxml.hpp"

#define MAX_STACK   1024
#define MAX_MSG_LEN 256
#define MARK_ERROR(x,note,what) snprintf( x,MAX_MSG_LEN,"%s:%s",note,what )

void lua_rapidxml_error( lua_State *L,const char *msg )
{
    luaL_error( L,msg );
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

                /* element name */
                lua_pushstring( L,"name" );
                lua_pushlstring( L,child->name(),child->name_size() );
                lua_rawset( L,-3 );

                /* element value */
                lua_pushstring( L,"value" );
                if ( decode_node( L,child->first_node(),msg ) < 0 )
                {
                    lua_settop( L,top );
                    return -1;
                }
                lua_rawset( L,-3 );

                /* attribute */
                rapidxml::xml_attribute<> *attr = child->first_attribute();
                if ( attr )
                {
                    lua_pushstring( L,"attribute" );
                    lua_newtable( L );
                    for ( ; attr; attr = attr->next_attribute() )
                    {
                        lua_pushlstring( L,attr->name(),attr->name_size() );
                        lua_pushlstring( L,attr->value(),attr->value_size() );

                        lua_rawset( L,-3 );
                    }
                    lua_rawset( L,-3 );
                }
            }break;
            case rapidxml::node_data:  /* fall through */
            case rapidxml::node_cdata:
                lua_pushlstring( L,child->value(),child->value_size() );
                break;
            default:
                MARK_ERROR( msg,"xml decode fail","unsupport xml type" );
                lua_settop( L,top );
                return -1;
        }

        lua_rawseti( L,top + 1,index );
        ++index;
    }

    return 1;
}

int do_decode( lua_State *L,const char *str,char *msg )
{
    /* luaL_error do a longjump,conflict with C++ stack unwind */
    rapidxml::xml_document<> doc;
    try
    {
        /* nerver modify str */
        doc.parse<rapidxml::parse_non_destructive>( const_cast<char *>(str) );
    }
    catch ( const std::runtime_error& e )
    {
        MARK_ERROR( msg,"xml decode fail",e.what() );
        return -1;
    }
    catch (const rapidxml::parse_error& e)
    {
        MARK_ERROR( msg,"invalid xml string",e.what() );
        return -1;
    }
    catch (const std::exception& e)
    {
        MARK_ERROR( msg,"xml decode fail",e.what() );
        return -1;
    }
    catch (...)
    {
        MARK_ERROR( msg,"xml decode fail","unknow error" );
        return -1;
    }

    return decode_node( L,doc.first_node(),msg );
}

int decode( lua_State *L )
{
    const char *str = luaL_checkstring( L,1 );

    char msg[MAX_MSG_LEN] = { 0 };

    int return_code = do_decode( L,str,msg );
    if ( return_code < 0 )
    {
        lua_rapidxml_error( L,msg );
        return 0;
    }

    return 1;
}

int decode_from_file( lua_State *L )
{
    const char *path = luaL_checkstring( L,1 );

    int return_code = 0;
    char msg[MAX_MSG_LEN] = { 0 };
    {
        rapidxml::file<> fdoc( path );
        return_code = do_decode( L,fdoc.data(),msg );
    }

    if ( return_code < 0 )
    {
        lua_rapidxml_error( L,msg );
        return 0;
    }

    return 1;
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
    {"decode_from_file", decode_from_file},
    {NULL, NULL}
};


int luaopen_lua_rapidxml( lua_State *L )
{
    luaL_newlib(L, lua_rapidxml_lib);
    return 1;
}
