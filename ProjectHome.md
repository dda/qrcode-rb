libqrencode, by Fukuchi Kentaro, provides an easy way to produce QRCode barcodes. This RB project interface with the library to produce QRCode barcodes as Picture objects.

Included in the distribution: the original libqrencode.3.dylib (compiled with -arch i386) and a "helper" dylib, quickQR.dylib, made from qrenc.c, also compiled with -arch i386 (see the original source code of the library).

Download the Barcode source code from the provided external links provided, and compile with the provided Makefile.in (plus makeDylib.sh). Or use the compile libbarcode.dylib I included.