
#include "FrameMem.h"

#include <sstream>

#include <H5Fpublic.h>
#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Spublic.h>

/** These loadFromH5 functions are convenience functions to load calibration constants and
* do regression tests. The dataset can be double or uint16, and 2 or 3 dimensions. There
* are some differences between the types which is unfortunately why we need two
* functions (=template specializations). They don't need to be fast (h5 isnt)
* Memory needs to be allocated BEFORE calling them (do I want to change this?)
* You need to have m_rows,m_cols equal to the dataset dims.
*
* @ret negative is failure.
*/
template<>
int64_t FrameMem<double>::loadFromH5(std::string filename, std::string dataset, int frameNo)
{
    int64_t ret = -1;
    hid_t file_id = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if(0<=file_id)
    {
        hid_t ds_id = H5Dopen(file_id, dataset.c_str(), H5P_DEFAULT);
        if(0<=ds_id)
        {
           hid_t dspace_id = H5Dget_space(ds_id);
           H5Sselect_all(dspace_id);
           hsize_t dimsCount2[2]={m_rows, m_cols};
           hid_t memspace_id = H5Screate_simple(2, dimsCount2, NULL);
           int ndims = H5Sget_simple_extent_ndims(dspace_id);
           if(ndims == 2)
           {
               hsize_t dims[2];
               H5Sget_simple_extent_dims(dspace_id, dims, NULL);
               if(dims[0] == m_rows && dims[1]==m_cols)
               {
                   herr_t err = H5Dread(ds_id, H5T_IEEE_F64LE, memspace_id, dspace_id, H5P_DEFAULT, data());
                   if(0<=err)
                       ret = 0;
               }
               else
               {
                   LOG4CXX_ERROR(m_logger, "Error dimension mismatch loading from H5:");
                   LOG4CXX_ERROR(m_logger, " dataset is " << dims[0] << "x" << dims[1] << " memblock is " << m_rows << "x" << m_cols);
               }
           }
           else if(ndims == 3)
           {
               hsize_t dims[3], dimsStart[3]={frameNo,0,0}, dimsCount[3]={1, m_rows, m_cols};
               H5Sget_simple_extent_dims(dspace_id, dims, NULL);
               if(dims[1] == m_rows && dims[2]==m_cols)
               {
                   H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, dimsStart, NULL, dimsCount, NULL);
                   herr_t err = H5Dread(ds_id, H5T_IEEE_F64LE, memspace_id, dspace_id, H5P_DEFAULT, data());
                   if(0<=err)
                       ret = 0;
               }
               else
               {
                   LOG4CXX_ERROR(m_logger, "Error dimension mismatch:");
                   LOG4CXX_ERROR(m_logger, " dataset is " << dims[0] << "x" << dims[1] << "x" << dims[2] << " memblock is " << m_rows << "x" << m_cols);
               }
           }

           H5Sclose(memspace_id);
           H5Sclose(dspace_id);
           H5Dclose(ds_id);
        }
        else
        {
            LOG4CXX_ERROR(m_logger, "could not open dataset " << dataset << " in " << filename);
        }

        H5Fclose(file_id);
    }
    else
    {
        LOG4CXX_ERROR(m_logger, "could not open file " << filename);
    }

    return ret;
}


template<>
int64_t FrameMem<uint16_t>::loadFromH5(std::string filename, std::string dataset, int frameNo)
{
    int64_t rc = -1;
    hid_t file_id = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if(0<=file_id)
    {
        hid_t ds_id = H5Dopen(file_id, dataset.c_str(), H5P_DEFAULT);
        if(0<=ds_id)
        {
           hid_t dspace_id = H5Dget_space(ds_id);
           int ndims = H5Sget_simple_extent_ndims(dspace_id);
           if(ndims == 3)
           {
               hsize_t dims[3], dimsStart[3]={frameNo,0,0}, dimsCount[3]={1, m_rows, m_cols}, dimsCount2[2]={m_rows, m_cols};
               hid_t memspace_id = H5Screate_simple(2, dimsCount2, NULL);
    // looks like they are all selected anyway!
     //          H5Sselect_all(memspace_id);
               H5Sget_simple_extent_dims(dspace_id, dims, NULL);
               if(dims[1] == m_rows && dims[2]==m_cols)
               {
                   H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, dimsStart, NULL, dimsCount, NULL);
                   herr_t err = H5Dread(ds_id, H5T_STD_U16LE, memspace_id, dspace_id, H5P_DEFAULT, data());
                   if(0<=err)
                       rc = 0;
               }
               else
               {
                   LOG4CXX_ERROR(m_logger, "Error dimension mismatch:");
                   LOG4CXX_ERROR(m_logger, " dataset is " << dims[0] << "x" << dims[1] << "x" << dims[2] << " memblock is " << m_rows << "x" << m_cols);
               }
               H5Sclose(memspace_id);
           }
           else
           {
               LOG4CXX_ERROR(m_logger, "Error dimension mismatch; dataset must be 3d");
           }

           H5Sclose(dspace_id);
           H5Dclose(ds_id);
        }
        else
        {
            LOG4CXX_ERROR(m_logger, "could not open dataset " << dataset);
        }

        H5Fclose(file_id);
    }
    else
    {
        LOG4CXX_ERROR(m_logger, "could not open file " << filename);
    }


    return rc;
}


