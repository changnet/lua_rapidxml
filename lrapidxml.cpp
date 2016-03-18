#include <rapidxml.hpp>
#include "lrapidxml.hpp"

#include <iostream>
#include <stdexcept>
#include "rapidxml_print.hpp"

#define MAX_MSG_LEN 256
#define MARK_ERROR(x,what) snprintf( msg,MAX_MSG_LEN,"%s",what )

void lua_rapidxml_error( lua_State *L,const char *msg )
{
    luaL_error( L,msg );
}

int decode_node( lua_State *L,rapidxml::xml_node<> *node,char *msg )
{
    lua_pushstring( L,"aaaaaaaaaaaaaaaaaa");
    return 1;
}

/* xml string to lua table */
int do_decode( lua_State *L,char *msg )
{
    const char *str = luaL_checkstring( L,1 );

    rapidxml::xml_document<>  doc;
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

    rapidxml::print(std::cout,doc,0);

    return decode_node( L,doc.first_node(),msg );
}

int decode( lua_State *L )
{
    /* luaL_error do a longjump,conflict with C++ stack unwind */
    char msg[MAX_MSG_LEN] = { 0 };

    int return_code = do_decode( L,msg );
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
