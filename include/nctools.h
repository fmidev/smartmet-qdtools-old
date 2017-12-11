
#pragma once

#include <macgyver/CsvReader.h>
#include <newbase/NFmiEnumConverter.h>
#include <list>
#include <map>
#include <netcdfcpp.h>

#define DEBUG_PRINT 0
#define POLAR_STEREOGRAPHIC "polar_stereographic"
#define LAMBERT_CONFORMAL_CONIC "lambert_conformal_conic"
#define LATITUDE_LONGITUDE "latitude_longitude"
#define LAMBERT_AZIMUTHAL "lambert_azimuthal_equal_area"

class NFmiFastQueryInfo;

namespace nctools
{
typedef std::map<std::string, std::string> attributesMap;
// ----------------------------------------------------------------------
/*!
 * \brief Container for command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  Options();

  bool verbose;              // -v
  std::string infile;        // -i
  std::string outfile;       // -o
  std::string configfile;    // -c
  std::string producername;  // --producername
  long producernumber;       // --producernumber
  long timeshift;            // -t <minutes>
  bool memorymap;            // --mmap
  bool fixstaggered;  // -s (muuttaa staggered datat perusdatan muotoon, interpoloi datan perus
                      // hilaan)
  bool experimental;  // -x enable features which are known to be not work in all situations
  bool debug;         // Enable debugging output
  std::list<std::string> ignoreUnitChangeParams;  // -u name1,name2,...
  std::list<std::string> excludeParams;           // -x name1,name2,...
  std::string projection;  // -P // data konvertoidaan haluttuun projektioon ja alueeseen
  attributesMap cmdLineGlobalAttributes;  // -a optiolla voidaan antaa dataan liittyvi� globaali
                                          // attribuutteja (esim. -a DX=1356.3;DY=1265.3)
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
  FmiParameterName id;
  bool isregular;
  bool isspeed;
  std::string x_component;
  std::string y_component;
  std::string name;

  ParamInfo()
      : id(kFmiBadParameter), isregular(true), isspeed(false), x_component(), y_component(), name()
  {
  }
};

class NcFileExtended : public NcFile
{
 public:
  NcFileExtended(const char *path,
                 FileMode = ReadOnly,
                 size_t *bufrsizeptr = NULL,  // optional tuning parameters
                 size_t initialsize = 0,
                 FileFormat = Classic);
  std::string grid_mapping();
  unsigned long xsize();                 // Count of elements on x-axis
  unsigned long ysize();                 // Count of elements on y-axis
  unsigned long zsize();                 // Count of elements on z-axis
  unsigned long tsize();                 // Count of elements on t-axis
  unsigned long axis_size(NcVar *axis);  // Generic dimension of an axis(=count of elements)
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
  NcVar *x_axis();                           // Find x-axis from predefined(known) set
  NcVar *y_axis();                           // Find y-axis from predefined(known) set
  NcVar *z_axis();                           // Find z-axis
  NcVar *t_axis();                           // Find time axis
  NcVar *axis(const std::string &axisname);  // Find generic axis by name
  void copy_values(
      const Options &options,
      NFmiFastQueryInfo &info,
      const ParamConversions &paramconvs,
      bool useAutoGeneratedIds = false);  // Copy data to already existing querydata object

 private:
  std::shared_ptr<std::string> projectionName;
  NcVar *x;
  NcVar *y;
  NcVar *z;
  NcVar *t;
  bool minmaxfound;
  double _xmin, _xmax, _ymin, _ymax, _zmin, _zmax;
  void find_axis_bounds(NcVar *, int n, double *x1, double *x2, const char *name);
  void find_lonlat_bounds(double &lon1, double &lat1, double &lon2, double &lat2);
  void find_bounds();
  void copy_values(NFmiFastQueryInfo &info,
                   const ParamInfo &pinfo,
                   const nctools::Options *options);
  void copy_values(const Options &options, NcVar *var, NFmiFastQueryInfo &info);
};

NFmiEnumConverter &get_enumconverter(void);
ParamInfo parse_parameter(const std::string &name,
                          const ParamConversions &paramconvs,
                          bool useAutoGeneratedIds = false);
std::string get_name(NcVar *var);
ParamInfo parse_parameter(NcVar *var,
                          const ParamConversions &paramconvs,
                          bool useAutoGeneratedIds = false);
NcVar *find_variable(const NcFile &ncfile, const std::string &name);
float get_missingvalue(NcVar *var);
float get_scale(NcVar *var);
float get_offset(NcVar *var);
float normalize_units(float value, const std::string &units);
void report_units(NcVar *var,
                  const std::string &units,
                  const Options &options,
                  bool ignoreUnitChange = false);
bool parse_options(int argc, char *argv[], Options &options);
ParamConversions read_netcdf_config(const Options &options);
bool is_name_in_list(const std::list<std::string> &nameList, const std::string name);

#if DEBUG_PRINT
void print_att(const NcAtt &att);
void debug_output(const NcFile &ncfile);
#endif

}  // namespace nctools
