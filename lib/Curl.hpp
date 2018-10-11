/*
 * Created by Dmitry Lyssenko, last modified August 31, 2014.
 *
 * a trivial C++ wrapper around curl easy: use -lcurl when compiling
 *
 *
 *
 * SYNOPSIS: read data from web locations
 *  Curl curl;
 *  curl.deliver("http://ipinfo.io/");
 *
 *  if(curl.rc() != CURLE_OK)
 *   cerr << "curl failed: " << curl.error();
 *
 *  cout << curl.delivered() << endl;
 *
 */

#pragma once

#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>            // std::any_of, ...
#include <curl/curl.h>
#include "extensions.hpp"
#include "dbg.hpp"
#include "IBtime.hpp"           // required to generate date stamp for CurlSmtp




class Curl {
    friend void         swap(Curl &l, Curl &r) {
                         using std::swap;                       // enable ADL
                         swap(l.curl_, r.curl_);
                         swap(l.result_, r.result_);
                         swap(l.url_, r.url_);
                         swap(l.buffer_, r.buffer_);
                         swap(l.unget_, r.unget_);
                        }
 public:
    #define THROWREASON \
                curl_handle_init_falure, \
                failed_to_setup_write_handler, \
                failed_to_encode_string, \
                failed_to_decode_string, \
                failed_to_setup_url, \
                failed_to_setup_callback, \
                end_of_throw
    ENUMSTR(ThrowReason, THROWREASON)

                        Curl(void);                             // DC
                        Curl(const Curl &) = delete;            // CC: class is not copyable
                        Curl(Curl && other)                     // MC: class is movable
                         { swap(*this, other); }
                       ~Curl(void)                              // DD
                         { curl_easy_cleanup(curl_); }

    Curl &              operator=(const Curl & jn) = delete;    // CA: class is not copy assignable
    Curl &              operator=(Curl && other)                // MA: class move assignable
                         { swap(*this, other); return *this; }


    CURLcode            rc(void) const
                         { return result_; }
    const char *        error(void) const
                         { return curl_easy_strerror(result_); }
    CURL *              curl(void) { return curl_; }

    const std::string & url(void) const
                         { return url_; }
    Curl &              url(const std::string & url)
                         { url_ = url; return *this; }

    Curl &              append(const std::string & tail)
                         { url_ += tail; return *this; }

    std::string         encode_str(const std::string & str) const;
    Curl &              encode_url(void)
                         { url_ = encode_str(url_); return *this; }

    std::string         decode_str(const std::string & str) const;
    Curl &              decode_url(void)
                         { url_ = decode_str(url_); return *this; }
    template<class X>
    Curl &              setopt(CURLoption option, X param) {
                         result_ = curl_easy_setopt(curl_, option, param);
                         return *this;
                        }

    Curl &              deliver(const char * url = nullptr);
    Curl &              deliver(const std::string & url)
                         { return deliver(url.c_str()); };
    const std::string & delivered(void) const
                         { return buffer_; }
    Curl &              unget(const std::string & str)
                         { unget_ = true; buffer_ = str; return *this; }
    Curl &              perform(void) {
                         result_ = curl_easy_perform(curl_);
                         return *this;
                        }

    DEBUGGABLE()
    EXCEPTIONS(ThrowReason)                                     // see "enums.hpp"

 protected:
    CURL *              curl_{nullptr};
    CURLcode            result_;                                // curle return result

    std::string         url_;                                   // url to process
    std::string         buffer_;                                // returned data or encoded data

    bool                unget_{false};                          // unget flag
 private:

    static size_t       trivial_write_(char *ptr, size_t size, size_t n, Curl *curlPtr);
};

STRINGIFY(Curl::ThrowReason, THROWREASON)
#undef THROWREASON


Curl::Curl(void) {
 // DC: initialize culrlib, setup write handler, default option(s)
 curl_ = curl_easy_init();
 if(curl_ == nullptr)
  throw EXP(curl_handle_init_falure);

 if( setopt(CURLOPT_WRITEFUNCTION, trivial_write_).rc() != CURLE_OK)
  throw EXP(failed_to_setup_write_handler);                 // that's critical
 setopt(CURLOPT_FOLLOWLOCATION, 1L);                        // follow http redirects
}


std::string Curl::encode_str(const std::string & str) const {
 char *tmp = curl_easy_escape(curl_, str.c_str(), str.size());
 if(tmp == nullptr)
  throw EXP(failed_to_encode_string);
 std::string temp{tmp};
 curl_free(tmp);
 return temp;
}


std::string Curl::decode_str(const std::string & str) const {
 char *tmp = curl_easy_unescape(curl_, str.c_str(), str.size(), nullptr);
 if(tmp == nullptr)
  throw EXP(failed_to_decode_string);
 std::string temp{tmp};
 curl_free(tmp);
 return temp;
}


Curl & Curl::deliver(const char * url) {
 // deliver: either url from parameter, or (if null) url from class
 if(url) url_ = url;
 if(unget_)
  { unget_ = false; result_ = CURLE_OK; return *this; }

 if(setopt(CURLOPT_URL, url_.c_str()).rc() != CURLE_OK)
  throw EXP(failed_to_setup_url);
 DBG(0) DOUT() << "url: " << url_ << std::endl;

 buffer_.clear();
 if(setopt(CURLOPT_WRITEDATA, this).rc() != CURLE_OK)
  throw EXP(failed_to_setup_callback);
 perform();
 DBG(0) {
  if(result_ == CURLE_OK)
   DOUT() << "delivered: " << delivered() << std::endl;
  else
   DOUT() << "failed to deliver, rc: " << result_ << std::endl;
 }
 return *this;
}


size_t Curl::trivial_write_(char *ptr, size_t size, size_t n, Curl *me) {
 size_t total = size * n;
 me->buffer_.append(ptr, total);

 return total;
}













/* This is a basic smtp wrapper, curl based.
 *
 * Allows sending e-mails to smtp and smtps servers (these days smtp port is only open on
 * local networks only and all public mail providers accept connection on smtps port only)
 * E.g.: google accepts: 'aspmx.l.google.com' smtp host but restricts the mail only to google
 * users/apps (if you're lucky and your ISP does not block smtp port).
 *
 * SYNOPSIS: send mail
 *   CurlSmtp sm("smtp.gmail.com");
 *   sm.ssl("user@gmail.com", "user_password")
 *     .from("user@gmail.com")
 *     .subject("This is a subject")
 *     .add_to("user1@some.net")
 *     .send("Message ...\nBest regards,\n/Yours ...");
 *
 * After send()'ing all headers have to be re-set again.
 */

#define CS_EOL "\r\n"
#define MIME_ENCODER "base64"

class CurlSmtp {
    friend void         swap(CurlSmtp &l, CurlSmtp &r) {
                         using std::swap;                       // enable ADL
                         swap(l.curl_, r.curl_);
                         swap(l.recipients_, r.recipients_);
                         swap(l.ssl_, r.ssl_);
                         swap(l.username_, r.username_);
                         swap(l.password_, r.password_);
                         swap(l.headers_, r.headers_);
                         swap(l.hi_, r.hi_);
                         swap(l.scheme_, r.scheme_);
                         swap(l.host_, r.host_);
                         swap(l.separator_sent_, r.separator_sent_);
                         swap(l.mbi_, r.mbi_);
                         swap(l.mei_, r.mei_);
                        }

    #define MIMESETUP \
                plain_text, \
                mime_parts
    ENUM(MimeSetup, MIMESETUP)

 public:
    #define THROWREASON \
                curlsmpt_failed_adding_header, \
                curlsmtp_host_unset, \
                curlsmtp_recipients_unset, \
                curlsmtp_setopt_falure, \
                mime_initialization_failure, \
                mime_setup_failure, \
                end_of_throw
    ENUMSTR(ThrowReason, THROWREASON)

    #define HEADERS \
                From, \
                To, \
                Cc, \
                Bcc, \
                Subject, \
                Date, \
                end_of_headers
    ENUMSTR(Headers, HEADERS)

    typedef std::map<Headers, std::string> headersMap;


                        CurlSmtp(void)                          // DC
                         { init_headers_(); }
                        CurlSmtp(const std::string & smtpHost): host_(smtpHost)
                         { init_headers_(); }
                        CurlSmtp(const CurlSmtp &) = delete;    // CC: class is not copyable
                        CurlSmtp(CurlSmtp && other)             // MC: class is movable
                         { swap(*this, other); }

    CurlSmtp &          operator=(const CurlSmtp & jn) = delete;// CA: class is not copy assignable
    CurlSmtp &          operator=(CurlSmtp && other)            // MA: class move assignable
                         { swap(*this, other); return *this; }

    // class interface
    CURLcode            rc(void) const { return curl_.rc(); }
    const char *        error(void) { return curl_.error(); }

    // send mail specific interface (preparing a mail)
    CurlSmtp &          host(const std::string & server) { host_ = server; return *this; }
    const std::string & host(void) const { return host_; }

    CurlSmtp &          add_header(Headers header, const std::string & value);
    const headersMap &  headers(void) const { return headers_; };

    CurlSmtp &          from(const std::string & str) { return add_header(From, str); }
    const std::string & from(void) const { return headers_.at(From); }

    CurlSmtp &          subject(const std::string & str) { return add_header(Subject, str); }
    const std::string & subject(void) const { return headers_.at(Subject); }

    CurlSmtp &          add_to(const std::string & str) { return add_header(To, str); };
    const std::string & to(void) const { return headers_.at(To); }

    CurlSmtp &          add_cc(const std::string & str) { return add_header(Cc, str); }
    const std::string & cc(void) const { return headers_.at(Cc); }

    CurlSmtp &          add_bcc(const std::string & str) { return *this; }
    const std::string & bcc(void) const { return headers_.at(Bcc); }

    CurlSmtp &          attach_file(std::string str)
                         { files_.emplace_back(str); return *this; }

    CurlSmtp &          ssl(const std::string &, const std::string &); // ssl (username/pass)
    CurlSmtp &          ssl_reset(void) { ssl_ = false; scheme_ = "smtp://"; return *this; }

    // send email
    CurlSmtp &          send(const std::string & msg);

    DEBUGGABLE()
    EXCEPTIONS(ThrowReason)                                     // see "enums.hpp"

 protected:
    Curl                curl_;
    struct curl_slist * recipients_{nullptr};

    bool                ssl_ = false;
    std::string         username_;
    std::string         password_;

 private:
    CurlSmtp &          send_mime_(const std::string & msg);
    void                setup_mime_parts_(const std::string & msg);
    void                setup_send_options_(MimeSetup opt=plain_text);
    std::string         date_str_(void);
    void                init_headers_(void);
    static size_t       feed_payload_(char *ptr, size_t size, size_t n, CurlSmtp *myself);

    headersMap          headers_;
    int                 hi_;                                    // headers iterator
    std::string         scheme_{"smtp://"};
    std::string         host_;                                  // smtp host (mail server)
    bool                separator_sent_{false};                 // headers from body (RFC5322)
    std::string::const_iterator
                        mbi_;
    std::string::const_iterator
                        mei_;
    std::vector<std::string>
                        files_;

};

STRINGIFY(CurlSmtp::ThrowReason, THROWREASON)
#undef THROWREASON

STRINGIFY(CurlSmtp::Headers, HEADERS)
#undef HEADERS



CurlSmtp & CurlSmtp::ssl(const std::string &u, const std::string &p) {
 // set username/password, setup smtps protocol
 username_ = u;
 password_ = p;
 ssl_ = true;
 scheme_ = "smtps://";

 return *this;
}


CurlSmtp & CurlSmtp::send(const std::string & msg) {
 // if file attachment is given, or message (msg) is outside of ascii char set,
 // divert to mime type of sending, otherwise default to plain text send and:
 // add Date header, setup send options and send mail

 if(host_.empty()) throw EXP(curlsmtp_host_unset);
 if(recipients_ == nullptr) throw EXP(curlsmtp_recipients_unset);
 add_header(Date, date_str_());                                 // generate date

 if(not files_.empty() or std::any_of(msg.begin(), msg.end(), [](char c){ return c<0; }))
  return send_mime_(msg);

 setup_send_options_();                                         // this is a plan text mail
 DBG(0) DOUT() << "sending to: " << scheme_ << host_ << std::endl;
 hi_ = 0;
 mbi_ = msg.cbegin();
 mei_ = msg.cend();
 curl_.perform();                                               // send mail here

 DBG(0) {
  if(rc() != CURLE_OK)
   DOUT() << "returned error: " << error() << std::endl;
  else
   DOUT() << "sending done" << std::endl;
 }

 curl_slist_free_all(recipients_);                              // clean up after sending
 init_headers_();
 separator_sent_ = false;
 return *this;
}


CurlSmtp & CurlSmtp::send_mime_(const std::string & msg) {
 // send msg and attached files using mime
 setup_send_options_(mime_parts);

 // build header list and set it up
 struct curl_slist *headers = nullptr;
 for(int h = static_cast<int>(From); h < static_cast<int>(end_of_headers); ++h) {
  if(h == static_cast<int>(Bcc)) continue;
  if(headers_[static_cast<Headers>(h)].empty()) continue;
  headers = curl_slist_append(headers, (std::string{ENUMS(Headers, h)} + ": " +
                                       headers_[static_cast<Headers>(h)]).c_str());
  DBG(0) DOUT() << "posted header " << ENUMS(Headers, h) << ": "
                << headers_[static_cast<Headers>(h)] << std::endl;
 }
 curl_.setopt(CURLOPT_HTTPHEADER, headers);

 setup_mime_parts_(msg);
 curl_.perform();                // send mail
 DBG(0) {
  if(rc() != CURLE_OK)
   DOUT() << "returned error: " << error() << std::endl;
  else
   DOUT() << "sending done" << std::endl;
 }

 curl_slist_free_all(headers);
 files_.clear();
 init_headers_();
 return *this;
}


void CurlSmtp::setup_mime_parts_(const std::string & msg) {
 // setup mime parts for msg and attached files

 curl_mime *mime = curl_mime_init(curl_.curl());
 if(mime == nullptr) throw EXP(mime_initialization_failure);

 curl_mimepart *part = nullptr;
 if(not msg.empty()) {                                          // mime msg
  part = curl_mime_addpart(mime);
  curl_mime_data(part, msg.c_str(), msg.size());
  curl_mime_encoder(part, MIME_ENCODER);
 }

 for(auto &file: files_) {                                      // // mime all attached files
  part = curl_mime_addpart(mime);
  curl_mime_filedata(part, file.c_str());
  curl_mime_encoder(part, MIME_ENCODER);
  DBG(0) DOUT() << "posted file: '" << file << "'" << std::endl;
 }

 if(curl_.setopt(CURLOPT_MIMEPOST, mime).rc() != CURLE_OK)       // setup mime
  throw EXP(mime_setup_failure);
}


void CurlSmtp::setup_send_options_(MimeSetup opt) {
 // setup all required options (as well as feed handler), prepare for sending mail
 std::string url = scheme_ + host_;
 std::vector<CURLcode> r;
 r.reserve(10);

 if(opt == plain_text) {
  r.push_back(curl_.setopt(CURLOPT_READFUNCTION, feed_payload_).rc());
  r.push_back(curl_.setopt(CURLOPT_READDATA, this).rc());
  r.push_back(curl_.setopt(CURLOPT_UPLOAD, 1L).rc());
 }
 r.push_back(curl_.setopt(CURLOPT_URL, url.c_str()).rc());

 if(ssl_) {
  r.push_back(curl_.setopt(CURLOPT_USERNAME, username_.c_str()).rc());
  r.push_back(curl_.setopt(CURLOPT_PASSWORD, password_.c_str()).rc());
  r.push_back(curl_.setopt(CURLOPT_SSL_VERIFYPEER, 0L).rc());
  r.push_back(curl_.setopt(CURLOPT_SSL_VERIFYHOST, 0L).rc());
 }

 if(not headers_[From].empty())
  r.push_back(curl_.setopt(CURLOPT_MAIL_FROM, headers_[From].c_str()).rc());
 r.push_back(curl_.setopt(CURLOPT_MAIL_RCPT, recipients_).rc());

 if(std::any_of(r.begin(), r.end(), [](CURLcode cc) { return cc != CURLE_OK; }))
  throw EXP(curlsmtp_setopt_falure);
}


CurlSmtp & CurlSmtp::add_header(Headers h, const std::string & str) {
 // add any of handled headers; headers are encased into `<', `>', comma separated if multiple
 if(h >= end_of_headers) return *this;
 DBG(0) DOUT() << "adding header '" << ENUMS(Headers, h) << "'..." << std::endl;

 if(h AMONG(To, Cc, Bcc)) {                                     // i.e. if recipients
  recipients_ = curl_slist_append(recipients_, str.c_str());    // add to list of recipients
  if(recipients_ == nullptr)
   throw EXP(curlsmpt_failed_adding_header);
 }

 if(not (h AMONG(To, Cc, Bcc)))
  headers_[h].clear();                                          // non-recipients are not additive
 if(not headers_[h].empty())
  headers_[h] += ", ";
 if(not (h AMONG(Subject, Date))) headers_[h] += '<';           // if it's email header, enclose
 headers_[h] += str;
 if(not (h AMONG(Subject, Date))) headers_[h] += '>';           // if it's email header, enclose

 DBG(0) DOUT() << "'" << ENUMS(Headers, h) << "': '" << headers_[h] << '\'' << std::endl;
 return *this;
}


std::string CurlSmtp::date_str_(void) {
 // generate 'Date: ' header, e.g.: Date: Wed, 4 Jul 2018 14:21:58 +0200
 DateTime t;
 std::string date(t.weekday_c_str());                           // e.g: Wed

 date += ", " + std::to_string(t.day()) + " " + t.month_c_string(); // e.g: Wed, 4 Jul
 date += " " + std::to_string(t.year()) + " " + t.time_str();   // e.g.: Wed, 4 Jul 2018 14:21:58

 auto hd = t.utc_offset()/3600;                                 // hours delta (from UTC)
 auto md = (t.utc_offset()%3600)/60;                            // minutes delta
 std::stringstream tzo;                                         // time zone offset
 tzo << ' ' << (hd >= 0? '+': '-')
            << std::setfill('0') << std::setw(2) << hd << std::setw(2) << md;

 DBG(0) DOUT() << "Generated date: '" << date << tzo.str() << "'" << std::endl;
 return date + tzo.str();
}


size_t CurlSmtp::feed_payload_(char *ptr, size_t size, size_t n, CurlSmtp *my) {
 if((n*size) < 1) return 0;

 CurlSmtp &me = *my;
 std::string s;                                                 // s will collect all fed data
 size_t max = n * size;

 while(me.hi_ < end_of_headers and                              // skip empty and Bcc header
       (me.hi_ == Bcc or me.headers_[static_cast<Headers>(me.hi_)].empty()))
  ++me.hi_;

 if(me.hi_ < end_of_headers) {                                  // upload headers first (one by one)
  s = ENUMS(Headers, me.hi_);
  s += ": " + me.headers_[static_cast<Headers>(me.hi_)];
  ++me.hi_;
  if(s.size() > max) {
    DBG(me, 1) DOUT(me) << "header size > max allowed (" << max << "): '" << s << "'" << std::endl;
    s.resize(max);                                              // truncate if it's too long
  }
 }
 else {                                                         // headers done, upload message now
  if(me.separator_sent_) {                                      // sending body then
   size_t left = me.mei_ - me.mbi_;
   DBG(me, 1) DOUT(me) << "#bytes left to send: " << left << ", max: " << max << std:: endl;
   if(left == 0) return 0;

   for(auto end_it = left < max? me.mei_: me.mbi_+max; me.mbi_ != end_it; ++me.mbi_)
    s += *me.mbi_;                                              // copy message to sending string s
  }
  else                                                          // separator not sent: send now
   me.separator_sent_ = true;
 }

 DBG(me, 1) DOUT(me) << "uploading: '" << s << "'" << std:: endl;

 s += CS_EOL;
 memcpy(ptr, s.c_str(), s.size());
 return s.size();
}


void CurlSmtp::init_headers_(void) {
 for(int h=0; h<end_of_headers; ++h)
  headers_[static_cast<CurlSmtp::Headers>(h)].clear();
 recipients_ = nullptr;
}





#undef CS_EOL
#undef MIME_ENCODER













