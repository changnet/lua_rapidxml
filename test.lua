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
<?xml version="1.0" encoding="ISO-8859-1"?>
<!--  Copyright w3school.com.cn -->
<note href = "www.baidu.com">
	<to>George</to>
	<from>John</from>
	<heading>Reminder</heading>
	<body>Don't forget the meeting!</body>
</note>
<document>
  <article>
    <p>This is the first paragraph.</p>
    <h2 class='opt'>Title with opt style</h2>
  </article>
  <article>
    <p>Some <b>important</b> text.</p>
    <p1>Some <b> very <b1>important</b1></b> text.</p1>
  </article>
</document>]==]

local xml_test = [==[
<note>
	<to>George</to>
	<from>John</from>
	<heading>Reminder</heading>
	<body>Don't forget the meeting!</body>
</note>
]==]
--local xml_tb = xml.decode( xml_str )
--vd( xml_tb )
--vd( getmetatable(xml_tb[1]))


vd( xml.decode_from_file("test.xml") )
-- 注意栈深度保护
-- 检测元表是否已存在
