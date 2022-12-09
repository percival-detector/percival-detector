
#include "CalibratorSample.h"
#include "CalibratorReset.h"

#include <hdf5.h>

#include "log4cxx/basicconfigurator.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <cassert>
#include <map>

namespace bfs = boost::filesystem;
// using namespace boost::program_options;
namespace po = boost::program_options;
int rows = 1484;
int cols = 1440;
CalibratorSample g_calibratorS(rows, cols);
CalibratorReset g_calibratorR(rows, cols);
static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("Calib-ms");
bool g_cds = false;
int g_cma = -1;
std::map<int,int> g_rows_to_use;
const int ROWSPERROWGP = 7;

// this func needs rewriting to accept counts too.
void saveMultigainToH5(std::string filename, std::vector<MemBlockF>& gain2tot, std::vector<MemBlockI16>& gain2count)
{
    herr_t status;
    std::cout << "saving " << filename << std::endl;
    const int ndims = 2;
    hsize_t  	dims[ndims] = {rows, cols};
    hid_t fh = H5Fcreate (filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    hid_t space = H5Screate_simple (ndims, dims, NULL);

    for (int i=0;i<gain2tot.size();++i)
    {
      char totname[128];
      char countname[128];
      sprintf(totname, "g%d_tot", i);
      sprintf(countname, "g%d_count", i);

      hid_t dset;
      dset = H5Dcreate (fh, totname, H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                  H5P_DEFAULT, H5P_DEFAULT);
      status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, gain2tot.data());
      status = H5Dclose (dset);

      dset = H5Dcreate (fh, countname, H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                  H5P_DEFAULT, H5P_DEFAULT);
      status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, gain2count.data());
      status = H5Dclose (dset);
    }

    status = H5Sclose (space);
    status = H5Fclose (fh);
    assert(status==0);
}

void saveMVToH5(std::string filename, MemBlockF& avg, MemBlockF& var, std::vector<float> avg_by_row, std::vector<float> var_by_row)
{
    herr_t status;
    std::cout << "saving " << filename << std::endl;
    const int ndims = 2;
    hsize_t  	dims[ndims] = {avg.rows(), avg.cols()};
    hid_t fh = H5Fcreate (filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    hid_t space = H5Screate_simple (ndims, dims, NULL);

    hid_t dset = H5Dcreate (fh, "avg", H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, avg.data());

    status = H5Dclose (dset);

    dset = H5Dcreate (fh, "var", H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, var.data());

    status = H5Dclose (dset);
    status = H5Sclose (space);

    dims[0] = 1; dims[1] = 7;
    space = H5Screate_simple (ndims, dims, NULL);

    dset = H5Dcreate (fh, "avg_by_row", H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, avg_by_row.data());
    status = H5Dclose (dset);

    dset = H5Dcreate (fh, "var_by_row", H5T_NATIVE_FLOAT, space, H5P_DEFAULT,
                H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, var_by_row.data());

    status = H5Dclose (dset);
    status = H5Sclose (space);
    status = H5Fclose (fh);
    assert(status==0);
}

struct Counter
{
  int m_count = 0;
  double m_total = 0.0;
};

// This function applies the calibrator to each frame and then calculates the mean-frame
//  and var-frame. Then it calculates the avg of these two frames (avoiding Nans) and
//  prints the answers to stdout. It also saves averages across rows in the outfile.

void process_file_meanvar(bfs::path filename, bool saveMV=false, int minFrame=3, int maxFrames=20)
{
    if(0<=g_cma)
      g_calibratorS.setCMA(true,g_cma);
    // note AM drops the first sample frame in his python because there's no reset for it.
    std::cout << "processing " << filename << std::endl;
    std::vector<MemBlockF> oframes(maxFrames);
    int64_t rc=0ull;
    for(int frameIdx=minFrame;frameIdx<maxFrames;++frameIdx)
    {
      MemBlockI16 rinput, sinput;
      rinput.init(logger,rows, cols);
      sinput.init(logger,rows, cols);
      oframes.at(frameIdx).init(logger,rows, cols);

      // loadFromH5 will check the dims match ok.
      rc = rinput.loadFromH5(filename.string(), "/reset", frameIdx-1);
      rc |= sinput.loadFromH5(filename.string(), "/data", frameIdx);
      if(rc==0)
      {
        if(g_cds)
          g_calibratorR.processFrameP(rinput, g_calibratorS.m_resetFrame);
        g_calibratorS.processFrameP(sinput, oframes[frameIdx]);
      }
      else
      {
        std::cout << "failed to load frame " << frameIdx << std::endl;
        exit(1);
      }
    }

    // calculate mean and variance pixelwise
    MemBlockF avg, var;
    avg.init(logger, rows, cols);
    var.init(logger, rows, cols);
    avg.setAll(0.0f);
    var.setAll(0.0f);
    for(int r=0;r<rows;++r)
    {
      for(int c=0;c<cols;++c)
      {
        for(int frameIdx=minFrame;frameIdx<maxFrames;++frameIdx)
        {
          avg.at(r,c) += oframes.at(frameIdx).at(r,c);
        }
        avg.at(r,c) /= (float) (maxFrames - minFrame);
        float thisAvg = avg.at(r,c);
        for(int frameIdx=minFrame;frameIdx<maxFrames;++frameIdx)
        {
          float dif = (oframes.at(frameIdx).at(r,c) - thisAvg);
          var.at(r,c) += dif * dif;
        }
        var.at(r,c) /= (float) (maxFrames - minFrame);
      }
    }

    //// here is where we calculate average values on a row-in-rowgroup basis
    Counter avg_counts[ROWSPERROWGP];
    Counter var_counts[ROWSPERROWGP];

    for(int r=0;r<rows;++r)
    {
      // we need to avoid the ref columns, ? do we need to check they E?
      for(int c=32;c<cols;++c)
      {
        float thisAvg = avg.at(r,c);
        float thisVar = var.at(r,c);
        int subrow = r % 7;
        if(!isnan(thisAvg))
        {
          avg_counts[subrow].m_count += 1;
          avg_counts[subrow].m_total += static_cast<double>(thisAvg);
          var_counts[subrow].m_count += 1;
          var_counts[subrow].m_total += static_cast<double>(thisVar);
        }
      }
    }

    std::vector<float> avg_by_row, var_by_row;
    for(int subrow=0; subrow<ROWSPERROWGP; ++subrow)
    {
      avg_by_row.push_back(avg_counts[subrow].m_total / avg_counts[subrow].m_count);
      var_by_row.push_back(var_counts[subrow].m_total / var_counts[subrow].m_count);
    }

    std::string outfile = filename.string();
    if (g_cds)
      outfile.replace(outfile.length() - 3, 3, "_cds_mv.h5");
    else
      outfile.replace(outfile.length() - 3, 3, "_mv.h5");
    if(saveMV && filename.string().length() < outfile.length())
    {
      saveMVToH5(outfile, avg, var, avg_by_row, var_by_row);
    }

    // I dont know if we need this any longer
    // ? can remove --rows and g_rows_to_use?
    double avg_tot = 0.0, var_tot = 0.0;
    int count = 0;
    for(int subrow=0; subrow<ROWSPERROWGP; ++subrow)
    {
      if(g_rows_to_use.count(subrow))
      {
        avg_tot += avg_counts[subrow].m_total;
        var_tot += var_counts[subrow].m_total;
        count += avg_counts[subrow].m_count;
      }
    }

    // note it would be sensible to print the mode-avg also because huge outliers may affect the mean-avg.
    std::cout << "pixels:" << count << " mean avg is " << avg_tot / count << ", mean var is " << (var_tot / count) << std::endl;
}


void process_file_multigain(bfs::path filename, bool saveMG=false, int minFrame=3, int maxFrames=20)
{
    // note that this function assumes that the number of frames is small, say 1000. If you want more
    // than this you will need to alter the totals to double, and the counts to int32.
    if(0<=g_cma)
      g_calibratorS.setCMA(true,g_cma);
    // note AM drops the first sample frame in his python because there's no reset for it.
    std::cout << "Processing " << filename << std::endl;
    std::vector<MemBlockF> oframes(maxFrames);
    std::vector<MemBlockI16> sframes(maxFrames);
    int64_t rc=0ull;
    for(int frameIdx=minFrame;frameIdx<maxFrames;++frameIdx)
    {
      MemBlockI16 rinput;
      rinput.init(logger,rows, cols);
      sframes.at(frameIdx).init(logger,rows, cols);
      oframes.at(frameIdx).init(logger,rows, cols);

      if(g_cds)
        rc |= rinput.loadFromH5(filename.string(), "/reset", frameIdx-1);
      rc |= sframes[frameIdx].loadFromH5(filename.string(), "/data", frameIdx);
      if(rc==0)
      {
        if(g_cds)
          g_calibratorR.processFrameP(rinput, g_calibratorS.m_resetFrame);
        g_calibratorS.processFrameP(sframes[frameIdx], oframes[frameIdx]);
      }
      else
      {
        std::cout << "failed to load frame " << frameIdx << std::endl;
        exit(1);
      }
    }

    // calculate the results pixelwise
    const int numGains = 4;
    std::vector<MemBlockF> gain2tot(numGains);
    std::vector<MemBlockI16> gain2count(numGains);
    for(int gain=0;gain<numGains;++gain)
    {
      gain2tot.at(gain).init(logger, rows, cols);
      gain2tot.at(gain).setAll(0.0f);
      gain2count.at(gain).init(logger, rows, cols);
      gain2count.at(gain).setAll(0);
    }

    // we loop over the frames in this acquisition, and pixelwise calculate the total&count
    // for each of the gain modes. 0 is the result if there are no pixels in that mode.
    // we keep the count because it is interesting information. AM thinks the pedestals are
    // better calculated pixelwise, and this is what you need to do so.    
    for(int frameIdx=minFrame;frameIdx<maxFrames;++frameIdx)
    {
      for(int r=0;r<rows;++r)
      {
        int subrow = r % 7;
        if(g_rows_to_use.count(subrow))
        {
          for(int c=0;c<cols;++c)
          {
            int gain = sframes.at(frameIdx).at(r,c);
            gain2tot.at(gain).at(r,c) += oframes.at(frameIdx).at(r,c);
            gain2count.at(gain).at(r,c) += 1;
          }
        }
      }
    }

    std::string outfile = filename.string();
    outfile.replace(outfile.length() - 3, 3, "_mg.h5");
    if(saveMG && filename.string().length() < outfile.length())
    {
      saveMultigainToH5(outfile, gain2tot, gain2count);
    }

    double avg_tot[numGains] = {};
    int count[numGains] = {};

    for(int r=0;r<rows;++r)
    {
      int subrow = r % 7;
      if(g_rows_to_use.count(subrow))
      {
        for(int c=0;c<cols;++c)
        {
          for(int gain=0;gain<3;++gain)
          {
            float val = gain2tot[gain].at(r,c);
            if(val && !isnan(val))
            {
              avg_tot[gain] += val;
              count[gain] += gain2count[gain].at(r,c);
            }
          }
        }
      }
    }


    std::cout << "Gain0 pixels:" << count[0] / (maxFrames - minFrame) << " mean avg is " << avg_tot[0] / (count[0]?count[0]:1) << std::endl;
    std::cout << "Gain1 pixels:" << count[1] / (maxFrames - minFrame) << " mean avg is " << avg_tot[1] / (count[1]?count[1]:1) << std::endl;
    std::cout << "Gain2 pixels:" << count[2] / (maxFrames - minFrame) << " mean avg is " << avg_tot[2] / (count[2]?count[2]:1) << std::endl;
}

int main(int argc, char* argv[])
{
    log4cxx::BasicConfigurator::configure();
    po::options_description desc("Allowed options");

    desc.add_options()
    ("help,h", "This will, for each acquisition file, create a frame showing pixelwise mean, and another\
                showing pixelwise variance. It will print the means of these two (where valid).\n\
                 In multigain mode, it creates a frame for each gain mode 0,1,2 and shows pixelwise\
                average across pixels in that gain mode. It will print the means of these frames.")
    // a default value will make it behave as if it's been set by the user
    ("minframe", po::value<int>()->default_value(2), "first frame to consider (def 2)")
    ("maxframe", po::value<int>()->default_value(0), "last frame to consider +1")
    // I don't know if we still need this row option
    ("row", po::value<int>()->default_value(-1), "restrict calculations to row n, (default 1,3,5)")
 //   ("save", po::bool_switch()->default_value(false), "save intermediate files")
    ("no-cds", po::bool_switch()->default_value(false), "do not subtract reset-frames")
    // do we want to put the multigain into a different app?
    ("multigain", po::bool_switch()->default_value(false), "print pixel-averages in the different gain-modes instead.")
//    ("cma", po::bool_switch()->default_value(false), "use cma based on col 0")
    ("label,l", po::value<std::string>(), "filelabel to find in filename")
    ("calib,c", po::value<std::string>(), "filepath to h5 file containing calibration coefficients")
    ("indir", po::value<std::string>(), "directory of capture-files _combined.h5")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc;
    //    std::cout << "\nIn addition -ffoo and -fno-foo syntax are recognized.\n";
        exit(0);
    }

    bfs::path calibfile, indir;
    int minframe, maxframe;
    if (vm.count("calib"))
    {
      calibfile = vm["calib"].as<std::string>();
    }

    if(!bfs::is_regular_file(calibfile))
    {
      std::cout << "Error: you must specify calib file" << std::endl;
      exit(1);
    }

    if (vm.count("indir"))
    {
      indir = vm["indir"].as<std::string>();
    }

    if(!bfs::is_directory(indir))
    {
      std::cout << "Error: you must specify directory containing acquisition files" << std::endl;
      exit(1);
    }

    minframe = vm["minframe"].as<int>();
    if(minframe==0)
    {
      printf("you can not have minframe 0 because of the reset frame numbering");
      exit(1);
    }

    if (vm.count("maxframe"))
    {
      maxframe = vm["maxframe"].as<int>();
    }

    int row = vm["row"].as<int>();
    if(row==-1)
    {
      g_rows_to_use[1] = 1;
      g_rows_to_use[3] = 1;
      g_rows_to_use[5] = 1;
    }
    else if(row==-2)
    {
      for(int i=0;i<7;++i)
        g_rows_to_use[i] = 1;
    }
    else if (row==1 || row==3 || row==5)
    {
      g_rows_to_use[row] = 1;
    }
    else
    {
      std::cout << "Error in --row " << row << std::endl; exit(1);
    }

    if(maxframe < minframe)
    {
      std::cout << "Error in frames specified" << std::endl; exit(1);
    }

    std::string label;
    if(vm.count("label"))
    {
      label = vm["label"].as<std::string>();
    }

    bool save_inter, cds;
    g_cds = !(vm["no-cds"].as<bool>());
    save_inter = true; // vm["save"].as<bool>();

    std::vector<bfs::path> files_to_process;
    bfs::directory_iterator end_itr;
    for (bfs::directory_iterator itr(indir); itr != end_itr; ++itr)
    {
        if (bfs::is_regular_file(itr->path())) 
        {
            bfs::path filename = itr->path().filename();
            // check that the filename is not the calib-file, that it has the label in it.
            if(calibfile.filename()!=filename.filename() && filename.string().find(label)!=std::string::npos
                && filename.string().find("combined.h5")!=std::string::npos)
            {
              files_to_process.push_back(itr->path());
            }
        }
    }

    std::sort(files_to_process.begin(), files_to_process.end());

    std::cout << "adc file:" << calibfile << std::endl;
    std::cout << "label " << label << ", acquisitions in " << indir << std::endl;
    std::cout << "minframe: " << minframe << " maxframe+1: " << maxframe << " cds:" << (g_cds?"on":"off") << " rows:";
    for (auto& it : g_rows_to_use)
    {
        std::cout << it.first << " ";
    }
    std::cout << std::endl;
    int64_t rc = g_calibratorR.loadADCGain(calibfile.string());

    rc |= g_calibratorS.loadADCGain(calibfile.string());
   // rc |= g_calibratorS.loadLatGain(calibfile.string());
    if(rc)
    {
      std::cout << "failed to load adc gain file" << std::endl;
    }
    else
    {
      for (auto& f : files_to_process)
      {
        if(vm["multigain"].as<bool>())
        {
          process_file_multigain(f, save_inter, minframe, maxframe);
        }
        else
        {
          process_file_meanvar(f, save_inter, minframe, maxframe);
        }
      }
    }

    return 0;
}
