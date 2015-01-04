FVNetworkUnlock
===============
FV Network Unlock is a UEFI boot loader providing automated unlcok of operating
system volumes by automating the entry of FileVault disk passwords.

Purpose
-------
There are situations where it may be desirable to enable FileVault on a Mac but
it is not desirable, or not practical, to enable users to decrypt the disk.  In
particular, in a corporate environment where users are authenticated with AD,
an automated way of unlocking FileVault may be required.

This boot loader is designed to be pulled over the network and it locates and
runs the standard Apple boot.efi.  The password is currently pulled from a file
in the same location as the boot loader.  The name of the password file is
based on the serial number of the machine (any invalid characters are replaced
with '_').

Requirements
------------
Compilation requires a working install of EDKII (see
http://sourceforge.net/projects/edk2/).

To use the bootloader, a DHCP/BSDP server and aa TFTP server is required.  The
Mac must also be set to boot from the network (using something like
`bless --netboot --server bsdp://255.255.255.255`).

Limitations
-----------
* When booting the mac, if there is a choice of user to unlock the disk, the
boot loader fails.  Ideally, no users should be allowed to unlock the disk and
a disk password should be.  The simplest way to do this is to run the following
on a decrypted disk: `diskutil cs convert / -stdinpassphrase`

License
------
    Copyright (c) 2014, baskingshark
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
