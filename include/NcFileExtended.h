#pragma once

#include "nctools.h"
#include <boost/date_time/posix_time/ptime.hpp>
#include <newbase/NFmiTimeList.h>
#include <memory>
#include <netcdfcpp.h>
#include <set>

namespace nctools
{
class NcFileExtended : public NcFile
{
 public:
  std::string path;
  NcFileExtended(std::string path,
                 long timeshift,
                 FileMode = ReadOnly,
                 size_t *bufrsizeptr = nullptr,  // optional tuning parameters
                 size_t initialsize = 0,
                 FileFormat = Classic);

  void initAxis(const boost::optional<std::string> &xname,
                const boost::optional<std::string> &yname,
                const boost::optional<std::string> &zname,
                const boost::optional<std::string> &tname);

  void printInfo() const;

  NcVar *x_axis() const { return x; }
  NcVar *y_axis() const { return y; }
  NcVar *z_axis() const { return z; }
  NcVar *t_axis() const { return t; }

  bool is_dim(const std::string &name) const;

  bool axis_match(NcVar *var) const;

  std::string grid_mapping();
  unsigned long xsize();                 // Count of elements on x-axis
  unsigned long ysize();                 // Count of elements on y-axis
  unsigned long zsize();                 // Count of elements on z-axis
  unsigned long tsize();                 // Count of elements on t-axis
  unsigned long axis_size(NcVar *axis);  // Generic dimension of an axis(=count of elements)
  std::shared_ptr<std::string> get_axis_units(
      NcVar *axis);  // String presentation of a particular units on an axis
  double get_axis_scale(NcVar *axis,
                        std::shared_ptr<std::string> *source_units,
                        const std::string *target_units = nullptr);  // Get scaling multiplier to
                                                                     // convert axis to target
                                                                     // units, default target being
                                                                     // meters
  double x_scale();  // x scaling multiplier to meters
  double y_scale();  // y scaling multiplier to meters
  double z_scale();  // z scaling multiplier to meters
  double xmin();
  double xmax();
  double ymin();
  double ymax();
  double zmin();
  double zmax();
  bool xinverted();        // True, if x axis is descending
  bool yinverted();        // True, if y axis is descending
  bool isStereographic();  // True, if this is a stereographic projection
  double longitudeOfProjectionOrigin;
  double latitudeOfProjectionOrigin;

  void copy_values(
      const Options &options,
      NFmiFastQueryInfo &info,
      const ParamConversions &paramconvs,
      bool useAutoGeneratedIds = false);  // Copy data to already existing querydata object

  bool joinable(NcFileExtended &ncfile, std::vector<std::string> *failreasons = nullptr);
  NcVar *find_variable(const std::string &name);
  NFmiTimeList timeList(std::string varName = "time", std::string unitAttrName = "units");
  long timeshift;  // Desired timeshift in minutes for time axis reading
  void require_conventions(const std::string *reference);  // Validate data conforms to the
                                                           // reference in string(nullptr or empty
                                                           // string=always validates)
  double tolerance;                                        // Axis stepping tolerance

 private:
  std::shared_ptr<std::string> projectionName;
  NcVar *x;
  NcVar *y;
  NcVar *z;
  NcVar *t;
  bool minmaxfound;
  double _xmin, _xmax, _ymin, _ymax, _zmin, _zmax;
  bool _xinverted, _yinverted, _zinverted;
  std::shared_ptr<std::string> x_units, y_units, z_units;
  double xscale, yscale, zscale;

  NcVar *axis(const std::set<std::string> &axisnames);  // Find generic axis by name
  void find_axis_bounds(
      NcVar *, int n, double &x1, double &x2, const char *name, bool &isdescending);
  void find_lonlat_bounds(double &lon1, double &lat1, double &lon2, double &lat2);
  void find_bounds();
  void parse_time_units(boost::posix_time::ptime *origintime, long *timeunit) const;
  void copy_values(NFmiFastQueryInfo &info,
                   const ParamInfo &pinfo,
                   const nctools::Options *options);
  void copy_values(const Options &options, NcVar *var, NFmiFastQueryInfo &info);
  std::shared_ptr<NFmiTimeList> timelist;
};

std::string get_name(NcVar *var);
ParamInfo parse_parameter(NcVar *var, const ParamConversions &paramconvs, bool useAutoGeneratedIds);
NcVar *find_variable(const NcFile &ncfile, const std::string &name);
float get_missingvalue(NcVar *var);
float get_scale(NcVar *var);
float get_offset(NcVar *var);
float normalize_units(float value, const std::string &units);
void report_units(NcVar *var,
                  const std::string &units,
                  const Options &options,
                  bool ignoreUnitChange = false);
unsigned long get_units_in_seconds(std::string unit_str);
NFmiMetTime tomettime(const boost::posix_time::ptime &t);
void parse_time_units(NcVar *t, boost::posix_time::ptime *origintime, long *timeunit);
}  // namespace nctools
