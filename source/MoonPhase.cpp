// http://www.ursa.fi/taivaalla/extra/lunarphases.html
#include "MoonPhase.h"
#include <cmath>

using namespace std;

double rad(double d)
{
  return d * 0.017453292519943295;
}
double Deg(double d1)
{
  return (d1 * 180) / 3.1415926535897931;
}
double julianDay(int day, int month, int year, int hour, int minute, int second)
{
  double d3 = hour * 3600 + minute * 60 + second;
  double d1 = day + d3 / 86400;
  if (month <= 2)
  {
    month += 12;
    year--;
  }
  int k1 = year / 100;
  int l1 = (2 - k1) + k1 / 4;
  double d2 = std::floor(365.25 * (year + 4716)) + std::floor(30.600100000000001 * (month + 1)) +
              d1 + l1 + -1524.5;
  return d2;
}

double reduce(double d1)
{
  d1 -= 6.2831853071795862 * static_cast<int>(d1 / 6.2831853071795862);
  if (d1 < 0.0)
    d1 += 6.2831853071795862;
  return d1;
}

double MoonFraction(const NFmiTime& theTime)
{
  double JDnow = julianDay(theTime.GetDay(),
                           theTime.GetMonth(),
                           theTime.GetYear(),
                           theTime.GetHour(),
                           theTime.GetMin(),
                           theTime.GetSec());
  double T = (JDnow - 2415020) / 36525;
  double d15 = T * T;
  double d16 = d15 * T;
  double d7 = 1.0 - 0.0024949999999999998 * T - 7.52E-06 * d15;
  double L1 =
      ((270.43416400000001 + 481267.88309999998 * T) - 0.0011329999999999999 * d15) + 1.9E-06 * d16;
  double M = (358.47583300000002 + 35999.049800000001 * T) - 0.00014999999999999999 * d15 -
             3.3000000000000002E-06 * d16;
  double M1 = 296.10460799999998 + 477198.84909999999 * T + 0.0091920000000000005 * d15 +
              1.4399999999999999E-05 * d16;
  double d = ((350.73748599999999 + 445267.11420000001 * T) - 0.001436 * d15) +
             1.9000000000000001E-05 * d16;
  double F = (11.250889000000001 + 483202.02510000003 * T) - 0.0032109999999999999 * d15 -
             2.9999999999999999E-07 * d16;
  double OM =
      (259.18327499999998 - 1934.1420000000001 * T) + 0.002078 * d15 + 2.2000000000000001E-06 * d16;
  double d1 = sin(rad(51.200000000000003 + 20.199999999999999 * T));
  double d2 = sin(rad(OM));
  double d3 = 0.0039639999999999996 * sin(rad((346.56 + 132.87 * T) - 0.0091731 * d15));
  L1 = L1 + 0.000233 * d1 + 0.001964 * d2 + d3;
  M = M - 0.0017780000000000001 * d1;
  M1 = M1 + 0.00081700000000000002 * d1 + 0.0025409999999999999 * d2 + d3;
  d = d + 0.0020110000000000002 * d1 + 0.001964 * d2 + d3;
  F = (F - 0.024691000000000001 * d2 -
       0.0043280000000000002 * sin(rad((OM + 275.05000000000001) - 2.2999999999999998 * T))) +
      d3;
  M = reduce(rad(M));
  M1 = reduce(rad(M1));
  d = reduce(rad(d));
  F = reduce(rad(F));
  OM = reduce(rad(OM));
  double d17 = 2 * d;
  double d18 = 2 * M1;
  double d19 = 2 * F;
  double d20 = 2 * M;
  double d21 = 4 * d;
  double LunarLong =
      ((((((((((((((((((L1 + 6.2887500000000003 * sin(M1) + 1.2740180000000001 * sin(d17 - M1) +
                        0.65830900000000003 * sin(d17) + 0.213616 * sin(d18)) -
                       0.18559600000000001 * sin(M) * d7 - 0.11433599999999999 * sin(d19)) +
                      0.058792999999999998 * sin(d17 - d18) +
                      0.057211999999999999 * sin(d17 - M - M1) * d7 +
                      0.053319999999999999 * sin(d17 + M1) +
                      0.045873999999999998 * sin(d17 - M) * d7 +
                      0.041023999999999998 * sin(M1 - M) * d7) -
                     0.034717999999999999 * sin(d) - 0.030464999999999999 * sin(M + M1) * d7) +
                    0.015325999999999999 * sin(d17 - d19)) -
                   0.012527999999999999 * sin(d19 + M1) - 0.01098 * sin(d19 - M1)) +
                  0.010673999999999999 * sin(d21 - M1) + 0.010034 * sin(3 * M1) +
                  0.008548 * sin(d21 - d18)) -
                 0.0079100000000000004 * sin((M - M1) + d17) * d7 - 0.006783 * sin(d17 + M) * d7) +
                0.0051619999999999999 * sin(M1 - d) + 0.0050000000000000001 * sin(M + d) * d7 +
                0.0040489999999999996 * sin((M1 - M) + d17) * d7 +
                0.0039960000000000004 * sin(d18 + d17) + 0.003862 * sin(d21) +
                0.0036649999999999999 * sin(d17 - 3 * M1) +
                0.0026949999999999999 * sin(d18 - M) * d7 +
                0.0026020000000000001 * sin(M1 - d19 - d17) +
                0.0023960000000000001 * sin(d17 - M - d18) * d7) -
               0.002349 * sin(M1 + d)) +
              0.0022490000000000001 * sin(d17 - d20) * d7 * d7) -
             0.0021250000000000002 * sin(d18 + M) * d7 -
             0.0020790000000000001 * sin(d20) * d7 * d7) +
            0.0020590000000000001 * sin(d17 - M1 - d20) * d7 * d7) -
           0.0017730000000000001 * sin((M1 + d17) - d19) - 0.0015950000000000001 * sin(d19 + d17)) +
          0.0012199999999999999 * sin(d21 - M - M1) * d7) -
         0.0011100000000000001 * sin(d18 + d19)) +
        0.000892 * sin(M1 - 3 * d)) -
       0.00081099999999999998 * sin(M + M1 + d17) * d7) +
      0.00076099999999999996 * sin(d21 - M - d18) * d7 +
      0.00071699999999999997 * sin(M1 - d20) * d7 * d7 +
      0.00070399999999999998 * sin(M1 - d20 - d17) * d7 * d7 +
      0.00069300000000000004 * sin((M - d18) + d17) * d7 +
      0.00059800000000000001 * sin(d17 - M - d19) * d7 + 0.00055000000000000003 * sin(M1 + d21) +
      0.00053799999999999996 * sin(4 * M1) + 0.00052099999999999998 * sin(d21 - M) * d7 +
      0.000486 * sin(d18 - d);
  double d4 =
      (((((((5.1281889999999999 * sin(F) + 0.28060600000000002 * sin(M1 + F) +
             0.27769300000000002 * sin(M1 - F) + 0.173238 * sin(d17 - F) +
             0.055412999999999997 * sin((d17 + F) - M1) + 0.046272000000000001 * sin(d17 - F - M1) +
             0.032572999999999998 * sin(d17 + F) + 0.017198000000000001 * sin(d18 + F) +
             0.0092669999999999992 * sin((d17 + M1) - F) + 0.0088229999999999992 * sin(d18 - F) +
             0.0082470000000000009 * sin(d17 - M - F) * d7 +
             0.0043229999999999996 * sin(d17 - F - d18) +
             0.0041999999999999997 * sin(d17 + F + M1) + 0.003372 * sin(F - M - d17) * d7 +
             0.0024719999999999998 * sin((d17 + F) - M - M1) * d7 +
             0.002222 * sin((d17 + F) - M) * d7 +
             0.0020720000000000001 * sin(d17 - F - M - M1) * d7 +
             0.001877 * sin((F - M) + M1) * d7 + 0.001828 * sin(d21 - F - M1)) -
            0.0018029999999999999 * sin(F + M) * d7 - 0.00175 * sin(3 * F)) +
           0.00157 * sin(M1 - M - F) * d7) -
          0.001487 * sin(F + d) - 0.0014809999999999999 * sin(F + M + M1) * d7) +
         0.0014170000000000001 * sin(F - M - M1) * d7 + 0.0013500000000000001 * sin(F - M) * d7 +
         0.00133 * sin(F - d) + 0.001106 * sin(F + 3 * M1) + 0.0010200000000000001 * sin(d21 - F) +
         0.00083299999999999997 * sin((F + d21) - M1) + 0.00078100000000000001 * sin(M1 - 3 * F) +
         0.00067000000000000002 * sin((F + d21) - d18) + 0.00060599999999999998 * sin(d17 - 3 * F) +
         0.00059699999999999998 * sin((d17 + d18) - F) +
         0.00049200000000000003 * sin((d17 + M1) - M - F) * d7 +
         0.00044999999999999999 * sin(d18 - F - d17) + 0.00043899999999999999 * sin(3 * M1 - F) +
         0.00042299999999999998 * sin(F + d17 + d18) +
         0.00042200000000000001 * sin(d17 - F - 3 * M1)) -
        0.00036699999999999998 * sin((M + F + d17) - M1) * d7 -
        0.00035300000000000002 * sin(M + F + d17) * d7) +
       0.00033100000000000002 * sin(F + d21) +
       0.00031700000000000001 * sin(((d17 + F) - M) + M1) * d7 +
       0.00030600000000000001 * sin(d17 - d20 - F) * d7 * d7) -
      0.00028299999999999999 * sin(M1 + 3 * F);
  double d5 = 0.00046640000000000001 * cos(OM);
  double d6 = 7.5400000000000003E-05 * cos(OM + rad(275.05000000000001 - 2.2999999999999998 * T));
  LunarLong = reduce(rad(LunarLong));
  double LunarLat = rad(d4 * (1.0 - d5 - d6));

  double d8 = 279.69668000000001 + 36000.768920000002 * T + 0.00030249999999999998 * d15;
  double d9 = reduce(rad((358.47582999999997 + 35999.049749999998 * T) -
                         0.00014999999999999999 * d15 - 3.3000000000000002E-06 * d16));
  double d10 = (1.9194599999999999 - 0.0047889999999999999 * T - 1.4E-05 * d15) * sin(d9) +
               (0.020094000000000001 - 0.0001 * T) * sin(2 * d9) +
               0.00029300000000000002 * sin(3 * d9);
  double d12 = reduce(rad(d8 + d10));
  d = acos(cos(LunarLong - d12) * cos(LunarLat));
  double d13 = 180 - Deg(d) -
               0.14680000000000001 *
                   ((1.0 - 0.054899999999999997 * sin(M1)) / (1.0 - 0.0167 * sin(M1))) * sin(d);
  double K = (1.0 + cos(rad(d13))) * 50 + 0.5;
  return K;
}
