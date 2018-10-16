# cmail - easy cli tool to send emails from command line 

cli utility to send emails from command line (plain text/mime/attachments) w/o a need for any prior setups:
  - sends email in plain text or mime/base64
  - supports sending attachments
  - works over `smtp` and `smtps`


#### Linux and MacOS precompiled binaries are available for download:
- [macOS 64 bit](https://github.com/ldn-softdev/cmail/raw/master/cmail-64.v.1.02)
- [macOS 32 bit](https://github.com/ldn-softdev/cmail/raw/master/cmail-32.v.1.02)
- Linux (to be published)

#### Compile and install instructions (for Mac os):
bulding cmail requires also compiling curl (as mac os x's distribution is not good enough (yet) to support mime). Thus assuming
here that all downloads go into ~/Downloads folder:
  1. Download [curl-7.61.1.zip](https://curl.haxx.se/download/curl-7.61.1.zip) (or higher)
  2. open terminal and run:

   - `cd ~/Downloads/`
   - `unzip curl-7.61.1.zip`
   - `cd cd curl-7.61.1`
   - `export CFLAGS="-Ofast"`
   - `./configure --disable-dict --disable-file --disable-ftp --disable-ftps --disable-gopher --disable-http --disable-https --disable-imap --disable-imaps --disable-pop3 --disable-pop3s --disable-rtsp --disable-smb --disable-smbs --disable-tftp --disable-telnet --disable-ftfp --disable-manual --disable-ares --disable-cookies --disable-proxy -disable-versioned-symbols --disable-libcurl-option --without-librtmp --disable-shared --without-ssl --with-darwinssl --without-zlib --disable-ldap --without-libidn 
`
   - `make`
  3. Download (or clone) [cmail](https://github.com/ldn-softdev/cmail/archive/master.zip)
  4. do in terminal:
  
   - `cd ~/Downloads`
   - `unzip cmail-master.zip`
   - `cd cmail-master`
   - `c++ -o cmail -std=c++14 -I ~/Downloads/curl-7.61.1/include ~/Downloads/curl-7.61.1/lib/.libs/libcurl.a -framework Foundation -lz -framework Security cmail.cpp
`
   - `sudo mv cmail /usr/local/bin/`

