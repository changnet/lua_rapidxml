#include <map>
#include <iostream>
#include <stdexcept>

#include <rapidxml.hpp>
#include "lrapidxml.hpp"

#define MAX_MSG_LEN 256
#define MARK_ERROR(x,what) snprintf( msg,MAX_MSG_LEN,"%s",what )

typedef enum
{
    NONE  = 0,
    K_V   = 1,    /* { k = v } */
    K_VT  = 2,    /* { k = {} } */
    K_V_T = 3    /* { {k = v} } */
}k_v_t;

inline int decode_node( lua_State *L,rapidxml::xml_node<> *node,char *msg,int index );
int decode_object_node( lua_State *L,rapidxml::xml_node<> *node,char *msg );

void lua_rapidxml_error( lua_State *L,const char *msg )
{
    luaL_error( L,msg );
}

int is_array( rapidxml::xml_node<> *node )
{
    int data_cnt = 0;
    std::map< std::string,int > _key;
    for (rapidxml::xml_node<> *child = node; child; child = child->next_sibling())
    {
        switch( child->type() )
        {
            if ( data_cnt ) return 1;

            case rapidxml::node_element:
            {
                std::string k( child->name(),child->name_size() );
                if ( _key.find( k ) != _key.end() )
                    return 1;
                else
                    _key.insert( std::pair< std::string,int >(k,1) );
            }break;
            case rapidxml::node_data:
                ++data_cnt;
                if ( !_key.empty() ) return 1;
                break;
            case rapidxml::node_cdata:
                ++data_cnt;
                if ( !_key.empty() ) return 1;
                break;
            default : /* ignore others */ break;
        }
    }

    assert( data_cnt <= 1 );
    return 0;
}

k_v_t key_value_type( rapidxml::xml_node<> *node )
{
    assert( rapidxml::node_element == node->type() );

    rapidxml::xml_node<> *sub_node = node->first_node();

    if ( rapidxml::node_element == sub_node->type() || sub_node->next_sibling() )
        return K_VT;

    if ( node->first_attribute() ) return K_V_T;

    return K_V;
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

int decode_array_node( lua_State *L,rapidxml::xml_node<> *node,char *msg )
{
    assert( rapidxml::node_element == node->type() );

    lua_newtable( L );
    int tb_index = lua_gettop( L );

    int index = 1;
    for ( rapidxml::xml_node<> *child = node; child; child = child->next_sibling() )
    {
        lua_newtable( L );
        switch ( child->type() )
        {
            case rapidxml::node_element:
            {
                switch( key_value_type( child ) )
                {
                    case K_V:
                    {
                        /* Value contains text of first data node. */
                        lua_pushlstring( L,child->name(),child->name_size() );
                        lua_pushlstring( L,child->value(),child->value_size() );
                        lua_rawset( L,tb_index + 1 );
                    }break;
                    case K_VT:
                    {
                        lua_pushlstring( L,child->value(),child->value_size() );
                        rapidxml::xml_node<> *sub_node = child->first_node();

                        int return_code = 0;
                        if ( is_array( sub_node ) )
                            return_code = decode_array_node( L,sub_node,msg );
                        else
                            return_code = decode_object_node( L,sub_node,msg );
                        if ( return_code < 0 )
                        {
                            lua_pop( L,3 );
                            return -1;
                        }

                        if ( decode_attribute( L,sub_node,lua_gettop(L) ) < 0 )
                        {
                            lua_pop( L,4 );
                            return -1;
                        }
                        lua_rawset( L,tb_index + 1 );
                    }break;
                    case K_V_T:
                    {
                        lua_pushlstring( L,child->name(),child->name_size() );
                        lua_pushlstring( L,child->value(),child->value_size() );

                        if ( decode_attribute( L,child,index ) < 0 )
                        {
                            MARK_ERROR( msg,"decode K_V_T attribute error" );
                            lua_pop( L,2 );
                            return -1;
                        }
                        lua_rawset( L,tb_index + 1 );
                    }break;
                    default :
                        assert( 0 );
                        break;
                }
            }break;
            case rapidxml::node_data:  /* fall through */
            case rapidxml::node_cdata:
                lua_pushlstring( L,child->value(),child->value_size() );
                break;
            default:
                lua_pop( L,2 );
                MARK_ERROR( msg,"unsupport xml type" );
                return -1;
        }
        lua_rawseti( L,tb_index,index );
        ++index;
    }

    return 1;
}


int decode_object_node( lua_State *L,rapidxml::xml_node<> *node,char *msg )
{
    assert( rapidxml::node_element == node->type() );

    lua_newtable( L );
    int tb_index = lua_gettop( L );

    lua_pushlstring( L,node->name(),node->name_size() );
    lua_pushlstring( L,node->value(),node->value_size() );

    if ( decode_attribute( L,node,tb_index ) < 0 )
    {
        MARK_ERROR( msg,"decode_object_node error" );
        lua_pop( L,3 );
        return -1;
    }
    lua_rawset( L,-3 );

    return 0;
}

inline int decode_node( lua_State *L,rapidxml::xml_node<> *node,char *msg,int index )
{
    int stack_inc = 0;
    switch( node->type() )
    {
        case rapidxml::node_element:
        {
            switch( key_value_type( node ) )
            {
                case K_V:
                {
                    /* Value contains text of first data node. */
                    lua_pushlstring( L,node->name(),node->name_size() );
                    lua_pushlstring( L,node->value(),node->value_size() );
                    lua_rawset( L,index );
                }break;
                case K_VT:
                {
                    lua_pushlstring( L,node->value(),node->value_size() );
                    rapidxml::xml_node<> *sub_node = node->first_node();

                    int return_code = 0;
                    if ( is_array( sub_node ) )
                        return_code = decode_array_node( L,sub_node,msg );
                    else
                        return_code = decode_object_node( L,sub_node,msg );
                    if ( return_code < 0 )
                    {
                        lua_pop( L,1 );
                        return -1;
                    }

                    if ( decode_attribute( L,sub_node,lua_gettop(L) ) < 0 )
                    {
                        lua_pop( L,2 );
                        return -1;
                    }
                    lua_rawset( L,index );
                }break;
                case K_V_T:
                {
                    std::cout << "K_V_T" << std::endl;
                    lua_pushlstring( L,node->name(),node->name_size() );
                    lua_pushlstring( L,node->value(),node->value_size() );

                    if ( decode_attribute( L,node,index ) < 0 )
                    {
                        MARK_ERROR( msg,"decode K_V_T attribute error" );
                        lua_pop( L,2 );
                        return -1;
                    }
                    lua_rawset( L,index );
                }break;
                default :
                    assert( 0 );
                    break;
            }
        }break;
        default :
            MARK_ERROR( msg,"unsupport node type" );
            return -1;
            break;
    }

    return stack_inc;
}

int decode( lua_State *L )
{
    const char *str = luaL_checkstring( L,1 );

    int return_code = 0;
    char msg[MAX_MSG_LEN] = { 0 };
    /* luaL_error do a longjump,conflict with C++ stack unwind */
    {
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

        rapidxml::xml_node<> *node = doc.first_node();
        if ( is_array( node ) ) return_code = decode_array_node( L,node,msg );
        else return_code = decode_object_node( L,node,msg );
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
