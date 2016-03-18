#include <iostream>
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

using namespace rapidxml;

int main()
{

    file<> fdoc("test.xml");
    std::cout<<fdoc.data()<<std::endl;
    xml_document<>  doc;
    doc.parse<0>(fdoc.data());

    std::cout << "Name of my first node is: " << doc.first_node()->name() << "\n";
    //xml_node<> *node = doc.first_node();
    for (xml_node<> *child = doc.first_node(); child; child = child->next_sibling())
    {
        std::cout << "node name:" << child->name() << "  value:" << child->value() << std::endl;
    }

    //print(std::cout,doc,0);

    return 0;
}
