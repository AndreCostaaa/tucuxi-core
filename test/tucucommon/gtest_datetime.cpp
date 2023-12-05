//@@license@@

#include <gtest/gtest.h>

#include "date/date.h"

#include "tucucommon/datetime.h"
#include "tucucommon/duration.h"
#include "tucucommon/timeofday.h"
#include "tucucommon/utils.h"

using namespace std::chrono_literals;
using namespace date;

class GTestDateTime
{
public:
    void checkDateTime(
            const Tucuxi::Common::DateTime& _date, int _year, int _month, int _day, int _hour, int _minute, int _second)
    {
        EXPECT_EQ(_date.year(), _year);
        EXPECT_EQ(_date.month(), _month);
        EXPECT_EQ(_date.day(), _day);
        EXPECT_EQ(_date.hour(), _hour);
        EXPECT_EQ(_date.minute(), _minute);
        EXPECT_EQ(_date.second(), _second);
    }

    void checkTimeOfDay(const Tucuxi::Common::TimeOfDay& _time, int _hour, int _minute, int _second)
    {
        EXPECT_EQ(_time.hour(), _hour);
        EXPECT_EQ(_time.minute(), _minute);
        EXPECT_EQ(_time.second(), _second);
    }
    void checkDuration(const Tucuxi::Common::Duration& _duration, double _nbSeconds)
    {
        bool isEmpty = _nbSeconds == 0.0;
        bool isNegative = _nbSeconds < 0.0;
        double nSeconds = _nbSeconds;
        EXPECT_TRUE(_duration.isEmpty() == isEmpty);
        EXPECT_TRUE(_duration.isNegative() == isNegative);
        EXPECT_DOUBLE_EQ(_duration.toSeconds(), nSeconds);
        EXPECT_DOUBLE_EQ(_duration.toMinutes(), nSeconds / 60);
        EXPECT_DOUBLE_EQ(_duration.toHours(), nSeconds / 60 / 60);
        EXPECT_DOUBLE_EQ(_duration.toDays(), nSeconds / 60 / 60 / 24);
        double d = static_cast<double>(_duration.toMilliseconds()) / 1000.0;
        EXPECT_DOUBLE_EQ(d, _nbSeconds);
    }
};

TEST (Common_DateTimeTest, DateTime){
    GTestDateTime testDateTime;

    // Test constructors and getter methods
    Tucuxi::Common::DateTime d1(2017_y / jan / 1);
    Tucuxi::Common::DateTime d2(2018_y / jan / 1);
    Tucuxi::Common::DateTime d3(2012_y / feb / 22, 22h);
    Tucuxi::Common::DateTime d4(2013_y / mar / 23, 23min);
    Tucuxi::Common::DateTime d5(2014_y / apr / 24, 22h + 23min);
    Tucuxi::Common::DateTime d6(2015_y / may / 25, 24s);
    Tucuxi::Common::DateTime d7(2016_y / jun / 26, 23min + 24s);
    Tucuxi::Common::DateTime d8(2017_y / jul / 27, 22h + 23min + 24s);
    Tucuxi::Common::DateTime d9(2018_y / aug / 28, Tucuxi::Common::TimeOfDay(22h + 23min + 24s));
    Tucuxi::Common::DateTime d10("2017-12-17 17:34:20", "%Y-%m-%d %H:%M:%S");
    Tucuxi::Common::DateTime d11(1951_y / may / 25, 24s);
    Tucuxi::Common::DateTime d12("1951-12-17 17:34:20", "%Y-%m-%d %H:%M:%S");

    testDateTime.checkDateTime(d1, 2017, 1, 1, 0, 0, 0);
    testDateTime.checkDateTime(d2, 2018, 1, 1, 0, 0, 0);
    testDateTime.checkDateTime(d3, 2012, 2, 22, 22, 0, 0);
    testDateTime.checkDateTime(d4, 2013, 3, 23, 0, 23, 0);
    testDateTime.checkDateTime(d5, 2014, 4, 24, 22, 23, 0);
    testDateTime.checkDateTime(d6, 2015, 5, 25, 0, 0, 24);
    testDateTime.checkDateTime(d7, 2016, 6, 26, 0, 23, 24);
    testDateTime.checkDateTime(d8, 2017, 7, 27, 22, 23, 24);
    testDateTime.checkDateTime(d9, 2018, 8, 28, 22, 23, 24);
    testDateTime.checkDateTime(d10, 2017, 12, 17, 17, 34, 20);
    testDateTime.checkDateTime(d11, 1951, 5, 25, 0, 0, 24);
    testDateTime.checkDateTime(d12, 1951, 12, 17, 17, 34, 20);

    // Test thrown exception
    EXPECT_THROW(
            Tucuxi::Common::DateTime d13("mauvais format", "%Y-%m-%dT%H:%M:%S"), std::runtime_error);
    EXPECT_THROW(
            Tucuxi::Common::DateTime d14("1911-12-17 s", "%Y-%m-%dT%H:%M:%S"), std::runtime_error);
    EXPECT_THROW(Tucuxi::Common::DateTime d15("", "%Y-%m-%dT%H:%M:%S"), std::runtime_error);
    EXPECT_THROW(Tucuxi::Common::DateTime d16("17:34:20", "%Y-%m-%dT%H:%M:%S"), std::runtime_error);

    // Test differences
    Tucuxi::Common::Duration diff = d2 - d1;
    EXPECT_TRUE(diff.toDays() == 365);
    EXPECT_TRUE(diff.toMonths() != 12); // Since a year is 365.2425 days
    EXPECT_TRUE(diff.toYears() != 1);   // Since a month is 365.2425/12
    diff = diff + Tucuxi::Common::Duration(24h) * 0.25;
    EXPECT_TRUE(diff.toMonths() == 12);
    EXPECT_TRUE(diff.toYears() == 1);

    // Test getDate
    Tucuxi::Common::DateTime d20(2017_y / jan / 1, 10h + 22min);
    testDateTime.checkDateTime(d20, 2017, 1, 1, 10, 22, 0);
    EXPECT_TRUE(d1.getDate() == d20.getDate());

    // Test getTimeOfDay
    testDateTime.checkTimeOfDay(d20.getTimeOfDay(), 10, 22, 0);

    // Test setDate and setTimeOfDay
    Tucuxi::Common::DateTime d21;
    d21.setDate(d1.getDate());
    d21.setTimeOfDay(Tucuxi::Common::TimeOfDay(11h + 22min + 30s));
    testDateTime.checkDateTime(d21, 2017, 1, 1, 11, 22, 30);

    // Test reset and isUndefined
    EXPECT_FALSE(d1.isUndefined());
    d1.reset();
    EXPECT_TRUE(d1.isUndefined());
    d1.setDate(d2.getDate());
    EXPECT_FALSE(d1.isUndefined());

    // Test to and from seconds
    double nSeconds = d10.toSeconds();
    Tucuxi::Common::DateTime d30 = Tucuxi::Common::DateTime::fromDurationSinceEpoch(
            Tucuxi::Common::Duration(std::chrono::seconds(static_cast<int64>(nSeconds))));
    testDateTime.checkDateTime(d30, 2017, 12, 17, 17, 34, 20);

    // Test addDays, addMonth and addYears
    Tucuxi::Common::DateTime d31 = d10;
    d31.addDays(3);
    testDateTime.checkDateTime(d31, 2017, 12, 20, 17, 34, 20);
    d31.addDays(31);
    testDateTime.checkDateTime(d31, 2018, 1, 20, 17, 34, 20);
    d31.addDays(365);
    testDateTime.checkDateTime(d31, 2019, 1, 20, 17, 34, 20);
    d31.addDays(-399);
    testDateTime.checkDateTime(d31, 2017, 12, 17, 17, 34, 20);

    Tucuxi::Common::DateTime d32 = d10;
    d32.addMonths(3);
    testDateTime.checkDateTime(d32, 2018, 3, 17, 17, 34, 20);
    d32.addMonths(25);
    testDateTime.checkDateTime(d32, 2020, 4, 17, 17, 34, 20);
    d32.addMonths(-12);
    testDateTime.checkDateTime(d32, 2019, 4, 17, 17, 34, 20);

    Tucuxi::Common::DateTime d33 = d10;
    d33.addYears(3);
    testDateTime.checkDateTime(d33, 2020, 12, 17, 17, 34, 20);
    d33.addYears(-5);
    testDateTime.checkDateTime(d33, 2015, 12, 17, 17, 34, 20);

    {

        // dateDiffInDays and related functions are in utils.cpp, but it seems
        // reasonable to test them here
        Tucuxi::Common::DateTime d100(2016_y / jun / 26, 23min + 24s);
        Tucuxi::Common::DateTime d101(2017_y / jun / 26, 23min + 24s);

        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInYears(d100, d101), 1);
        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInYears(d101, d100), 1);
        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInDays(d100, d101), 365);
        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInDays(d101, d100), 365);
        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInWeeks(d100, d101), 52);
        EXPECT_EQ(Tucuxi::Common::Utils::dateDiffInWeeks(d101, d100), 52);
    }
}

TEST (Common_DateTimeTest, TimeOfDay){
    GTestDateTime testDateTime;

    // Test constructors
    Tucuxi::Common::TimeOfDay t1(22h);
    Tucuxi::Common::TimeOfDay t2(22h + 23min);
    Tucuxi::Common::TimeOfDay t3(22h + 23min + 24s);
    Tucuxi::Common::TimeOfDay t4(333s);
    Tucuxi::Common::TimeOfDay t5(48h + 10min + 3s);
    Tucuxi::Common::TimeOfDay t6(26h);
    testDateTime.checkTimeOfDay(t1, 22, 0, 0);
    testDateTime.checkTimeOfDay(t2, 22, 23, 0);
    testDateTime.checkTimeOfDay(t3, 22, 23, 24);
    testDateTime.checkTimeOfDay(t4, 0, 5, 33);
    testDateTime.checkTimeOfDay(t5, 0, 10, 3);
    testDateTime.checkTimeOfDay(t6, 2, 0, 0);

}

TEST (Common_DateTimeTest, Duration){
    GTestDateTime testDateTime;

    // Test constructors
    Tucuxi::Common::Duration d1;
    testDateTime.checkDuration(d1, 0);
    Tucuxi::Common::Duration d2(2h);
    testDateTime.checkDuration(d2, 2 * 60 * 60);
    Tucuxi::Common::Duration d3(-3h);
    testDateTime.checkDuration(d3, -3 * 60 * 60);
    Tucuxi::Common::Duration d4(437ms);
    testDateTime.checkDuration(d4, 0.437);

    // Test operators
    Tucuxi::Common::Duration d5 = d2 * 2;
    testDateTime.checkDuration(d5, 4 * 60 * 60);
    d5 *= 2;
    testDateTime.checkDuration(d5, 8 * 60 * 60);
    Tucuxi::Common::Duration d6 = d2 / 2;
    testDateTime.checkDuration(d6, 1 * 60 * 60);
    d6 /= 2;
    testDateTime.checkDuration(d6, 0.5 * 60 * 60);
    Tucuxi::Common::Duration d7 = d2 + 20s;
    testDateTime.checkDuration(d7, 2 * 60 * 60 + 20);
    d7 += 20s;
    testDateTime.checkDuration(d7, 2 * 60 * 60 + 40);
    Tucuxi::Common::Duration d8 = d2 - 20s;
    testDateTime.checkDuration(d8, 2 * 60 * 60 - 20);
    d8 -= 20s;
    testDateTime.checkDuration(d8, 2 * 60 * 60 - 40);
    Tucuxi::Common::Duration d9 = d2 % 25min;
    testDateTime.checkDuration(d9, 20 * 60);
    Tucuxi::Common::Duration d10 = d2 % 7;
    testDateTime.checkDuration(d10, 4);

    double n = d2 / 15min;
    EXPECT_TRUE(n == 8);

    EXPECT_TRUE(d2 > d3);
    EXPECT_TRUE(d2 > d4);
    EXPECT_TRUE(d4 > d1);
    EXPECT_TRUE(d4 > d3);

    // Test other functions
    d2.clear();
    testDateTime.checkDuration(d2, 0);

    Tucuxi::Common::DateTime now = Tucuxi::Common::DateTime::now();
    Tucuxi::Common::DateTime later = now + 2h;
    EXPECT_TRUE(later - now == 2h);
    EXPECT_TRUE(now - later == -2h);

    // TODO : Rethink this test, as if done before midnight it fails
    Tucuxi::Common::TimeOfDay tod1 = now.getTimeOfDay().getRealDuration();
    Tucuxi::Common::TimeOfDay tod2 = later.getTimeOfDay().getRealDuration();
    EXPECT_TRUE(tod2 - tod1 == 2h);
}
