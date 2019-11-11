// ======================================================================
/*!
 * \brief NetCDF to querydata conversion for CF-conforming data
 *
 * http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.5/
 */
// ======================================================================

#include "NcFileExtended.h"
#include "nctools.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <macgyver/CsvReader.h>
#include <macgyver/StringConversion.h>
#include <macgyver/TimeParser.h>
#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiEnumConverter.h>
#include <newbase/NFmiFastQueryInfo.h>
#include <newbase/NFmiHPlaceDescriptor.h>
#include <newbase/NFmiLambertEqualArea.h>
#include <newbase/NFmiLatLonArea.h>
#include <newbase/NFmiParam.h>
#include <newbase/NFmiParamBag.h>
#include <newbase/NFmiParamDescriptor.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiQueryDataUtil.h>
#include <newbase/NFmiStereographicArea.h>
#include <newbase/NFmiTimeDescriptor.h>
#include <newbase/NFmiVPlaceDescriptor.h>
#include <spine/Exception.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <netcdfcpp.h>
#include <stdexcept>
#include <string>
#include <utility>

nctools::Options options;

using SmartMet::Spine::Exception;

// ----------------------------------------------------------------------
/*!
 * Check X-axis units
 */
// ----------------------------------------------------------------------

void check_xaxis_units(NcVar* var)
{
  NcAtt* att = var->get_att("units");
  if (att == 0) throw Exception(BCP, "X-axis has no units attribute");

  std::string units = att->values()->as_string(0);

  // Ref: CF conventions section 4.2 Longitude Coordinate
  if (units == "degrees_east") return;
  if (units == "degree_east") return;
  if (units == "degree_E") return;
  if (units == "degrees_E") return;
  if (units == "degreeE") return;
  if (units == "degreesE") return;
  if (units == "100  km") return;
  if (units == "m") return;
  if (units == "km") return;

  throw Exception(BCP, "X-axis has unknown units: " + units);
}

// ----------------------------------------------------------------------
/*!
 * Check Y-axis units
 */
// ----------------------------------------------------------------------

void check_yaxis_units(NcVar* var)
{
  NcAtt* att = var->get_att("units");
  if (att == 0) throw Exception(BCP, "Y-axis has no units attribute");

  std::string units = att->values()->as_string(0);

  // Ref: CF conventions section 4.1 Latitude Coordinate
  if (units == "degrees_north") return;
  if (units == "degree_north") return;
  if (units == "degree_N") return;
  if (units == "degrees_N") return;
  if (units == "degreeN") return;
  if (units == "degreesN") return;
  if (units == "100  km") return;
  if (units == "m") return;
  if (units == "km") return;

  throw Exception(BCP, "Y-axis has unknown units: " + units);
}

// ----------------------------------------------------------------------
/*!
 * Create horizontal descriptor
 */
// ----------------------------------------------------------------------
NFmiHPlaceDescriptor create_hdesc(nctools::NcFileExtended& ncfile)
{
  double x1 = ncfile.xmin();
  double y1 = ncfile.ymin();
  double x2 = ncfile.xmax();
  double y2 = ncfile.ymax();
  double nx = ncfile.xsize();
  double ny = ncfile.ysize();
  double centralLongitude = ncfile.longitudeOfProjectionOrigin;

  if (options.verbose)
  {
    if (options.infiles.size() > 1) std::cout << std::endl;
    std::cout << "Input file: " << ncfile.path << std::endl;
    std::cout << "  x1 => " << x1 << std::endl;
    std::cout << "  y1 => " << y1 << std::endl;
    std::cout << "  x2 => " << x2 << std::endl;
    std::cout << "  y2 => " << y2 << std::endl;
    std::cout << "  nx => " << nx << std::endl;
    std::cout << "  ny => " << ny << std::endl;
    if (ncfile.xinverted()) std::cout << "  x-axis is inverted" << std::endl;
    if (ncfile.yinverted()) std::cout << "  y-axis is inverted" << std::endl;
    std::cout << "  x-scaling multiplier to meters => " << ncfile.x_scale() << std::endl;
    std::cout << "  y-scaling multiplier to meters => " << ncfile.y_scale() << std::endl;
    std::cout << "  latitude_origin => " << ncfile.latitudeOfProjectionOrigin << std::endl;
    std::cout << "  longitude_origin => " << ncfile.longitudeOfProjectionOrigin << std::endl;
    std::cout << "  grid_mapping => " << ncfile.grid_mapping() << std::endl;
  }

  NFmiArea* area = nullptr;

  if (ncfile.grid_mapping() == POLAR_STEREOGRAPHIC)
    area = new NFmiStereographicArea(NFmiPoint(x1, y1), NFmiPoint(x2, y2), centralLongitude);
  else if (ncfile.grid_mapping() == LATITUDE_LONGITUDE)
    area = new NFmiLatLonArea(NFmiPoint(x1, y1), NFmiPoint(x2, y2));
  else if (ncfile.grid_mapping() == LAMBERT_CONFORMAL_CONIC)
    throw Exception(BCP, "Lambert conformal conic projection not supported");
  else if (ncfile.grid_mapping() == LAMBERT_AZIMUTHAL)
  {
    NFmiLambertEqualArea tmp(NFmiPoint(-90, 0),
                             NFmiPoint(90, 0),
                             ncfile.longitudeOfProjectionOrigin,
                             NFmiPoint(0, 0),
                             NFmiPoint(1, 1),
                             ncfile.latitudeOfProjectionOrigin);
    NFmiPoint bottomleft =
        tmp.WorldXYToLatLon(NFmiPoint(ncfile.x_scale() * x1, ncfile.y_scale() * y1));
    NFmiPoint topright =
        tmp.WorldXYToLatLon(NFmiPoint(ncfile.x_scale() * x2, ncfile.y_scale() * y2));
    area = new NFmiLambertEqualArea(bottomleft,
                                    topright,
                                    ncfile.longitudeOfProjectionOrigin,
                                    NFmiPoint(0, 0),
                                    NFmiPoint(1, 1),
                                    ncfile.latitudeOfProjectionOrigin);
  }
  else
    throw Exception(BCP, "Projection " + ncfile.grid_mapping() + " is not supported");

  NFmiGrid grid(area, nx, ny);
  NFmiHPlaceDescriptor hdesc(grid);

  return hdesc;
}

// ----------------------------------------------------------------------
/*!
 * Create vertical descriptor
 */
// ----------------------------------------------------------------------

NFmiVPlaceDescriptor create_vdesc(const nctools::NcFileExtended& ncfile)
{
  NcVar* z = ncfile.z_axis();

  // Defaults if there are no levels
  if (z == nullptr)
  {
    if (options.verbose) std::cerr << "  Extracting default level only\n";
    NFmiLevelBag bag(kFmiAnyLevelType, 0, 0, 0);
    return NFmiVPlaceDescriptor(bag);
  }

  // Otherwise collect all levels

  NFmiLevelBag bag;

  NcValues* zvalues = z->values();

  if (options.verbose) std::cerr << "  Extracting " << z->num_vals() << " levels:";

  for (int i = 0; i < z->num_vals(); i++)
  {
    auto value = zvalues->as_long(i);
    if (options.verbose) std::cerr << " " << value << std::flush;
    NFmiLevel level(kFmiAnyLevelType, value);
    bag.AddLevel(level);
  }

  if (options.verbose) std::cout << std::endl;

  return NFmiVPlaceDescriptor(bag);
}

// ----------------------------------------------------------------------
/*!
 * Create time descriptor
 *
 * CF reference "4.4. Time Coordinate" is crap. We support only
 * the simple stuff we need.
 *
 */
// ----------------------------------------------------------------------

NFmiTimeDescriptor create_tdesc(nctools::NcFileExtended& ncfile)
{
  NFmiTimeList tlist(ncfile.timeList());

  return NFmiTimeDescriptor(tlist.FirstTime(), tlist);
}

// ----------------------------------------------------------------------
/*!
 * Create parameter descriptor
 *
 * We extract all parameters which are recognized by newbase and use the
 * axes established by the command line options --xdim etc, or which
 * have been guessed based on standard names.
 */
// ----------------------------------------------------------------------

int add_to_pbag(const nctools::NcFileExtended& ncfile,
                const nctools::ParamConversions& paramconvs,
                NFmiParamBag& pbag)
{
  unsigned int added_variables = 0;

  const float minvalue = kFloatMissing;
  const float maxvalue = kFloatMissing;
  const float scale = kFloatMissing;
  const float base = kFloatMissing;
  const NFmiString precision = "%.1f";
  const FmiInterpolationMethod interpolation = kLinearly;

  // Number of dimensions the parameter must have
  int wanted_dims = 0;
  if (ncfile.x_axis() != nullptr) ++wanted_dims;
  if (ncfile.y_axis() != nullptr) ++wanted_dims;
  if (ncfile.z_axis() != nullptr) ++wanted_dims;
  if (ncfile.t_axis() != nullptr) ++wanted_dims;

  // Note: We loop over variables the same way as in copy_values

  for (int i = 0; i < ncfile.num_vars(); i++)
  {
    NcVar* var = ncfile.get_var(i);
    if (var == 0) continue;

    // Skip dimension variables
    if (ncfile.is_dim(var->name())) continue;

    // Check dimensions

    if (!ncfile.axis_match(var))
    {
      if (options.verbose)
        std::cout << "  Skipping variable " << nctools::get_name(var)
                  << " for not having requested dimensions\n";
      continue;
    }

    // Here we need to know only the id
    nctools::ParamInfo pinfo =
        nctools::parse_parameter(nctools::get_name(var), paramconvs, options.autoid);
    if (pinfo.id < 1)
    {
      if (options.verbose)
        std::cout << "  Skipping unknown variable '" << nctools::get_name(var) << "'\n";
      continue;
    }
    else if (options.verbose)
    {
      std::cout << "  Variable " << nctools::get_name(var) << " has id " << pinfo.id << " and name "
                << (nctools::get_enumconverter().ToString(pinfo.id).empty()
                        ? "undefined"
                        : nctools::get_enumconverter().ToString(pinfo.id))
                << std::endl;
    }

    // Check dimensions match

    NFmiParam param(pinfo.id,
                    nctools::get_enumconverter().ToString(pinfo.id),
                    minvalue,
                    maxvalue,
                    scale,
                    base,
                    precision,
                    interpolation);
    NFmiDataIdent ident(param);
    if (pbag.Add(ident, true)) added_variables++;
  }

  return added_variables;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program without exception handling
 */
// ----------------------------------------------------------------------

int run(int argc, char* argv[])
{
  try
  {
    // Parse options
    if (!parse_options(argc, argv, options)) return 0;

    // Parameter conversions
    const nctools::ParamConversions paramconvs = nctools::read_netcdf_configs(options);

    // Prepare empty target querydata
    std::unique_ptr<NFmiQueryData> data;

    int file_counter = 0;
    NFmiHPlaceDescriptor hdesc;
    NFmiVPlaceDescriptor vdesc;
    NFmiTimeDescriptor tdesc;
    NFmiParamBag pbag;

    using NcFileExtendedPtr = std::shared_ptr<nctools::NcFileExtended>;
    using NcFileExtendedList = std::vector<NcFileExtendedPtr>;

    NcFileExtendedPtr first_ncfile;
    NcFileExtendedList ncfilelist;

    unsigned int known_variables = 0;

    // Loop through the files once to check and to prepare the descriptors first

    for (std::string infile : options.infiles)
    {
      ++file_counter;

      try
      {
        NcError errormode(NcError::silent_nonfatal);
        auto ncfile = std::make_shared<nctools::NcFileExtended>(infile, options.timeshift);

        ncfile->tolerance = options.tolerance;

        if (!ncfile->is_valid())
          throw Exception(BCP, "File '" + infile + "' does not contain valid NetCDF", nullptr);

        // When --info is given we only print useful metadata instead of generating anything
        if (options.info)
        {
          ncfile->printInfo();
          continue;
        }

        // Verify convention requirement
        ncfile->require_conventions(&(options.conventions));

        // Establish wanted axis parameters, this throws if unsuccesful
        ncfile->initAxis(options.xdim, options.ydim, options.zdim, options.tdim);

        // Save initialized state for further processing
        ncfilelist.push_back(ncfile);

        std::string grid_mapping(ncfile->grid_mapping());

        if (ncfile->x_axis()->num_vals() < 1) throw Exception(BCP, "X-axis has no values");
        if (ncfile->y_axis()->num_vals() < 1) throw Exception(BCP, "Y-axis has no values");
        if (ncfile->z_axis() != nullptr && ncfile->zsize() < 1)
          throw Exception(BCP, "Z-axis has no values");
        if (ncfile->t_axis() != nullptr && ncfile->tsize() < 1)
          throw Exception(BCP, "T-axis has no values");

        check_xaxis_units(ncfile->x_axis());
        check_yaxis_units(ncfile->y_axis());

        if (ncfile->xsize() == 0) throw Exception(BCP, "X-dimension is of size zero");
        if (ncfile->ysize() == 0) throw Exception(BCP, "Y-dimension is of size zero");
        if (ncfile->zsize() == 0) throw Exception(BCP, "Z-dimension is of size zero");

        // Crate initial descriptors based on the first NetCDF file
        if (file_counter == 1)
        {
          first_ncfile = ncfile;

          tdesc = create_tdesc(*ncfile);
          hdesc = create_hdesc(*ncfile);
          vdesc = create_vdesc(*ncfile);
        }
        else
        {
          // Try to merge times and parameters from other files with the same grid and levels
          std::vector<std::string> failreasons;
          if (ncfile->joinable(*first_ncfile, &failreasons) == false)
          {
            std::cerr << "Unable to combine " << first_ncfile->path << " and " << infile << ":"
                      << std::endl;
            for (auto error : failreasons)
              std::cerr << "  " << error << std::endl;

            throw Exception(BCP, "Files not joinable", nullptr);
          }

          auto new_hdesc = create_hdesc(*ncfile);
          auto new_vdesc = create_vdesc(*ncfile);
          auto new_tdesc = create_tdesc(*ncfile);

          if (!(new_hdesc == hdesc))
            throw Exception(BCP, "Hdesc differs from " + first_ncfile->path);
          if (!(new_vdesc == vdesc))
            throw Exception(BCP, "Vdesc differs from " + first_ncfile->path);

          tdesc = tdesc.Combine(new_tdesc);
        }
        known_variables += add_to_pbag(*ncfile, paramconvs, pbag);
      }
      catch (...)
      {
        throw Exception(BCP, "File check failed on input " + infile, nullptr);
      }
    }

    if (options.info) return 0;

    // Check parameters
    if (known_variables == 0)
      throw Exception(BCP,
                      "No known parameters defined by conversion tables found from input file(s)");

    // Create querydata structures and target file
    NFmiParamDescriptor pdesc(pbag);
    NFmiFastQueryInfo qi(pdesc, tdesc, hdesc, vdesc);

    if (options.memorymap)
      data.reset(NFmiQueryDataUtil::CreateEmptyData(qi, options.outfile, true));
    else
      data.reset(NFmiQueryDataUtil::CreateEmptyData(qi));

    NFmiFastQueryInfo info(data.get());
    info.SetProducer(NFmiProducer(options.producernumber, options.producername));

    // Copy data from input files
    for (auto i = 0ul; i < options.infiles.size(); i++)
    {
      try
      {
        const auto& ncfile = ncfilelist[i];
        ncfile->copy_values(options, info, paramconvs);
      }
      catch (...)
      {
        throw Exception(BCP, "Operation failed on input " + options.infiles[i], nullptr);
      }
    }

    // Save output
    if (options.outfile == "-")
      data->Write();
    else if (!options.memorymap)
      data->Write(options.outfile);
  }
  catch (...)
  {
    throw Exception(BCP, "Operation failed!", nullptr);
  }
  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
    return run(argc, argv);
  }
  catch (...)
  {
    Exception e(BCP, "Operation failed!", nullptr);
    e.printError();
    return 1;
  }
}
