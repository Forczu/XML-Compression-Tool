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
	cXml.encode("samples/psd7003.xml");
	//cXml.saveEncodedToFile("samples/SwissProt_comp.xml");
	cXml.readEncodedBinaryFile("samples/psd7003.xml.bin", "samples/psd7003.2.xml");

	return 0;
}