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

    print(std::cout,doc,0);

    return 0;
}
