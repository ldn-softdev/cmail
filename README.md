# cmail - easy cli tool to send emails from command line 

cli utility to send emails from command line (plain text/mime/attachments) w/o a need for any prior setups:
  - sends email in plain text or mime/base64
  - supports sending attachments
  - works over `smtp` and `smtps`


#### Linux and MacOS precompiled binaries are available for download:
- [macOS 64 bit](https://github.com/ldn-softdev/cmail/raw/master/cmail-macos-64.v1.02)
- [macOS 32 bit](https://github.com/ldn-softdev/cmail/raw/master/cmail-macos-32.v.1.02)
- Linux (to be published)

#### Compile and install instructions (for Mac os):
building cmail requires also compiling curl (as mac os x's distribution is not good enough (yet) to support mime). Thus assuming
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
   - `c++ -o cmail -std=c++14 -I ~/Downloads/curl-7.61.1/include ~/Downloads/curl-7.61.1/lib/.libs/libcurl.a -framework Security cmail.cpp
`
   - `sudo mv cmail /usr/local/bin/`


#### help screen:
```
bash $ cmail -h
usage: cmail [-dh] [-H header] [-a attachment] [-p password] [-s subject]
             [-u username] to [smtp]

An easy utility based on libcurl to send emails from the command line
Version 1.02, developed by Dmitry Lyssenko (ldn.softdev@gmail.com)

optional arguments:
 -d             turn on debugs (multiple calls increase verbosity)
 -h             help screen
 -H header      append email header
 -a attachment  attach file
 -p password    password to use with username to access smtp server
 -s subject     set email subject
 -u username    username to access smtp server with

standalone arguments:
  to            'to' recipient(s)
  smtp          smtp server to connect to [default: <recover from username>]

if there are attachments or inputs contain unicode, the mail is sent using
mime/base64 encoding, otherwise it is sent as plain text

to send attachments only and suppress inputs, specify a bare qualifier `-',
predicated at least one option -a is given

- Option -H supports headers: `From', `To', `Cc', `Bcc', `Subject'
  headers should be given one per option and in the following format, e.g.:
   -H 'Subject: this is a subject'
- Headers `To', `Cc', `Bcc' are additive (multiple arguments could be given,
  listed over comma), while `From' and `Subject' are overridable (only the last
  given will be recorded)
- Argument `to' also may contain multiple recipients (like additive headers in
  option -H)
- Argument `smtp', if not given, is attempted to be recovered from the username
  (option -u, or header 'From:'): if it's is a fully qualified email, the domain
  part is extracted and prepended with "smtp."
- if header -H 'From: ...' is missed, it is attempted to be recovered from the
  username (option -u)
- setting a username (option -u) requires setting a password (-p) as well
- a password (-p) requires a username; if the username is not given, it is
  attempted to be recovered from `-H "From: ..."' header
- specifying a username/password automatically implies using `smtps://' protocol
  (instead of default `smtp://')
- subject could be passed either via -s or via -H 'Subject: ...'; the latter
  option overrides the former one

bash $ 
```


#### CAVEAT:
The obvious caveat using this tool is that it requires passing a password as a parameter (if you work with smpts).
Mac os let you working around this limitation by using `security` utility, which would let you to extracting the
password from the *Keychain Access* vault and passing it to the cmail, e.g.:
```
bash $ echo "test mail" | cmail -s "email subject" -u user@some_mailer.com -p `security find-internet-password -wa "user@some_mailer.com"` another_user@somewhere_else.com
```
