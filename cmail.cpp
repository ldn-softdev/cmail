#include <iostream>
#include <string>
#include <fstream>
#include <iterator>
#include "lib/getoptions.hpp"
#include "lib/Curl.hpp"

using namespace std;


#define VERSION "1.02"

// defined options
#define OPT_RDT -
#define OPT_ATT a
#define OPT_DBG d
#define OPT_APH H
#define OPT_PWD p
#define OPT_SBJ s
#define OPT_USR u
#define ARG_TO 0
#define ARG_SRV 1

#define SPACES " \t"


// facilitate option materialization
#define STR(X) XSTR(X)
#define XSTR(X) #X
#define CHR(X) XCHR(X)
#define XCHR(X) *#X


#define RETURN_CODES \
        RC_OK, \
        RC_NOK, \
        RC_INVTO, \
        RC_MISSUSR, \
        RC_MISSPWD, \
        RC_MISSMTP, \
        RC_END
ENUM(ReturnCodes, RETURN_CODES)

#define OFF_GETOPT RC_END                                       // offset for Getopt exceptions
#define OFF_CSMTP (OFF_GETOPT + Getopt::end_of_throw)           // offset for Curl SMTP exceptions


struct SharedResource {
    Getopt              opt;
    CurlSmtp            sm;                                     // send mail

    DEBUGGABLE()
};


#define __REFX__(A) auto & A = __common_resource__.A;
#define REVEAL(X, ARGS...) \
        auto & __common_resource__ = X; \
        MACRO_TO_ARGS(__REFX__, ARGS)
// usage: REVEAL(cr, opt, DBG())


// forward declarations
void post_parse(SharedResource &r);
void parse_headers(SharedResource &r);
void append_email_header(CurlSmtp::Headers hdr, string hdr_str, SharedResource &r);
CurlSmtp::Headers match_header(string hdr_str);
vector<string> split_by(char dlm, const string &str);
void try_recovering_from(SharedResource &r);
string trim_spaces(std::string str);




int main(int argc, char *argv[])
{
 SharedResource r;
 REVEAL(r, opt, sm, DBG())

 opt.prolog("\nAn easy utility based on libcurl to send emails from the command line\n" \
            "Version " VERSION ", developed by Dmitry Lyssenko (ldn.softdev@gmail.com)\n");
 opt[CHR(OPT_ATT)].desc("attach file").name("attachment");
 opt[CHR(OPT_DBG)].desc("turn on debugs (multiple calls increase verbosity)");
 opt[CHR(OPT_APH)].desc("append email header").name("header");
 opt[CHR(OPT_PWD)].desc("password to use with username to access smtp server").name("password");
 opt[CHR(OPT_SBJ)].desc("set email subject").name("subject");
 opt[CHR(OPT_USR)].desc("username to access smtp server with").name("username");
 opt[ARG_TO].name("to").desc("'to' recipient(s)");
 opt[ARG_SRV].name("smtp").desc("smtp server to connect to").bind("<recover from username>");
 opt.epilog("\n\
if there are attachments or inputs contain unicode, the mail is sent using\n\
mime/base64 encoding, otherwise it is sent as plain text\n\n\
to send attachments only and suppress inputs, specify a bare qualifier `-',\n\
predicated at least one option -" STR(OPT_ATT) " is given\n\n\
- Option -" STR(OPT_APH) " supports headers: `From', `To', `Cc', `Bcc', `Subject'\n\
  headers should be given one per option and in the following format, e.g.:\n\
   -" STR(OPT_APH) " 'Subject: this is a subject'\n\
- Headers `To', `Cc', `Bcc' are additive (multiple arguments could be given,\n\
  listed over comma), while `From' and `Subject' are overridable (only the last\n\
  given will be recorded)\n\
- Argument `to' also may contain multiple recipients (like additive headers in\n\
  option -" STR(OPT_APH) ")\n\
- Argument `smtp', if not given, is attempted to be recovered from the username\n\
  (option -" STR(OPT_USR) ", or header 'From:'): if it's is a fully qualified email, the domain\n\
  part is extracted and prepended with \"smtp.\"\n\
- if header -" STR(OPT_APH) " 'From: ...' is missed, it is attempted to be recovered from the\n\
  username (option -" STR(OPT_USR) ")\n\
- setting a username (option -" STR(OPT_USR) ") requires setting a password (-" STR(OPT_PWD) \
  ") as well\n\
- a password (-" STR(OPT_PWD) ") requires a username; if the username is not given, it is\n\
  attempted to be recovered from `-" STR(OPT_APH) " \"From: ...\"' header\n\
- specifying a username/password automatically implies using `smtps://' protocol\n\
  (instead of default `smtp://')\n\
- subject could be passed either via -" STR(OPT_SBJ) " or via -" STR(OPT_APH)
  " 'Subject: ...'; the latter\n\
  option overrides the former one\n");

 // parse options
 try { opt.parse(argc,argv); }
 catch(Getopt::stdException & e)
  { opt.usage(); return e.code() + OFF_GETOPT; }

 DBG().level(opt[CHR(OPT_DBG)].hits())
      .use_ostream(cerr)
      .increment(+1, sm, -1);

 post_parse(r);

 try {
  if(opt[CHR(OPT_USR)].hits() > 0)                              // setup ssl if username/password
   sm.ssl(opt[CHR(OPT_USR)].str(), opt[CHR(OPT_PWD)].str());

  sm.host(opt[ARG_SRV].str());
  for(auto &file: opt[CHR(OPT_ATT)])
   sm.attach_file(file);
  bool skip_input = opt[CHR(OPT_RDT)].hits() > 0 and opt[CHR(OPT_ATT)].hits() > 0;
  sm.send(string{skip_input? istream_iterator<char>{}: istream_iterator<char>(cin>>noskipws),
                 istream_iterator<char>{}});
 }
 catch (CurlSmtp::stdException & e) {
  DBG(0) DOUT() << "exception raised by: " << e.where() << endl;
  cerr << opt.prog_name() << " CurlSmtp exception: " << e.what() << endl;
  return e.code() + OFF_CSMTP;
 }

 if(sm.rc() != CURLE_OK)
  { cerr << "sending error: " << sm.error() << endl; return RC_NOK; }

 cout << "sending ok" << endl;
 return RC_OK;
}



void post_parse(SharedResource &r) {
 // check options requirements & compatibility, setup headers in CurlSmtp
 REVEAL(r, opt, sm, DBG())
 DBG(0) DOUT() << "begin processing options" << endl;

 append_email_header(CurlSmtp::To, opt[ARG_TO].str(), r);       // append header 'To' from arg[0]
 if(sm.to().empty())
  { cerr << "error: header 'To' must be a valid email" << endl; exit(RC_INVTO); }

 if(opt[CHR(OPT_SBJ)].hits() > 0)                               // append subj (if given)
  sm.subject(opt[CHR(OPT_SBJ)].str());

 parse_headers(r);

 if(opt[CHR(OPT_USR)].hits() > 0 and opt[CHR(OPT_PWD)].hits() == 0) // -u given
  { cerr << "error: password is required but not provided" << endl; exit(RC_MISSPWD); }

 if(opt[CHR(OPT_PWD)].hits() > 0) {                             // -p is given
  opt[CHR(OPT_PWD)] = trim_spaces(opt[CHR(OPT_PWD)].str());     // ensure to trim all spaces
  if(opt[CHR(OPT_USR)].hits() == 0) {                           // -u is not given
   if(sm.from().empty())                                        // header 'From:' is empty too
    { cerr << "error: username is required but not provided" << endl; exit(RC_MISSUSR); }
   opt[CHR(OPT_USR)] = sm.from().substr(1, sm.from().size() - 2);   // recover username from 'From'
  }
 }

 if(opt[ARG_SRV].hits() == 0) {                                 // smtp server is missed, recover:
  if(opt[CHR(OPT_USR)].hits() > 0 and opt[CHR(OPT_USR)].str().find('@') != string::npos) {
   opt[ARG_SRV] = "smtp." + opt[CHR(OPT_USR)].str().substr(opt[CHR(OPT_USR)].str().find('@') + 1);
   DBG(0) DOUT() << "recovered smtp from '-u' option: " << opt[ARG_SRV] << endl;
   return;
  }
  auto at_pos = sm.from().find('@');                            // if not succeeded from -u, try
  if(not sm.from().empty() and at_pos != string::npos) {        // recovering from -H 'from: ..'
   opt[ARG_SRV] = "smtp." + sm.from().substr(at_pos + 1, sm.from().size() - at_pos - 2);
   DBG(0) DOUT() << "recovered smtp from 'From:' header: " << opt[ARG_SRV] << endl;
   return;
  }
  cerr << "error: smtp server is required but not provided" << endl;
  exit(RC_MISSMTP);
 }
}


void append_email_header(CurlSmtp::Headers hdr, string hdr_str, SharedResource &r) {
 // add to the header (hdr) one by one emails listed (over comma) in hdr_str
 REVEAL(r, sm, DBG())

 // append header string to additive header (to, cc, bcc)
 for(const auto & email: split_by(',', hdr_str))
  if(email.find("@") != string::npos) {
   DBG(1) DOUT() << ENUMS(CurlSmtp::Headers, hdr) << ": " << email << endl;
   sm.add_header(hdr, email);
  }
  else
   cerr << "fail: email '" << email << "' does not seem to be valid, ignoring" << endl;
}


void parse_headers(SharedResource &r) {
 // process all -H options
 REVEAL(r, opt, sm, DBG())

 for(const auto &opt_hdr: opt[CHR(OPT_APH)]) {                  // go over all -H options
  auto header = match_header(opt_hdr.substr(0, opt_hdr.find(':')));

  if(header AMONG(CurlSmtp::Date, CurlSmtp::end_of_headers))    // -H "Date: ..." is unsupported
   { cerr << "fail: unrecognized header in '" << opt_hdr << "', ignoring" << endl; continue; }

  if(header AMONG(CurlSmtp::From, CurlSmtp::Subject)) {         // -H "From:..." & -H "Subject:..."
   sm.add_header(header, trim_spaces(opt_hdr.substr(opt_hdr.find(':') + 1)));   // are overridable
   DBG(1) DOUT() << "appended '" << ENUMS(CurlSmtp::Headers, header) << "': "
                 << trim_spaces(opt_hdr.substr(opt_hdr.find(':') + 1)) << endl;
  }
  else                                                          // must be either to/cc/bcc
   append_email_header(header, opt_hdr.substr(opt_hdr.find(':') + 1), r);   // those are append-able
 }

 if(sm.from().empty())                                          // -H 'From: ...' is not given
  try_recovering_from(r);                                       // then recover from -u
}


CurlSmtp::Headers match_header(string hdr_str) {
 // map header in string (hdr_str) to value CurlSmtp::Headers

 transform(hdr_str.begin(), hdr_str.end(), hdr_str.begin(), ::tolower);
 hdr_str.front() = toupper(hdr_str.front());                    // capitalize first char

 for(int h = 0; static_cast<CurlSmtp::Headers>(h) < CurlSmtp::end_of_headers; ++h)
  if(hdr_str == ENUMS(CurlSmtp::Headers, h))
   return static_cast<CurlSmtp::Headers>(h);

 return CurlSmtp::end_of_headers;
}


vector<string> split_by(char dlm, const string &str) {
 // split str by dlm and return vector of trimmed lexemes; empty lexemes are not recorded
 vector<string> vs;

 for(size_t last = 0, found = 0; found != string::npos; last=found + 1) {
  found = str.find(dlm, last);
  string lexeme = trim_spaces(str.substr(last, found-last));
  if(not lexeme.empty())
   vs.push_back( move(lexeme) );
 }

 return vs;
}


void try_recovering_from(SharedResource &r) {
 // recover header 'From' from username and if required from smtp server too
 REVEAL(r, opt, sm, DBG())

 if(opt[CHR(OPT_USR)].str().empty()) return;
 if(opt[CHR(OPT_USR)].str().find('@') != string::npos)          // -u contains '@', quick recovery:
  { sm.from(opt[CHR(OPT_USR)].str()); return; }                 // setup 'From:' from -u

 string domain;                                                 // extract domain from server param.
 vector<string> domain_lvl = split_by('.', opt[ARG_SRV].str());
 for(auto it = domain_lvl.rbegin(); it != domain_lvl.rend(); ++it)
  if(it->find("smtp") != string::npos) break;                   // domain should not contain 'smtp'
  else domain = *it + "." + domain;
 domain.pop_back();                                             // pop trailing dot '.'

 sm.from(trim_spaces(opt[CHR(OPT_USR)].str()) + domain);
 DBG(0) DOUT() << "recovered 'From': " << trim_spaces(opt[CHR(OPT_USR)].str()) + domain << endl;
}


string & trim_trailing_spaces(std::string &str) {
 // trim all trailing spaces
 return str.erase(str.find_last_not_of(SPACES) + 1);
}


string & trim_heading_spaces(std::string &str) {
 // trim all heading spaces
 return str.erase(0, str.find_first_not_of(SPACES));
}


string trim_spaces(std::string str) {
 // trim all surrounding spaces
 return trim_heading_spaces( trim_trailing_spaces(str) );
}









