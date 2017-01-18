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
	cXml.encode("samples/part.xml", "samples/part.xml.bin");
	cXml.decode("samples/part.xml.bin", "samples/part.2.xml");

	return 0;
}