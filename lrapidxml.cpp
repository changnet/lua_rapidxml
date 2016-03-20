#include <cmath>
#include <iostream>
#include <stdexcept>

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include "lrapidxml.hpp"

#define NAME_KEY    "name"
#define VALUE_KEY   "value"
#define ATTR_KEY    "attribute"

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
                lua_pushstring( L,NAME_KEY );
                lua_pushlstring( L,child->name(),child->name_size() );
                lua_rawset( L,-3 );

                /* element value */
                lua_pushstring( L,VALUE_KEY );
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
                    lua_pushstring( L,ATTR_KEY );
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
    rapidxml::xml_document<> doc;
    try
    {
        /* nerver modify str */
        doc.parse<rapidxml::parse_non_destructive>( const_cast<char *>(str) );
    }
    catch ( const std::runtime_error& e )
    {
        doc.clear();
        MARK_ERROR( msg,"xml decode fail",e.what() );
        return -1;
    }
    catch (const rapidxml::parse_error& e)
    {
        doc.clear();
        MARK_ERROR( msg,"invalid xml string",e.what() );
        return -1;
    }
    catch (const std::exception& e)
    {
        doc.clear();
        MARK_ERROR( msg,"xml decode fail",e.what() );
        return -1;
    }
    catch (...)
    {
        doc.clear();
        MARK_ERROR( msg,"xml decode fail","unknow error" );
        return -1;
    }

    int return_code = decode_node( L,doc.first_node(),msg );

    /* rapidxml static memory pool will never free,until you call clear */
    doc.clear();
    return return_code;
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
    /* luaL_error do a longjump,conflict with C++ stack unwind
     * so we make a code block here
     */
    {
        try
        {
            rapidxml::file<> fdoc( path );
            return_code = do_decode( L,fdoc.data(),msg );
        }
        catch ( const std::runtime_error& e )
        {
            MARK_ERROR( msg,"xml decode fail",e.what() );
            return_code = -1;
        }
    }

    if ( return_code < 0 )
    {
        lua_rapidxml_error( L,msg );
        return 0;
    }

    return 1;
}

int encode_table( lua_State *L,int index,rapidxml::xml_document<> *doc,
    rapidxml::xml_node<> *node,char *msg )
{
    int top = lua_gettop( L );

    lua_pushnil( L );
    while ( 0 != lua_next( L,index ) )
    {
        if ( !lua_istable( L,-1 ) )
        {
            lua_settop( L,top );
            MARK_ERROR( msg,"xml encode fail","every node must be a table" );
            return -1;
        }

        rapidxml::xml_node<> *child = 0;
        lua_pushstring( L,NAME_KEY );
        lua_rawget( L,-2 );
        if ( !lua_isstring( L,-1 ) )
        {
            lua_settop( L,top );
            MARK_ERROR( msg,"xml encode fail","node name must be string" );
            return -1;
        }
        size_t len = 0;
        const char *pname = lua_tolstring( L,-1,&len );
        /* do't worry about memory leak here,doc.clear() will free all */
        const char *name = doc->allocate_string( pname,len );
        lua_pop( L,1 ); /* pop name */

        lua_pushstring( L,VALUE_KEY );
        lua_rawget( L,-2 );
        switch ( lua_type( L,-1 ) )
        {
            case LUA_TNIL :
                lua_settop( L,top );
                MARK_ERROR( msg,"xml encode fail","no value was specified" );
                return -1;
                break;
            case LUA_TNUMBER :
            {
                char _buffer[64];
                double val = lua_tonumber( L,-1 );

                if ( floor(val) == val ) /* integer */
                {
                    snprintf( _buffer,64,"%.0f",val );
                }
                else
                {
                    snprintf( _buffer,64,"%f",val );
                }
                const char *value = doc->allocate_string( _buffer,0 );
                child = doc->allocate_node( rapidxml::node_element,name,value );
            }break;
            case LUA_TSTRING :
            {
                size_t len = 0;
                const char *val = lua_tolstring( L,-1,&len );
                const char *value = doc->allocate_string( val,len );
                child = doc->allocate_node( rapidxml::node_element,name,value );
            }break;
            case LUA_TTABLE :
            {
                child = doc->allocate_node( rapidxml::node_element,name );
                if ( encode_table( L,lua_gettop(L),doc,child,msg) < 0 )
                {
                    lua_settop( L,top );
                    return -1;
                }
            }break;
            default:
                lua_settop( L,top );
                MARK_ERROR( msg,"xml encode fail","unsupport type" );
                return -1;
                break;
        }
        lua_pop( L,1 ); /* pop value */

        assert( child );
        lua_pushstring( L,ATTR_KEY );
        lua_rawget( L,-2 );
        switch ( lua_type( L,-1 ) )
        {
            case LUA_TNIL :
                /* no attribute,do nothing */
                break;
            case LUA_TTABLE :
            {
                lua_pushnil( L );
                while ( 0 != lua_next( L,-1 ) )
                {
                    if ( LUA_TSTRING != lua_type( L,-1 ) ||
                        LUA_TSTRING != lua_type( L,-1 ) )
                    {
                        lua_settop( L,top );
                        MARK_ERROR( msg,"xml attribute decode fail",
                            "all attribute key and value must be string" );
                        return -1;
                    }
                    size_t key_len = 0;
                    size_t val_len = 0;
                    const char *key = lua_tolstring( L,-2,&key_len );
                    const char *val = lua_tolstring( L,-1,&val_len );

                    child->append_attribute( doc->allocate_attribute(
                        doc->allocate_string( key,key_len ),
                        doc->allocate_string( val,val_len )
                    ));
                }
            }break;
            default:
                lua_settop( L,top );
                MARK_ERROR( msg,"xml encode fail","attribute must be a table" );
                return -1;
        }
        lua_pop( L,1 ); /* pop attribute */

        /* append to parent */
        node->append_node( child );
        child = 0;

        lua_pop( L,1 ); /* pop table value,iterate to next */
    }

    return 0;
}

int do_encode( lua_State *L,int index,char *msg )
{
    rapidxml::xml_document<> doc;
    /* document type */
    rapidxml::xml_node<>* dt = doc.allocate_node( rapidxml::node_pi,
        doc.allocate_string("xml version='1.0' encoding='utf-8'") );
    doc.append_node( dt );

    if ( encode_table( L,index,&doc,&doc,msg) < 0 )
    {
        doc.clear();
        return -1;
    }

    doc.clear();
    return 1;
}

int encode( lua_State *L )
{
    if ( !lua_istable( L,1) )
    {
        return luaL_error( L,"argument #1 table expect" );
    }

    char msg[MAX_MSG_LEN] = { 0 };
    int return_code = do_encode( L,1,msg );
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
    {"encode", encode},
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
