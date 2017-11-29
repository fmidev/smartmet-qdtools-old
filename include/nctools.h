
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
void copy_values(const Options &options, NcVar *var, NFmiFastQueryInfo &info);
void copy_values(const NcFile &ncfile, NFmiFastQueryInfo &info, const ParamInfo &pinfo);
void copy_values(const Options &options,
                 const NcFile &ncfile,
                 NFmiFastQueryInfo &info,
                 const ParamConversions &paramconvs,
                 bool useAutoGeneratedIds = false);
bool is_name_in_list(const std::list<std::string> &nameList, const std::string name);

#if DEBUG_PRINT
void print_att(const NcAtt &att);
void debug_output(const NcFile &ncfile);
#endif

}  // namespace nctools
