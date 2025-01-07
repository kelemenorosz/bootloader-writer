## Bootloader and USB removable media reader/writer

How to build:

* ``git clone`` this repository
* Navigate to ``/``
* Create ``build`` directory
* Navigate to ``/build``
* Open developer command prompt for VS
* Run ``cmake ../``
* Run ``cmake --build . --target install``
* Navigate to ``/``
* Run ``nasm -f bin bl_load_relocate.asm -o bl_load_relocate.bin``
* Run ``nasm -f bin ss_load_stub.asm -o ss_load_stub.bin``

Use ``drive_writer.exe`` to write the two binaries to a USB removable media. 