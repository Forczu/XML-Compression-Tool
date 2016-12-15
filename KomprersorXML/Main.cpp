#include <iostream>
#include "CompresorXml.h"

int main()
{
	CompresorXml cXml;
	cXml.encode("samples/serialized_graph.xml");
	cXml.saveEncodedToFile("samples/compressed_xml.xml");
	cXml.saveEncodedToBinaryFile("samples/compressed_xml.bin");
	cXml.readEncodedBinaryFile("samples/compressed_xml.bin", "samples/decompressed_xml.xml");

	return 0;
}