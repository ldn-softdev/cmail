/*
 * Classes Time, Date and DateTime convert time date from/to string format.
 * class strictly require string in format YYYYMMDD HH:MM:SS (no time zone!)
 * timezone notion can be programmatically provided either LT (whatever local TZ) or UTC
 */
#pragma once


#include <iomanip>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include "extensions.hpp"



class DateTime {
 public:
    enum Locality { LT, UTC };                                  // LT = Local Time

    #define THROWREASON \
                space_only_separator_allowed, \
                invalid_date_or_time_format, \
                invalid_date_format, \
                invalid_time_format, \
                stamp_missing, \
                bogus_date_stamp, \
                year_below_1970, \
                month_out_of_range_01_12, \
                day_out_of_range_01_31, \
                seconds_out_of_range_00_59, \
                minutes_out_of_range_00_59, \
                hours_out_of_range_00_23, \
                trailing_symbols_disallowed
    ENUMSTR(ThrowReason, THROWREASON);

    #define MON Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
    ENUMSTR( Mon, MON )

    #define MONTH January, February_, March, April, may, June, \
                  July, August, Septempter, October, November, December
    ENUMSTR( Month, MONTH )

    #define WD Sun, Mon, Tue, Wed, Thu, Fri, Sat
    ENUMSTR( Weekday, WD )

    #define FULLWD Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday
    ENUMSTR( FullWeekday, FULLWD )


                        DateTime(void) { now(); }
                        DateTime(time_t stamp): stamp_(stamp) { }
                        DateTime(const std::string &str, Locality l=LT) { set_DateTime(str, l); }
                        DateTime(std::string &&str, Locality l=LT): DateTime(str, l) {}

    // set date and time
    // set custom date/time
    DateTime &          set_DateTime(const std::string &str, Locality l=LT );
    DateTime &          set_time(const std::string &str, Locality l=LT);     // e.g., str: '11:22:33'
    DateTime &          set_time(int hour, int munites, int seconds, Locality l=LT);
    DateTime &          set_time(int ts, Locality l=LT)
                         { return set_time(ts/10000, ts%10000/100, ts%100, l); }
    DateTime &          set_date(const std::string &str, Locality l=LT);     // e.g., str: 19710915
    DateTime &          set_date(int day, int month, int year, Locality l=LT);
    DateTime &          set_date(int ds, Locality l=LT)
                         { return set_date(ds%100, ds%10000/100, ds/10000, l); }
    // set current date and time
    DateTime &          now(void);                              // set timestamp to now localtime

    // manipulate date and time
    DateTime &          add(time_t n) { stamp_ += n; return *this; }
    DateTime &          add_minutes(long n) { stamp_ += n *60; return *this; }
    DateTime &          add_hours(long n) { stamp_ += n *60*60 ; return *this; }
    DateTime &          add_days(long n) { stamp_ += n *60*60*24; return *this; }
    DateTime &          add_weeks(long n) { stamp_ += n *60*60*24*7; return *this; }
    DateTime &          add_months(long n);
    DateTime &          add_years(long n) { return add_months(n*12); }

    // return date and time:
    // return time stamp (epoch seconds)
    time_t              stamp(void) const { return stamp_; }
    DateTime &          stamp(time_t s) { stamp_ = s; return *this; }
    // cmp is in the future (to me) - delta is positive, cmp is in the past - delta is negative
    time_t              delta(time_t stmp)const { return stmp - stamp(); }
    time_t              delta(const DateTime &cmp) const { return cmp.stamp() - stamp(); }
    time_t              delta_minutes(const DateTime &cmp) const { return delta(cmp) / (60); }
    time_t              delta_hours(const DateTime &cmp) const { return delta(cmp) / (60*60); }
    time_t              delta_days(const DateTime &cmp) const { return delta(cmp) / (60*60*24); }
    time_t              delta_weeks(const DateTime &cmp) const { return delta(cmp) / (60*60*24*7); }

    // return date/time components
    int                 seconds(Locality l=LT) const;           // seconds (0 - 60)
    int                 minutes(Locality l=LT) const;           // minutes (0 - 59)
    int                 hours(Locality l=LT) const;             // hours (0 - 23)
    int                 day(Locality l=LT) const;               // day of month (1 - 31)
    int                 month(Locality l=LT) const;             // month of year (1 - 12)
    const char *        month_c_str(Locality l=LT)
                         const { return Mon_str[month(l)-1]; }
    const char *        month_c_string(Locality l=LT) const
                         { auto m = month(l)-1; return m==may? Mon_str[may]: Month_str[m]; }
    int                 year(Locality l=LT) const;              // year
    int                 weekday(Locality l=LT) const;           // day of week (Sunday = 0)
    const char *        weekday_c_str(Locality l=LT) const
                         { return Weekday_str[weekday(l)]; }
    const char *        weekday_c_string(Locality l=LT) const
                         { return FullWeekday_str[weekday(l)]; }
    int                 yearday(Locality l=LT) const;           // day of year (0 - 365)
    int                 datestamp(Locality l=LT) const;         // e.g: 20150131 (year+month+day)
    int                 timestamp(Locality l=LT) const;         // e.g: 93059 (hours+minutes+seconds)
    int                 utc_offset(void);                       // offset to UTC in seconds

    // return string representation of date/time
    std::string         time_str(Locality l=LT) const;
    std::string         date_str(Locality l=LT) const;
    std::string         str(Locality l=LT) const;               // full date time format
    const char*         c_str(Locality l=LT) { return str(l).c_str(); }     // full format in c-str
                        operator std::string(void) { return str(); }
    bool                operator >(const DateTime & rhs)const { return stamp_ > rhs.stamp(); }
    bool                operator >=(const DateTime & rhs)const { return stamp_ >= rhs.stamp(); }
    bool                operator <(const DateTime & rhs)const { return stamp_ < rhs.stamp(); }
    bool                operator <=(const DateTime & rhs)const { return stamp_ <= rhs.stamp(); }
    bool                operator ==(const DateTime & rhs)const { return stamp_ == rhs.stamp(); }
    bool                operator !=(const DateTime & rhs) const { return stamp_ != rhs.stamp(); }

    // setting any dateSeparator triggers stringification of Mon
    DateTime &          date_separator(const std::string & s) { ds = s; return *this; }
    const std::string & date_separator(void) const { return ds; }
    DateTime &          time_separator(const std::string & s) { ts = s; return *this; }
    const std::string & time_separator(void) const { return ds; }

    EXCEPTIONS(ThrowReason)                                     // see "enums.hpp"

 private:
    time_t              stamp_;                                 // [seconds], UTC

    void                parseDate_(const std::string &, tm &);
    void                parseTime_(const std::string &, tm &);
    time_t              timegm_portable(struct tm *);

    static std::string  ds;                                    // output date separator
    static std::string  ts;                                    // output time separator
};

STRINGIFY( DateTime::ThrowReason, THROWREASON)
#undef THROWREASON

STRINGIFY( DateTime::Mon, MON)
#undef MON

STRINGIFY( DateTime::Month, MONTH)
#undef MONTH

STRINGIFY( DateTime::Weekday, WD )
#undef WD

STRINGIFY( DateTime::FullWeekday, FULLWD )
#undef FULLWD

std::string DateTime::ds;
std::string DateTime::ts{":"};
// typically, programs use the same uniform date-time format through
// out entire program, hence defining separators as class static


DateTime & DateTime::set_DateTime(const std::string &str, Locality l) {
 // str in format: "YYYYMMDD HH:MM:SS"; locality is either LT or UTC
 tm dateTime{0};
 parseDate_(str, dateTime);
 int date = (dateTime.tm_year + 1900) * 10000 + (dateTime.tm_mon + 1) * 100 + dateTime.tm_mday;

 if(str[8] != ' ') throw EXP(space_only_separator_allowed);
 parseTime_(&str[9], dateTime);

 if(l == UTC) stamp_ = timegm_portable(& dateTime);
 else stamp_ = mktime(& dateTime);
 if(stamp_ == -1) throw EXP(invalid_date_or_time_format);
 if(datestamp() != date) throw EXP(bogus_date_stamp);
 return *this;
}


DateTime & DateTime::set_time(const std::string &str, Locality l) {
 // str in format: "HH:MM:SS"; locality is either LT or UTC
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 parseTime_(str, *tmp);
 if(l == UTC) stamp_ = timegm_portable(tmp);
 else stamp_ = mktime(tmp);
 if(stamp_ == -1) throw EXP(invalid_time_format);
 return *this;
}


DateTime & DateTime::set_time(int hours, int minutes, int seconds, Locality l) {
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 tmp->tm_hour = hours;
 tmp->tm_min = minutes;
 tmp->tm_sec = seconds;
 if(l == UTC) stamp_ = timegm_portable(tmp);
 else stamp_ = mktime(tmp);
 if(stamp_ == -1) throw EXP(invalid_time_format);
 return *this;
}


DateTime & DateTime::set_date(const std::string &str, Locality l) {
 // str in format: "YYYYMMDD"; locality is either LT or UTC
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 parseDate_(str, *tmp);
 if(l == UTC) stamp_ = timegm_portable(tmp);
 else stamp_ = mktime(tmp);
 if(stamp_ == -1) throw EXP(invalid_date_format);
 return *this;
}


DateTime & DateTime::set_date(int day, int month, int year, Locality l) {
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 tmp->tm_mday = day;
 tmp->tm_mon = month-1;
 tmp->tm_year = year-1900;
 if(l == UTC) stamp_ = timegm_portable(tmp);
 else stamp_ = mktime(tmp);
 if(stamp_ == -1) throw EXP(invalid_date_format);
 return *this;
}


DateTime & DateTime::now(void) {
 // now() is setting epoch seconds (since 0 hour, Jan 1, 1970 in UTC - it's universal time
 struct timeval tv;
 gettimeofday(&tv, nullptr);
 stamp_ = tv.tv_sec;
 return *this;
}


DateTime & DateTime::add_months(long n) {
 tm * tmp;
 tmp = gmtime(& stamp_);
 int originalDay = tmp->tm_mday;

 int newMonth = (tmp->tm_mon + n) % 12;
 int newYearDelta = (tmp->tm_mon + n) / 12;
 if(newMonth < 0) {
  newMonth += 12;
  newYearDelta--;
 }
 tmp->tm_mon = newMonth;
 tmp->tm_year += newYearDelta;

 stamp_ = timegm_portable(tmp);

 // check for month adjustments. each month is different in length thus need to adjust
 tmp = gmtime(& stamp_);
 if( tmp->tm_mday != originalDay)
  add_days(-tmp->tm_mday);

 return *this;
}


int DateTime::seconds(Locality l) const {
 // returns seconds part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_sec;
}


int DateTime::minutes(Locality l) const {
 // returns minutes part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_min;
}


int DateTime::hours(Locality l) const {
 // returns hours part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_hour;
}


int DateTime::day(Locality l) const {
 // returns day part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_mday;
}


int DateTime::month(Locality l) const {
 // returns month part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_mon+1;
}


int DateTime::year(Locality l) const {
 // returns year part in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_year+1900;
}


int DateTime::weekday(Locality l) const {
 // returns weekday of the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_wday;
}

int DateTime::yearday(Locality l) const {
 // returns day of a year in the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);
 return tmp->tm_yday;
}

int DateTime::datestamp(Locality l) const {                     // e.g: 20150131 (year+month+day)
 // returns date stamp expressed as an integer
 return year(l) * 10000 + month(l)*100 + day(l);
}

int DateTime::timestamp(Locality l) const {                     // e.g: 93059 (hours+minutes+seconds)
  // returns time stamp expressed as an integer
return hours(l) * 10000 + minutes(l) * 100 + seconds(l);
}

int DateTime::utc_offset(void) {
 // returns seconds local time offset from UTC in seconds
 tm * tmp;
 tmp = localtime(& stamp_);
 return tmp->tm_gmtoff;
}

void DateTime::parseDate_(const std::string &str, tm & dateTime) {
 // fills dateTime structure tm from string "YYYYMMDD"
 long y=-1, m=-1, d=-1;                                         // year, month, day

 std::stringstream sin(str);
 sin.seekg(0) >> d;
 if(d == -1) throw EXP(stamp_missing);

 y = d/10000-1900; d %= 10000;
 if(y < 70) throw EXP(year_below_1970);

 m = d/100-1; d %= 100;
 if(m > 11) throw EXP(month_out_of_range_01_12);
 if(d < 1 or d > 31) throw EXP(day_out_of_range_01_31);

 dateTime.tm_year = y;
 dateTime.tm_mon = m;
 dateTime.tm_mday = d;
}


void DateTime::parseTime_(const std::string & str, tm & dateTime) {
 // fills dateTime structure tm from string "HH:MM:SS"
 long h=-1, m=-1, s=-1;                                         // hours, mins, secs
 std::string timeStr{str + ':'};
 std::stringstream sin(timeStr);
 sin.seekg(0);
 int so = 0;                                                    // space offset

 while(sin.peek() == ' ') { sin.get(); ++so; }
 sin >> h;
 if(h > 23 or sin.tellg() != (2+so)) throw EXP(hours_out_of_range_00_23);

 sin.ignore(1,':') >> m;
 if(m > 59 or sin.tellg() != (5+so)) throw EXP(minutes_out_of_range_00_59);

 sin.ignore(1,':') >> s;
 if(s > 59 or sin.tellg() != (8+so)) throw EXP(seconds_out_of_range_00_59);

 char c;
 sin >> c >> c;
 if(c != ':' or not sin.eof()) throw EXP(trailing_symbols_disallowed);

 dateTime.tm_hour = h;
 dateTime.tm_min = m;
 dateTime.tm_sec = s;
}


std::string DateTime::time_str(Locality l) const {
 // return string representing time (HH:MM:SS) from the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);

 std::stringstream so;
 so << std::setfill('0') << std::setw(2) << tmp->tm_hour << ts
    << std::setfill('0') << std::setw(2) << tmp->tm_min << ts
    << std::setfill('0') << std::setw(2) << tmp->tm_sec;
 return so.str();
}


std::string DateTime::date_str(Locality l) const {
 // return string representing date (YYYYMMDD) from the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);

 std::stringstream so;
 so << tmp->tm_year+1900 << ds;
 if( ds.empty() )
  so << std::setfill('0') << std::setw(2) << tmp->tm_mon+1 << ds;
 else
  so << Mon_str[tmp->tm_mon] << ds;
 so << std::setfill('0') << std::setw(2) << tmp->tm_mday;
 return so.str();
}


std::string DateTime::str(Locality l) const {
 // return string representing date-time (YYYYMMDD HH:MM:SS) from the stamp
 tm * tmp;
 if(l == UTC) tmp = gmtime(& stamp_);
 else tmp = localtime(& stamp_);

 std::stringstream so;
 so << tmp->tm_year+1900 << ds;
 if( ds.empty() )
  so << std::setfill('0') << std::setw(2) << tmp->tm_mon+1 << ds;
 else
  so << Mon_str[tmp->tm_mon] << ds;
 so << std::setfill('0') << std::setw(2) << tmp->tm_mday << ' '
    << std::setfill('0') << std::setw(2) << tmp->tm_hour << ts
    << std::setfill('0') << std::setw(2) << tmp->tm_min << ts
    << std::setfill('0') << std::setw(2) << tmp->tm_sec;
 return so.str();
}



// implementation as per 'man timegm'
time_t DateTime::timegm_portable(struct tm * dateTime) {
 time_t ret;
 char *tz;

 tz = getenv("TZ");
 setenv("TZ", "", 1);
 tzset();
  ret = mktime(dateTime);
 if (tz) setenv("TZ", tz, 1);
 else unsetenv("TZ");
 tzset();
 return ret;
}




















