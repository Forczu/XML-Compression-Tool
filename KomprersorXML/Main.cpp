#include <iostream>
#include "CompresorXml.h"
#include "port.h"
#include "qsmodel.h"
#include "rangecod.h"
#include <io.h>
#include <fcntl.h>

int main()
{
#ifndef unix
	_setmode(_fileno(stdin), O_BINARY);
	_setmode(_fileno(stdout), O_BINARY);
#endif

	CompresorXml cXml;
	cXml.encode("samples/part.xml");
	//cXml.saveEncodedToFile("samples/SwissProt_comp.xml");
	cXml.saveEncodedToBinaryFile("samples/part.bin");
	cXml.readEncodedBinaryFile("samples/part.bin", "samples/part2.xml");

	return 0;
}