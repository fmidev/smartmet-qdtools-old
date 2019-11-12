
#pragma once

#include <boost/optional.hpp>
#include <macgyver/CsvReader.h>
#include <newbase/NFmiEnumConverter.h>
#include <list>
#include <map>
#include <string>
#include <vector>

#define DEBUG_PRINT 0
#define POLAR_STEREOGRAPHIC "polar_stereographic"
#define LAMBERT_CONFORMAL_CONIC "lambert_conformal_conic"
#define LATITUDE_LONGITUDE "latitude_longitude"
#define LAMBERT_AZIMUTHAL "lambert_azimuthal_equal_area"

#if DEBUG_PRINT
#include <netcdfcpp.h>
#endif

class NFmiFastQueryInfo;

namespace nctools
{
using AttributesMap = std::map<std::string, std::string>;
// ----------------------------------------------------------------------
/*!
 * \brief Container for command line options
 */
// ----------------------------------------------------------------------

struct Options
{
#ifdef UNIX
  std::string configfile =
      "/usr/share/smartmet/formats/netcdf.conf";  // -c Config, replace standard config
#else
  std::string configfile = "netcdf.conf";
#endif
  std::vector<std::string> infiles;               // Multiple input files
  std::vector<std::string> configs;               // -C extra configs, take precedence over standard
  std::vector<std::string> parameters;            // Define extra parameters on command line
  std::list<std::string> ignoreUnitChangeParams;  // -u name1,name2,...
  std::list<std::string> excludeParams;           // -x name1,name2,...
  std::list<std::string> addParams;               // --addparameter name1,name2,...
  std::string outfile = "-";                      // -o
  std::string producername = "UNKNOWN";           // --producername
  std::string conventions = "CF-1.0";             // -n Name of Conventions to use
  std::string projection;                         // -P Desired projection and area

  // Default names for the coordinate variables. An empty value means the program will the
  // coordinate parameter. Note that by default we do not extract level data, the Z dimension
  // must be given.

  boost::optional<std::string> xdim;  // --xdim name. Usually 'lon', sometimes 'longitude' or 'xc'
  boost::optional<std::string> ydim;  // --ydim name. Usually 'lat', sometimes 'latitude' or 'yc'
  boost::optional<std::string> zdim{""};  // --zdim name. Usually 'lev' or 'zc'
  boost::optional<std::string> tdim;      // --tdim name. Usually 'time', sometimes 't'

  AttributesMap cmdLineGlobalAttributes;  // -a Add global attrubutes, f.ex. -a DX=1356.3;DY=1265.3
  double tolerance = 1e-3;                // Axis stepping tolerance
  long producernumber = 0;                // --producernumber
  long timeshift = 0;                     // -t <minutes>
  bool verbose = false;                   // -v
  bool memorymap = false;                 // --mmap
  bool fixstaggered = false;              // -s interpolate staggered data to a regular grid
  bool experimental = false;  // -x enable features which are known to be not work in all situations
  bool debug = false;         // -d enable debugging output
  bool autoid = false;        // -U --autoids use autogenerated ids for unknown parameters
  bool info = false;          // --info Print information on dimensions etc, do no other work
};

// ----------------------------------------------------------------------
/*!
 * \brief List of parameter conversions from NetCDF to newbase
 */
// ----------------------------------------------------------------------

typedef std::list<Fmi::CsvReader::row_type> ParamConversions;

struct CsvParams
{
  ParamConversions paramconvs;
  const Options &options;

  CsvParams(const Options &optionsIn);
  void add(const Fmi::CsvReader::row_type &row);

 private:
  CsvParams &operator=(const CsvParams &);  // estet��n sijoitus opereraattori, est�� varoituksen
                                            // VC++ 2012 k��nt�j�ss�
};

// ----------------------------------------------------------------------
/*!
 * \brief Parameter parsing info
 *
 * This structure is used for identifying parameters consisting of
 * X- and Y-components which should be converted into a speed
 * and direction variable.
 */
// ----------------------------------------------------------------------

struct ParamInfo
{
  FmiParameterName id = kFmiBadParameter;
  bool isregular = true;
  bool isspeed = false;
  std::string x_component;
  std::string y_component;
  std::string name;
};

extern int unknownParIdCounterBegin;  // Beginning of unknown ids

NFmiEnumConverter &get_enumconverter(void);
ParamInfo parse_parameter(const std::string &name,
                          const ParamConversions &paramconvs,
                          bool useAutoGeneratedIds);
bool parse_options(int argc, char *argv[], Options &options);
ParamConversions read_netcdf_configs(const Options &options);
bool is_name_in_list(const std::list<std::string> &nameList, const std::string name);

#if DEBUG_PRINT
void print_att(const NcAtt &att);
void debug_output(const NcFile &ncfile);
#endif

}  // namespace nctools
