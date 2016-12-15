#include <iostream>
#include "CompresorXml.h"

int main()
{
	CompresorXml cXml;
	cXml.encode("samples/serialized_graph.xml");
	cXml.saveEncodedToFile("samples/compressed_xml.xml");

	return 0;
}