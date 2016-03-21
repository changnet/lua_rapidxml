local xml = require "lua_rapidxml"


function var_dump(data, max_level, prefix)
    if type(prefix) ~= "string" then
        prefix = ""
    end

    if type(data) ~= "table" then
        print(prefix .. tostring(data))
    else
        print(data)
        if max_level ~= 0 then
            local prefix_next = prefix .. "    "
            print(prefix .. "{")
            for k,v in pairs(data) do
                io.stdout:write(prefix_next .. k .. " = ")
                if type(v) ~= "table" or (type(max_level) == "number" and max_level <= 1) then
                    print(v)
                else
                    if max_level == nil then
                        var_dump(v, nil, prefix_next)
                    else
                        var_dump(v, max_level - 1, prefix_next)
                    end
                end
            end
            print(prefix .. "}")
        end
    end
end

--[[
    eg: local b = {aaa="aaa",bbb="bbb",ccc="ccc"}
]]
function vd(data, max_level)
    var_dump(data, max_level or 20)
end

local xml_str = [==[
<?xml version="1.0" encoding="utf-8"?>
<module xmlns:n = "lua_rapidxml">
    <n:name>rapidxml</name>
</module>
<library note = "thanks" url = "github.com">
    <name>lua</name>
    <name>rapidxml</name>
</library>
<cdata>
    <![CDATA[c = a > b ? a : b]]>
</cdata>
<entity>
    <e>less than(&lt;)</e>
    <e>greater than(&gt)</e>
    <e>ampersand(&amp;)</e>
    <e>apostrophe(&apos;)</e>
    <e>quotation mark(&quot;)</e>
</entity>
<note>rapid is a <b>fast</b> xml parse library.</note>
]==]

local x = [==[

]==]

--[[

1.文档转换为数组
2.元素转换为table,分别存在
    name
    value如果值为单个，则转换为字符串，否则为数组
    attribute,k-v结构，不允许重复
3.命名空间被当作属性一样对待
4.pre-defined entity references 实体引用当作字符串，需要各人得到字符串后解析
]]

local xml_tb = xml.decode( xml_str )
vd( xml_tb )

local ret = xml.encode_to_file( xml_tb,"test.xml",true )
assert( ret,"encode_to_file fail" )

local _xml_tb = xml.decode_from_file( "test.xml" );
local _xml_str = xml.encode( _xml_tb,true )
vd( _xml_str )
