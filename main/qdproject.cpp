// ======================================================================
/*!
 * \file
 * \brief Implementation of the qdproject program
 */
// ======================================================================
/*!
 * \page qdproject qdproject
 *
 * qdproject ohjelmalla voi projisoida koordinaatteja lonlat muodosta
 * maailmankoordinaatteihin tai pikselikoordinaatteihin, sekä päin
 * vastaiseen suuntaan.
 *
 * Projisoitavat pisteet annetaan komentorivillä. Vaihtoehtoisesti
 * voidaan interpoloida annettu querydata uuteen projektioon.
 *
 * Käyttö:
 * \code
 * qdproject [options] x1,y1 x2,y2 ...
 * \endcode
 *
 * Optiot:
 * <dl>
 * <dt>-d desc</dt>
 * <dd>Projektion kuvaus</dd>
 * <dt>-c conf</dt>
 * <dd>Tiedosto, josta projektion kuvaus etsitään (a'la qdcontour)</dd>
 * <dt>-q querydata</dt>
 * <dd>Tiedosto, josta projektion otetaan</dd>
 * <dt>-l</dt>
 * <dd>Komentorivillä annetaan longitudi-latitude pareja projisoitavaksi
 *     xy-koordinaatistoon</dd>
 * <dt>-L</dt>
 * <dd>Komentorivillä annetaan longitudi-latitude pareja projisoitavaksi
 *     maailmankoordinaatistoon</dd>
 * <dt>-i</dt>
 * <dd>Käänteisprojektio, komentorivillä annetaan xy-koordinaatteja
 *     projisoitavaksi takaisin longitudi-latitudi pareiksi</dd>
 * <dt>-I</dt>
 * <dd>Käänteisprojektio, komentorivillä annetaan xy-koordinaatteja
 *     projisoitavaksi takaisin maailmankoordinaatistoon</dd>
 * <dt>-v</dt>
 * <dd>Verbose moodi - alkuperäiset koordinaatit tulostetaan projisoitujen
 *     koordinaattien perässä.</dd>
 * </dl>
 *
 */
// ======================================================================

#include <newbase/NFmiArea.h>
#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiPreProcessor.h>
#include <newbase/NFmiQueryData.h>
#include <newbase/NFmiStringTools.h>

#include <boost/shared_ptr.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage information
 */
// ----------------------------------------------------------------------

void usage()
{
  cout << "Usage: qdproject [options] x1,y1 x2,y2 x3,y3 ..." << endl
       << endl
       << "Available options:" << endl
       << endl
       << "  -h\t\tPrint this usage information" << endl
       << "  -v\t\tVerbose mode - the original coordinates are also printed" << endl
       << "  -d desc\tThe projection description" << endl
       << "  -c conf\tFile containing the projection description" << endl
       << "  -q querydata\tQuerydata in the desired projection" << endl
       << "  -l\t\tConvert lon-lat pairs to pixel coordinates" << endl
       << "  -L\t\tConvert lon-lat pairs to world coordinates" << endl
       << "  -i\t\tConvert pixel coordinates to lon-lat coordinates" << endl
       << "  -I\t\tConvert world coordinates to lon-lat coordinates" << endl
       << endl
       << "The projection descriptions are given in the same form as for" << endl
       << "qdcontour and qdprojection, for example" << endl
       << endl
       << "   stereographic,20:6,51.3,49,70.2:600,-1" << endl
       << endl
       << "In the case of the -c option the projection description is expected to reside" << endl
       << "in a configuration file and to be preceded by a 'projection' command." << endl
       << "This permits qdproject to read the files used by shape2ps when creating maps." << endl
       << endl
       << "As a special case, if there are no command line arguments, the standard input" << endl
       << "is read for whitespace separated coordinate pairs. This enables one to" << endl
       << "project coordinates from a file by using a command like" << endl
       << endl
       << "   qdproject -d stereographic,20:6,51.3,49,70.2:600,-1 -l < filename" << endl
       << endl
       << "Examples:" << endl
       << endl
       << "   qdproject -d stereographic,20:6,51.3,49,70.2:600,-1 -L 25,60" << endl
       << "   qdproject -q referencedata.sqd -L 25,60" << endl
       << "   qdproject -c area.cnf -L 25,60" << endl
       << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract a projection from querydata
 */
// ----------------------------------------------------------------------

auto_ptr<NFmiArea> create_projection_from_querydata(const string& theFile)
{
  NFmiQueryData qd(theFile);
  if (!qd.IsGrid()) throw runtime_error("The given querydata does not define a projection");

  NFmiArea* area = qd.GridInfo().Area();
  if (!area) throw runtime_error("The given querydata does not define a projection");

  return auto_ptr<NFmiArea>(area->Clone());
}

// ----------------------------------------------------------------------
/*!
 * \brief Extract a projection from the given configuration file
 */
// ----------------------------------------------------------------------

boost::shared_ptr<NFmiArea> create_projection_from_conf(const string& theFile)
{
  const bool strip_pound = false;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include", "", "");
  processor.SetDefine("#define");
  if (!processor.ReadAndStripFile(theFile))
    throw runtime_error("Preprocessor failed to read '" + theFile + "'");

  string text = processor.GetString();
  istringstream script(text);

  boost::shared_ptr<NFmiArea> area;

  string token;
  while (script >> token)
  {
    if (token == "#")
      script.ignore(1000000, '\n');
    else if (token == "projection")
    {
      script >> token;
      area = NFmiAreaFactory::Create(token);
      break;
    }
    else
    { /* ignore unknown tokens */
    }
  }

  if (area.get() == 0) throw runtime_error("Failed to find a projection from '" + theFile + "'");

  return area;
}

// ----------------------------------------------------------------------
/*!
 * \brief Project latlon to pixel coordinates
 */
// ----------------------------------------------------------------------

void project_coordinates(bool verbose,
                         const NFmiCmdLine& theParameters,
                         unsigned char theOption,
                         const NFmiArea& theArea)
{
  // First extract a vector of coordinates to be projected

  vector<NFmiPoint> coords;

  const unsigned int n = theParameters.NumberofParameters();

  if (n > 0)
  {
    for (unsigned int i = 1; i <= n; i++)
    {
      string opt = theParameters.Parameter(i);
      vector<double> nums = NFmiStringTools::Split<vector<double> >(opt);
      if (nums.size() != 2) throw runtime_error("Could not convert '" + opt + "' to a coordinate");
      coords.push_back(NFmiPoint(nums[0], nums[1]));
    }
  }
  else
  {
    double x, y;
    while (cin >> x >> y)
      coords.push_back(NFmiPoint(x, y));
    if (cin.bad()) throw runtime_error("Error while reading coordinates from standard input");
  }

  // Now project the coordinates and print them

  NFmiPoint result;
  for (vector<NFmiPoint>::const_iterator it = coords.begin(); it != coords.end(); ++it)
  {
    switch (theOption)
    {
      case 'l':
        result = theArea.ToXY(*it);
        break;
      case 'L':
        result = theArea.LatLonToWorldXY(*it);
        break;
      case 'i':
        result = theArea.ToLatLon(*it);
        break;
      case 'I':
        result = theArea.WorldXYToLatLon(*it);
        break;
      default:
        throw runtime_error("Internal error - unhandled projection while projecting");
    }
    cout << setprecision(10) << result.X() << ' ' << result.Y();
    if (verbose) cout << ' ' << it->X() << ' ' << it->Y();
    cout << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Main algorithm
 */
// ----------------------------------------------------------------------

int run(int argc, const char* argv[])
{
  // Parse the command line

  NFmiCmdLine cmdline(argc, argv, "hvd!c!q!lLiI");

  if (cmdline.Status().IsError()) throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h'))
  {
    usage();
    return 0;
  }

  bool verbose = cmdline.isOption('v');

  // Expect an area specification

  boost::shared_ptr<NFmiArea> area;

  int projcount = (cmdline.isOption('d') + cmdline.isOption('c') + cmdline.isOption('q'));

  if (projcount > 1) throw runtime_error("Options -d -c and -q are mutually exclusive");

  if (projcount == 0) throw runtime_error("At least one of options -d -c or -q must be used");

  if (cmdline.isOption('d'))
    area = NFmiAreaFactory::Create(cmdline.OptionValue('d'));
  else if (cmdline.isOption('c'))
    area = create_projection_from_conf(cmdline.OptionValue('c'));
  else if (cmdline.isOption('q'))
    area = create_projection_from_querydata(cmdline.OptionValue('q'));

  if (area.get() == 0) throw runtime_error("Failed to create a projection");

  // Check how many of the conversion options are given

  int opts = (cmdline.isOption('l') + cmdline.isOption('L') + cmdline.isOption('i') +
              cmdline.isOption('I'));

  if (opts == 0) throw runtime_error("Atleast one of options -l -L -i -I must be given");

  if (opts > 1)
    throw runtime_error("Only one of options -l -L -i -I can be given at the same time");

  // Call proper projection handlers

  if (cmdline.isOption('l'))
    project_coordinates(verbose, cmdline, 'l', *area);
  else if (cmdline.isOption('L'))
    project_coordinates(verbose, cmdline, 'L', *area);
  else if (cmdline.isOption('i'))
    project_coordinates(verbose, cmdline, 'i', *area);
  else if (cmdline.isOption('I'))
    project_coordinates(verbose, cmdline, 'I', *area);
  else
    throw runtime_error("Internal error - unhandled projection selection");

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  try
  {
    return run(argc, argv);
  }
  catch (exception& e)
  {
    cout << "Error: " << e.what() << endl;
    return 1;
  }
  catch (...)
  {
    cout << "Error: Caught an unknown exception" << endl;
    return 1;
  }
}
